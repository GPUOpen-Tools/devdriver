/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_api_registry.h>
#include <dd_modules_manager.h>
#include <dd_tool_connection.h>
#include <dd_router_utils.h>
#include <dd_uber_trace.h>
#include <dd_driver_utils.h>
#include <dd_memory_trace.h>
#include <dd_gpu_detective.h>
#include <dd_gpu_profiling.h>
#include <dd_clocks.h>
#include <dd_enhanced_crash_info.h>
#include <dd_pipelines.h>
#include <dd_settings.h>
#include <dd_event_logging.h>
#include <dd_amd_log_utils.h>

#include <dd_logger_api.h>
#include <dd_mutex.h>

#include <ddNet.h>
#include <ddModule.h>
#include <msgChannel.h>

#include <string>
#include <thread>

namespace DevDriver
{

class Tool
{
    std::string m_description;

    ApiRegistry   m_apiRegistryImpl;
    DDApiRegistry m_apiRegistry;

    ModulesManager m_modulesManager;

    std::string    m_logFilePath;
    DDLoggerApi    m_loggerApi;
    bool           m_isCustomLogger;

    DDNetConnection          m_net;
    DDModuleSystemClientInfo m_systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_COUNT];
    ToolConnection           m_connection;

    std::thread m_routerDisconnectListenerThread;
    bool        m_routerDisconnectListenerStop;

    RouterUtils                 m_routerUtils;
    UberTrace                   m_uberTrace;
    DriverUtils                 m_driverUtils;
    MemoryTrace                 m_memoryTrace;
    Clocks                      m_clocks;
    EnhancedCrashInfo           m_enhancedCrashInfo;
    GpuDetective                m_gpuDetective;
    GpuProfiling                m_profiling;
    Pipelines                   m_pipelines;
    Settings                    m_settings;
    EventLogging                m_eventLogging;
    AmdLogUtilsManager          m_amdLogUtils;

public:
    Tool(std::string&& description, std::string&& modulesDir);
    Tool(std::string&& description, std::string&& modulesDir, std::string&& logFilePath);
    Tool(std::string&& description, std::string&& modulesDir, DDLoggerApi logger);

    ~Tool();

    DD_RESULT Initialize();

    DD_RESULT      LoadModules();
    DDApiRegistry* GetApiRegistry();

    DD_RESULT Connect(const char* pIpAddr, uint16_t port);
    void      Disconnect();

    void     HandleClientHalted(const void* pEventData);
    uint32_t GetConnectionCount() { return m_connection.GetConnectionCount(); }

    DD_RESULT UpdateSystemContextInfo();

    DDClientId GetAmdlogClientId() { return m_systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_UTILITY_DRIVER].id; }
    DDNetConnection GetDDNetConnection() { return m_net; }

private:
    static bool SystemClientDiscoveryFunc(void* pUserdata, const DiscoveredClientInfo& info);

    // Currently the underlying netcode doesn't provide APIs for us to listen to router disconnect
    // event, so we call this function in a thread that repeatedly probes the router connectivity.
    // We don't need to listen for connect event, because we call `Connect()` directly.
    void RouterDisconnectListener();

    Tool(Tool&& manager)                 = delete;
    Tool(const Tool& manager)            = delete;
    Tool& operator=(Tool&& manager)      = delete;
    Tool& operator=(const Tool& manager) = delete;
};

} // namespace DevDriver
