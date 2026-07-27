/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <gpuopen.h>

#include <listener/transports/abstractListenerTransport.h>

#include <thread>

namespace DevDriver
{
    class TransportThread
    {
    public:
        TransportThread();
        ~TransportThread();

        void Start(class RouterCore *pListener, IListenerTransport *pTransport);
        void Stop();
    private:
        void ReceiveThreadFunc(RouterCore *pRouter, IListenerTransport *pTransport);
        DD_STATIC_CONST uint32 kReceiveDelayInMs = 25;
        std::thread         m_thread;
        volatile bool       m_active;
    };
} // DevDriver
