/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <routerContext.h>

#include <util/ddJsonWriter.h>

#include <listener/listener.h>
#include <msgChannel.h>

using namespace DevDriver;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Router::Router(const RouterCreateInfo& createInfo)
    : m_createInfo(createInfo)
    , m_ddAlloc({ &m_createInfo.apiAlloc, &ddApiAlloc, &ddApiFree })
    , m_pListener(nullptr)
    , m_logger(m_createInfo.logger)
    , m_hRpcServer(DD_API_INVALID_HANDLE)
    , m_hEventServer(DD_API_INVALID_HANDLE)
    , m_modules(m_ddAlloc)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Router::~Router()
{
    // Destroy each module in-place
    for (ToolModule* pModule : m_modules)
    {
        DestroyModule(pModule);
    }

    // And then drop the dangling pointers
    m_modules.Clear();

    // Tear down the Event & RPC Servers after destroying modules. They need these alive to unregister themselves.
    ddEventServerDestroy(m_hEventServer);
    ddRpcServerDestroy(m_hRpcServer);

    // Destroy our local connection before we tear down the network
    ddNetDestroyConnection(m_hConnection);

    // Lastly, destroy the Listener. This must be last because everything depends on this.
    if (m_pListener != nullptr)
    {
        m_pListener->Destroy();

        DD_ROUTER_LOG(m_logger, DD_LOG_LEVEL_INFO, "Shut down the developer mode message bus.");

        DD_DELETE(m_pListener, DDAlloc());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT Router::LoadBuiltinModule(const DDModuleInterface* pInterface, DDModuleLoadedInfo* pLoadedInfo)
{
    ToolModule* pModule = nullptr;
    DD_RESULT   result  = ToolModule::LoadBuiltin(&m_logger, ApiAlloc(), pInterface, &pModule);

    if (result == DD_RESULT_SUCCESS)
    {
        result = ConnectModule(pModule);
    }

    if (result == DD_RESULT_SUCCESS)
    {
        if (pLoadedInfo != nullptr)
        {
            *pLoadedInfo = pModule->GetModuleInfo();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT Router::LoadDynamicModule(const char* pModulePath, DDModuleLoadedInfo* pLoadedInfo)
{
    ToolModule* pModule = nullptr;
    DD_RESULT   result  = ToolModule::LoadDynamic(&m_logger, ApiAlloc(), pModulePath, &pModule);

    if (result == DD_RESULT_SUCCESS)
    {
        result = ConnectModule(pModule);
    }

    if (result == DD_RESULT_SUCCESS)
    {
        if (pLoadedInfo != nullptr)
        {
            *pLoadedInfo = pModule->GetModuleInfo();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT Router::UnloadModule(DDModuleContext hContext)
{
    size_t indexToRemove = SIZE_MAX;
    for (size_t i = 0; i < m_modules.Size(); i += 1)
    {
        if (m_modules[i]->GetModuleInfo().hContext == hContext)
        {
            indexToRemove = i;
            break;
        }
    }

    DD_RESULT result = DD_RESULT_COMMON_DOES_NOT_EXIST;

    if (indexToRemove < m_modules.Size())
    {
        DestroyModule(m_modules[indexToRemove]);

        // Note: Does not maintain order! #RadeonRebellion
        m_modules.Remove(indexToRemove);

        result = DD_RESULT_SUCCESS;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT Router::ConnectModule(ToolModule* pModule)
{
    DD_RESULT result = DD_RESULT_COMMON_DOES_NOT_EXIST;

    DD_ROUTER_PUSHF(m_logger, DD_LOG_LEVEL_INFO, "Connecting module %s", pModule->GetDescription().pName);

    if (pModule->HasConnectionApi())
    {
        DDModuleConnectionContext           hConnectionContext = DD_API_INVALID_HANDLE;
        DDModuleConnectionContextCreateInfo connectionInfo     = {};

        connectionInfo.loader.apiAllocCb.pfnAlloc  = m_createInfo.apiAlloc.pAllocCallback;
        connectionInfo.loader.apiAllocCb.pfnFree   = m_createInfo.apiAlloc.pFreeCallback;
        connectionInfo.loader.apiAllocCb.pUserdata = m_createInfo.apiAlloc.pUserdata;
        connectionInfo.loader.logger               = m_createInfo.logger;

        connectionInfo.hConnection  = m_hConnection;
        connectionInfo.hRpcServer   = m_hRpcServer;
        connectionInfo.hEventServer = m_hEventServer;

        result = pModule->CreateConnectionContext(connectionInfo, &hConnectionContext);

        if (result == DD_RESULT_SUCCESS)
        {
            DD_ROUTER_LOG(m_logger, DD_LOG_LEVEL_INFO, "Successfully created a connection context");

            // Save our connection context with the module
            pModule->SetUserdata(hConnectionContext);

            // Save our loaded module
            m_modules.PushBack(pModule);
        }
        else
        {
            DD_ROUTER_LOGF(
                m_logger,
                DD_LOG_LEVEL_ERROR,
                "Failed to create a connection context: %s",
                ddApiResultToString(result));
        }
    }
    else
    {
        DD_ROUTER_LOGF(
            m_logger,
            DD_LOG_LEVEL_ERROR,
            "Unable to connect module - \"%s\" has no Connection Api",
            pModule->GetDescription().pName);
    }

    DD_ROUTER_POP(m_logger);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Router::DestroyModule(ToolModule* pModule)
{
    DD_ASSERT(pModule != nullptr);

    DD_ROUTER_LOGF(m_logger, DD_LOG_LEVEL_INFO, "Unloading module %s", pModule->GetDescription().pName);

    const auto hContext = reinterpret_cast<DDModuleConnectionContext>(pModule->GetUserdata());
    pModule->DestroyConnectionContext(hContext);
    pModule->Destroy();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT Router::Create(const DDRouterCreateInfo& createInfo, Router** ppOutRouter)
{
    RouterCreateInfo routerCreateInfo = {};

    DD_RESULT result = ValidateLog(createInfo.logger, &routerCreateInfo.logger);

    // This logger is valid here and can be used freely throughout the function.
    LoggerUtil logger(routerCreateInfo.logger);

    Router* pRouter = nullptr;

    if ((result == DD_RESULT_SUCCESS) && (ppOutRouter != nullptr) && (createInfo.pDescription != nullptr))
    {
        AllocCb allocCb = {};
        ConvertAllocCallbacks(createInfo.alloc, &routerCreateInfo.apiAlloc, &allocCb);

        // Copy the router description into the create info
        Platform::Strncpy(routerCreateInfo.pDescription,
            createInfo.pDescription,
            sizeof(routerCreateInfo.pDescription));

        pRouter = DD_NEW(Router, allocCb)(routerCreateInfo);
        if (pRouter != nullptr)
        {
            ListenerCreateInfo listenerInfo = {};

            // Client description
            listenerInfo.pDescription       = createInfo.pDescription;
            listenerInfo.clientTimeoutCount = createInfo.clientTimeoutCount;

            // Router config flags

            // Enable the kernel transport and embedded client by default as long as the caller didn't disable it.
            const bool enableKernelTransport = (createInfo.transportFlags.fields.disableKernelTransport == 0);

            listenerInfo.flags.enableKernelTransport = enableKernelTransport;
            listenerInfo.flags.enableEmbeddedClient = enableKernelTransport;
            listenerInfo.flags.kernelTransportKMDOnly = createInfo.transportFlags.fields.kernelTransportKMDOnly;

            // Listener server config parameters

            ListenerBindAddress address = {};

            // Set up an address binding if the caller requested a remote transport
            const bool remoteTransportEnabled = (createInfo.transportFlags.fields.disableRemoteTransport == 0);
            if (remoteTransportEnabled)
            {
                const bool externalNetworkEnabled = (createInfo.transportFlags.fields.disableExternalNetwork == 0);

                // When external network access is enabled, listen on all interfaces, otherwise just listen on localhost
                address.pHostAddress = externalNetworkEnabled ? "0.0.0.0" : "localhost";

                address.port = (createInfo.remotePort == 0) ? DD_API_DEFAULT_NETWORK_PORT : createInfo.remotePort;

                listenerInfo.pAddressesToBind = &address;
                listenerInfo.numAddresses = 1;
            }
            else
            {
                listenerInfo.pAddressesToBind = nullptr;
                listenerInfo.numAddresses = 0;
            }

            // Memory allocation callbacks
            listenerInfo.allocCb = pRouter->DDAlloc();

            listenerInfo.localPort = createInfo.localPort;

            // Attempt to create a listener and a local connection to it
            IListener* pListener = nullptr;
            DDNetConnection hConnection = DD_API_INVALID_HANDLE;
            result = DevDriverToDDResult(CreateListener(listenerInfo, &pListener));

            if (result == DD_RESULT_SUCCESS)
            {
                // Make sure we actually got a valid listener pointer
                DD_ASSERT(pListener != nullptr);

                DDNetConnectionInfo connectionInfo = {};
                connectionInfo.pDescription = "ddRouter Internal Client";
                connectionInfo.type         = DD_NET_CLIENT_TYPE_SERVER;
                connectionInfo.port         = createInfo.localPort;

                result = ddNetCreateConnection(&connectionInfo, &hConnection);

                // If we fail to create our local connection, we need to destroy the listener we created earlier
                if (result != DD_RESULT_SUCCESS)
                {
                    pListener->Destroy();

                    DD_DELETE(pListener, pRouter->DDAlloc());
                }
            }

            if (result == DD_RESULT_SUCCESS)
            {
                // Save the final handles in the context object
                pRouter->m_pListener = pListener;
                pRouter->m_hConnection = hConnection;

                // Return the context object to the caller
                *ppOutRouter = pRouter;
            }
            else
            {
                // Clean up the context object
                const AllocCb alloc = pRouter->DDAlloc();

                // Use a local copy of Alloc so that we're not calling a method on data as it's deallocated.
                DD_DELETE(pRouter, alloc);
            }
        }
        else
        {
            result = DD_RESULT_COMMON_OUT_OF_HEAP_MEMORY;
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        // We should never end up in this block of code with a null output context
        DD_ASSERT(ppOutRouter != nullptr);
        DD_ASSERT((*ppOutRouter) != nullptr);
        DD_ASSERT(pRouter != nullptr);

        DD_ROUTER_LOGF(
            logger,
            DD_LOG_LEVEL_INFO,
            "Successfully initialized a developer mode message bus with client: %hu",
            ddNetQueryClientId(pRouter->m_hConnection));
    }
    else
    {
        DD_ROUTER_LOGF(
            logger,
            DD_LOG_LEVEL_ERROR,
            "Error connecting to local developer mode message bus. Error: %s",
            ddRouterResultToString(result));
    }

    // Now that we have a connection, start our Rpc server
    if (result == DD_RESULT_SUCCESS)
    {
        DD_ASSERT(pRouter != nullptr);

        DDRpcServerCreateInfo serverCreateInfo = {};
        serverCreateInfo.hConnection           = pRouter->m_hConnection;

        result = ddRpcServerCreate(&serverCreateInfo, &pRouter->m_hRpcServer);
    }

    // Start the event server
    if (result == DD_RESULT_SUCCESS)
    {
        DD_ASSERT(pRouter != nullptr);

        DDEventServerCreateInfo serverCreateInfo = {};
        serverCreateInfo.hConnection             = pRouter->m_hConnection;

        result = ddEventServerCreate(&serverCreateInfo, &pRouter->m_hEventServer);
    }

    return result;
}
