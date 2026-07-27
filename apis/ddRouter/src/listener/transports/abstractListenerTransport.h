/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <gpuopen.h>

namespace DevDriver
{
    const uint32 kConnectionInfoDataSize = 128;
    typedef uint32 TransportHandle;

    // A structure that holds information about an associated bus message
    // Required to send or receive messages on the transport
    struct ConnectionInfo
    {
        char            data[kConnectionInfoDataSize]; // Used to store extra information about the associated message
        size_t          size;                          // The number of bytes stored in the data field
        TransportHandle handle;                        // Identifies which transport the associated message is intended
                                                       // for or what transport it originated from.
    };

    class RouterCore;

    class IListenerTransport
    {
    public:
        virtual ~IListenerTransport() {}

        virtual Result Enable(RouterCore* pRouter, TransportHandle handle) = 0;
        virtual Result ReceiveMessage(ConnectionInfo &connectionInfo, MessageBuffer &message, uint32 timeoutInMs) = 0;
        virtual Result TransmitMessage(const ConnectionInfo &connectionInfo, const MessageBuffer &message) = 0;
        virtual Result TransmitBroadcastMessage(const MessageBuffer &message) = 0;
        virtual Result Disable() = 0;

        virtual TransportHandle GetHandle() = 0;
        virtual bool ForwardingConnection() = 0;
        virtual const char* GetTransportName() = 0;

    protected:
        IListenerTransport() {}
    };
} // DevDriver
