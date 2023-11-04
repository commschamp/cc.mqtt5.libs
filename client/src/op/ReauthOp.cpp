//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/ReauthOp.h"
#include "ClientImpl.h"

#include "comms/Assert.h"
#include "comms/util/assign.h"
#include "comms/util/ScopeGuard.h"
#include "comms/units.h"

#include <algorithm>
#include <limits>

namespace cc_mqtt5_client
{

namespace op
{

namespace 
{

inline ReauthOp* asReauthOp(void* data)
{
    return reinterpret_cast<ReauthOp*>(data);
}

} // namespace 
    

ReauthOp::ReauthOp(ClientImpl& client) : 
    Base(client),
    m_timer(client.timerMgr().allocTimer())
{
}   

CC_Mqtt5ErrorCode ReauthOp::configAuth(const CC_Mqtt5AuthConfig& config)
{
    if (config.m_authCb == nullptr) {
        errorLog("Reauth callback is not provided.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    auto& connectAuthMethod = client().sessionState().m_authMethod;
    COMMS_ASSERT(!connectAuthMethod.empty()); // The op mustn't be created 
    if ((config.m_authMethod != nullptr) && (connectAuthMethod != config.m_authMethod)) {
        errorLog("Authentication method must match the one used during CONNECT.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    m_authCb = config.m_authCb;
    m_authCbData = config.m_authCbData;

    m_authMsg.field_reasonCode().setExists();
    m_authMsg.field_reasonCode().field().value() = AuthMsg::Field_reasonCode::Field::ValueType::ReAuth;
    
    m_authMsg.field_propertiesList().setExists();
    auto& propsField = m_authMsg.field_propertiesList().field();

    {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add connect auth property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_authMethod();
        auto& valueField = propBundle.field_value();        
        valueField.value() = connectAuthMethod;
    }

    if (config.m_authDataLen > 0U) {
        if (config.m_authData == nullptr) {
            errorLog("Bad authentication data parameter.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (!canAddProp(propsField)) {
            errorLog("Cannot add connect auth property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_authData();
        auto& valueField = propBundle.field_value();        
        comms::util::assign(valueField.value(), config.m_authData, config.m_authData + config.m_authDataLen); 
    }

    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode ReauthOp::addUserProp(const CC_Mqtt5UserProp& prop)
{
    m_authMsg.field_propertiesList().setExists();
    auto& propsField = m_authMsg.field_propertiesList().field();
    return addUserPropToList(propsField, prop);
}

CC_Mqtt5ErrorCode ReauthOp::send(CC_Mqtt5ReauthCompleteCb cb, void* cbData) 
{
    auto completeOnError = 
        comms::util::makeScopeGuard(
            [this]()
            {
                opComplete();
            });

    if (cb == nullptr) {
        errorLog("Connect completion callback is not provided.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (!m_timer.isValid()) {
        errorLog("The library cannot allocate required number of timers.");
        return CC_Mqtt5ErrorCode_InternalError;
    }    

    if (m_authCb == nullptr) {
        errorLog("Authentication hasn't been configred.");
        return CC_Mqtt5ErrorCode_InsufficientConfig;
    }

    m_cb = cb;
    m_cbData = cbData;
    
    auto result = client().sendMessage(m_authMsg); 
    if (result != CC_Mqtt5ErrorCode_Success) {
        return result;
    }

    completeOnError.release(); // don't complete op yet
    auto guard = client().apiEnter();
    restartTimer();
    return CC_Mqtt5ErrorCode_Success;
}

void ReauthOp::handle(AuthMsg& msg)
{
    m_timer.cancel();

    auto status = CC_Mqtt5AsyncOpStatus_ProtocolError;
    UserPropsList userProps; // Will be referenced in response or inInfo
    auto info = CC_Mqtt5AuthInfo();

    auto disconnectReason = DisconnectReason::ProtocolError;
    auto terminateOnExit = 
        comms::util::makeScopeGuard(
            [&cl = client(), &disconnectReason]()
            {
                terminationWithReason(cl, disconnectReason);
            });     

    auto completeOpOnExit = 
        comms::util::makeScopeGuard(
            [this, &status, &info]()
            {
                auto* infoPtr = &info;
                if (status != CC_Mqtt5AsyncOpStatus_Complete) {
                    infoPtr = nullptr;
                }                
                completeOpInternal(status, infoPtr);
            });


    bool continueAuth = 
        msg.field_reasonCode().doesExist() && 
        msg.field_reasonCode().field().value() == AuthMsg::Field_reasonCode::Field::ValueType::ContinueAuth;

    bool complete = 
        msg.field_reasonCode().isMissing() || 
        msg.field_reasonCode().field().value() == AuthMsg::Field_reasonCode::Field::ValueType::Success;

    if ((!continueAuth) && (!complete)) {
        errorLog("Unexpected reason code in reauthentication AUTH");
        return;
    }

    if (msg.field_propertiesList().doesExist()) {
        PropsHandler propsHandler;
        for (auto& p : msg.field_propertiesList().field().value()) {
            p.currentFieldExec(propsHandler);
        }

        if (propsHandler.isProtocolError()) {
            errorLog("Protocol error in AUTH properties");
            return;
        }    

        // Auth method needs to be the same
        if ((propsHandler.m_authMethod != nullptr) && (client().sessionState().m_authMethod != propsHandler.m_authMethod->field_value().value())) {
            errorLog("Invalid authentication method in AUTH.");
            return;
        }      
        
        // Auth method needs to be the same
        if (propsHandler.m_authMethod == nullptr) {
            errorLog("No authentication method in AUTH.");
            return;
        }
        
        if (propsHandler.m_reasonStr != nullptr) {
            if (!client().sessionState().m_problemInfoAllowed) {
                errorLog("Received reason string in AUTH when \"problem information\" was disabled in CONNECT.");
                return; 
            }

            info.m_reasonStr = propsHandler.m_reasonStr->field_value().value().c_str();
        }

        if (propsHandler.m_authData != nullptr) {
            auto& vec = propsHandler.m_authData->field_value().value();
            comms::cast_assign(info.m_authDataLen) = vec.size();
            if (info.m_authDataLen > 0U) {
                info.m_authData = &vec[0];
            }
        }

        if (!propsHandler.m_userProps.empty()) {
            if (!client().sessionState().m_problemInfoAllowed) {
                errorLog("Received user properties in AUTH when \"problem information\" was disabled in CONNECT.");
                return; 
            }

            fillUserProps(propsHandler, userProps);
            info.m_userProps = &userProps[0];
            comms::cast_assign(info.m_userPropsCount) = userProps.size();
        }
    }

    if (complete) {
        terminateOnExit.release();
        status = CC_Mqtt5AsyncOpStatus_Complete; // Reported in op completion callback
        return;
    }

    COMMS_ASSERT(continueAuth);

    auto outInfo = CC_Mqtt5AuthInfo();
    auto authEc = m_authCb(m_authCbData, &info, &outInfo);
    if (authEc == CC_Mqtt5AuthErrorCode_Disconnect) {
        disconnectReason = DisconnectReason::NotAuthorized;
        status = CC_Mqtt5AsyncOpStatus_Aborted;
        // No members access after this point, the op will be deleted
        return;
    }

    COMMS_ASSERT(authEc == CC_Mqtt5AuthErrorCode_Continue);

    // Any early return will report another error in disconnect
    disconnectReason = DisconnectReason::UnspecifiedError;

    AuthMsg respMsg;
    respMsg.field_reasonCode().setExists();
    respMsg.field_reasonCode().field().setValue(AuthMsg::Field_reasonCode::Field::ValueType::ContinueAuth);

    respMsg.field_propertiesList().setExists();
    auto& propsField = respMsg.field_propertiesList().field();

    {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add auth property, reached available limit.");
            status = CC_Mqtt5AsyncOpStatus_OutOfMemory;
            return;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_authMethod();
        auto& valueField = propBundle.field_value();        
        valueField.value() = client().sessionState().m_authMethod.c_str();
    }

    if (outInfo.m_authDataLen > 0U) {
        if (outInfo.m_authData == nullptr) {
            status = CC_Mqtt5AsyncOpStatus_BadParam;
            return;
        }

        if (!canAddProp(propsField)) {
            errorLog("Cannot add auth property, reached available limit.");
            status = CC_Mqtt5AsyncOpStatus_OutOfMemory;
            return;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_authData();
        auto& valueField = propBundle.field_value();        
        comms::util::assign(valueField.value(), outInfo.m_authData, outInfo.m_authData + outInfo.m_authDataLen); 
    }    

    if (outInfo.m_reasonStr != nullptr) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add auth property, reached available limit.");
            status = CC_Mqtt5AsyncOpStatus_OutOfMemory;
            return;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_reasonStr();
        auto& valueField = propBundle.field_value();  
        valueField.value() = outInfo.m_reasonStr;           
    }

    if (outInfo.m_userPropsCount > 0U) {
        if (outInfo.m_userProps == nullptr) {
            errorLog("Bad user properties pointer in AUTH");
            status = CC_Mqtt5AsyncOpStatus_BadParam;
            return;
        }

        for (auto idx = 0U; idx < outInfo.m_userPropsCount; ++idx) {
            auto& prop = outInfo.m_userProps[idx];

            if (prop.m_key == nullptr) {
                errorLog("Bad user property key pointer in AUTH");
                status = CC_Mqtt5AsyncOpStatus_BadParam;
                return;
            }

            if (!canAddProp(propsField)) {
                errorLog("Cannot add auth property, reached available limit.");
                status = CC_Mqtt5AsyncOpStatus_OutOfMemory;
                return;
            }

            auto& propVar = addProp(propsField);
            auto& propBundle = propVar.initField_userProperty();            
            auto& valueField = propBundle.field_value();
            valueField.field_first().value() = prop.m_key;

            if (prop.m_value != nullptr) {
                valueField.field_second().value() = prop.m_value;
            }
        }
    }

    auto sentEc = client().sendMessage(respMsg);
    if (sentEc != CC_Mqtt5ErrorCode_Success) {
        errorLog("Failed to send AUTH message in reauth");
        status = CC_Mqtt5AsyncOpStatus_OutOfMemory;
        return;
    }

    terminateOnExit.release();
    completeOpOnExit.release();
    restartTimer();
}

Op::Type ReauthOp::typeImpl() const
{
    return Type_Reauth;
}

void ReauthOp::terminateOpImpl(CC_Mqtt5AsyncOpStatus status)
{
    completeOpInternal(status);
}

void ReauthOp::completeOpInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5AuthInfo* response)
{
    auto cb = m_cb;
    auto* cbData = m_cbData;
    opComplete(); // mustn't access data members after destruction
    if (cb != nullptr) {
        cb(cbData, status, response);    
    }
}

void ReauthOp::opTimeoutInternal()
{
    completeOpInternal(CC_Mqtt5AsyncOpStatus_Timeout);
}

void ReauthOp::restartTimer()
{
    m_timer.wait(getResponseTimeout(), &ReauthOp::opTimeoutCb, this);
}

void ReauthOp::opTimeoutCb(void* data)
{
    asReauthOp(data)->opTimeoutInternal();
}

} // namespace op

} // namespace cc_mqtt5_client
