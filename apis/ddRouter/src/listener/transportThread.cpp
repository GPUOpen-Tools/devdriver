/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <listener/transportThread.h>
#include <listener/routerCore.h>
#include <ddPlatform.h>

namespace DevDriver
{
    void TransportThread::ReceiveThreadFunc(RouterCore *pRouter, IListenerTransport *pTransport)
    {
        if ((pRouter != nullptr) & (pTransport != nullptr))
        {
            RoutingCache cache(pRouter);
            MessageContext recvMsgContext = {};
            std::deque<MessageContext> recvQueue;
            std::deque<MessageContext> retryQueue;

            while (m_active)
            {
                size_t firstNewMessageIndex = recvQueue.size();
                // Check for new local messages.
                Result readResult = pTransport->ReceiveMessage(recvMsgContext.connectionInfo, recvMsgContext.message, kReceiveDelayInMs);
                if (readResult == Result::Success)
                {
                    do
                    {
                        recvQueue.emplace_back(recvMsgContext);
                        readResult = pTransport->ReceiveMessage(recvMsgContext.connectionInfo, recvMsgContext.message, kNoWait);
                    } while (readResult == Result::Success);
                }

                size_t messageNumber = 0;
                for (const auto &message : recvQueue)
                {
                    messageNumber++;
                    // only requeue messages if it's the first time we've tried to send them
                    if ((cache.RouteMessage(message) == Result::NotReady) & (messageNumber > firstNewMessageIndex))
                    {
                        retryQueue.emplace_back(message);
                    }
                }
                recvQueue.clear();
                recvQueue.swap(retryQueue);
            }
        }
    }

    void TransportThread::Start(RouterCore *pRouter, IListenerTransport *pTransport)
    {
        DD_ASSERT(m_active == false);
        m_active = true;
        // TODO: Replace this with Platform::Thread
        m_thread = std::thread(&DevDriver::TransportThread::ReceiveThreadFunc, this, pRouter, pTransport);
        DD_ASSERT(m_thread.joinable());
    }

    void TransportThread::Stop()
    {
        if (m_active)
        {
            m_active = false;
            if (m_thread.joinable())
                m_thread.join();
        }
    }

    TransportThread::TransportThread() :
        m_active(false)
    {
    }

    TransportThread::~TransportThread()
    {
        if (m_active)
            Stop();
    }
} // DevDriver
