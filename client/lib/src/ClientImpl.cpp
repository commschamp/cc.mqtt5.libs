//
// Copyright 2023 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ClientImpl.h"

#include "comms/cast.h"
#include "comms/Assert.h"
#include "comms/process.h"
#include "comms/util/ScopeGuard.h"

#include <algorithm>
#include <type_traits>

namespace cc_mqtt5_client
{

namespace 
{

template <typename TList>
unsigned eraseFromList(const op::Op* op, TList& list)
{
    auto iter = 
        std::find_if(
            list.begin(), list.end(),
            [op](auto& opPtr)
            {
                return op == opPtr.get();
            });

    auto result = static_cast<unsigned>(std::distance(list.begin(), iter));

    COMMS_ASSERT(iter != list.end());
    if (iter != list.end()) {
        list.erase(iter);
    }

    return result;
}

void updateEc(CC_Mqtt5ErrorCode* ec, CC_Mqtt5ErrorCode val)
{
    if (ec != nullptr) {
        *ec = val;
    }
}

} // namespace 

ClientImpl::ClientImpl() :
    m_sessionExpiryTimer(m_timerMgr.allocTimer())
{
    COMMS_ASSERT(m_sessionExpiryTimer.isValid());
}

ClientImpl::~ClientImpl()
{
    COMMS_ASSERT(m_apiEnterCount == 0U);
    terminateOps(CC_Mqtt5AsyncOpStatus_Aborted, TerminateMode_AbortSendRecvOps);
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
    COMMS_ASSERT(!m_clientState.m_networkDisconnected);
    
    if (m_clientState.m_networkDisconnected) {
        errorLog("Incoming data when network is disconnected");
        return 0U;
    }

    auto disconnectReason = DisconnectMsg::Field_reasonCode::Field::ValueType::ProtocolError;
    auto disconnectOnExitGuard = 
        comms::util::makeScopeGuard(
            [this, &disconnectReason]()
            {
                sendDisconnectMsg(disconnectReason);    
                brokerDisconnected(CC_Mqtt5BrokerDisconnectReason_ProtocolError, CC_Mqtt5AsyncOpStatus_ProtocolError);
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

void ClientImpl::notifyNetworkDisconnected()
{
    auto guard = apiEnter();
    m_clientState.m_networkDisconnected = true;
    if (m_sessionState.m_disconnecting) {
        return; // No need to go through broker disconnection
    }
    
    brokerDisconnected();
}

bool ClientImpl::isNetworkDisconnected() const
{
    return m_clientState.m_networkDisconnected;
}

op::ConnectOp* ClientImpl::connectPrepare(CC_Mqtt5ErrorCode* ec)
{
    op::ConnectOp* connectOp = nullptr;
    do {
        m_clientState.m_networkDisconnected = false;

        if (!m_clientState.m_initialized) {
            if (m_apiEnterCount > 0U) {
                errorLog("Cannot prepare connect from within callback");
                updateEc(ec, CC_Mqtt5ErrorCode_RetryLater);
                break;
            }

            auto initEc = initInternal();
            if (initEc != CC_Mqtt5ErrorCode_Success) {
                updateEc(ec, initEc);
                break;
            }
        }
                
        if (!m_connectOps.empty()) {
            // Already allocated
            errorLog("Another connect operation is in progress.");
            updateEc(ec, CC_Mqtt5ErrorCode_Busy);
            break;
        }

        if (m_sessionState.m_disconnecting) {
            errorLog("Session disconnection is in progress, cannot initiate connection.");
            updateEc(ec, CC_Mqtt5ErrorCode_Disconnecting);
            break;
        }

        if (m_sessionState.m_connected) {
            errorLog("Client is already connected.");
            updateEc(ec, CC_Mqtt5ErrorCode_AlreadyConnected);
            break;
        }        

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start connect operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt5ErrorCode_RetryLater);
            break;
        }

        if (m_preparationLocked) {
            errorLog("Another operation is being prepared, cannot prepare \"connect\" without \"send\" or \"cancel\" of the previous.");
            updateEc(ec, CC_Mqtt5ErrorCode_PreparationLocked);            
            break;
        }

        auto ptr = m_connectOpAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new connect operation.");
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_preparationLocked = true;
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

        if (m_sessionState.m_disconnecting) {
            errorLog("Session disconnection is in progress, cannot initiate disconnection.");
            updateEc(ec, CC_Mqtt5ErrorCode_Disconnecting);
            break;
        }

        if (m_clientState.m_networkDisconnected) {
            errorLog("Network is disconnected.");
            updateEc(ec, CC_Mqtt5ErrorCode_NetworkDisconnected);
            break;            
        }        

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start disconnect operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt5ErrorCode_RetryLater);
            break;
        }   

        if (m_preparationLocked) {
            errorLog("Another operation is being prepared, cannot prepare \"disconnect\" without \"send\" or \"cancel\" of the previous.");
            updateEc(ec, CC_Mqtt5ErrorCode_PreparationLocked);            
            break;
        }            

        auto ptr = m_disconnectOpsAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new disconnect operation.");
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_preparationLocked = true;
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
        if (!m_sessionState.m_connected) {
            errorLog("Client must be connected to allow subscription.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotConnected);
            break;
        }

        if (m_sessionState.m_disconnecting) {
            errorLog("Session disconnection is in progress, cannot initiate subscription.");
            updateEc(ec, CC_Mqtt5ErrorCode_Disconnecting);
            break;
        }

        if (m_clientState.m_networkDisconnected) {
            errorLog("Network is disconnected.");
            updateEc(ec, CC_Mqtt5ErrorCode_NetworkDisconnected);
            break;            
        }        

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start subscribe operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt5ErrorCode_RetryLater);
            break;
        }  

        if (m_preparationLocked) {
            errorLog("Another operation is being prepared, cannot prepare \"subscribe\" without \"send\" or \"cancel\" of the previous.");
            updateEc(ec, CC_Mqtt5ErrorCode_PreparationLocked);            
            break;
        }            

        auto ptr = m_subscribeOpsAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new subscribe operation.");
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_preparationLocked = true;
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
        if (!m_sessionState.m_connected) {
            errorLog("Client must be connected to allow unsubscription.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotConnected);
            break;
        }

        if (m_sessionState.m_disconnecting) {
            errorLog("Session disconnection is in progress, cannot initiate unsubscription.");
            updateEc(ec, CC_Mqtt5ErrorCode_Disconnecting);
            break;
        }

        if (m_clientState.m_networkDisconnected) {
            errorLog("Network is disconnected.");
            updateEc(ec, CC_Mqtt5ErrorCode_NetworkDisconnected);
            break;            
        }        

        if (m_ops.max_size() <= m_ops.size()) {
            errorLog("Cannot start subscribe operation, retry in next event loop iteration.");
            updateEc(ec, CC_Mqtt5ErrorCode_RetryLater);
            break;
        }    

        if (m_preparationLocked) {
            errorLog("Another operation is being prepared, cannot prepare \"unsubscribe\" without \"send\" or \"cancel\" of the previous.");
            updateEc(ec, CC_Mqtt5ErrorCode_PreparationLocked);            
            break;
        }             

        auto ptr = m_unsubscribeOpsAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new unsubscribe operation.");
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_preparationLocked = true;
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
        if (!m_sessionState.m_connected) {
            errorLog("Client must be connected to allow publish.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotConnected);
            break;
        }

        if (m_sessionState.m_disconnecting) {
            errorLog("Session disconnection is in progress, cannot initiate publish.");
            updateEc(ec, CC_Mqtt5ErrorCode_Disconnecting);
            break;
        }

        if (m_clientState.m_networkDisconnected) {
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

        if (m_preparationLocked) {
            errorLog("Another operation is being prepared, cannot prepare \"unsubscribe\" without \"send\" or \"cancel\" of the previous.");
            updateEc(ec, CC_Mqtt5ErrorCode_PreparationLocked);            
            break;
        }          

        m_preparationLocked = true;
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
                
        if (!m_sessionState.m_connected) {
            errorLog("Client must be connected to allow reauth.");
            updateEc(ec, CC_Mqtt5ErrorCode_NotConnected);
            break;
        }

        if (m_sessionState.m_disconnecting) {
            errorLog("Session disconnection is in progress, cannot initiate reauth.");
            updateEc(ec, CC_Mqtt5ErrorCode_Disconnecting);
            break;
        }

        if (m_clientState.m_networkDisconnected) {
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

        if (m_preparationLocked) {
            errorLog("Another operation is being prepared, cannot prepare \"reauth\" without \"send\" or \"cancel\" of the previous.");
            updateEc(ec, CC_Mqtt5ErrorCode_PreparationLocked);            
            break;
        }           

        auto ptr = m_reauthOpsAlloc.alloc(*this);
        if (!ptr) {
            errorLog("Cannot allocate new reauth operation.");
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_preparationLocked = true;
        m_ops.push_back(ptr.get());
        m_reauthOps.push_back(std::move(ptr));
        reauthOp = m_reauthOps.back().get();
        updateEc(ec, CC_Mqtt5ErrorCode_Success);
    } while (false);

    return reauthOp;
}

CC_Mqtt5ErrorCode ClientImpl::allocPubTopicAlias(const char* topic, unsigned qos0RegsCount)
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

        if (m_sessionState.m_disconnecting) {
            errorLog("Session disconnection is in progress, cannot allocate topic alias.");
            return CC_Mqtt5ErrorCode_Disconnecting;
        }    

        if (m_sessionState.m_maxSendTopicAlias <= m_clientState.m_sendTopicAliases.size()) {
            errorLog("Broker doesn't support usage of any more aliases.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (m_clientState.m_sendTopicAliases.max_size() <= m_clientState.m_sendTopicAliases.size()) {
            errorLog("Amount of topic aliases has reached their maximum allowed memory.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        if (std::numeric_limits<std::uint8_t>::max() < qos0RegsCount) {
            errorLog("The qos0RegsCount value is too high");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        auto iter = 
            std::lower_bound(
                m_clientState.m_sendTopicAliases.begin(), m_clientState.m_sendTopicAliases.end(), topic,
                [](auto& info, const char* topicParam)
                {
                    return info.m_topic < topicParam;
                });

        if ((iter != m_clientState.m_sendTopicAliases.end()) && 
            (iter->m_topic == topic)) {
            comms::cast_assign(iter->m_lowQosRegRemCount) = std::max(qos0RegsCount, 1U);
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
        auto infoIter = m_clientState.m_sendTopicAliases.insert(iter, TopicAliasInfo());
        infoIter->m_topic = topic;
        infoIter->m_alias = alias;
        comms::cast_assign(infoIter->m_lowQosRegRemCount) = std::max(qos0RegsCount, 1U);
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
                m_clientState.m_sendTopicAliases.begin(), m_clientState.m_sendTopicAliases.end(), topic,
                [](auto& info, const char* topicParam)
                {
                    return info.m_topic < topicParam;
                });

        if ((iter == m_clientState.m_sendTopicAliases.end()) || (iter->m_topic != topic)) {
            errorLog("Alias for provided topic hasn't been allocated before.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        m_sessionState.m_sendTopicFreeAliases.push_back(iter->m_alias);
        m_clientState.m_sendTopicAliases.erase(iter);
        return CC_Mqtt5ErrorCode_Success;
    }
    else {
        return CC_Mqtt5ErrorCode_NotSupported;
    }        
}

CC_Mqtt5ErrorCode ClientImpl::setPublishOrdering(CC_Mqtt5PublishOrdering ordering)
{
    if (CC_Mqtt5PublishOrdering_ValuesLimit <= ordering) {
        errorLog("Bad publish ordering value");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    m_configState.m_publishOrdering = ordering;
    return CC_Mqtt5ErrorCode_Success;
}

unsigned ClientImpl::pubTopicAliasCount() const
{
    if constexpr (Config::HasTopicAliases) {
        return static_cast<unsigned>(m_clientState.m_sendTopicAliases.size());
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
                m_clientState.m_sendTopicAliases.begin(), m_clientState.m_sendTopicAliases.end(), topic,
                [](auto& info, const char* topicParam)
                {
                    return info.m_topic < topicParam;
                });

        return ((iter != m_clientState.m_sendTopicAliases.end()) && (iter->m_topic == topic));
    }
    else {
        return false;
    }  
}

void ClientImpl::handle(PublishMsg& msg)
{
    if (m_sessionState.m_disconnecting) {
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

        using Qos = op::Op::Qos;
        auto qos = msg.transportField_flags().field_qos().value();
        if ((qos == Qos::AtMostOnceDelivery) || 
            (qos == Qos::AtLeastOnceDelivery)) {
            createRecvOp();
            break;
        }

        if constexpr (Config::MaxQos >= 2) {
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

            PubrecMsg pubrecMsg;
            pubrecMsg.field_packetId().setValue(msg.field_packetId().field().value());

            if (!msg.transportField_flags().field_dup().getBitValue_bit()) {
                pubrecMsg.field_reasonCode().setExists();
                pubrecMsg.field_properties().setExists();
                pubrecMsg.field_reasonCode().field().value() = PubrecMsg::Field_reasonCode::Field::ValueType::PacketIdInUse;
            }
            else {
                // Duplicate detected, just re-confirming
                (*iter)->resetTimer();
            }

            sendMessage(pubrecMsg);
            return;
        }
        else {
            createRecvOp();
            break;
        }
    } while (false);

    if (disconnectSent) {
        brokerDisconnected(CC_Mqtt5BrokerDisconnectReason_ProtocolError);
        return;
    }
}

#if CC_MQTT5_CLIENT_MAX_QOS >= 1
void ClientImpl::handle(PubackMsg& msg)
{
    static_assert(Config::MaxQos >= 1);
    if (!processPublishAckMsg(msg, msg.field_packetId().value(), false)) {
        errorLog("PUBACK with unknown packet id");
    }
}
#endif // #if CC_MQTT5_CLIENT_MAX_QOS >= 1

#if CC_MQTT5_CLIENT_MAX_QOS >= 2
void ClientImpl::handle(PubrecMsg& msg)
{
    static_assert(Config::MaxQos >= 2);
    if (!processPublishAckMsg(msg, msg.field_packetId().value(), false)) {
        errorLog("PUBREC with unknown packet id");
        PubrelMsg pubrelMsg;
        pubrelMsg.field_packetId().setValue(msg.field_packetId().value());
        pubrelMsg.field_reasonCode().setExists();
        pubrelMsg.field_properties().setExists();        
        pubrelMsg.field_reasonCode().field().value() = PubrecMsg::Field_reasonCode::Field::ValueType::PacketIdNotFound;
        sendMessage(pubrelMsg);
    }
}

void ClientImpl::handle(PubrelMsg& msg)
{
    static_assert(Config::MaxQos >= 2);
    for (auto& opPtr : m_keepAliveOps) {
        msg.dispatch(*opPtr);
    }

    auto iter = 
        std::find_if(
            m_recvOps.begin(), m_recvOps.end(),
            [&msg](auto& opPtr)
            {
                COMMS_ASSERT(opPtr);
                return opPtr->packetId() == msg.field_packetId().value();
            });

    if (iter == m_recvOps.end()) {
        errorLog("PUBREL with unknown packet id");
        PubcompMsg pubcompMsg;
        pubcompMsg.field_packetId().setValue(msg.field_packetId().value());
        pubcompMsg.field_reasonCode().setExists();
        pubcompMsg.field_properties().setExists();        
        pubcompMsg.field_reasonCode().field().value() = PubcompMsg::Field_reasonCode::Field::ValueType::PacketIdNotFound;
        sendMessage(pubcompMsg);
        return;
    }

    msg.dispatch(**iter);
}

void ClientImpl::handle(PubcompMsg& msg)
{
    if (!processPublishAckMsg(msg, msg.field_packetId().value(), true)) {
        errorLog("PUBCOMP with unknown packet id");
    }
}

#endif // #if CC_MQTT5_CLIENT_MAX_QOS >= 2

void ClientImpl::handle(ProtMessage& msg)
{
    if (m_sessionState.m_disconnecting) {
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
        if (m_sessionState.m_disconnecting) {
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

    COMMS_ASSERT(m_sendOutputDataCb != nullptr);
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

void ClientImpl::brokerConnected(bool sessionPresent)
{
    m_sessionExpiryTimer.cancel();
    
    m_clientState.m_firstConnect = false;
    m_sessionState.m_connected = true;

    do {
        if (sessionPresent) {
            for (auto& sendOpPtr : m_sendOps) {
                sendOpPtr->postReconnectionResend();
            }  

            for (auto& recvOpPtr : m_recvOps) {
                recvOpPtr->postReconnectionResume();
            }    

            COMMS_ASSERT(0U < m_sessionState.m_highQosSendLimit);  
            auto resumeUntilIdx = std::min(std::size_t(m_sessionState.m_highQosSendLimit), m_sendOps.size()); 
            auto resumeFromIdx = resumeUntilIdx; 
            for (auto count = resumeUntilIdx; count > 0U; --count) {
                auto idx = count - 1U;
                auto& sendOpPtr = m_sendOps[idx];
                if (!sendOpPtr->isPaused()) {
                    break;
                }

                resumeFromIdx = idx;
            }

            if (resumeFromIdx < resumeUntilIdx) {
                resumeSendOpsSince(static_cast<unsigned>(resumeFromIdx));
            }
            break;
        }

        // Old stored session, terminate pending ops
        for (auto* op : m_ops) {
            auto opType = op->type();
            if ((opType != op::Op::Type::Type_Send) && 
                (opType != op::Op::Type::Type_Recv)) {
                continue;
            }

            op->terminateOp(CC_Mqtt5AsyncOpStatus_Aborted);
        }
    } while (false);

    m_clientState.m_sendTopicAliases.clear();    

    createKeepAliveOpIfNeeded();    
}

void ClientImpl::brokerDisconnected(
    CC_Mqtt5BrokerDisconnectReason reason, 
    CC_Mqtt5AsyncOpStatus status, 
    const CC_Mqtt5DisconnectInfo* info)
{
    COMMS_ASSERT((reason == CC_Mqtt5BrokerDisconnectReason_DisconnectMsg) || (info == nullptr));
    m_clientState.m_initialized = false; // Require re-initialization
    m_sessionState.m_connected = false;

    bool preserveSendRecv = 
        (m_sessionState.m_sessionExpiryIntervalMs > 0U) && 
        ((!m_recvOps.empty()) || (!m_sendOps.empty()));

    auto termMode = TerminateMode_AbortSendRecvOps;
    if (preserveSendRecv) {
        termMode = TerminateMode_KeepSendRecvOps;
    }

    m_sessionState.m_disconnecting = true;
    terminateOps(status, termMode);    

    if (preserveSendRecv) {
        for (auto* op : m_ops) {
            if (op != nullptr) {
                op->connectivityChanged();
            }
        } 

        const auto NeverExpires = (static_cast<decltype(m_sessionState.m_sessionExpiryIntervalMs)>(CC_MQTT5_SESSION_NEVER_EXPIRES) * 1000U);
        if (m_sessionState.m_sessionExpiryIntervalMs != NeverExpires) {
            m_sessionExpiryTimer.wait(m_sessionState.m_sessionExpiryIntervalMs, &ClientImpl::sessionExpiryTimeoutCb, this);
        }    
    }    

    if (reason < CC_Mqtt5BrokerDisconnectReason_ValuesLimit) {
        COMMS_ASSERT(m_brokerDisconnectReportCb != nullptr);
        m_brokerDisconnectReportCb(m_brokerDisconnectReportData, reason, info);
    }
}

void ClientImpl::reportMsgInfo(const CC_Mqtt5MessageInfo& info)
{
    COMMS_ASSERT(m_messageReceivedReportCb != nullptr);
    m_messageReceivedReportCb(m_messageReceivedReportData, &info);
}

bool ClientImpl::hasPausedSendsBefore(const op::SendOp* sendOp) const
{
    auto riter = 
        std::find_if(
            m_sendOps.rbegin(), m_sendOps.rend(),
            [sendOp](auto& opPtr)
            {
                return opPtr.get() == sendOp;
            });

    COMMS_ASSERT(riter != m_sendOps.rend());
    if (riter == m_sendOps.rend()) {
        return false;
    }

    auto iter = riter.base() - 1;
    auto idx = static_cast<unsigned>(std::distance(m_sendOps.begin(), iter));
    COMMS_ASSERT(idx < m_sendOps.size());
    if (idx == 0U) {
        return false;
    }

    auto& prevSendOpPtr = m_sendOps[idx - 1U];
    return prevSendOpPtr->isPaused();
}

bool ClientImpl::hasHigherQosSendsBefore(const op::SendOp* sendOp, op::Op::Qos qos) const
{
    for (auto& sendOpPtr : m_sendOps) {
        if (sendOpPtr.get() == sendOp) {
            return false;
        }

        if (sendOpPtr->qos() > qos) {
            return true;
        }
    }

    COMMS_ASSERT(false); // Mustn't reach here
    return false;
}

void ClientImpl::allowNextPrepare()
{
    COMMS_ASSERT(m_preparationLocked);
    m_preparationLocked = false;
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

void ClientImpl::terminateOps(CC_Mqtt5AsyncOpStatus status, TerminateMode mode)
{
    for (auto* op : m_ops) {
        if (op == nullptr) {
            continue;
        }

        if (mode == TerminateMode_KeepSendRecvOps) {
            auto opType = op->type();

            if ((opType == op::Op::Type_Recv) || (opType == op::Op::Type_Send)) {
                continue;
            }
        }

        op->terminateOp(status);
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

CC_Mqtt5ErrorCode ClientImpl::initInternal()
{
    auto guard = apiEnter();
    if ((m_sendOutputDataCb == nullptr) ||
        (m_brokerDisconnectReportCb == nullptr) ||
        (m_messageReceivedReportCb == nullptr)) {
        errorLog("Hasn't set all must have callbacks");
        return CC_Mqtt5ErrorCode_NotIntitialized;
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
            return CC_Mqtt5ErrorCode_NotIntitialized;
        }
    }

    terminateOps(CC_Mqtt5AsyncOpStatus_Aborted, TerminateMode_KeepSendRecvOps);
    m_sessionState = SessionState();
    m_clientState.m_initialized = true;
    return CC_Mqtt5ErrorCode_Success;
}

void ClientImpl::resumeSendOpsSince(unsigned idx)
{
    while (idx < m_sendOps.size()) {
        auto& opToResumePtr = m_sendOps[idx];
        if (!opToResumePtr->isPaused()) {
            ++idx;
            continue;
        }         
        
        if (!opToResumePtr->resume()) {
            break;
        }

        // After resuming some (QoS0) ops can complete right away, increment idx next iteration
    }
}

void ClientImpl::sessionExpiryTimeoutInternal()
{
    COMMS_ASSERT(m_apiEnterCount > 0U);
    COMMS_ASSERT(!m_sessionState.m_connected);
    for (auto* op : m_ops) {
        if (op == nullptr) {
            continue;
        }

        auto opType = op->type();

        if ((opType != op::Op::Type_Recv) && (opType != op::Op::Type_Send)) {
            continue;
        }

        op->terminateOp(CC_Mqtt5AsyncOpStatus_BrokerDisconnected);
    }
}

op::SendOp* ClientImpl::findSendOp(std::uint16_t packetId)
{
    auto iter = 
        std::find_if(
            m_sendOps.begin(), m_sendOps.end(),
            [packetId](auto& opPtr)
            {
                return opPtr->packetId() == packetId;
            });

    if (iter == m_sendOps.end()) {
        return nullptr;
    }

    return iter->get();
}

bool ClientImpl::isLegitSendAck(const op::SendOp* sendOp, bool pubcompAck) const
{
    if (!sendOp->isPublished()) {
        return false;
    }

    for (auto& sendOpPtr : m_sendOps) {
        if (sendOpPtr.get() == sendOp) {
            return true;
        }

        if (!sendOpPtr->isAcked()) {
            return false;
        }

        if (pubcompAck && (sendOp != m_sendOps.front().get())) {
            return false;
        }
    }

    COMMS_ASSERT(false); // Should not be reached;
    return false;
}

void ClientImpl::resendAllUntil(op::SendOp* sendOp)
{
    // Do index controlled iteration because forcing dup resend can
    // cause early message destruction.
    for (auto idx = 0U; idx < m_sendOps.size();) {
        auto& sendOpPtr = m_sendOps[idx];
        COMMS_ASSERT(sendOpPtr);
        auto* opBeforeResend = sendOpPtr.get();
        sendOpPtr->forceDupResend(); // can destruct object
        if (opBeforeResend == sendOp) {
            break;
        }

        auto* opAfterResend = sendOpPtr.get();
        if (opBeforeResend != opAfterResend) {
            // The op object was destructed and erased, 
            // do not increment index;
            continue;
        }

        ++idx;
    }
}

bool ClientImpl::processPublishAckMsg(ProtMessage& msg, std::uint16_t packetId, bool pubcompAck)
{
    for (auto& opPtr : m_keepAliveOps) {
        msg.dispatch(*opPtr);
    }     

    auto* sendOp = findSendOp(packetId);
    if (sendOp == nullptr) {
        return false;
    }

    if (isLegitSendAck(sendOp, pubcompAck)) {
        msg.dispatch(*sendOp);
        return true;        
    }

    resendAllUntil(sendOp);
    return true;
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
    auto idx = eraseFromList(op, m_sendOps);
    if (m_sessionState.m_disconnecting) {
        return;
    }

    resumeSendOpsSince(idx);
}

void ClientImpl::opComplete_Reauth(const op::Op* op)
{
    eraseFromList(op, m_reauthOps);
}

void ClientImpl::sessionExpiryTimeoutCb(void* data)
{
    reinterpret_cast<ClientImpl*>(data)->sessionExpiryTimeoutInternal();
}

} // namespace cc_mqtt5_client
