// Copyright (c) 2022 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "iceoryx_hoofs/cxx/optional.hpp"
#include "iceoryx_posh/internal/popo/base_client.hpp"
#include "iceoryx_posh/testing/mocks/posh_runtime_mock.hpp"
#include "mocks/client_mock.hpp"
#include "mocks/trigger_handle_mock.hpp"

#include "test.hpp"

namespace
{
using namespace ::testing;
using namespace iox;
using namespace iox::capro;
using namespace iox::cxx;
using namespace iox::mepoo;
using namespace iox::popo;
using namespace iox::runtime;
using ::testing::_;

using BaseClientWithMocks = BaseClient<MockClientPortUser, MockTriggeHandle>;

class TestBaseClient : public BaseClientWithMocks
{
  public:
    TestBaseClient(ServiceDescription sd, ClientOptions options)
        : BaseClientWithMocks::BaseClient(sd, options)
    {
    }

    using BaseClientWithMocks::port;

    using BaseClientWithMocks::disableEvent;
    using BaseClientWithMocks::disableState;
    using BaseClientWithMocks::enableEvent;
    using BaseClientWithMocks::enableState;
    using BaseClientWithMocks::getCallbackForIsStateConditionSatisfied;
    using BaseClientWithMocks::invalidateTrigger;
    using BaseClientWithMocks::m_trigger;
};

class BaseClient_test : public Test
{
  public:
    void SetUp() override
    {
        // we only need one non default option to check whether they are correctly passed to the underlying port
        options.nodeName = "engage";

        // the default ctor is used in the getMiddlewareClient call
        PortConfigInfo portInfo;
        MemoryManager memoryManager;
        ClientPortData portData{sd, runtimeName, options, &memoryManager, portInfo.memoryInfo};
        EXPECT_CALL(*mockRuntime, getMiddlewareClient(sd, options, portInfo)).WillOnce(Return(&portData));

        sut.emplace(sd, options);
    }

    void TearDown() override
    {
        if (sut.has_value())
        {
            EXPECT_CALL(sut->port(), destroy).Times(1);
            sut.reset();
        }
    }

    iox::RuntimeName_t runtimeName{"HYPNOTOAD"};
    std::unique_ptr<PoshRuntimeMock> mockRuntime = PoshRuntimeMock::create(runtimeName);

    ServiceDescription sd{"make", "it", "so"};
    iox::popo::ClientOptions options;
    optional<TestBaseClient> sut;
};

TEST_F(BaseClient_test, DestructorCallsDestroyOnUnderlyingPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "fa8f6649-7889-41b1-867a-591cef414075");

    EXPECT_CALL(sut->port(), destroy).Times(1);

    sut.reset(); // reset from the optional calls the dtor of the inner type
}

TEST_F(BaseClient_test, GetUidCallsUnderlyingPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "4c1f401c-9ee2-40f9-8f97-2ae7dae594b3");

    const UniquePortId uid;
    EXPECT_CALL(sut->port(), getUniqueID).WillOnce(Return(uid));

    EXPECT_THAT(sut->getUid(), Eq(uid));
}

TEST_F(BaseClient_test, GetServiceDescriptionCallsUnderlyingPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "d2d46bbe-479e-4c7b-9068-7c1003584c2f");

    EXPECT_CALL(sut->port(), getCaProServiceDescription).WillOnce(ReturnRef(sd));

    EXPECT_THAT(sut->getServiceDescription(), Eq(sd));
}

TEST_F(BaseClient_test, ConnectCallsUnderlyingPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "3e364583-c26b-4ba0-b55f-5121b4ed1b5f");

    EXPECT_CALL(sut->port(), connect).Times(1);

    sut->connect();
}

TEST_F(BaseClient_test, GetConnectionStateCallsUnderlyingPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "f093652b-421b-43e1-b69a-6bde15f18e6d");

    constexpr ConnectionState CONNECTION_STATE{ConnectionState::WAIT_FOR_OFFER};
    EXPECT_CALL(sut->port(), getConnectionState).WillOnce(Return(CONNECTION_STATE));

    EXPECT_THAT(sut->getConnectionState(), Eq(CONNECTION_STATE));
}

TEST_F(BaseClient_test, DisconnectCallsUnderlyingPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "025b478a-c9b7-4f08-821f-f3f4abdc6f65");

    EXPECT_CALL(sut->port(), disconnect).Times(1);

    sut->disconnect();
}

TEST_F(BaseClient_test, HasResponsesCallsUnderlyingPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "8d50f56a-a489-4c5c-9d17-c966fb7e171c");

    constexpr bool HAS_RESPONSES{true};
    EXPECT_CALL(sut->port(), hasNewResponses).WillOnce(Return(HAS_RESPONSES));

    EXPECT_THAT(sut->hasResponses(), Eq(HAS_RESPONSES));
}

TEST_F(BaseClient_test, HasMissedResponsesCallsUnderlyingPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "0a0a8bf6-47af-4ce4-acbb-adf7c09513f6");

    constexpr bool HAS_MISSED_RESPONSES{true};
    EXPECT_CALL(sut->port(), hasLostResponsesSinceLastCall).WillOnce(Return(HAS_MISSED_RESPONSES));

    EXPECT_THAT(sut->hasMissedResponses(), Eq(HAS_MISSED_RESPONSES));
}

TEST_F(BaseClient_test, ReleaseQueuedResponsesCallsUnderlyingPort)
{
    ::testing::Test::RecordProperty("TEST_ID", "bd72358c-dc0c-4900-bea5-52be800f1448");

    EXPECT_CALL(sut->port(), releaseQueuedResponses).Times(1);

    sut->releaseQueuedResponses();
}

// BEGIN Listener and WaitSet related test

TEST_F(BaseClient_test, InvalidateTriggerWithFittingTriggerIdCallsUnderlyingPortAndTriggerHandle)
{
    ::testing::Test::RecordProperty("TEST_ID", "6a779c0c-a8b9-4b1c-a98a-5d074a63cea2");

    constexpr uint64_t TRIGGER_ID{13U};

    EXPECT_CALL(sut->m_trigger, getUniqueId).WillOnce(Return(TRIGGER_ID));
    EXPECT_CALL(sut->port(), unsetConditionVariable).Times(1);
    EXPECT_CALL(sut->m_trigger, invalidate).Times(1);

    sut->invalidateTrigger(TRIGGER_ID);
}

TEST_F(BaseClient_test, InvalidateTriggerWithUnfittingTriggerIdDoesNotCallUnderlyingPortAndTriggerHandle)
{
    ::testing::Test::RecordProperty("TEST_ID", "98165eac-4a34-4dcc-b945-d2b60ff38541");

    constexpr uint64_t TRIGGER_ID_1{1U};
    constexpr uint64_t TRIGGER_ID_2{2U};

    EXPECT_CALL(sut->m_trigger, getUniqueId).WillOnce(Return(TRIGGER_ID_2));
    EXPECT_CALL(sut->port(), unsetConditionVariable).Times(0);
    EXPECT_CALL(sut->m_trigger, invalidate).Times(0);

    sut->invalidateTrigger(TRIGGER_ID_1);
}

TEST_F(BaseClient_test, EnableStateCallsUnderlyingPortAndTriggerHandle)
{
    ::testing::Test::RecordProperty("TEST_ID", "43277404-5391-4d8f-a651-cad5ed50777c");

    for (const bool clientAttachedIndicator : {false, true})
    {
        SCOPED_TRACE(std::string("Test 'enableState' with client ")
                     + (clientAttachedIndicator ? "attached" : " not attached"));

        const uint64_t TRIGGER_ID{clientAttachedIndicator ? 42U : 73U};
        MockTriggeHandle triggerHandle;
        triggerHandle.triggerId = TRIGGER_ID;
        ConditionVariableData condVar{runtimeName};

        EXPECT_THAT(sut->m_trigger.triggerId, Ne(TRIGGER_ID));

        EXPECT_CALL(sut->m_trigger, operatorBoolMock).WillOnce(Return(clientAttachedIndicator));
        EXPECT_CALL(sut->m_trigger, getConditionVariableData).WillOnce(Return(&condVar));
        EXPECT_CALL(sut->m_trigger, getUniqueId).WillOnce(Return(TRIGGER_ID));

        EXPECT_CALL(sut->port(), setConditionVariable(Ref(condVar), TRIGGER_ID)).Times(1);

        bool errorDetected{false};
        auto errorHandlerGuard = iox::ErrorHandler::setTemporaryErrorHandler([&](const iox::Error error,
                                                                                 const std::function<void()>,
                                                                                 const iox::ErrorLevel errorLevel) {
            EXPECT_THAT(
                error,
                Eq(iox::Error::
                       kPOPO__BASE_CLIENT_OVERRIDING_WITH_STATE_SINCE_HAS_RESPONSE_OR_RESPONSE_RECEIVED_ALREADY_ATTACHED));
            EXPECT_THAT(errorLevel, Eq(iox::ErrorLevel::MODERATE));
            errorDetected = true;
        });

        sut->enableState(std::move(triggerHandle), ClientState::HAS_RESPONSE);

        EXPECT_THAT(sut->m_trigger.triggerId, Eq(TRIGGER_ID));
        EXPECT_THAT(errorDetected, Eq(clientAttachedIndicator));
    }
}

TEST_F(BaseClient_test, GetCallbackForIsStateConditionSatisfiedReturnsCallbackToSelf)
{
    ::testing::Test::RecordProperty("TEST_ID", "8e0bcb91-e4fb-4129-a75a-92e1ef13add4");

    auto callback = sut->getCallbackForIsStateConditionSatisfied(ClientState::HAS_RESPONSE);

    constexpr bool HAS_RESPONSES{true};
    EXPECT_CALL(sut->port(), hasNewResponses).WillOnce(Return(HAS_RESPONSES));
    EXPECT_FALSE(callback().has_error());
}

TEST_F(BaseClient_test, DisableStateCallsUnderlyingPortAndTriggerHandle)
{
    ::testing::Test::RecordProperty("TEST_ID", "3e204a48-37e5-476c-b6b9-4f29a24302e9");

    EXPECT_CALL(sut->m_trigger, reset).Times(1);
    EXPECT_CALL(sut->port(), unsetConditionVariable).Times(1);

    sut->disableState(ClientState::HAS_RESPONSE);
}

TEST_F(BaseClient_test, EnableEventCallsUnderlyingPortAndTriggerHandle)
{
    ::testing::Test::RecordProperty("TEST_ID", "c78ad5f7-5e0b-4fad-86bf-75eb1d762010");

    for (const bool clientAttachedIndicator : {false, true})
    {
        SCOPED_TRACE(std::string("Test 'enableEvent' with client ")
                     + (clientAttachedIndicator ? "attached" : " not attached"));

        const uint64_t TRIGGER_ID{clientAttachedIndicator ? 42U : 73U};
        MockTriggeHandle triggerHandle;
        triggerHandle.triggerId = TRIGGER_ID;
        ConditionVariableData condVar{runtimeName};

        EXPECT_THAT(sut->m_trigger.triggerId, Ne(TRIGGER_ID));

        EXPECT_CALL(sut->m_trigger, operatorBoolMock).WillOnce(Return(clientAttachedIndicator));
        EXPECT_CALL(sut->m_trigger, getConditionVariableData).WillOnce(Return(&condVar));
        EXPECT_CALL(sut->m_trigger, getUniqueId).WillOnce(Return(TRIGGER_ID));

        EXPECT_CALL(sut->port(), setConditionVariable(Ref(condVar), TRIGGER_ID)).Times(1);

        bool errorDetected{false};
        auto errorHandlerGuard = iox::ErrorHandler::setTemporaryErrorHandler([&](const iox::Error error,
                                                                                 const std::function<void()>,
                                                                                 const iox::ErrorLevel errorLevel) {
            EXPECT_THAT(
                error,
                Eq(iox::Error::
                       kPOPO__BASE_CLIENT_OVERRIDING_WITH_EVENT_SINCE_HAS_RESPONSE_OR_RESPONSE_RECEIVED_ALREADY_ATTACHED));
            EXPECT_THAT(errorLevel, Eq(iox::ErrorLevel::MODERATE));
            errorDetected = true;
        });

        sut->enableEvent(std::move(triggerHandle), ClientEvent::RESPONSE_RECEIVED);

        EXPECT_THAT(sut->m_trigger.triggerId, Eq(TRIGGER_ID));
        EXPECT_THAT(errorDetected, Eq(clientAttachedIndicator));
    }
}

TEST_F(BaseClient_test, DisableEventCallsUnderlyingPortAndTriggerHandle)
{
    ::testing::Test::RecordProperty("TEST_ID", "c2f75387-d223-47df-a81c-7d7ab47b9b0d");

    EXPECT_CALL(sut->m_trigger, reset).Times(1);
    EXPECT_CALL(sut->port(), unsetConditionVariable).Times(1);

    sut->disableEvent(ClientEvent::RESPONSE_RECEIVED);
}

// END Listener and WaitSet related test

} // namespace