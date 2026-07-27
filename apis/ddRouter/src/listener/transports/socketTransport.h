/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <listener/transports/abstractListenerTransport.h>
#include <listener/transportThread.h>
#include <ddAbstractSocket.h>

namespace DevDriver
{
    static_assert(sizeof(sockaddr) <= kConnectionInfoDataSize, "ConnectionInfo struct not large enough to hold address information");

    class RouterCore;

    class SocketListenerTransport : public IListenerTransport
    {
    public:
        SocketListenerTransport(const HostInfo& hostInfo);
        ~SocketListenerTransport() override;

        Result ReceiveMessage(ConnectionInfo &connectionInfo, MessageBuffer &message, uint32 timeoutInMs) override;
        Result TransmitMessage(const ConnectionInfo &connectionInfo, const MessageBuffer &message) override;
        Result TransmitBroadcastMessage(const MessageBuffer &message) override;

        Result Enable(RouterCore *pRouter, TransportHandle handle) override;
        Result Disable() override;

        TransportHandle GetHandle() override { return m_transportHandle; };
        bool ForwardingConnection() override { return false; };
        const char* GetTransportName() override { return m_hostDescription; };

    protected:
        char            m_hostname[kMaxStringLength];
        char            m_hostDescription[kMaxStringLength];
        Socket          m_clientSocket;
        SocketType      m_socketType;
        uint16          m_port;
        TransportHandle m_transportHandle;
        bool            m_listening;
        TransportThread m_transportThread;
    };
} // DevDriver
