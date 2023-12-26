//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Generator.h"

#include "comms/process.h"

#include <cassert>
#include <type_traits>

namespace cc_mqtt5_client_afl_fuzz
{

namespace 
{

template <typename TField>
decltype(auto) addProp(TField& field)
{
    auto& vec = field.value();
    vec.resize(vec.size() + 1U);
    return vec.back();    
}

std::string pubTopicFromFilter(const std::string& filter)
{
    std::string result;
    std::size_t pos = 0U;
    while (pos < filter.size()) {
        auto wildcardPos = filter.find_first_of("#+", pos);
        if (wildcardPos == std::string::npos) {
            break;
        }

        result.append(filter.substr(pos, wildcardPos - pos));
        pos = wildcardPos + 1U;
        
        if (filter[wildcardPos] == '#') {
            result.append("hash");
            pos = filter.size();
            break;
        }

        assert(filter[wildcardPos] == '+');
        result.append("plus");
    }

    if (pos < filter.size()) {
        result.append(filter.substr(pos));
    }

    return result;
}

} // namespace 
    

bool Generator::prepare(const std::string& inputFile)
{
    m_stream.open(inputFile);
    if (!m_stream) {
        m_logger.errorLog() << "Failed to open " << inputFile << " for writing" << std::endl;
        return false;
    }

    return true;
}

void Generator::processData(const std::uint8_t* buf, unsigned bufLen)
{
    [[maybe_unused]] auto consumed = comms::processAllWithDispatch(buf, bufLen, m_frame, *this);
    assert(consumed == bufLen);
}

void Generator::handle(const Mqtt5ConnectMsg& msg)
{
    m_logger.infoLog() << "Processing " << msg.name() << "\n";
    PropsHandler propsHandler;
    for (auto& p : msg.field_properties().value()) {
        p.currentFieldExec(propsHandler);
    }    

    if (!propsHandler.m_authMethod.empty()) {
        m_cachedConnect = std::make_unique<Mqtt5ConnectMsg>(msg);
        sendAuth(propsHandler, Mqtt5AuthMsg::Field_reasonCode::Field::ValueType::ContinueAuth);
        return;
    }

    sendConnack(msg, propsHandler);
}

void Generator::handle(const Mqtt5SubscribeMsg& msg)
{
    m_logger.infoLog() << "Processing " << msg.name() << "\n";
    auto& subsVec = msg.field_list().value();

    Mqtt5SubackMsg outMsg;
    outMsg.field_packetId().value() = msg.field_packetId().value();
    auto& ackVec = outMsg.field_list().value();
    ackVec.resize(subsVec.size());

    for (auto& elem : ackVec) {
        using ElemType = std::decay_t<decltype(elem)>;
        elem.setValue(ElemType::ValueType::GrantedQos2);
    }

    sendMessage(outMsg);

    for (auto& subElemField : subsVec) {
        auto topic = pubTopicFromFilter(subElemField.field_topic().value());
        sendPublish(topic, 0);
        sendPublish(topic, 1);
        sendPublish(topic, 2);
    }
}

void Generator::handle(const Mqtt5PublishMsg& msg)
{
    m_logger.infoLog() << "Processing " << msg.name() << "\n";

    using QosValueType = Mqtt5PublishMsg::TransportField_flags::Field_qos::ValueType;
    auto qos = msg.transportField_flags().field_qos().value();
    if (qos == QosValueType::AtMostOnceDelivery) {
        return;
    }

    if (qos == QosValueType::AtLeastOnceDelivery) {
        Mqtt5PubackMsg outMsg;
        outMsg.field_packetId().setValue(msg.field_packetId().field().getValue());
        sendMessage(outMsg);
        return;
    }

    assert(qos == QosValueType::ExactlyOnceDelivery);
    Mqtt5PubrecMsg outMsg;
    outMsg.field_packetId().setValue(msg.field_packetId().field().getValue());
    sendMessage(outMsg);
    return;
}

void Generator::handle(const Mqtt5PubrecMsg& msg)
{
    m_logger.infoLog() << "Processing " << msg.name() << "\n";
    Mqtt5PubrelMsg outMsg;
    outMsg.field_packetId().setValue(msg.field_packetId().getValue());
    sendMessage(outMsg);
    return;    
}

void Generator::handle(const Mqtt5PubrelMsg& msg)
{
    m_logger.infoLog() << "Processing " << msg.name() << "\n";
    Mqtt5PubcompMsg outMsg;
    outMsg.field_packetId().setValue(msg.field_packetId().getValue());
    sendMessage(outMsg);
    return;    
}

void Generator::handle(const Mqtt5AuthMsg& msg)
{
    m_logger.infoLog() << "Processing " << msg.name() << "\n";
    if (!m_connected) {
        assert(m_cachedConnect);

        PropsHandler propsHandler;
        for (auto& p : m_cachedConnect->field_properties().value()) {
            p.currentFieldExec(propsHandler);
        }        

        sendConnack(*m_cachedConnect, propsHandler);
        m_cachedConnect.reset();
        return;
    }

    PropsHandler propsHandler;
    for (auto& p : m_cachedConnect->field_properties().value()) {
        p.currentFieldExec(propsHandler);
    }  


    if (msg.field_reasonCode().field().value() == Mqtt5AuthMsg::Field_reasonCode::Field::ValueType::ReAuth) {
        sendAuth(propsHandler, Mqtt5AuthMsg::Field_reasonCode::Field::ValueType::ContinueAuth);
        return;
    }

    assert(msg.field_reasonCode().field().value() == Mqtt5AuthMsg::Field_reasonCode::Field::ValueType::ContinueAuth);
    sendAuth(propsHandler, Mqtt5AuthMsg::Field_reasonCode::Field::ValueType::Success);
}

void Generator::handle([[maybe_unused]] const Mqtt5Message& msg)
{
    m_logger.infoLog() << "Ignoring " << msg.name() << "\n";
}

unsigned Generator::allocPacketId()
{
    ++m_lastPacketId;
    return m_lastPacketId;
}

unsigned Generator::allocTopicAlias(const std::string& topic)
{
    if (m_topicAliases.size() <= m_topicAliasLimit) {
        return 0U;
    }

    auto iter = m_topicAliases.find(topic);
    if (iter != m_topicAliases.end()) {
        return iter->second;
    }

    auto topicAlias = static_cast<unsigned>(m_topicAliases.size() + 1U);
    m_topicAliases[topic] = topicAlias;
    return topicAlias;
}

void Generator::sendMessage(Mqtt5Message& msg)
{
    m_logger.infoLog() << "Generating " << msg.name() << "\n";
    msg.refresh();
    RawDataBuf outBuf;
    outBuf.reserve(m_frame.length(msg));
    auto iter = std::back_inserter(outBuf);
    [[maybe_unused]] auto es = m_frame.write(msg, iter, outBuf.max_size());
    assert(es == comms::ErrorStatus::Success);
    assert(m_dataReportCb);
    
    std::ostreambuf_iterator<char> outIter(m_stream);
    std::copy(outBuf.begin(), outBuf.end(), outIter);
    m_dataReportCb(outBuf.data(), outBuf.size());
}

void Generator::sendConnack(const Mqtt5ConnectMsg& msg, const PropsHandler& propsHandler)
{
    Mqtt5ConnackMsg outMsg;
    auto& propsField = outMsg.field_properties();

    if (msg.field_clientId().value().empty()) {
        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_assignedClientId();
        auto& valueField = propBundle.field_value();        
        valueField.setValue("some_client_id");                
    }

    if (propsHandler.m_receiveMax > 0U) {
        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_receiveMax();
        auto& valueField = propBundle.field_value();        
        valueField.setValue(propsHandler.m_receiveMax);        
    }

    if (propsHandler.m_maxPacketSize > 0U) {
        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_maxPacketSize();
        auto& valueField = propBundle.field_value();        
        valueField.setValue(propsHandler.m_maxPacketSize);        
    }

    if (propsHandler.m_topicAliasMax > 0U) {
        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_topicAliasMax();
        auto& valueField = propBundle.field_value();        
        valueField.setValue(propsHandler.m_topicAliasMax);        
        m_topicAliasLimit = propsHandler.m_topicAliasMax;
    }    

    sendMessage(outMsg);
    m_connected = true;
}

void Generator::sendPublish(const std::string& topic, unsigned qos)
{
    Mqtt5PublishMsg outMsg;

    outMsg.transportField_flags().field_qos().setValue(qos);
    outMsg.field_topic().setValue(topic);
    if (qos > 0U) {
        outMsg.field_packetId().field().setValue(allocPacketId());
    }
    outMsg.field_payload().value().assign(topic.begin(), topic.end());

    auto topicAlias = allocTopicAlias(topic);
    if (topicAlias != 0U) {
        auto& propsField = outMsg.field_properties();
        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_topicAlias();
        auto& valueField = propBundle.field_value();        
        valueField.setValue(topicAlias);        
    } 

    sendMessage(outMsg);
}

void Generator::sendAuth(const PropsHandler& propsHandler, Mqtt5AuthMsg::Field_reasonCode::Field::ValueType reasonCode)
{
    Mqtt5AuthMsg outMsg;
    outMsg.field_reasonCode().field().setValue(reasonCode);

    auto& propsField = outMsg.field_properties().field();
    if (!propsHandler.m_authMethod.empty()) {
        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_authMethod();
        auto& valueField = propBundle.field_value();        
        valueField.setValue(propsHandler.m_authMethod);        
    }

    if (!propsHandler.m_authData.empty()) {
        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_authData();
        auto& valueField = propBundle.field_value();        
        valueField.setValue(propsHandler.m_authData);     
        if ((valueField.value().size() & 0x1) != 0) {
            valueField.value().push_back('1');
        }
    }    

    sendMessage(outMsg);
}

} // namespace cc_mqtt5_client_afl_fuzz
