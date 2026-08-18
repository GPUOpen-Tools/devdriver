/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <gpuopen.h>
#include <listener/transports/abstractListenerTransport.h>
#include <memory>

namespace DevDriver
{
    class IClientManager
    {
    public:
        virtual ~IClientManager() {}

        // TODO: explicit initialize and destroy?
        // anything else missing?
        virtual Result RegisterHost(ClientId* pClientId) = 0;
        virtual Result UnregisterHost() = 0;

        virtual Result RegisterClient(ClientId* pClientId) = 0;
        virtual Result UnregisterClient(ClientId clientId) = 0;

        virtual const char *GetClientManagerName() const = 0;
        virtual ClientId GetHostClientId() const = 0;

    protected:
        IClientManager() {}
    };

} // DevDriver
