/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <gpuopen.h>
#include <listener/listener.h>
#include <vector>
#include <mutex>
#include <listener/routerCore.h>
#include <msgChannel.h>
#include <ddPlatform.h>

namespace DevDriver
{
    // Listener Core
    // Designed to be a self contained class that manages all of the complexity of routing packets between clients on the message bus.
    // Allows for limited configuration through the ListenerCreateInfo struct and otherwise behaves in a specific manner depending on
    // the underlying platform.
    class ListenerCore : public IListener
    {
    public:
        // Constructor
        ListenerCore();

        // Destructor
        ~ListenerCore();

        // Initialization
        Result Initialize(const ListenerCreateInfo& createInfo);

        // Destruction
        void Destroy() override;

        // Constructs a vector of the currently connected clients and returns it
        // This function has to acquire an internal lock so it should not be considered a "cheap" function
        std::vector<ClientInfo> GetConnectedClientList();

        // Returns a list of the currently managed transports.
        const std::vector<std::shared_ptr<IListenerTransport>>& GetManagedTransports() const { return m_managedTransports; }

        // Returns the client manager pointer.
        const IClientManager* GetClientManager() const { return m_pClientManager; }

    private:
        Result RegisterRouterTransport(const std::shared_ptr<IListenerTransport> &pTransport);

        RouterCore                                       m_routerCore;         // The underlying router core object
        std::vector<std::shared_ptr<IListenerTransport>> m_managedTransports;  // A vector of all transports that are managed by the listener
        std::mutex                                       m_routerMutex;        // A mutex used to make access to the router thread safe
        IClientManager*                                  m_pClientManager;     // Pointer to the current client manager object
        bool                                             m_started;            // True if the listener has been started, false otherwise
                                                                               // (Used for internal resource cleanup logic)
};

} // DevDriver
