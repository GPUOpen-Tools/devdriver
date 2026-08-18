/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "ddPlatform.h"
#include "listener.h"
#include "../listener/listenerCore.h"

namespace DevDriver
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create a listener object
    Result CreateListener(const ListenerCreateInfo& createInfo, IListener** ppListener)
    {
        Result result = Result::InvalidParameter;

        ListenerCore* pListenerCore = nullptr;

        if (ppListener != nullptr)
        {
            result = Result::InsufficientMemory;

            // Make sure we have reasonable allocator functions before we try to use them
            DD_ASSERT(createInfo.allocCb.pfnAlloc != nullptr);
            DD_ASSERT(createInfo.allocCb.pfnFree != nullptr);

            pListenerCore = DD_NEW(ListenerCore, createInfo.allocCb);

            if (pListenerCore != nullptr)
            {
                result = pListenerCore->Initialize(createInfo);
            }

            if (result != Result::Success)
            {
                DD_DELETE(pListenerCore, createInfo.allocCb);
            }
        }

        if (result == Result::Success)
        {
            *ppListener = pListenerCore;
        }

        return result;
    }
}
