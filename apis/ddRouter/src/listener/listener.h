/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <gpuopen.h>
#include <ddPlatform.h>

namespace DevDriver
{
    class IMsgChannel;
    class IListener;

    // Flags for configuring the listener behavior
    union ListenerConfigFlags
    {
        struct
        {
            uint32 enableKernelTransport        : 1;  // Enables a special transport that allows clients to communicate across
                                                      // the user mode / kernel mode boundary
            uint32 enableEmbeddedClient         : 1;  // Enables the kernel version of the built-in listener server
            uint32 kernelTransportKMDOnly       : 1;  // If kernel transport is enabled then the only kernel transport enabled will be KMD
            uint32 reserved                     : 28; // Reserved for future usage
        };
        uint32     value;
    };

    // An address and port pair that the listener can listen for connections on
    struct ListenerBindAddress
    {
        const char* pHostAddress; // Network host address
        uint16      port;         // Network port
    };

    // Creation information for the listener object
    struct ListenerCreateInfo
    {
        const char*          pDescription;       // Description string used to identify the listener on the message bus
        uint16               localPort;          // An identifier for local inter-process communication
        ListenerConfigFlags  flags;              // Configuration flags
        ListenerBindAddress* pAddressesToBind;   // A list of addresses to listen for connections on
        uint32               numAddresses;       // The number of entries in pAddressesToBind
        AllocCb              allocCb;            // An allocation callback that is used to manage memory allocations
        uint32               clientTimeoutCount; // Number of pings to give the client driver before disconnecting
    };

    // Create a new listener object
    Result CreateListener(const ListenerCreateInfo& createInfo, IListener** ppListener);

    class IStructuredWriter;

    class IListener
    {
    public:
        virtual ~IListener() {}

        virtual void Destroy() = 0;

    protected:
        IListener() {};
    };

} // DevDriver
