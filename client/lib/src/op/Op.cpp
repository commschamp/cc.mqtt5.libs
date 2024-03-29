//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/Op.h"

#include "ClientImpl.h"

#include "comms/util/ScopeGuard.h"
#include "comms/cast.h"

#include <algorithm>
#include <type_traits>

namespace cc_mqtt5_client
{

namespace op
{

namespace 
{

static constexpr char TopicSep = '/';
static constexpr char MultLevelWildcard = '#';
static constexpr char SingleLevelWildcard = '+';

} // namespace 



Op::Op(ClientImpl& client) : 
    m_client(client),
    m_responseTimeoutMs(client.configState().m_responseTimeoutMs)
{
}    

void Op::sendMessage(const ProtMessage& msg)
{
    m_client.sendMessage(msg);
}

void Op::terminateOpImpl([[maybe_unused]] CC_Mqtt5AsyncOpStatus status)
{
    opComplete();
}

void Op::networkConnectivityChangedImpl()
{
    // Do nothing by default
}

void Op::opComplete()
{
    m_client.opComplete(this);
}

void Op::doApiGuard()
{
    m_client.doApiGuard();
}

std::uint16_t Op::allocPacketId()
{
    static constexpr auto MaxPacketId = std::numeric_limits<std::uint16_t>::max();
    auto& allocatedPacketIds = m_client.clientState().m_allocatedPacketIds;

    if ((allocatedPacketIds.max_size() <= allocatedPacketIds.size()) || 
        (MaxPacketId <= allocatedPacketIds.size())) {
        errorLog("No more available packet IDs for allocation");
        return 0U;
    }

    auto& lastPacketId = m_client.clientState().m_lastPacketId;
    auto nextPacketId = static_cast<std::uint16_t>(lastPacketId + 1U);

    if (nextPacketId == 0U) {
        nextPacketId = 1U;
    }
    
    while (true) {
        if (allocatedPacketIds.empty() || (allocatedPacketIds.back() < nextPacketId)) {
            allocatedPacketIds.push_back(nextPacketId);
            break;
        }

        auto iter = std::lower_bound(allocatedPacketIds.begin(), allocatedPacketIds.end(), nextPacketId);
        if ((iter == allocatedPacketIds.end()) || (*iter != nextPacketId)) {
            allocatedPacketIds.insert(iter, nextPacketId);
            break;
        }

        ++nextPacketId;
    } 

    lastPacketId = static_cast<std::uint16_t>(nextPacketId);
    return lastPacketId;
}

void Op::releasePacketId(std::uint16_t id)
{
    if (id == 0U) {
        return;
    }
    
    auto& allocatedPacketIds = m_client.clientState().m_allocatedPacketIds;
    auto iter = std::lower_bound(allocatedPacketIds.begin(), allocatedPacketIds.end(), id);
    if ((iter == allocatedPacketIds.end()) || (*iter != id)) {
        [[maybe_unused]] static constexpr bool ShouldNotHappen = false;
        COMMS_ASSERT(ShouldNotHappen);
        return;
    }    

    allocatedPacketIds.erase(iter);
}

void Op::sendDisconnectWithReason(ClientImpl& client, DisconnectReason reason)
{
    DisconnectMsg disconnectMsg;
    disconnectMsg.field_reasonCode().setExists();
    disconnectMsg.field_properties().setExists();
    disconnectMsg.field_reasonCode().field().setValue(reason);    
    client.sendMessage(disconnectMsg);
}

void Op::sendDisconnectWithReason(DisconnectReason reason)
{
    sendDisconnectWithReason(m_client, reason);
}

void Op::terminationWithReasonStatic(ClientImpl& client, DisconnectReason reason)
{
    sendDisconnectWithReason(client, reason);
    client.brokerDisconnected(true);
}

void Op::terminationWithReason(DisconnectReason reason)
{
    terminationWithReasonStatic(m_client, reason);
}

void Op::protocolErrorTermination(ClientImpl& client)
{
    terminationWithReasonStatic(client, DisconnectReason::ProtocolError);
}

void Op::protocolErrorTermination()
{
    protocolErrorTermination(m_client);
}

void Op::fillUserProps(const PropsHandler& propsHandler, UserPropsList& userProps)
{
    if constexpr (Config::HasUserProps) {    
        userProps.reserve(std::min(propsHandler.m_userProps.size() + userProps.size(), userProps.max_size()));
        auto endIter = propsHandler.m_userProps.end();
        if constexpr (Config::UserPropsLimit > 0U) {
            endIter = propsHandler.m_userProps.begin() + std::min(propsHandler.m_userProps.size(), std::size_t(Config::UserPropsLimit));
        }

        std::transform(
            propsHandler.m_userProps.begin(), endIter, std::back_inserter(userProps),
            [](auto* field)
            {
                return UserPropsList::value_type{field->field_value().field_first().value().c_str(), field->field_value().field_second().value().c_str()};
            });
    }
}

bool Op::isSharedTopicFilter(const char* filter)
{
    static const char SharePrefix[] = {'$', 's', 'h', 'a', 'r', 'e', '/' };
    static const size_t SharePrefixSize = std::extent<decltype(SharePrefix)>::value;

    for (auto idx = 0U; idx < SharePrefixSize; ++idx) {
        if (filter[idx] == '\0') {
            return false;
        }

        if (SharePrefix[idx] != filter[idx]) {
            return false;
        }
    }

    return true;
}

void Op::errorLogInternal(const char* msg)
{
    if constexpr (Config::HasErrorLog) {
        m_client.errorLog(msg);
    }    
}

bool Op::verifySubFilterInternal(const char* filter)
{
    if (Config::HasTopicFormatVerification) {
        if (!m_client.configState().m_verifyOutgoingTopic) {
            return true;
        }

        COMMS_ASSERT(filter != nullptr);
        if (filter[0] == '\0') {
            return false;
        }

        do {
            if (!isSharedTopicFilter(filter)) {
                break;
            }

            if (!m_client.sessionState().m_sharedSubsAvailable) {
                errorLog("Shared subscriptions not accepted by the broker, cannot use \"$share/\" prefixed topics.");
                return false;
            }

            auto nextIdx = sizeof("$share/") - 1U; // 7 due to \0;
            COMMS_ASSERT(nextIdx == 7U);
            if ((filter[nextIdx] == '\0') || (filter[nextIdx] == '/')) {
                errorLog("Shared subscriptions name must have at least one character.");
                return false;
            }        

            while (filter[nextIdx] != '/') {
                if (filter[nextIdx] == '\0') {
                    errorLog("Shared subscriptions name must have topic after share name.");
                    return false;
                }

                if ((filter[nextIdx] == MultLevelWildcard) || (filter[nextIdx] == SingleLevelWildcard)) {
                    errorLog("Shared subscriptions name must not have wildcards.");
                    return false;
                }                

                ++nextIdx;
            }

            ++nextIdx;
            if (filter[nextIdx] == '\0') {
                errorLog("Shared subscriptions name must have topic after share name.");
                return false;
            }            

        } while (false);

        auto pos = 0U;
        int lastSep = -1;
        while (filter[pos] != '\0') {
            auto incPosGuard = 
                comms::util::makeScopeGuard(
                    [&pos]()
                    {
                        ++pos;
                    });

            auto ch = filter[pos];

            if (ch == TopicSep) {
                comms::cast_assign(lastSep) = pos;
                continue;
            }   

            if (ch == MultLevelWildcard) {
                if (!m_client.sessionState().m_wildcardSubAvailable) {
                    errorLog("Wildcard subscriptions not accepted by the broker, cannot use \'#\'.");
                    return false;
                }
                                
                if (filter[pos + 1] != '\0') {
                    errorLog("Multi-level wildcard \'#\' must be last.");
                    return false;
                }

                if (pos == 0U) {
                    return true;
                }

                if ((lastSep < 0) || (static_cast<decltype(lastSep)>(pos - 1U) != lastSep)) {
                    errorLog("Multi-level wildcard \'#\' must follow separator.");
                    return false;
                }

                return true;
            }

            if (ch != SingleLevelWildcard) {
                continue;
            }

            if (!m_client.sessionState().m_wildcardSubAvailable) {
                errorLog("Wildcard subscriptions not accepted by the broker, cannot use \'+\'.");
                return false;
            }

            auto nextCh = filter[pos + 1];
            if ((nextCh != '\0') && (nextCh != TopicSep)) {
                errorLog("Single-level wildcard \'+\' must be last of followed by /.");
                return false;                
            }           

            if (pos == 0U) {
                continue;
            }

            if ((lastSep < 0) || (static_cast<decltype(lastSep)>(pos - 1U) != lastSep)) {
                errorLog("Single-level wildcard \'+\' must follow separator.");
                return false;
            }            
        }

        return true;
    }
    else {
        [[maybe_unused]] static constexpr bool ShouldNotBeCalled = false;
        COMMS_ASSERT(ShouldNotBeCalled);
        return false;
    }
}

bool Op::verifyPubTopicInternal(const char* topic, bool outgoing)
{
    if (Config::HasTopicFormatVerification) {
        if (outgoing && (!m_client.configState().m_verifyOutgoingTopic)) {
            return true;
        }

        if ((!outgoing) && (!m_client.configState().m_verifyIncomingTopic)) {
            return true;
        }

        COMMS_ASSERT(topic != nullptr);
        if (topic[0] == '\0') {
            return false;
        }

        if (outgoing && (topic[0] == '$')) {
            errorLog("Cannot start topic with \'$\'.");
            return false;
        }

        auto pos = 0U;
        while (topic[pos] != '\0') {
            auto incPosGuard = 
                comms::util::makeScopeGuard(
                    [&pos]()
                    {
                        ++pos;
                    });

            auto ch = topic[pos];

            if ((ch == MultLevelWildcard) || 
                (ch == SingleLevelWildcard)) {
                errorLog("Wildcards cannot be used in publish topic");
                return false;
            }
        }

        return true;
    }
    else {
        [[maybe_unused]] static constexpr bool ShouldNotBeCalled = false;
        COMMS_ASSERT(ShouldNotBeCalled);
        return false;
    }
}

} // namespace op

} // namespace cc_mqtt5_client
