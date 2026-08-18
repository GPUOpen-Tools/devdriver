/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "ddPlatform.h"
#include "messageChannel.h"

#define DD_SUPPORT_SOCKET_TRANSPORT ((DD_PLATFORM_WINDOWS_UM && DEVDRIVER_BUILD_REMOTE_TRANSPORT)|| (DD_PLATFORM_IS_POSIX))
#if DD_SUPPORT_SOCKET_TRANSPORT
#include "socketMsgTransport.h"
#endif

#if defined(DD_ENABLE_UWP_TRANSPORT)
    #include "localMsgTransport.h"
#endif

#if defined(DD_PLATFORM_WINDOWS_UM)
    #include "win/ddWinPipeMsgTransport.h"

        // TODO: Move the utility driver transport to cross platform includes once it's fully supported
        #include <ddLocalNgMsgTransport.h>
#endif

namespace DevDriver
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create a new message channel object
    Result CreateMessageChannel(const MessageChannelCreateInfo2& createInfo, IMsgChannel** ppMessageChannel)
    {
        Result result = Result::InvalidParameter;

        IMsgChannel* pMsgChannel = nullptr;

        if (ppMessageChannel != nullptr)
        {
            result = Result::InsufficientMemory;

            // Make sure we have reasonable allocator functions before we try to use them
            DD_ASSERT(createInfo.allocCb.pfnAlloc != nullptr);
            DD_ASSERT(createInfo.allocCb.pfnFree != nullptr);

#if defined(DD_PLATFORM_WINDOWS_UM)
            if (createInfo.hostInfo.type == TransportType::Local)
            {
                using MsgChannelPipe = MessageChannel<WinPipeMsgTransport>;
                pMsgChannel = DD_NEW(MsgChannelPipe, createInfo.allocCb)(createInfo.allocCb,
                    createInfo.channelInfo,
                    createInfo.hostInfo);
            }
            else if (createInfo.hostInfo.type == TransportType::Remote)
            {
#if DD_SUPPORT_SOCKET_TRANSPORT
                using MsgChannelSocket = MessageChannel<SocketMsgTransport>;
                pMsgChannel = DD_NEW(MsgChannelSocket, createInfo.allocCb)(createInfo.allocCb,
                    createInfo.channelInfo,
                    createInfo.hostInfo);
#endif
            }
            else if (createInfo.hostInfo.type == TransportType::LocalNg)
            {
                using MsgChannelUtilityDriver = MessageChannel<LocalNgMsgTransport>;
                pMsgChannel = DD_NEW(MsgChannelUtilityDriver, createInfo.allocCb)(createInfo.allocCb,
                    createInfo.channelInfo,
                    createInfo.allocCb,
                    createInfo.channelInfo.componentType,
                    createInfo.channelInfo.initialFlags);
            }
    #if defined(DD_ENABLE_UWP_TRANSPORT)
            else if (createInfo.hostInfo.type == TransportType::MessageBus)
            {
                using MsgChannelLocal = MessageChannel<LocalMsgTransport>;
                pMsgChannel = DD_NEW(MsgChannelLocal, createInfo.allocCb)(createInfo.allocCb,
                    createInfo.channelInfo,
                    createInfo.allocCb,
                    createInfo.channelInfo.componentType,
                    createInfo.channelInfo.initialFlags);
            }
    #endif
#elif defined(DD_PLATFORM_WINDOWS_KM)
            DD_UNUSED(createInfo);
            // This if block is here for two reasons:
            //      (1) We need to fill this out, and we're going to! This will happen pretty early in
            //          the kernel driver bringup.
            //      (2) Until then, we need the else block below to compile.
            if (true)
            {
                DD_ASSERT_REASON("Message channel is not correctly implemented for Windows KM yet - CreateMessageChannel will fail and return NULL");
            }
// Windows is handled above, so we check posix UM platforms here.
#elif DD_PLATFORM_IS_POSIX
            if ((createInfo.hostInfo.type == TransportType::Remote) |
                (createInfo.hostInfo.type == TransportType::Local))
            {
#if DD_SUPPORT_SOCKET_TRANSPORT
                using MsgChannelSocket = MessageChannel<SocketMsgTransport>;
                pMsgChannel = DD_NEW(MsgChannelSocket, createInfo.allocCb)(createInfo.allocCb,
                    createInfo.channelInfo,
                    createInfo.hostInfo);
#endif
            }
            // TODO: Support the utility driver transport here once it's cross platform
#endif
            else
            {
                // Invalid transport type
                DD_WARN_REASON("Invalid transport type specified");
            }

            if (pMsgChannel != nullptr)
            {
                result = Result::Success;
            }
        }

        if (result == Result::Success)
        {
            *ppMessageChannel = pMsgChannel;
        }

        return result;
    }
}
