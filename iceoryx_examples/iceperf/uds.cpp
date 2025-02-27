// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2022 by Apex.AI Inc. All rights reserved.
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

#include "uds.hpp"
#include "iceoryx_hoofs/cxx/helplets.hpp"
#include "iceoryx_hoofs/posix_wrapper/posix_call.hpp"

#include <chrono>
#include <thread>

UDS::UDS(const std::string& publisherName, const std::string& subscriberName) noexcept
    : m_publisherSocketName(PREFIX + publisherName)
    , m_subscriberSocketName(PREFIX + subscriberName)
{
    initSocketAddress(m_sockAddrPublisher, m_publisherSocketName);
    initSocketAddress(m_sockAddrSubscriber, m_subscriberSocketName);
}

void UDS::initSocketAddress(sockaddr_un& socketAddr, const std::string& socketName)
{
    memset(&socketAddr, 0, sizeof(sockaddr_un));
    socketAddr.sun_family = AF_LOCAL;
    constexpr uint64_t NULL_TERMINATION_SIZE{1};
    const uint64_t maxDestinationLength = iox::cxx::arrayCapacity(socketAddr.sun_path) - NULL_TERMINATION_SIZE;
    iox::cxx::Ensures(maxDestinationLength >= socketName.size() && "Socketname too large!");
    strncpy(socketAddr.sun_path, socketName.c_str(), socketName.size());
}

void UDS::cleanupOutdatedResources(const std::string& publisherName, const std::string& subscriberName) noexcept
{
    auto publisherSocketName = PREFIX + publisherName;
    sockaddr_un sockAddrPublisher;
    initSocketAddress(sockAddrPublisher, publisherSocketName);
    iox::posix::posixCall(unlink)(sockAddrPublisher.sun_path)
        .failureReturnValue(ERROR_CODE)
        .ignoreErrnos(ENOENT)
        .evaluate()
        .or_else([](auto& r) {
            std::cout << "unlink error " << r.getHumanReadableErrnum() << std::endl;
            exit(1);
        });

    auto subscriberSocketName = PREFIX + subscriberName;
    sockaddr_un sockAddrSubscriber;
    initSocketAddress(sockAddrSubscriber, subscriberSocketName);

    iox::posix::posixCall(unlink)(sockAddrSubscriber.sun_path)
        .failureReturnValue(ERROR_CODE)
        .ignoreErrnos(ENOENT)
        .evaluate()
        .or_else([](auto& r) {
            std::cout << "unlink error " << r.getHumanReadableErrnum() << std::endl;
            exit(1);
        });
}

void UDS::initLeader() noexcept
{
    init();

    std::cout << "waiting for follower" << std::endl;
    waitForFollower();

    receivePerfTopic();
}

void UDS::initFollower() noexcept
{
    init();

    std::cout << "registering with the leader" << std::endl;
    waitForLeader();

    sendPerfTopic(sizeof(PerfTopic), RunFlag::RUN);
}

void UDS::init() noexcept
{
    // init subscriber
    iox::posix::posixCall(iox_socket)(AF_LOCAL, SOCK_DGRAM, 0)
        .failureReturnValue(ERROR_CODE)
        .evaluate()
        .and_then([this](auto& r) { m_sockfdSubscriber = r.value; })
        .or_else([](auto& r) {
            std::cout << "socket error " << r.getHumanReadableErrnum() << std::endl;
            exit(1);
        });

    iox::posix::posixCall(iox_bind)(
        m_sockfdSubscriber, reinterpret_cast<struct sockaddr*>(&m_sockAddrSubscriber), sizeof(m_sockAddrSubscriber))
        .failureReturnValue(ERROR_CODE)
        .evaluate()
        .or_else([](auto& r) {
            std::cout << "bind error " << r.getHumanReadableErrnum() << std::endl;
            exit(1);
        });

    // init publisher
    iox::posix::posixCall(iox_socket)(AF_LOCAL, SOCK_DGRAM, 0)
        .failureReturnValue(ERROR_CODE)
        .evaluate()
        .and_then([this](auto& r) { m_sockfdPublisher = r.value; })
        .or_else([](auto& r) {
            std::cout << "socket error " << r.getHumanReadableErrnum() << std::endl;
            exit(1);
        });
}

void UDS::waitForLeader() noexcept
{
    // try to send an empty message
    constexpr bool TRY_TO_SEND{true};
    while (TRY_TO_SEND)
    {
        auto sendCall = iox::posix::posixCall(iox_sendto)(m_sockfdPublisher,
                                                          nullptr,
                                                          0,
                                                          0,
                                                          reinterpret_cast<struct sockaddr*>(&m_sockAddrPublisher),
                                                          sizeof(m_sockAddrPublisher))
                            .failureReturnValue(ERROR_CODE)
                            .ignoreErrnos(ENOENT)
                            .evaluate()
                            .or_else([](auto& r) {
                                std::cout << "send error " << r.getHumanReadableErrnum() << std::endl;
                                exit(1);
                            })
                            .value();

        if (sendCall.errnum == ENOENT)
        {
            constexpr std::chrono::milliseconds RETRY_INTERVAL{10};
            std::this_thread::sleep_for(RETRY_INTERVAL);
            continue;
        }

        break;
    }
}

void UDS::waitForFollower() noexcept
{
    // try to receive the empty message
    receive(m_message);
}

void UDS::shutdown() noexcept
{
    if (m_sockfdPublisher != INVALID_FD)
    {
        iox::posix::posixCall(iox_close)(m_sockfdPublisher)
            .failureReturnValue(ERROR_CODE)
            .evaluate()
            .or_else([](auto& r) {
                std::cout << "close error " << r.getHumanReadableErrnum() << std::endl;
                exit(1);
            });
    }

    if (m_sockfdSubscriber != INVALID_FD)
    {
        iox::posix::posixCall(iox_closesocket)(m_sockfdSubscriber)
            .failureReturnValue(ERROR_CODE)
            .evaluate()
            .or_else([](auto& r) {
                std::cout << "close error " << r.getHumanReadableErrnum() << std::endl;
                exit(1);
            });

        iox::posix::posixCall(unlink)(m_sockAddrSubscriber.sun_path)
            .failureReturnValue(ERROR_CODE)
            .evaluate()
            .or_else([](auto& r) {
                std::cout << "unlink error " << r.getHumanReadableErrnum() << std::endl;
                exit(1);
            });
    }
}

void UDS::sendPerfTopic(const uint32_t payloadSizeInBytes, const RunFlag runFlag) noexcept
{
    char* buffer = new char[payloadSizeInBytes];
    auto sample = reinterpret_cast<PerfTopic*>(&buffer[0]);

    // Specify the payload size for the measurement
    sample->payloadSize = payloadSizeInBytes;
    sample->runFlag = runFlag;
    if (payloadSizeInBytes <= MAX_MESSAGE_SIZE)
    {
        sample->subPackets = 1;
        send(&buffer[0], payloadSizeInBytes);
    }
    else
    {
        sample->subPackets = payloadSizeInBytes / MAX_MESSAGE_SIZE;
        for (uint32_t i = 0U; i < sample->subPackets; ++i)
        {
            send(&buffer[0], MAX_MESSAGE_SIZE);
        }
    }
    delete[] buffer;
}

PerfTopic UDS::receivePerfTopic() noexcept
{
    receive(&m_message[0]);

    auto receivedSample = reinterpret_cast<const PerfTopic*>(&m_message[0]);

    if (receivedSample->subPackets > 1)
    {
        for (uint32_t i = 0U; i < receivedSample->subPackets - 1; ++i)
        {
            receive(&m_message[0]);
        }
    }

    return *receivedSample;
}

void UDS::send(const char* buffer, uint32_t length) noexcept
{
    // only return from this loop when the message could be send successfully
    // if the OS socket message buffer if full, retry until it is free'd by
    // the OS and the message could be send
    while (iox::posix::posixCall(iox_sendto)(m_sockfdPublisher,
                                             buffer,
                                             length,
                                             0,
                                             reinterpret_cast<struct sockaddr*>(&m_sockAddrPublisher),
                                             sizeof(m_sockAddrPublisher))
               .failureReturnValue(ERROR_CODE)
               .ignoreErrnos(ENOBUFS)
               .evaluate()
               .or_else([](auto& r) {
                   std::cout << std::endl << "send error " << r.getHumanReadableErrnum() << std::endl;
                   exit(1);
               })
               ->errnum
           == ENOBUFS)
    {
    }
}

void UDS::receive(char* buffer) noexcept
{
    iox::posix::posixCall(iox_recvfrom)(m_sockfdSubscriber, buffer, MAX_MESSAGE_SIZE, 0, nullptr, nullptr)
        .failureReturnValue(ERROR_CODE)
        .evaluate()
        .or_else([](auto& r) {
            std::cout << "receive error " << r.getHumanReadableErrnum() << std::endl;
            exit(1);
        });
}
