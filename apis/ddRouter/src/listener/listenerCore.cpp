/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <listener/listenerCore.h>
#include <iostream>
#include <new>
#include <ddCommon.h>
#include <ddPlatform.h>

#ifdef DD_PLATFORM_WINDOWS_UM
    #include <listener/transports/winPipeTransport.h>

        // TODO: Move the utility driver transport to cross platform includes once it's fully supported
        #include <listener/transports/ddLocalNgListenerTransport.h>
#endif

#include <listener/transports/socketTransport.h>
#include <listener/clientmanagers/listenerClientManager.h>

namespace DevDriver
{
#if defined(DD_PLATFORM_WINDOWS_UM)
    const uint32 minDevDriverInAmdLogVersion = 1;
#endif

    // Enumeration of possible routing network identifiers
    enum class RoutingNetworkId : uint16
    {
        Listener = 0,
        UtilityDriver = 1,
#if defined(DD_PLATFORM_WINDOWS_UM)
        UWP = 7, // The network id for UWP is fixed at 7 due to back-compat
                 // The enum value of 7 translates into the bit pattern of 0b1110_0000_0000_0000 which is what the old
                 // routing prefix used to be set to directly.
#endif
    };

    // Calculates the routing prefix for the provided routing network id
    ClientId CalculateRoutingPrefix(RoutingNetworkId networkId)
    {
        // Ensure that the network id fits within the router prefix width
        DD_ASSERT(static_cast<uint16>(networkId) < (1 << kRouterPrefixWidth));

        return (static_cast<uint16>(networkId) << kRouterPrefixShift);
    }

    // =====================================================================================================================
    // Constructs an instance of the current platform's local pipe transport and returns it.
    std::shared_ptr<IListenerTransport> CreateLocalPipeTransport(uint16_t port)
    {
        HostInfo hostInfo = kDefaultNamedPipe;
        hostInfo.port = port;

#if defined(DD_PLATFORM_WINDOWS_UM)
        auto pPipeTransport = std::make_shared<PipeListenerTransport>(hostInfo);
#else
        auto pPipeTransport = std::make_shared<SocketListenerTransport>(hostInfo);
#endif

        return pPipeTransport;
    }

    // =====================================================================================================================
    // Constructs a vector of the currently connected clients and returns it
    std::vector<ClientInfo> ListenerCore::GetConnectedClientList()
    {
        return m_routerCore.GetConnectedClientList();
    }

    // =====================================================================================================================
    // Attempts to register a transport with our internal router core object
    Result ListenerCore::RegisterRouterTransport(const std::shared_ptr<IListenerTransport>& pTransport)
    {
        Result result = Result::InvalidParameter;

        if (pTransport != nullptr)
        {
            result = m_routerCore.RegisterTransport(pTransport);

            if (result == Result::Success)
            {
                m_managedTransports.emplace_back(pTransport);
            }
        }

        return result;
    }

    // =====================================================================================================================
    // Constructor
    ListenerCore::ListenerCore() :
        m_routerCore(),
        m_pClientManager(nullptr),
        m_started(false)
    {
    }

    // =====================================================================================================================
    // Destructor
    ListenerCore::~ListenerCore()
    {
        Destroy();
    }

    // =====================================================================================================================
    // Initializes the listener core object and binds to all provided addresses
    Result ListenerCore::Initialize(const ListenerCreateInfo& createInfo)
    {
        DD_ASSERT(m_pClientManager == nullptr);

        std::lock_guard<std::mutex> lock(m_routerMutex);

        // Initialize Client Manager
        ListenerClientManagerInfo infoStruct = {};
        infoStruct.routerPrefix = CalculateRoutingPrefix(RoutingNetworkId::Listener);
#if defined(DD_PLATFORM_WINDOWS_UM)
        infoStruct.routerPrefixMask = kRouterPrefixMask;
#else
        infoStruct.routerPrefixMask = 0;
#endif

        IClientManager* pClientManager = new(std::nothrow) ListenerClientManager(createInfo.allocCb, infoStruct);
        Result result = (pClientManager != nullptr) ? Result::Success : Result::InsufficientMemory;

        if (result == Result::Success)
        {
            result = m_routerCore.SetClientManager(pClientManager);

            m_routerCore.SetClientTimeoutCount(createInfo.clientTimeoutCount);

            if (result == Result::Success)
            {
                m_pClientManager = pClientManager;
            }
            else
            {
                delete pClientManager;
                pClientManager = nullptr;
            }
        }

#if defined(DD_PLATFORM_WINDOWS_UM)
        // Initialize Kernel Transport
        if ((result == Result::Success) && createInfo.flags.enableKernelTransport)
        {
            if (createInfo.flags.kernelTransportKMDOnly == 0)
            {
                // Backwards compatibility:
                // First try to initialize the ioctl interface to amdlog. If this
                // fails then this suggests the system has an older amdlog, so
                // fall back to the escape interface to kmd
                std::shared_ptr<LocalNgListenerTransport> pUtilityDriverTransport =
                    std::make_shared<LocalNgListenerTransport>(createInfo.allocCb,
                                                               CalculateRoutingPrefix(RoutingNetworkId::Listener),
                                                               CalculateRoutingPrefix(RoutingNetworkId::UtilityDriver));
                result = (pUtilityDriverTransport != nullptr) ? Result::Success : Result::InsufficientMemory;

                uint32 versionFromAmdLog;
                DevDriver::DeveloperModeFlags featuresFromAmdLog;
                result = pUtilityDriverTransport->GetKernelDevDriverVersion(versionFromAmdLog, featuresFromAmdLog);

                if (result == Result::Success)
                {
                    if (versionFromAmdLog >= minDevDriverInAmdLogVersion)
                    {
                        result = RegisterRouterTransport(pUtilityDriverTransport);
                    }
                    else
                    {
                        // set `result` to an error, to trigger the KMD fallback path below
                        result = Result::VersionMismatch;
                    }
                }
            }
            if ((result != Result::Success) || createInfo.flags.kernelTransportKMDOnly)
            {
                // KMD Listener Transport is not available in this build configuration
                result = Result::Unavailable;
            }
        }
#endif

        // Initialize Local "Pipe" Transport
        // NOTE: This may or may not use actual OS pipes depending on the platform
        if (result == Result::Success)
        {
            auto pPipeTransport = CreateLocalPipeTransport(createInfo.localPort);
            result = (pPipeTransport != nullptr) ? Result::Success : Result::InsufficientMemory;

            if (result == Result::Success)
            {
                // If we encounter any errors here, the shared pointer will automatically destroy the transport.
                result = RegisterRouterTransport(pPipeTransport);
            }
        }

        // Initialize Remote Transports
        if (result == Result::Success)
        {
            for (uint32 addressIndex = 0; addressIndex < createInfo.numAddresses; addressIndex++)
            {
                // todo: validate this.
                ListenerBindAddress& address = createInfo.pAddressesToBind[addressIndex];

                HostInfo hostInfo = {};

                hostInfo.type      = TransportType::Remote;
                hostInfo.pHostname = address.pHostAddress;
                hostInfo.port      = address.port;

                auto pRemoteTransport =
                    std::make_shared<SocketListenerTransport>(hostInfo);
                result = (pRemoteTransport != nullptr) ? Result::Success : Result::InsufficientMemory;

                if (result == Result::Success)
                {
                    // If we encounter any errors here, the shared pointer will automatically destroy the transport.
                    result = RegisterRouterTransport(pRemoteTransport);
                }

                if (result != Result::Success)
                {
                    // Stop initializing remote transports if we encounter a failure
                    break;
                }
            }
        }

        if (result == Result::Success)
        {
            DD_PRINT(LogLevel::Info, "[ListenerCore] Using %s client manager", m_pClientManager->GetClientManagerName());
            for (const auto &pTransport : m_managedTransports)
            {
                DD_PRINT(LogLevel::Info, "[ListenerCore] Listening for connections on %s", pTransport->GetTransportName());
            }

            result = m_routerCore.Start(createInfo.pDescription);

            if (result == Result::Success)
            {
                m_started = true;
            }
        }

        // Clean up any initialized resources if we encounter errors
        if (result != Result::Success)
        {
            for (const auto &pTransport : m_managedTransports)
            {
                m_routerCore.RemoveTransport(pTransport);
            }
            m_managedTransports.clear();
            m_routerCore.Stop();

            if (m_pClientManager != nullptr)
            {
                delete m_pClientManager;
                m_pClientManager = nullptr;
            }
        }

        return result;
    }

    // =====================================================================================================================
    // Destroys the listener core object and shuts down all communications
    void ListenerCore::Destroy()
    {
        std::lock_guard<std::mutex> lock(m_routerMutex);

        if (m_started)
        {
            for (const auto &pTransport : m_managedTransports)
            {
                m_routerCore.RemoveTransport(pTransport);
            }
            m_managedTransports.clear();
            m_routerCore.Stop();
            m_started = false;

            if (m_pClientManager != nullptr)
            {
                delete m_pClientManager;
                m_pClientManager = nullptr;
            }
        }
    }

} // DevDriver
