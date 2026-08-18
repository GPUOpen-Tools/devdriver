/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddDevModeControlDevice.h>
#include <ddDevModeQueue.h>
#include <listener/transports/abstractListenerTransport.h>
#include <listener/transportThread.h>

namespace DevDriver
{
    class RouterCore;

    class LocalNgListenerTransport : public IListenerTransport
    {
    public:
        LocalNgListenerTransport(const AllocCb& allocCb, RouterPrefix listenerPrefix, RouterPrefix kmdPrefix);
        ~LocalNgListenerTransport() override;

        DD_NODISCARD Result ReceiveMessage(ConnectionInfo& connectionInfo, MessageBuffer& message, uint32 timeoutInMs) override;
        DD_NODISCARD Result TransmitMessage(const ConnectionInfo& connectionInfo, const MessageBuffer& message) override;
        DD_NODISCARD Result TransmitBroadcastMessage(const MessageBuffer& message) override;

        DD_NODISCARD Result Enable(RouterCore *pRouter, TransportHandle handle) override;
        DD_NODISCARD Result Disable() override;

        DD_NODISCARD Result GetKernelDevDriverVersion(uint32 &version, DevDriver::DeveloperModeFlags &features);

        TransportHandle GetHandle() override { return m_transportHandle; }
        bool ForwardingConnection() override { return true; }
        const char* GetTransportName() override { return "Local Ng"; }

    protected:
        AllocCb              m_allocCb;
        SharedQueue          m_sharedQueue;
        TransportThread      m_transportThread; // This thread is only required to handle some crazy caching stuff which
                                                // will be unnecessary once the listener core is replaced.
        DevModeControlDevice m_devModeControlDevice;
        RouterCore*          m_pRouter;
        TransportHandle      m_transportHandle;
        const RouterPrefix   m_listenerPrefix;
        const RouterPrefix   m_kmdPrefix;
        bool                 m_isEnabled;

        DD_STATIC_CONST uint32 kTransmitTimeoutInMs = 50;
        DD_STATIC_CONST uint32 kReceiveTimeoutInMs = 50;
    };
} // DevDriver
