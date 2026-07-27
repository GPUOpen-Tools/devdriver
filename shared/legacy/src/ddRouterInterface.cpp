/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <ddRouterInterface.h>
#include <ddPlatform.h>

#include <ddMsgRouter.h>

namespace DevDriver
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create a router object
    Result CreateRouter(const RouterCreateInfo& createInfo, IRouter** ppRouter)
    {
        return CreateRouter(createInfo, ppRouter, 0, nullptr);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create a router object
    Result CreateRouter(const RouterCreateInfo& createInfo, IRouter** ppRouter, size_t servicesCount, IService* const* pServices)
    {
        Result result = Result::InvalidParameter;

        MsgRouter* pMsgRouter = nullptr;

        if (ppRouter != nullptr)
        {
            result = Result::InsufficientMemory;

            // Make sure we have reasonable allocator functions before we try to use them
            DD_ASSERT(createInfo.allocCb.pfnAlloc != nullptr);
            DD_ASSERT(createInfo.allocCb.pfnFree != nullptr);

            pMsgRouter = DD_NEW(MsgRouter, createInfo.allocCb)(createInfo.allocCb,
                                                               createInfo.kernalEnableCb,
                                                               createInfo.kernalDisableCb);

            if (pMsgRouter != nullptr)
            {
                result = pMsgRouter->Initialize(createInfo);
            }

            if (result != Result::Success)
            {
                DD_DELETE(pMsgRouter, createInfo.allocCb);
            }
        }

        if (result == Result::Success)
        {
            pMsgRouter->RegisterServices(servicesCount, pServices);
            *ppRouter = pMsgRouter;
        }

        return result;
    }

    void DestroyRouter(IRouter** ppRouter)
    {
        if (ppRouter != nullptr)
        {
            if ((*ppRouter) != nullptr)
            {
                MsgRouter* pRouter = static_cast<MsgRouter*>(*ppRouter);
                pRouter->Destroy();

                DD_DELETE(pRouter, pRouter->GetAllocCb());

                *ppRouter = nullptr;
            }
        }
    }
}
