/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <gpuopen.h>
#include <ddDevModeControl.h>
#include <ddAmdLogInterface.h>
#include <dd_event/common.h>

namespace DevDriver
{
    // Flags for configuring the router behavior
    union RouterConfigFlags
    {
        struct
        {
            uint32 enableKernelTransport : 1;  // Enables a special transport that allows clients to communicate across
                                               // the user mode / kernel mode boundary
            uint32 enableServer          : 1;  // Enables the built-in router server which allows the router to
                                               // communicate at an application protocol level with other clients on
                                               // the bus
            uint32 reserved              : 30; // Reserved for future usage
        };
        uint32     value;
    };

    // An address and port pair that the router can listen for connections on
    struct RouterBindAddress
    {
        const char* pHostAddress; // Network host address
        uint32      port;         // Network port
    };

    // Creation information for the built in router server.
    struct RouterServerCreateInfo
    {
        ProtocolFlags enabledProtocols;
    };

    // Function pointers to allow devdriver to call to kernel when devmode is enable/disable
    typedef void (*pfnNotifyKernalEnable) (void);
    typedef void (*pfnNotifyKernalDisable) (void);

    // Creation information for the router object
    struct RouterCreateInfo
    {
        const char*              pDescription;     // Description string used to identify the router on the message bus
        RouterConfigFlags        flags;            // Configuration flags
        RouterServerCreateInfo   serverCreateInfo; // Creation information for the built in router server
        const RouterBindAddress* pAddressToBind;   // An network address to receive connections on
        AllocCb                  allocCb;          // An allocation callback that is used to manage memory allocations
        const char*              pPrivateBusId;    // Private message bus id
        pfnNotifyKernalEnable    kernalEnableCb;   // Callback called when enabled
        pfnNotifyKernalDisable   kernalDisableCb;  // Callback called when disabled
    };

    class IService;

    class IRouter
    {
    public:
        /// Process a DevMode command from the process `processId`
        ///
        /// See ddDevModeControlCmds.h for more details.
        virtual Result ProcessDevModeCmd(ProcessId processId, DevModeCmd cmd, size_t bufferSize, void* pBuffer) = 0;

        /// Called when a process is closed.
        ///
        /// This is used to handle cleanup when devdriver is disconnected without properly shutting it down.
        virtual void OnProcessClose(ProcessId processId) = 0;

        /// Called to send a notification to the KMD via the callback object.
        ///
        /// See the definition of DEVDRIVER_CBOBJ_NOTIFICATION for more details.
        virtual void SendNotificationToKmd(uint32_t notificationType, void *pData) = 0;

    protected:
        IRouter() {};
        virtual ~IRouter() {}
    };

    // Create a new router object
    //
    // Allocates the memory required for the object and initializes it before returning
    // To register services, use the overload with the service slice input
    Result CreateRouter(const RouterCreateInfo& createInfo, IRouter** ppRouter);
    Result CreateRouter(const RouterCreateInfo& createInfo, IRouter** ppRouter, size_t servicesCount, IService* const* pServices);

    // Destroy an existing router object
    // Frees the memory for the object and consumes the ppRouter pointer before returning
    void DestroyRouter(IRouter** ppRouter);

} // DevDriver
