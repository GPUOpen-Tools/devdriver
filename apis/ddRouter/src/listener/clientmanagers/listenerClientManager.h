/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <gpuopen.h>
#include <listener/clientmanagers/abstractClientManager.h>
#include <util/hashSet.h>
#include <ddPlatform.h>

namespace DevDriver
{
    struct ListenerClientManagerInfo
    {
        ClientId routerPrefix;
        ClientId routerPrefixMask;
    };

    class ListenerClientManager : public IClientManager
    {
    public:
        ListenerClientManager(const AllocCb& allocCb, const ListenerClientManagerInfo& clientManagerInfo);
        ~ListenerClientManager();

        // TODO: explicit initialize and destroy?
        // anything else missing?

        Result RegisterHost(ClientId* pClientId) override;
        Result UnregisterHost() override;

        Result RegisterClient(ClientId* pClientId) override;
        Result UnregisterClient(ClientId clientId) override;

        const char *GetClientManagerName() const override { return "Internal"; };
        ClientId GetHostClientId() const override { return (m_initialized ? m_hostClientId : kBroadcastClientId); };

    protected:

        const ListenerClientManagerInfo m_clientManagerInfo;
        bool                            m_initialized;
        ClientId                        m_hostClientId;
        Platform::Mutex                 m_clientMutex;
        HashSet<ClientId>               m_clientInfo;
        Platform::Random                m_rand;

        ClientId GenerateClientId();
    };

} // DevDriver
