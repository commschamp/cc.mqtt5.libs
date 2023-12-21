//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ClientImpl.h"

#include "comms/Assert.h"
#include "comms/process.h"

#include <algorithm>
#include <type_traits>

namespace cc_mqtt5_client
{

namespace 
{

template <typename TList>
void eraseFromList(const op::Op* op, TList& list)
{
    auto iter = 
        std::find_if(
            list.begin(), list.end(),
            [op](auto& opPtr)
            {
                return op == opPtr.get();
            });

    COMMS_ASSERT(iter != list.end());
    if (iter == list.end()) {
        return;
    }

    list.erase(iter);
}

void updateEc(CC_Mqtt5ErrorCode* ec, CC_Mqtt5ErrorCode val)
{
    if (ec != nullptr) {
        *ec = val;
    }
}

} // namespace 

ClientImpl::~ClientImpl()
{
    COMMS_ASSERT(m_apiEnterCount == 0U);
    terminateAllOps(CC_Mqtt5AsyncOpStatus_Aborted);
}

CC_Mqtt5ErrorCode ClientImpl::init()
{
    if (m_apiEnterCount > 0U) {
        errorLog("Cannot (re)init from within callback");
        return CC_Mqtt5ErrorCode_RetryLater;
    }

    auto guard = apiEnter();
    if ((m_sendOutputDataCb == nullptr) ||
        (m_brokerDisconnectReportCb == nullptr) ||
        (m_messageReceivedReportCb == nullptr)) {
        errorLog("Hasn't set all must have callbacks");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    bool hasTimerCallbacks = 
        (m_nextTickProgramCb != nullptr) ||
        (m_cancelNextTickWaitCb != nullptr);

    if (hasTimerCallbacks) {
        bool hasAllTimerCallbacks = 
            (m_nextTickProgramCb != nullptr) &&
            (m_cancelNextTickWaitCb != nullptr);

        if (!hasAllTimerCallbacks) {
            errorLog("Hasn't set all timer management callbacks callbacks");
            return CC_Mqtt5ErrorCode_BadParam;
        }
    }

    terminateAllOps(CC_Mqtt5AsyncOpStatus_Aborted);
    bool firstConnect = m_sessionState.m_firstConnect;
    m_sessionState = SessionState();
    m_sessionState.m_initialized = true;
    m_sessionState.m_firstConnect = firstConnect;
    COMMS_ASSERT(m_timerMgr.getMinWait() == 0U);
    return CC_Mqtt5ErrorCode_Success;
}

void ClientImpl::tick(unsigned ms)
{
    COMMS_ASSERT(m_apiEnterCount == 0U);
    ++m_apiEnterCount;
    m_timerMgr.tick(ms);
    doApiExit();
}

unsigned ClientImpl::processData(const std::uint8_t* iter, unsigned len)
{
    auto guard = apiEnter();
    COMMS_ASSERT(!m_sessionState.m_networkDisconnected);
    
    if (m_sessionState.m_networkDisconnected) {
        return 0U;
    }

    auto disconnectReason = DisconnectMsg::Field_reasonCode::Field::ValueType::ProtocolError;
    auto disconnectOnExitGuard = 
        comms::util::makeScopeGuard(
            [this, &disconnectReason]()
            {
                sendDisconnectMsg(disconnectReason);    
                notifyDisconnected(true, CC_Mqtt5AsyncOpStatus_ProtocolError);
            });

    unsigned consumed = 0;
    while (consumed < len) {
        auto remLen = len - consumed;
        auto* iterTmp = iter;

        using IdAndFlagsField = ProtFrame::Layer_idAndFlags::Field;
        static_assert(IdAndFlagsField::minLength() == IdAndFlagsField::maxLength());

        if (remLen <= IdAndFlagsField::minLength()) {
            // Size info is not available
            break;
        }

        using SizeField = ProtFrame::Layer_size::Field;
        SizeField sizeField;
        std::advance(iterTmp, IdAndFlagsField::minLength());
        auto es = sizeField.read(iterTmp, remLen - IdAndFlagsField::minLength());
        if (es == comms::ErrorStatus::NotEnoughData) {
            break;
        }

        if (es != comms::ErrorStatus::Success) {
            return len; // Disconnect
        }        

        if (m_sessionState.m_maxRecvPacketSize > 0U) {
            auto prefixSize = IdAndFlagsField::minLength() + sizeField.length();
            COMMS_ASSERT(prefixSize <= m_sessionState.m_maxRecvPacketSize);

            auto maxAllowedSize = m_sessionState.m_maxRecvPacketSize - prefixSize;
            if (maxAllowedSize < sizeField.value()) {
                errorLog("The message length exceeded max packet size");
                disconnectReason = DisconnectMsg::Field_reasonCode::Field::ValueType::PacketTooLarge;
                return len;
            }
        }

        iterTmp = iter;
        ProtFrame::MsgPtr msg;
        es = m_frame.read(msg, iterTmp, remLen);
        if (es == comms::ErrorStatus::NotEnoughData) {
            break;
        }

        if (es != comms::ErrorStatus::Success) {
            errorLog("Unexpected error in framing / payload parsing");
            return len;
        }

        COMMS_ASSERT(msg);
        msg->dispatch(*this);
        consumed += static_cast<unsigned>(std::distance(iter, iterTmp));
        iter = iterTmp;
    }

    disconnectOnExitGuard.release();
    return consumed;    
}

void ClientImpl::notifyNetworkDisconnected(bool disconnected)
{
    auto guard = apiEnter();
    m_sessionState.m_networkDisconnected = disconnected;
    if (disconnected) {
        for (auto& aliasInfo : m_sessionState.m_sendTopicAliases) {
            aliasInfo.m_lowQosRegRemCount = aliasInfo.m_lowQosRegCountRequest;
            aliasInfo.m_highQosRegRemCount = TopicAliasInfo::DefaultHighQosRegRemCount;
        }
    }

    for (auto* op : m_ops) {
        if (op != nullptr) {
            op->networkConnectivityChanged();
        }
    }
}

bool ClientImpl::isNetworkDisconnected() const
{
    return m_sessionState.m_networkDisconnected;
}

op::ConnectOp* ClientImpl::connectPrepare(CC_Mqtt5ErrorCode* ec)
{
    op::ConnectOp* connectOp = nullptr;
    do {
        if (!m_connectOps.empty()) {
            // Already allocated
            errorLog("Another connect operation is in progress.");
            updateEc(ec, CC_Mqtt5ErrorCode_Busy);
            break;
        }

        if (!m_sessionState.m_initialized) {
            errorLog("Client must be initialized to allow connect.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotIntitialized);
            break;
        }

        if (m_sessionState.m_terminating) {
            errorLog("Session termination is in progress, cannot initiate connection.");
            updateEc(ec, CC_Mqtt5ErrorCode_Terminating);
            break;
        }

        if (m_sessionState.m_connected) {
            errorLog("Client is already connected.");
            updateEc(ec, CC_Mqtt5ErrorCode_AlreadyConnected);
            break;
        }        

        if (m_sessionState.m_networkDisconnected) {
            errorLog("Network is disconnected.");
            updateEc(ec, CC_Mqtt5ErrorCode_NetworkDisconnected);
            break;            
        }

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start connect operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt5ErrorCode_RetryLater);
            break;
        }

        auto ptr = m_connectOpAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new connect operation.");
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_ops.push_back(ptr.get());
        m_connectOps.push_back(std::move(ptr));
        connectOp = m_connectOps.back().get();
        updateEc(ec, CC_Mqtt5ErrorCode_Success);
    } while (false);

    return connectOp;
}

op::DisconnectOp* ClientImpl::disconnectPrepare(CC_Mqtt5ErrorCode* ec)
{
    op::DisconnectOp* disconnectOp = nullptr;
    do {
        if (!m_sessionState.m_initialized) {
            errorLog("Client must be initialized to allow disconnect.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotIntitialized);
            break;
        }

        if (!m_sessionState.m_connected) {
            errorLog("Client must be connected to allow disconnect.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotConnected);
            break;
        }

        if (!m_disconnectOps.empty()) {
            errorLog("Another disconnect operation is in progress.");
            updateEc(ec, CC_Mqtt5ErrorCode_Busy);
            break;
        }        

        if (m_sessionState.m_terminating) {
            errorLog("Session termination is in progress, cannot initiate disconnection.");
            updateEc(ec, CC_Mqtt5ErrorCode_Terminating);
            break;
        }

        if (m_sessionState.m_networkDisconnected) {
            errorLog("Network is disconnected.");
            updateEc(ec, CC_Mqtt5ErrorCode_NetworkDisconnected);
            break;            
        }        

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start disconnect operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt5ErrorCode_RetryLater);
            break;
        }        

        auto ptr = m_disconnectOpsAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new disconnect operation.");
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_ops.push_back(ptr.get());
        m_disconnectOps.push_back(std::move(ptr));
        disconnectOp = m_disconnectOps.back().get();
        updateEc(ec, CC_Mqtt5ErrorCode_Success);
    } while (false);

    return disconnectOp;
}

op::SubscribeOp* ClientImpl::subscribePrepare(CC_Mqtt5ErrorCode* ec)
{
    op::SubscribeOp* subOp = nullptr;
    do {
        if (!m_sessionState.m_initialized) {
            errorLog("Client must be initialized to allow subscription.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotIntitialized);
            break;
        }

        if (!m_sessionState.m_connected) {
            errorLog("Client must be connected to allow subscription.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotConnected);
            break;
        }

        if (m_sessionState.m_terminating) {
            errorLog("Session termination is in progress, cannot initiate subscription.");
            updateEc(ec, CC_Mqtt5ErrorCode_Terminating);
            break;
        }

        if (m_sessionState.m_networkDisconnected) {
            errorLog("Network is disconnected.");
            updateEc(ec, CC_Mqtt5ErrorCode_NetworkDisconnected);
            break;            
        }        

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start subscribe operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt5ErrorCode_RetryLater);
            break;
        }        

        auto ptr = m_subscribeOpsAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new subscribe operation.");
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_ops.push_back(ptr.get());
        m_subscribeOps.push_back(std::move(ptr));
        subOp = m_subscribeOps.back().get();
        updateEc(ec, CC_Mqtt5ErrorCode_Success);
    } while (false);

    return subOp;
}

op::UnsubscribeOp* ClientImpl::unsubscribePrepare(CC_Mqtt5ErrorCode* ec)
{
    op::UnsubscribeOp* unsubOp = nullptr;
    do {
        if (!m_sessionState.m_initialized) {
            errorLog("Client must be initialized to allow unsubscription.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotIntitialized);
            break;
        }

        if (!m_sessionState.m_connected) {
            errorLog("Client must be connected to allow unsubscription.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotConnected);
            break;
        }

        if (m_sessionState.m_terminating) {
            errorLog("Session termination is in progress, cannot initiate unsubscription.");
            updateEc(ec, CC_Mqtt5ErrorCode_Terminating);
            break;
        }

        if (m_sessionState.m_networkDisconnected) {
            errorLog("Network is disconnected.");
            updateEc(ec, CC_Mqtt5ErrorCode_NetworkDisconnected);
            break;            
        }        

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start subscribe operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt5ErrorCode_RetryLater);
            break;
        }        

        auto ptr = m_unsubscribeOpsAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new unsubscribe operation.");
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_ops.push_back(ptr.get());
        m_unsubscribeOps.push_back(std::move(ptr));
        unsubOp = m_unsubscribeOps.back().get();
        updateEc(ec, CC_Mqtt5ErrorCode_Success);
    } while (false);

    return unsubOp;
}

op::SendOp* ClientImpl::publishPrepare(CC_Mqtt5ErrorCode* ec)
{
    op::SendOp* sendOp = nullptr;
    do {
        if (!m_sessionState.m_initialized) {
            errorLog("Client must be initialized to allow publish.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotIntitialized);
            break;
        }

        if (!m_sessionState.m_connected) {
            errorLog("Client must be connected to allow publish.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotConnected);
            break;
        }

        if (m_sessionState.m_terminating) {
            errorLog("Session termination is in progress, cannot initiate publish.");
            updateEc(ec, CC_Mqtt5ErrorCode_Terminating);
            break;
        }

        if (m_sessionState.m_networkDisconnected) {
            errorLog("Network is disconnected.");
            updateEc(ec, CC_Mqtt5ErrorCode_NetworkDisconnected);
            break;            
        }        

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start publish operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt5ErrorCode_RetryLater);
            break;
        }        

        auto ptr = m_sendOpsAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new publish operation.");
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_ops.push_back(ptr.get());
        m_sendOps.push_back(std::move(ptr));
        sendOp = m_sendOps.back().get();
        updateEc(ec, CC_Mqtt5ErrorCode_Success);
    } while (false);

    return sendOp;
}

op::ReauthOp* ClientImpl::reauthPrepare(CC_Mqtt5ErrorCode* ec)
{
    op::ReauthOp* reauthOp = nullptr;
    do {
        if (!m_reauthOps.empty()) {
            // Already allocated
            errorLog("Another reauth operation is in progress.");
            updateEc(ec, CC_Mqtt5ErrorCode_Busy);
            break;
        }
                
        if (!m_sessionState.m_initialized) {
            errorLog("Client must be initialized to allow reauth.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotIntitialized);
            break;
        }

        if (!m_sessionState.m_connected) {
            errorLog("Client must be connected to allow reauth.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotConnected);
            break;
        }

        if (m_sessionState.m_terminating) {
            errorLog("Session termination is in progress, cannot initiate reauth.");
            updateEc(ec, CC_Mqtt5ErrorCode_Terminating);
            break;
        }

        if (m_sessionState.m_networkDisconnected) {
            errorLog("Network is disconnected.");
            updateEc(ec, CC_Mqtt5ErrorCode_NetworkDisconnected);
            break;            
        }      

        if (m_sessionState.m_authMethod.empty()) {
            errorLog("Reauthentication allowed only when CONNECT used authentication.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotAuthenticated);
            break;            
        }  

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start reauth operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt5ErrorCode_RetryLater);
            break;
        }        

        auto ptr = m_reauthOpsAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new reauth operation.");
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_ops.push_back(ptr.get());
        m_reauthOps.push_back(std::move(ptr));
        reauthOp = m_reauthOps.back().get();
        updateEc(ec, CC_Mqtt5ErrorCode_Success);
    } while (false);

    return reauthOp;
}

CC_Mqtt5ErrorCode ClientImpl::allocPubTopicAlias(const char* topic, std::uint8_t qos0RegsCount)
{
    if constexpr (Config::HasTopicAliases) {
        if ((topic == nullptr) || (topic[0] == '\0')) {
            errorLog("Invalid topic in the alias allocation attempt.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (!m_sessionState.m_connected) {
            errorLog("Client must be connected to topic alias allocation.");
            return CC_Mqtt5ErrorCode_NotConnected;
        }

        if (m_sessionState.m_terminating) {
            errorLog("Session termination is in progress, cannot allocate topic alias.");
            return CC_Mqtt5ErrorCode_Terminating;
        }    

        if (m_sessionState.m_maxSendTopicAlias <= m_sessionState.m_sendTopicAliases.size()) {
            errorLog("Broker doesn't support usage of any more aliases.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (m_sessionState.m_sendTopicAliases.max_size() <= m_sessionState.m_sendTopicAliases.size()) {
            errorLog("Amount of topic aliases has reached their maximum allowed memory.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto iter = 
            std::lower_bound(
                m_sessionState.m_sendTopicAliases.begin(), m_sessionState.m_sendTopicAliases.end(), topic,
                [](auto& info, const char* topicParam)
                {
                    return info.m_topic < topicParam;
                });

        if ((iter != m_sessionState.m_sendTopicAliases.end()) && 
            (iter->m_topic == topic)) {
            iter->m_lowQosRegCountRequest = qos0RegsCount;
            iter->m_lowQosRegRemCount = qos0RegsCount;
            iter->m_highQosRegRemCount = TopicAliasInfo::DefaultHighQosRegRemCount;
            return CC_Mqtt5ErrorCode_Success;
        }

        unsigned alias = 0U;
        if (!m_sessionState.m_sendTopicFreeAliases.empty()) {
            alias = m_sessionState.m_sendTopicFreeAliases.back();
            COMMS_ASSERT(alias > 0U);
            m_sessionState.m_sendTopicFreeAliases.pop_back();
        }

        if (alias == 0U) {
            comms::cast_assign(alias) = m_sessionState.m_sendTopicFreeAliases.size() + 1U;
        }

        COMMS_ASSERT(alias > 0U);
        COMMS_ASSERT(alias <= m_sessionState.m_maxSendTopicAlias);
        auto infoIter = m_sessionState.m_sendTopicAliases.insert(iter, TopicAliasInfo());
        infoIter->m_topic = topic;
        infoIter->m_alias = alias;
        infoIter->m_lowQosRegRemCount = qos0RegsCount;
        return CC_Mqtt5ErrorCode_Success;
    }
    else {
        return CC_Mqtt5ErrorCode_NotSupported;
    }
}

CC_Mqtt5ErrorCode ClientImpl::freePubTopicAlias(const char* topic)
{
    if constexpr (Config::HasTopicAliases) {
        if ((topic == nullptr) || (topic[0] == '\0')) {
            errorLog("Invalid topic in the alias free attempt.");
            return CC_Mqtt5ErrorCode_BadParam;
        }
        
        auto iter = 
            std::lower_bound(
                m_sessionState.m_sendTopicAliases.begin(), m_sessionState.m_sendTopicAliases.end(), topic,
                [](auto& info, const char* topicParam)
                {
                    return info.m_topic < topicParam;
                });

        if ((iter == m_sessionState.m_sendTopicAliases.end()) || (iter->m_topic != topic)) {
            errorLog("Alias for provided topic hasn't been allocated before.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        m_sessionState.m_sendTopicFreeAliases.push_back(iter->m_alias);
        m_sessionState.m_sendTopicAliases.erase(iter);
        return CC_Mqtt5ErrorCode_Success;
    }
    else {
        return CC_Mqtt5ErrorCode_NotSupported;
    }        
}

unsigned ClientImpl::pubTopicAliasCount() const
{
    if constexpr (Config::HasTopicAliases) {
        return static_cast<unsigned>(m_sessionState.m_sendTopicAliases.size());
    }
    else {
        return 0U;
    }  
}

bool ClientImpl::pubTopicAliasIsAllocated(const char* topic) const
{
    if constexpr (Config::HasTopicAliases) {
        if (topic == nullptr) {
            return false;
        }
        
        auto iter = 
            std::lower_bound(
                m_sessionState.m_sendTopicAliases.begin(), m_sessionState.m_sendTopicAliases.end(), topic,
                [](auto& info, const char* topicParam)
                {
                    return info.m_topic < topicParam;
                });

        return ((iter != m_sessionState.m_sendTopicAliases.end()) && (iter->m_topic == topic));
    }
    else {
        return false;
    }  
}

void ClientImpl::handle(PublishMsg& msg)
{
    if (m_sessionState.m_terminating) {
        return;
    }

    for (auto& opPtr : m_keepAliveOps) {
        msg.dispatch(*opPtr);
    }       

    bool disconnectSent = false;
    do {
        auto createRecvOp = 
            [this, &disconnectSent, &msg]()
            {
                auto ptr = m_recvOpsAlloc.alloc(*this);
                if (!ptr) {
                    errorLog("Failed to allocate handling op for the incoming PUBLISH message.");
                    sendDisconnectMsg(DisconnectMsg::Field_reasonCode::Field::ValueType::ReceiveMaxExceeded);
                    disconnectSent = true;
                    return; 
                }

                m_ops.push_back(ptr.get());
                m_recvOps.push_back(std::move(ptr));
                msg.dispatch(*m_recvOps.back());
            };

        using Qos = PublishMsg::TransportField_flags::Field_qos::ValueType;
        if ((msg.transportField_flags().field_qos().value() == Qos::AtMostOnceDelivery) || 
            (msg.transportField_flags().field_qos().value() == Qos::AtLeastOnceDelivery)) {
            createRecvOp();
            break;
        }

        auto iter = 
            std::find_if(
                m_recvOps.begin(), m_recvOps.end(),
                [&msg](auto& opPtr)
                {
                    return opPtr->packetId() == msg.field_packetId().field().value();
                });

        if (iter == m_recvOps.end()) {
            createRecvOp();
            break;            
        }

        if (!msg.transportField_flags().field_dup().getBitValue_bit()) {
            PubrecMsg pubrecMsg;
            pubrecMsg.field_packetId().setValue(msg.field_packetId().field().value());
            pubrecMsg.field_reasonCode().setExists();
            pubrecMsg.field_properties().setExists();
            pubrecMsg.field_reasonCode().field().value() = PubackMsg::Field_reasonCode::Field::ValueType::PacketIdInUse;
            sendMessage(pubrecMsg);
            return;
        }

        // Duplicate attempt to deliver 
        (*iter)->reset();
        msg.dispatch(**iter);
    } while (false);

    if (disconnectSent) {
        notifyDisconnected(true);
        return;
    }
}

void ClientImpl::handle(PubackMsg& msg)
{
    for (auto& opPtr : m_keepAliveOps) {
        msg.dispatch(*opPtr);
    }            

    auto iter = 
        std::find_if(
            m_sendOps.begin(), m_sendOps.end(),
            [&msg](auto& opPtr)
            {
                return opPtr->packetId() == msg.field_packetId().value();
            });

    if (iter == m_sendOps.end()) {
        errorLog("PUBACK with unknown packet id");
        return;
    }

    msg.dispatch(**iter);
}

void ClientImpl::handle(PubrecMsg& msg)
{
    for (auto& opPtr : m_keepAliveOps) {
        msg.dispatch(*opPtr);
    }  

    auto iter = 
        std::find_if(
            m_sendOps.begin(), m_sendOps.end(),
            [&msg](auto& opPtr)
            {
                return opPtr->packetId() == msg.field_packetId().value();
            });

    if (iter == m_sendOps.end()) {
        errorLog("PUBREC with unknown packet id");
        PubrelMsg pubrelMsg;
        pubrelMsg.field_packetId().setValue(msg.field_packetId().value());
        pubrelMsg.field_reasonCode().setExists();
        pubrelMsg.field_properties().setExists();        
        pubrelMsg.field_reasonCode().field().value() = PubackMsg::Field_reasonCode::Field::ValueType::PacketIdNotFound;
        sendMessage(pubrelMsg);
        return;
    }

    msg.dispatch(**iter);
}

void ClientImpl::handle(PubrelMsg& msg)
{
    for (auto& opPtr : m_keepAliveOps) {
        msg.dispatch(*opPtr);
    }

    auto iter = 
        std::find_if(
            m_recvOps.begin(), m_recvOps.end(),
            [&msg](auto& opPtr)
            {
                return opPtr->packetId() == msg.field_packetId().value();
            });

    if (iter == m_recvOps.end()) {
        errorLog("PUBREL with unknown packet id");
        PubcompMsg pubcompMsg;
        pubcompMsg.field_packetId().setValue(msg.field_packetId().value());
        pubcompMsg.field_reasonCode().setExists();
        pubcompMsg.field_properties().setExists();        
        pubcompMsg.field_reasonCode().field().value() = PubackMsg::Field_reasonCode::Field::ValueType::PacketIdNotFound;
        sendMessage(pubcompMsg);
        return;
    }


    msg.dispatch(**iter);
}

void ClientImpl::handle(PubcompMsg& msg)
{
    for (auto& opPtr : m_keepAliveOps) {
        msg.dispatch(*opPtr);
    }

    auto iter = 
        std::find_if(
            m_sendOps.begin(), m_sendOps.end(),
            [&msg](auto& opPtr)
            {
                return opPtr->packetId() == msg.field_packetId().value();
            });

    if (iter == m_sendOps.end()) {
        errorLog("PUBCOMP with unknown packet id");
        return;
    }

    msg.dispatch(**iter);
}

void ClientImpl::handle(ProtMessage& msg)
{
    if (m_sessionState.m_terminating) {
        return;
    }

    // During the dispatch to callbacks can be called and new ops issues,
    // the m_ops vector can be resized and iterators invalidated.
    // As the result, the iteration needs to be performed using indices 
    // instead of iterators.
    // Also do not dispatch the message to new ops.
    auto count = m_ops.size();
    for (auto idx = 0U; idx < count; ++idx) {
        auto* op = m_ops[idx];
        if (op == nullptr) {
            // ops can be deleted, but the pointer will be nullified
            // until last api guard.
            continue;
        }

        msg.dispatch(*op);

        // After message dispatching the whole session may be in terminating state
        // Don't continue iteration
        if (m_sessionState.m_terminating) {
            break;
        }    
    }
}

CC_Mqtt5ErrorCode ClientImpl::sendMessage(const ProtMessage& msg)
{
    auto len = m_frame.length(msg);
    if ((m_sessionState.m_maxSendPacketSize > 0U) && (m_sessionState.m_maxSendPacketSize < len)) {
        errorLog("The packet length exceeds limit set by the broker.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (m_buf.max_size() < len) {
        errorLog("Output buffer overflow.");
        return CC_Mqtt5ErrorCode_BufferOverflow;
    }

    m_buf.resize(len);
    auto writeIter = comms::writeIteratorFor<ProtMessage>(&m_buf[0]);
    auto es = m_frame.write(msg, writeIter, len);
    COMMS_ASSERT(es == comms::ErrorStatus::Success);
    if (es != comms::ErrorStatus::Success) {
        errorLog("Failed to serialize output message.");
        return CC_Mqtt5ErrorCode_InternalError;
    }

    m_sendOutputDataCb(m_sendOutputDataData, &m_buf[0], static_cast<unsigned>(len));

    for (auto& opPtr : m_keepAliveOps) {
        opPtr->messageSent();
    }

    return CC_Mqtt5ErrorCode_Success;
}

void ClientImpl::opComplete(const op::Op* op)
{
    auto iter = std::find(m_ops.begin(), m_ops.end(), op);
    COMMS_ASSERT(iter != m_ops.end());
    if (iter == m_ops.end()) {
        return;
    }

    *iter = nullptr;
    m_opsDeleted = true;

    using ExtraCompleteFunc = void (ClientImpl::*)(const op::Op*);
    static const ExtraCompleteFunc Map[] = {
        /* Type_Connect */ &ClientImpl::opComplete_Connect,
        /* Type_KeepAlive */ &ClientImpl::opComplete_KeepAlive,
        /* Type_Disconnect */ &ClientImpl::opComplete_Disconnect,
        /* Type_Subscribe */ &ClientImpl::opComplete_Subscribe,
        /* Type_Unsubscribe */ &ClientImpl::opComplete_Unsubscribe,
        /* Type_Recv */ &ClientImpl::opComplete_Recv,
        /* Type_Send */ &ClientImpl::opComplete_Send,
        /* Type_Reauth */ &ClientImpl::opComplete_Reauth,
    };
    static const std::size_t MapSize = std::extent<decltype(Map)>::value;
    static_assert(MapSize == op::Op::Type_NumOfValues);

    auto idx = static_cast<unsigned>(op->type());
    COMMS_ASSERT(idx < MapSize);
    if (MapSize <= idx) {
        return;
    }

    auto func = Map[idx];
    (this->*func)(op);
}

void ClientImpl::doApiGuard()
{
    auto guard = apiEnter();
}

void ClientImpl::notifyConnected()
{
    m_sessionState.m_connected = true;
    createKeepAliveOpIfNeeded();
}

void ClientImpl::notifyDisconnected(bool reportDisconnection, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5DisconnectInfo* info)
{
    COMMS_ASSERT(reportDisconnection || (info == nullptr));
    m_sessionState.m_initialized = false; // Require re-initialization
    m_sessionState.m_connected = false;

    terminateAllOps(status);

    if (reportDisconnection) {
        COMMS_ASSERT(m_brokerDisconnectReportCb != nullptr);
        m_brokerDisconnectReportCb(m_brokerDisconnectReportData, info);
    }
}

void ClientImpl::reportMsgInfo(const CC_Mqtt5MessageInfo& info)
{
    COMMS_ASSERT(m_messageReceivedReportCb != nullptr);
    m_messageReceivedReportCb(m_messageReceivedReportData, &info);
}

void ClientImpl::doApiEnter()
{
    ++m_apiEnterCount;
    if ((m_apiEnterCount > 1U) || (m_cancelNextTickWaitCb == nullptr)) {
        return;
    }

    auto prevWait = m_timerMgr.getMinWait();
    if (prevWait == 0U) {
        return;
    }

    auto elapsed = m_cancelNextTickWaitCb(m_cancelNextTickWaitData);
    m_timerMgr.tick(elapsed);
}

void ClientImpl::doApiExit()
{
    COMMS_ASSERT(m_apiEnterCount > 0U);
    --m_apiEnterCount;
    if (m_apiEnterCount > 0U) {
        return;
    }

    cleanOps();

    if (m_nextTickProgramCb == nullptr) {
        return;
    }

    auto nextWait = m_timerMgr.getMinWait();
    if (nextWait == 0U) {
        return;
    }

    m_nextTickProgramCb(m_nextTickProgramData, nextWait);
}

void ClientImpl::createKeepAliveOpIfNeeded()
{
    if (!m_keepAliveOps.empty()) {
        return;
    }

    auto ptr = m_keepAliveOpsAlloc.alloc(*this);
    if (!ptr) {
        COMMS_ASSERT(false); // Should not happen
        return;
    }    

    m_ops.push_back(ptr.get());
    m_keepAliveOps.push_back(std::move(ptr));
}

void ClientImpl::terminateAllOps(CC_Mqtt5AsyncOpStatus status)
{
    m_sessionState.m_terminating = true;
    for (auto* op : m_ops) {
        if (op != nullptr) {
            op->terminateOp(status);
        }
    }
}

void ClientImpl::cleanOps()
{
    if (!m_opsDeleted) {
        return;
    }

    m_ops.erase(
        std::remove_if(
            m_ops.begin(), m_ops.end(),
            [](auto* op)
            {
                return op == nullptr;
            }),
        m_ops.end());

    m_opsDeleted = false;
}

void ClientImpl::errorLogInternal(const char* msg)
{
    if constexpr (Config::HasErrorLog) {
        if (m_errorLogCb == nullptr) {
            return;
        }

        m_errorLogCb(m_errorLogData, msg);
    }
}

void ClientImpl::sendDisconnectMsg(DisconnectMsg::Field_reasonCode::Field::ValueType reason)
{
    DisconnectMsg disconnectMsg;
    disconnectMsg.field_reasonCode().setExists();
    disconnectMsg.field_properties().setExists();
    disconnectMsg.field_reasonCode().field().setValue(reason);    
    sendMessage(disconnectMsg);
}

void ClientImpl::opComplete_Connect(const op::Op* op)
{
    eraseFromList(op, m_connectOps);
}

void ClientImpl::opComplete_KeepAlive(const op::Op* op)
{
    eraseFromList(op, m_keepAliveOps);
}

void ClientImpl::opComplete_Disconnect(const op::Op* op)
{
    eraseFromList(op, m_disconnectOps);
}

void ClientImpl::opComplete_Subscribe(const op::Op* op)
{
    eraseFromList(op, m_subscribeOps);
}

void ClientImpl::opComplete_Unsubscribe(const op::Op* op)
{
    eraseFromList(op, m_unsubscribeOps);
}

void ClientImpl::opComplete_Recv(const op::Op* op)
{
    eraseFromList(op, m_recvOps);
}

void ClientImpl::opComplete_Send(const op::Op* op)
{
    eraseFromList(op, m_sendOps);
}

void ClientImpl::opComplete_Reauth(const op::Op* op)
{
    eraseFromList(op, m_reauthOps);
}

} // namespace cc_mqtt5_client
