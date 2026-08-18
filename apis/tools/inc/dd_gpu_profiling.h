/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <atomic>

#include <dd_mutex.h>
#include <dd_common_api.h>
#include <dd_api_registry_api.h>
#include <dd_gpu_profiling_api.h>
#include <dd_logger_api.h>

#include <ddNet.h>
#include <protocols/rgpClient.h>
#include <protocols/driverControlClient.h>
#include "g_DriverUtilsClient.h"

namespace DevDriver
{
class GpuProfiling
{
private:
    struct Client
    {
        RGPProtocol::RGPClient                     rgpClient{nullptr};
        DriverControlProtocol::DriverControlClient driverControlClient{nullptr};
        ::DriverUtils::DriverUtilsClient           driverUtilsClient;
        std::vector<DDGpuProfilingSpmCounterId>    m_spmCounters;

        std::atomic_bool executing_trace = false;
        std::atomic_bool abort_trace = false;
    };

    DDNetConnection                             m_net;
    DevDriver::Mutex                            m_clientsMutex;
    std::unordered_map<DDConnectionId, Client*> m_clients;
    DDLoggerApi*                                m_pLogger;

public:
    GpuProfiling();
    ~GpuProfiling();

    DD_RESULT Initialize(DDApiRegistry* pApiRegistry);
    void ClearAfterRouterDisconnect();
    void SetRpcClientInfo(DDNetConnection ddNet);

    DD_RESULT EnableTracing(DDConnectionId umdConnectionId, const DDGpuProfilingConfig* pConfig);
    void DisableTracing(DDConnectionId umdConnectionId);

    DD_RESULT ExecuteTrace(DDConnectionId umdConnectionId, const DDGpuProfilingTraceArgs* pConfig);
    void AbortTrace(DDConnectionId umdConnectionId);

    DD_RESULT QueryClientProtocolVersion(DDConnectionId umdConnectionId, uint16_t* pVersion);

    DD_RESULT SetSpmCounters(DDConnectionId umdConnectionId, const DDGpuProfilingSpmCounterId* pCounters, uint32_t numCounters);

private:
    Result UpdateTraceParameters(Client& client, const DDGpuProfilingConfig* pConfig);
    void PopulateTraceConfig(const DDGpuProfilingConfig* pConfig, RGPProtocol::ClientTraceParametersInfo& rgpConfig);
    Result PopulateSpmConfig(Client&                                       client,
                             const DDGpuProfilingConfig*                   pConfig,
                             RGPProtocol::ClientSpmConfig*                 pSpmConfig,
                             std::vector<RGPProtocol::ClientSpmCounterId>* pCounters);

};
} // namespace DevDriver
