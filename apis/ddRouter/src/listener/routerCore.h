/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <gpuopen.h>
#include <unordered_map>
#include <mutex>
#include <deque>
#include <vector>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <unordered_set>
#include <memory>

#include <listener/transportThread.h>

#include <listener/transports/abstractListenerTransport.h>
#include <listener/clientmanagers/abstractClientManager.h>

namespace DevDriver
{

    struct ClientInfo
    {
        char clientName[kMaxStringLength];
        char clientDescription[kMaxStringLength];
        ProcessId clientPid;
        ClientId clientId;
        bool hasBeenIdentified;
    };

    struct ClientContext
    {
        ClientInfo clientInfo;
        ConnectionInfo connectionInfo;
        uint32 pingRetryCount;
        bool receivedPong;
        bool registeredClient;
    };

    struct TransportContext
    {
        std::shared_ptr<IListenerTransport> pTransport;
        std::unordered_map<ClientId, ConnectionInfo> clientMap;
    };

    struct MessageContext
    {
        MessageBuffer message;
        ConnectionInfo connectionInfo;
    };

    struct ProcessingQueue
    {
        std::deque<MessageContext> queue;
        std::condition_variable signal;
        std::mutex mutex;
        std::thread thread;
        volatile bool active = 0;
    };

    class RoutingCache
    {
    public:
        explicit RoutingCache(RouterCore *pRouter) : m_pRouter(pRouter) {};
        ~RoutingCache() {};

        Result RouteMessage(const MessageContext &messageContext);
    private:
        struct CacheClientContext
        {
            ConnectionInfo connectionInfo;
            std::shared_ptr<IListenerTransport> pTransport;
        };

        RouterCore*                 m_pRouter;
        std::unordered_map<ClientId, CacheClientContext> m_routingCache;

        ClientId            m_currentClientId       = kBroadcastClientId;
        CacheClientContext* m_pCurrentClientContext = nullptr;
    };

    class RouterCore
    {
        friend RoutingCache;
    public:
        RouterCore();
        ~RouterCore();

        Result Start(const char* pDescription);

        Result SetClientManager(IClientManager *pClientManager);
        Result RegisterTransport(const std::shared_ptr<IListenerTransport> &pTransport);
        Result RemoveTransport(const std::shared_ptr<IListenerTransport> &pTransport);

        void SetClientTimeoutCount(uint32_t timeoutCount) { m_clientTimeoutCount = timeoutCount; }
        void Stop();

        std::vector<ClientInfo> GetConnectedClientList();

    private:
        DD_STATIC_CONST uint32 kClientDiscoveryIntervalInMs = 3000;
        DD_STATIC_CONST uint32 kClientTimeoutCount = 3;
        DD_STATIC_CONST uint32 kThreadWaitTimeoutInMs = 250;

        std::mutex m_clientMutex;
        std::unordered_map<ClientId, ClientContext> m_clientMap;
        std::mutex m_transportMutex;
        std::unordered_map<TransportHandle, TransportContext> m_transportMap;

        IClientManager* m_pClientManager;
        TransportHandle m_lastTransportId;
        ClientId        m_clientId;
        uint64          m_lastClientPingTimeInMs;
        uint32          m_clientTimeoutCount;
        ProcessingQueue m_clientThread;
        MessageBuffer   m_clientInfoResponse;

        void RouterThreadFunc(ProcessingQueue &pQueueState);
        void UpdateClients();
        void ProcessRouterMessage(const MessageContext &messageContext);

        ClientContext* FindClientById(ClientId clientId);
        ClientContext* FindExternalClientByConnection(const ConnectionInfo & connectionInfo);

        void AddClient(const ClientId clientId, const ConnectionInfo &connectionInfo, const bool registeredClient);
        void RemoveClient(ClientId clientId);

        void SendBroadcastMessage(const MessageBuffer &message, const std::shared_ptr<IListenerTransport> &pTransport);
        void ProcessClientManagementMessage(const MessageContext &messageContext);

        // methods for interfacing with RoutingCache
        bool ConnectionInfoForClientId(ClientId clientId, ConnectionInfo *pConnectionInfo);
        std::shared_ptr<IListenerTransport> TransportForTransportHandle(TransportHandle handle);
        void RouteBroadcastMessage(const MessageContext& msgContext);
        void RouteInternalMessage(const MessageContext& recvMsgContext);
        bool IsRoutableMessage(const MessageContext& recvMsgContext);
    };
} // DevDriver
