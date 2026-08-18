/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <listener/clientmanagers/listenerClientManager.h>
#include <ddPlatform.h>

namespace DevDriver
{
    ListenerClientManager::ListenerClientManager(const AllocCb& allocCb, const ListenerClientManagerInfo& clientManagerInfo) :
        m_clientManagerInfo(clientManagerInfo),
        m_initialized(false),
        m_hostClientId(kBroadcastClientId),
        m_clientMutex(),
        m_clientInfo(allocCb),
        m_rand()
    {
        DD_ASSERT((clientManagerInfo.routerPrefix &
                   clientManagerInfo.routerPrefixMask) == clientManagerInfo.routerPrefix);
    }

    ListenerClientManager::~ListenerClientManager()
    {
        if (m_initialized)
            UnregisterHost();
    }

    Result ListenerClientManager::RegisterHost(ClientId* pClientId)
    {
        Result result = Result::Error;
        if (!m_initialized)
        {
            Platform::LockGuard<Platform::Mutex> clientLock(m_clientMutex);

            m_hostClientId = GenerateClientId();
            DD_ASSERT(m_hostClientId != kBroadcastClientId);
            m_initialized = true;
            *pClientId = m_hostClientId;
            m_clientInfo.Insert(m_hostClientId);
            result = Result::Success;
        }
        return result;
    }

    Result ListenerClientManager::UnregisterHost()
    {
        Result result = Result::Error;
        if (m_initialized)
        {
            Platform::LockGuard<Platform::Mutex> clientLock(m_clientMutex);
            m_clientInfo.Clear();
            m_hostClientId = kBroadcastClientId;
            m_initialized = false;
            result = Result::Success;
        }
        return result;
    }

    Result ListenerClientManager::RegisterClient(ClientId* pClientId)
    {
        Result result = Result::Error;
        if (m_initialized)
        {
            Platform::LockGuard<Platform::Mutex> clientLock(m_clientMutex);

            const ClientId tempClientId = GenerateClientId();
            if (tempClientId != kBroadcastClientId)
            {
                const ClientId clientMask = ~m_clientManagerInfo.routerPrefixMask;
                DD_UNUSED(clientMask);
                DD_ASSERT((tempClientId & clientMask) != kBroadcastClientId);
                m_clientInfo.Insert(tempClientId);
                *pClientId = tempClientId;
                result = Result::Success;
            }
            else
            {
                // This is a critical failure and shouldn't happen under normal conditions
                DD_WARN_REASON("Client manager was unable to generate a new client ID");
            }
        }
        return result;
    }

    Result ListenerClientManager::UnregisterClient(ClientId clientId)
    {
        Result result = Result::Error;
        if (m_initialized & (clientId != m_hostClientId))
        {
            Platform::LockGuard<Platform::Mutex> clientLock(m_clientMutex);
            // Attempt to erase a client with the specified client ID. If >0 we have successfully removed
            // the client ID from the set.
            if (m_clientInfo.Erase(clientId) == Result::Success)
            {
                result = Result::Success;
            }
        }
        return result;
    }

    // Generate a random client ID that has not already been allocated
    ClientId ListenerClientManager::GenerateClientId()
    {
        ClientId tempClientId = kBroadcastClientId;

        const ClientId routerPrefix = m_clientManagerInfo.routerPrefix;
        const ClientId clientMask = ~m_clientManagerInfo.routerPrefixMask;

        // The maximum number of clients we can allocate is equal the client ID mask, less one for the broadcast ID
        DD_STATIC_CONST size_t kMaxNumberOfClients = (kClientIdMask - 1);

        if (m_clientInfo.Size() < kMaxNumberOfClients)
        {
            // Loop until we have found a client ID that is not the broadcast client ID and that we haven't allocated
            do
            {
                // Add one since the range is typically 0 <= x < Max
                const uint32 randVal = m_rand.Generate() + 1;
                tempClientId = static_cast<ClientId>(randVal & clientMask) | routerPrefix;
            } while (((tempClientId & clientMask) == kBroadcastClientId) || m_clientInfo.Contains(tempClientId));
        }

        DD_ASSERT(tempClientId != kBroadcastClientId);
        return tempClientId;
    }
} // DevDriver
