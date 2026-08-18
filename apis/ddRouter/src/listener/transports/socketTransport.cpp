/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <listener/transports/socketTransport.h>
#include <ddPlatform.h>
#include <listener/routerCore.h>

namespace DevDriver
{
    // Take a TransportType and find the associated SocketType for the current platform
    inline static SocketType TransportToSocketType(TransportType type)
    {
        SocketType result = SocketType::Unknown;
        switch (type)
        {
#if !defined(DD_PLATFORM_WINDOWS_UM)
        case TransportType::Local:
            result = SocketType::Local;
            break;
#endif
        case TransportType::Remote:
            result = SocketType::Udp;
            break;
        default:
            DD_WARN_REASON("Invalid transport type specified");
            break;
        }
        return result;
    }

    SocketListenerTransport::SocketListenerTransport(const HostInfo& hostInfo) :
        m_socketType(TransportToSocketType(hostInfo.type)),
        m_port(hostInfo.port),
        m_listening(false)
    {
        if (m_socketType == SocketType::Local)
        {
            // The hostname field should always be nullptr for local sockets
            DD_ASSERT(hostInfo.pHostname == nullptr);

            Platform::Snprintf(m_hostDescription, sizeof(m_hostDescription), "Local:%hu", m_port);
        }
        else if (m_socketType == SocketType::Udp)
        {
            Platform::Strncpy(m_hostname, hostInfo.pHostname);

            Platform::Snprintf(m_hostDescription, sizeof(m_hostDescription), "Remote:%u", m_port);
        }
        else
        {
            // Invalid type specified
            Platform::Snprintf(m_hostDescription, sizeof(m_hostDescription), "Unknown");
            DD_WARN_REASON("Unknown socket type requested");
        }
    }

    SocketListenerTransport::~SocketListenerTransport()
    {
        if (m_listening)
            Disable();
    }

    Result SocketListenerTransport::ReceiveMessage(ConnectionInfo& connectionInfo, MessageBuffer& message, uint32 timeoutInMs)
    {
        bool canRead = false;
        bool exceptState = false;
        connectionInfo.handle = m_transportHandle;
        Result result = m_clientSocket.Select(&canRead, nullptr, &exceptState, timeoutInMs);
        if (result == Result::Success)
        {
            if (exceptState)
            {
                result = Result::Error;
            }
            else if (canRead)
            {
                connectionInfo.size = sizeof(connectionInfo.data);

                size_t bytesReceived = 0;
                result = m_clientSocket.ReceiveFrom(reinterpret_cast<void *>(&connectionInfo.data[0]),
                    &connectionInfo.size,
                    reinterpret_cast<uint8*>(&message),
                    sizeof(MessageBuffer),
                    &bytesReceived);

                if (result == Result::Success)
                {
                    result = ValidateMessageBuffer(&message, bytesReceived);
                }
            }
            else
            {
                result = Result::NotReady;
            }
        }
        return result;
    }

    Result SocketListenerTransport::TransmitMessage(const ConnectionInfo& connectionInfo, const MessageBuffer& message)
    {
        Result result = Result::Error;

        if ((connectionInfo.handle == m_transportHandle) && (message.header.payloadSize <= kMaxPayloadSizeInBytes))
        {
            const size_t totalMsgSize = (sizeof(MessageHeader) + message.header.payloadSize);

            size_t bytesSent = 0;
            result = m_clientSocket.SendTo(reinterpret_cast<const void*>(&connectionInfo.data[0]),
                connectionInfo.size,
                reinterpret_cast<const uint8*>(&message),
                totalMsgSize,
                &bytesSent);

            if (result == Result::Success)
            {
                result = (bytesSent == totalMsgSize) ? Result::Success : Result::Error;
            }
        }

        return result;
    }

    Result SocketListenerTransport::TransmitBroadcastMessage(const MessageBuffer& message)
    {
        DD_UNUSED(message);

        return Result::Error;
    }

    Result SocketListenerTransport::Enable(RouterCore *pRouter, TransportHandle handle)
    {
        Result result = Result::Error;

        if (m_clientSocket.Init(true, m_socketType) == DevDriver::Result::Success)
        {
            // Use the "all interfaces" address for remote sockets
            // Local sockets use the address as a prefix instead
            const char* pAddress = (m_socketType == SocketType::Udp) ? m_hostname : "AMD-Developer-Service";
            if (m_clientSocket.Bind(pAddress, m_port) == Result::Success)
            {
                result = Result::Success;
                m_transportHandle = handle;
                m_transportThread.Start(pRouter, this);
            }
        }
        return result;
    }

    Result SocketListenerTransport::Disable()
    {
        Result result = Result::Error;
        if (m_transportHandle != 0)
        {
            m_transportHandle = 0;
            m_transportThread.Stop();
            result = Result::Success;
        }
        return result;
    }
} // DevDriver
