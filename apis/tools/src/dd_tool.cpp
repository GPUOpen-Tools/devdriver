/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_tool.h>
#include <dd_modules_manager.h>
#include <dd_timeout_constants.h>
#include <dd_tool_api.h>
#include <dd_assert.h>
#include <dd_constants.h>

#include <msgChannel.h>

#include <chrono>

#define LOG_ERROR(fmt, ...) s_pLogger->Log(              \
                                s_pLogger->pInstance,    \
                                DD_LOG_LVL_ERROR,        \
                                "[DDTool] " fmt,         \
                                ## __VA_ARGS__)

#define LOG_INFO(fmt, ...) s_pLogger->Log(               \
                                s_pLogger->pInstance,    \
                                DD_LOG_LVL_INFO,         \
                                "[DDTool] " fmt,         \
                                ## __VA_ARGS__)

namespace
{

constexpr uint32_t kRouterDisconnectListenerSleepMillisec = 500;

DDLoggerApi* s_pLogger;

void MsgChannelBusEventCallback(
    void* pUserdata,
    DevDriver::BusEventType type,
    const void* pEventData, size_t eventDataSize)
{
    (void)eventDataSize;
    DevDriver::Tool* pTool = reinterpret_cast<DevDriver::Tool*>(pUserdata);

    switch (type)
    {
    case DevDriver::BusEventType::ClientHalted:
    {
        DD_ASSERT(pEventData != nullptr);

        pTool->HandleClientHalted(pEventData);
    } break;
    case DevDriver::BusEventType::PongRequest:
    {
        // We don't need to do any special pong handling here
    } break;
    default:
    {
        LOG_ERROR("Unrecognized BusEventType: %d", type);
    } break;
    }
}

} // anonymous namespace

namespace DevDriver
{

Tool::Tool(std::string&& description, std::string&& modulesDir)
    : m_description{description}
    , m_apiRegistryImpl{}
    , m_apiRegistry {
        DDVersion {
            DD_API_REGISTRY_API_VERSION_MAJOR,
            DD_API_REGISTRY_API_VERSION_MINOR,
            DD_API_REGISTRY_API_VERSION_PATCH},
        reinterpret_cast<DDApiRegistryInstance*>(&m_apiRegistryImpl),
        DDApiRegistry_Add,
        DDApiRegistry_Get
    }
    , m_modulesManager{&m_apiRegistry, std::move(modulesDir)}
    , m_net{DD_API_INVALID_HANDLE}
    , m_systemClients{}
    , m_routerDisconnectListenerThread{}
    , m_routerDisconnectListenerStop{false}
    , m_driverUtils{this}
{
}

Tool::Tool(std::string&& description, std::string&& modulesDir, std::string&& logFilePath)
    : Tool(std::move(description), std::move(modulesDir))
{
    m_logFilePath    = std::move(logFilePath);
    m_isCustomLogger = false;
}

Tool::Tool(std::string&& description, std::string&& modulesDir, DDLoggerApi logger)
    : Tool(std::move(description), std::move(modulesDir))
{
    m_loggerApi      = logger;
    m_isCustomLogger = true;
}

Tool::~Tool()
{
    Disconnect();

    m_modulesManager.DestroyModules();
    m_modulesManager.UnloadDynamicModules();

    if (!m_isCustomLogger)
    {
        DDLoggerDestroy(&m_loggerApi);
    }
}

DD_RESULT Tool::Initialize()
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    if (!m_isCustomLogger)
    {
        DDLoggerCreateInfo info = {};
        info.file.pFilePath     = m_logFilePath.data();
        info.file.filePathSize  = static_cast<uint32_t>(m_logFilePath.size());

        result = DDLoggerCreate(&info, &m_loggerApi);
        DD_ASSERT(result == DD_RESULT_SUCCESS);
    }

    // `s_pLogger` allows free (non-member) functions in this translation to log.
    s_pLogger = &m_loggerApi;

    result = m_apiRegistryImpl.Add(
        DD_LOGGER_API_NAME,
        DDVersion {
            DD_LOGGER_API_VERSION_MAJOR,
            DD_LOGGER_API_VERSION_MINOR,
            DD_LOGGER_API_VERSION_PATCH},
        &m_loggerApi,
        sizeof(m_loggerApi));

    if (result == DD_RESULT_SUCCESS)
    {
        m_connection.Initialize(&m_apiRegistry);
        m_routerUtils.Initialize(&m_apiRegistry);
        m_driverUtils.Initialize(&m_apiRegistry);
        m_clocks.Initialize(&m_apiRegistry);
        m_uberTrace.Initialize(&m_apiRegistry);
        m_memoryTrace.Initialize(&m_apiRegistry);
        m_enhancedCrashInfo.Initialize(&m_apiRegistry);
        m_gpuDetective.Initialize(&m_apiRegistry);
        m_profiling.Initialize(&m_apiRegistry);
        m_pipelines.Initialize(&m_apiRegistry);
        m_settings.Initialize(&m_apiRegistry);
        m_eventLogging.Initialize(&m_apiRegistry);
        m_amdLogUtils.Initialize(&m_apiRegistry);

        // ModulesManager needs to initialize after all built-in apis have been registered.
        result = m_modulesManager.Initialize();
    }

    return result;
}

DD_RESULT Tool::LoadModules()
{
    DD_RESULT result = m_modulesManager.LoadDynamicModules();
    if (result == DD_RESULT_SUCCESS)
    {
        result = m_modulesManager.InitializeModules();
    }
    return result;
}

DDApiRegistry* Tool::GetApiRegistry()
{
    return &m_apiRegistry;
}

DD_RESULT Tool::Connect(const char* pIpAddr, uint16_t port)
{
    (void)port;

    DD_RESULT result = DD_RESULT_SUCCESS;

    DDNetConnectionInfo connInfo{};
    connInfo.timeoutInMs  = kRouterConnectionTimeoutMillisec;

    if (pIpAddr)
    {
        connInfo.type         = DD_NET_CLIENT_TYPE_TOOL_WITH_DRIVER_INIT;
        connInfo.pHostname    = pIpAddr;
        connInfo.port         = port;
        connInfo.pDescription = "DevDriverAPI Remote Connection";
    }
    else
    {
        connInfo.type         = DD_NET_CLIENT_TYPE_TOOL_WITH_DRIVER_INIT;
        connInfo.pDescription = "DevDriverAPI Local Connection";
    }

    IMsgChannel* pMsgChannel = nullptr;
    result = ddNetCreateConnection(&connInfo, &m_net);
    if (result == DD_RESULT_SUCCESS)
    {
        pMsgChannel = reinterpret_cast<IMsgChannel*>(m_net);

        m_connection.SetDDNet(m_net);

        if (pMsgChannel->IsConnected())
        {
            BusEventCallback busEventCb = {};
            busEventCb.pfnEventCallback = &MsgChannelBusEventCallback;
            busEventCb.pUserdata = this;

            pMsgChannel->SetBusEventCallback(busEventCb);

            result = UpdateSystemContextInfo();
            if (result == DD_RESULT_SUCCESS)
            {
                const DDClientId routerClientId = m_systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_ROUTER].id;
                const DDClientId amdlogClientId = m_systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_UTILITY_DRIVER].id;

                m_routerUtils.SetRpcClientInfo(m_net, routerClientId);
                m_driverUtils.SetRpcClientInfo(m_net);
                m_clocks.SetRpcClientInfo(m_net, amdlogClientId);
                m_uberTrace.SetRpcClientInfo(m_net);
                m_memoryTrace.SetRpcClientInfo(m_net);
                m_gpuDetective.SetRpcClientInfo(m_net);
                m_profiling.SetRpcClientInfo(m_net);
                m_pipelines.SetRpcClientInfo(m_net);
                m_settings.SetRpcClientInfo(m_net, routerClientId, amdlogClientId);
                m_eventLogging.SetRpcClientInfo(m_net);
                m_memoryTrace.SetSystemClients(m_systemClients);
                m_gpuDetective.SetSystemClients(m_systemClients);
                m_enhancedCrashInfo.SetRpcClientInfo(m_net, amdlogClientId);
                m_amdLogUtils.SetRpcClientInfo(m_net, amdlogClientId);

                m_connection.OnRouterConnected(routerClientId);

                m_routerDisconnectListenerStop = false;

                if (m_routerDisconnectListenerThread.joinable())
                {
                    // If router disconnects unexpectedly, Tool::Disconnect() is not called. Need to
                    // call join() here first before re-assigning the thread.
                    m_routerDisconnectListenerThread.join();
                }
                m_routerDisconnectListenerThread = std::thread(&Tool::RouterDisconnectListener, this);
            }
            else
            {
                LOG_ERROR("Failed to update system info. DD_RESULT: %s", ddApiResultToString(result));
            }
        }
        else
        {
            LOG_ERROR("Net connection is created, but not connected.");
            result = DD_RESULT_COMMON_UNKNOWN;
        }
    }
    else
    {
        LOG_ERROR("Failed to create net connection. DD_RESULT: %s", ddApiResultToString(result));
    }

    if (result != DD_RESULT_SUCCESS)
    {
        if (pMsgChannel != nullptr)
        {
            ddNetDestroyConnection(m_net);
        }
    }

    return result;
}

void Tool::Disconnect()
{
    if (m_net != DD_API_INVALID_HANDLE)
    {
        IMsgChannel* pMsgChannel = reinterpret_cast<IMsgChannel*>(m_net);
        if (pMsgChannel->IsConnected())
        {
            m_routerDisconnectListenerStop = true;
        }
    }

    if (m_routerDisconnectListenerThread.joinable())
    {
        m_routerDisconnectListenerThread.join();
        DD_ASSERT(m_net == DD_API_INVALID_HANDLE);
    }
}

void Tool::HandleClientHalted(const void* pEventData)
{
    const BusEventClientHalted* pClientHalted = static_cast<const BusEventClientHalted*>(pEventData);
    const uint16_t umdConnectionId = pClientHalted->clientId;

    m_connection.HandleDriverConnection(
        umdConnectionId,
        m_systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_UTILITY_DRIVER].id);
}

DD_RESULT Tool::UpdateSystemContextInfo()
{
    // All system clients have the Server component type so use that as a discovery filter
    ClientMetadata filter = {};
    filter.clientType = Component::Server;

    DiscoverClientsInfo discoverInfo = {};
    discoverInfo.pfnCallback         = &SystemClientDiscoveryFunc;
    discoverInfo.pUserdata           = this;
    discoverInfo.filter              = filter;
    discoverInfo.timeoutInMs         = 500;

    IMsgChannel* pMsgChannel = reinterpret_cast<IMsgChannel*>(m_net);
    Result result = pMsgChannel->DiscoverClients(discoverInfo);
    if (result == Result::NotReady)
    {
        // It's okay if we don't find any system clients. They aren't guaranteed to exist.
        result = Result::Success;
    }

    return DevDriverToDDResult(result);
}

bool Tool::SystemClientDiscoveryFunc(void* pUserdata, const DiscoveredClientInfo& info)
{
    LOG_INFO("Discovered client %d - 0x%x (0x%x)", info.id, info.id, info.id & kRouterPrefixMask);

    DD_ASSERT(pUserdata != nullptr);

    Tool* pTool = reinterpret_cast<Tool*>(pUserdata);

    // Gfx Kmd
    if (Platform::Strcmpi("Radeon Developer Driver System Client", info.clientInfo.data.clientDescription) == 0)
    {
        if (pTool->m_systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_GRAPHICS_DRIVER].id == kBroadcastClientId)
        {
            pTool->m_systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_GRAPHICS_DRIVER].id = info.id;
        }
        else
        {
            DD_PRINT(LogLevel::Warn, "Found second GfxKmd system client - this is unexpected and will be ignored.");
        }
    }
    // Utility Driver
    else if (Platform::Strcmpi("Amd Utility Driver System Client", info.clientInfo.data.clientDescription) == 0)
    {
        if (pTool->m_systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_UTILITY_DRIVER].id == kBroadcastClientId)
        {
            pTool->m_systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_UTILITY_DRIVER].id = info.id;
        }
        else
        {
            DD_PRINT(LogLevel::Warn, "Found second Utility Driver system client - this is unexpected and will be ignored.");
        }
    }
    // Anything else is from a standard ddRouter instance
    else
    {
        if (pTool->m_systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_ROUTER].id == kBroadcastClientId)
        {
            pTool->m_systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_ROUTER].id = info.id;
        }
        else
        {
            DD_PRINT(LogLevel::Warn, "Found multiple standard router clients - this will be ignored.");
        }
    }

    // Once we have all three client types found, we can stop searching
    return (
        (pTool->m_systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_ROUTER         ].id == kBroadcastClientId) ||
        (pTool->m_systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_GRAPHICS_DRIVER].id == kBroadcastClientId) ||
        (pTool->m_systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_UTILITY_DRIVER ].id == kBroadcastClientId)
    );
}

void Tool::RouterDisconnectListener()
{
    IMsgChannel* pMsgChannel = reinterpret_cast<IMsgChannel*>(m_net);

    bool routerDisconnected = false;

    while (m_routerDisconnectListenerStop == false)
    {
        if (pMsgChannel->IsConnected() == false)
        {
            routerDisconnected = true;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(kRouterDisconnectListenerSleepMillisec));
    }

    if (routerDisconnected)
    {
        LOG_INFO("Lost connection to router unexpectedly.");
    }
    else
    {
        LOG_INFO("Disconnected from router.");
    }

    m_connection.CloseDriverConnections();

    pMsgChannel->Unregister();

    m_routerUtils.ClearAfterRouterDisconnect();
    m_uberTrace.ClearAfterRouterDisconnect();
    m_clocks.ClearAfterRouterDisconnect();
    m_enhancedCrashInfo.ClearAfterRouterDisconnect();
    m_memoryTrace.ClearAfterRouterDisconnect();
    m_gpuDetective.ClearAfterRouterDisconnect();
    m_profiling.ClearAfterRouterDisconnect();
    m_pipelines.ClearAfterRouterDisconnect();
    m_settings.ClearAfterRouterDisconnect();
    m_eventLogging.ClearAfterRouterDisconnect();

    m_connection.OnRouterDisconnected();

    // Reset connection ids.
    for (int i = 0; i < DD_MODULE_SYSTEM_CLIENT_TYPE_COUNT; ++i)
    {
        m_systemClients[i].id = kBroadcastClientId;
    }

    ddNetDestroyConnection(m_net);
    m_net = DD_API_INVALID_HANDLE;

    m_connection.SetDDNet(m_net);
}

} // namespace DevDriver

namespace
{

// Below are wrapper functions for DDToolApi.

DD_RESULT LoadModules(DDToolInstance* pInstance)
{
    DevDriver::Tool* pTool = reinterpret_cast<DevDriver::Tool*>(pInstance);
    return pTool->LoadModules();
}

DDApiRegistry* GetApiRegistry(DDToolInstance* pInstance)
{
    DevDriver::Tool* pTool = reinterpret_cast<DevDriver::Tool*>(pInstance);
    return pTool->GetApiRegistry();
}

DD_RESULT Connect(DDToolInstance* pInstance, const char* pIpAddr, uint16_t port)
{
    DevDriver::Tool* pTool = reinterpret_cast<DevDriver::Tool*>(pInstance);
    return pTool->Connect(pIpAddr, port);
}

void Disconnect(DDToolInstance* pInstance)
{
    DevDriver::Tool* pTool = reinterpret_cast<DevDriver::Tool*>(pInstance);
    pTool->Disconnect();
}

DDClientId GetAmdlogClientId(DDToolInstance* pInstance)
{
    DevDriver::Tool* pTool = reinterpret_cast<DevDriver::Tool*>(pInstance);
    return pTool->GetAmdlogClientId();
}

DDNetConnection GetDDNetConnection(DDToolInstance* pInstance)
{
    DevDriver::Tool* pTool = reinterpret_cast<DevDriver::Tool*>(pInstance);
    return pTool->GetDDNetConnection();
}

} // anonymous namespace

DD_RESULT DDToolApiCreate(const DDToolApiCreateInfo* pCreateInfo, DDToolApi** ppOutTool)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    *ppOutTool = nullptr;

    if (pCreateInfo->pDescription && (pCreateInfo->descriptionSize > 0))
    {
        std::string desc(pCreateInfo->pDescription, pCreateInfo->descriptionSize);

        std::string modulesDir;
        if ((pCreateInfo->pModulesDir != nullptr) && (pCreateInfo->moduleDirSize > 0))
        {
            modulesDir = std::string(pCreateInfo->pModulesDir, pCreateInfo->moduleDirSize);
        }

        DevDriver::Tool* pTool = nullptr;

        if (pCreateInfo->customLogger.Log != nullptr)
        {
            pTool = new DevDriver::Tool(std::move(desc), std::move(modulesDir), pCreateInfo->customLogger);
        }
        else
        {
            std::string logFilePath;
            if ((pCreateInfo->pLogFilePath != nullptr) && (pCreateInfo->logFilePathSize > 0))
            {
                logFilePath = std::string(pCreateInfo->pLogFilePath, pCreateInfo->logFilePathSize);
            }

            pTool = new DevDriver::Tool(std::move(desc), std::move(modulesDir), std::move(logFilePath));
        }

        TimeoutConstants timeouts         = {};
        timeouts.retryTimeoutInMs         = pCreateInfo->retryTimeoutInMs;
        timeouts.communicationTimeoutInMs = pCreateInfo->communicationTimeoutInMs;
        timeouts.connectionTimeoutInMs    = pCreateInfo->connectionTimeoutInMs;

        TimeoutConstantsInitialize(&timeouts);

        result = pTool->Initialize();
        if (result == DD_RESULT_SUCCESS)
        {
            *ppOutTool = new DDToolApi {
                reinterpret_cast<DDToolInstance*>(pTool),
                LoadModules,
                GetApiRegistry,
                Connect,
                Disconnect,
                GetAmdlogClientId,
                GetDDNetConnection};
        }
        else
        {
            delete pTool;
        }
    }
    else
    {
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    return result;
}

void DDToolApiDestroy(DDToolApi** ppToolApi)
{
    if (ppToolApi != nullptr && *ppToolApi != nullptr)
    {
        if ((*ppToolApi)->pInstance != nullptr)
        {
            DevDriver::Tool* pTool = reinterpret_cast<DevDriver::Tool*>((*ppToolApi)->pInstance);
            delete pTool;
        }
        delete *ppToolApi;
        *ppToolApi = nullptr;
    }
}
