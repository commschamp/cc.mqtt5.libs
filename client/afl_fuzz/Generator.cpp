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

    // TODO: send publish
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
    }    

    sendMessage(outMsg);
    m_connected = true;
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
