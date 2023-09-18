#pragma once

#include "UnitTestDefs.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <vector>

class UnitTestCommonBase
{

protected:
    void unitTestSetUp()
    {
        assert(!m_client);
        m_tickReq.clear();
        m_sentData.clear();
    }

    void unitTestTearDown()
    {
        m_client.reset();
    }

    UnitTestClientPtr::pointer unitTestAllocClient()
    {
        m_client.reset(cc_mqtt5_client_new());
        cc_mqtt5_client_set_broker_disconnect_report_callback(m_client.get(), &UnitTestCommonBase::unitTestBrokerDisconnectedCb, this);
        cc_mqtt5_client_set_send_output_data_callback(m_client.get(), &UnitTestCommonBase::unitTestSendOutputDataCb, this);
        cc_mqtt5_client_set_next_tick_program_callback(m_client.get(), &UnitTestCommonBase::unitTestProgramNextTickCb, this);
        cc_mqtt5_client_set_cancel_next_tick_wait_callback(m_client.get(), &UnitTestCommonBase::unitTestCancelNextTickWaitCb, this);
        auto ec = cc_mqtt5_client_init(m_client.get());
        assert(ec == CC_Mqtt5ErrorCode_Success);
        return m_client.get();
    }

    decltype(auto) unitTestSentData()
    {
        return m_sentData;
    }

    decltype(auto) unitTestTickReq()
    {
        assert(!m_tickReq.empty());
        return m_tickReq.front();
    }

    bool unitTestCheckNoTicks()
    {
        return m_tickReq.empty();
    }

    void unitTestTick(unsigned ms = 0, bool forceTick = false)
    {
        assert(!m_tickReq.empty());
        auto& tick = m_tickReq.front();
        auto rem = tick.m_requested - tick.m_elapsed;
        assert(ms <= rem);
        if (ms == 0U) {
            ms = rem;
        }

        if (ms < rem) {
            tick.m_elapsed += ms;

            if (!forceTick) {
                return;
            }
        }

        m_tickReq.erase(m_tickReq.begin());
        assert(m_client);
        cc_mqtt5_client_tick(m_client.get(), ms);
    }

private:
    struct TickInfo
    {
        unsigned m_requested = 0U;
        unsigned m_elapsed = 0U;
    };

    static void unitTestBrokerDisconnectedCb(void* obj, const CC_Mqtt5DisconnectInfo* info)
    {
        static_cast<void>(obj);
        static_cast<void>(info);
        // TODO:
    }

    static void unitTestSendOutputDataCb(void* obj, const unsigned char* buf, unsigned bufLen)
    {
        auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
        std::copy_n(buf, bufLen, std::back_inserter(realObj->m_sentData));
    }

    static void unitTestProgramNextTickCb(void* obj, unsigned duration)
    {
        auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
        assert(realObj->m_tickReq.empty());
        realObj->m_tickReq.push_back({duration, 0U});
    }

    static unsigned unitTestCancelNextTickWaitCb(void* obj) {
        auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
        assert(!realObj->m_tickReq.empty());
        auto elapsed = realObj->m_tickReq.front().m_elapsed;
        realObj->m_tickReq.erase(realObj->m_tickReq.begin());
        return elapsed;
    }

    UnitTestClientPtr m_client;
    std::vector<TickInfo> m_tickReq;
    std::vector<std::uint8_t> m_sentData;
};