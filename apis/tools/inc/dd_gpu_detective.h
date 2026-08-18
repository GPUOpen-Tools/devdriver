/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_gpu_detective_api.h>
#include <dd_connection_api.h>

#include <dd_api_registry_api.h>
#include <dd_logger_api.h>
#include <dd_mutex.h>
#include <dd_router_utils_api.h>

#include <ddRdf.h>
#include <ddNet.h>
#include <ddModule.h>

#include <RmtEventTracer.h>

#include <cstdint>
#include <unordered_map>

class GPUDetectiveStreamer;

namespace DevDriver
{

class GpuDetective
{
    struct TraceEnabledClient
    {
        TraceChunkApiInfo          m_apiInfo;
        DevDriver::RmtEventTracer* m_rmtTracer;
        GPUDetectiveStreamer*      m_krnlCrashAnalysisStreamer;
        GPUDetectiveStreamer*      m_umdCrashAnalysisStreamer;
    };

    DDNetConnection                 m_net;
    const DDModuleSystemClientInfo* m_pSystemClients;
    Vector<uint8_t>                 m_sysInfoBuffer;
    DDRouterUtilsApi*               m_pRouterUtilsApi;
    DDConnectionApi*                m_pConnectionApi;

    RWLock                                                 m_clientsMutex;
    std::unordered_map<DDConnectionId, TraceEnabledClient> m_traceEnabledClients;

    DDLoggerApi* m_pLogger;

public:
    GpuDetective();
    ~GpuDetective();

    DD_RESULT Initialize(DDApiRegistry* pApiRegistry);
    void      ClearAfterRouterDisconnect();

    void OnDriverConnected(const DDConnectionInfo* pConnInfo);
    DD_RESULT UpdateSysInfoBuffer();

    void SetRpcClientInfo(DDNetConnection ddNet);
    void SetSystemClients(const DDModuleSystemClientInfo* pSysClients);

    DD_RESULT EnableTracing(DDConnectionId umdConnectionId, DDProcessId processId);
    void      DisableTracing(DDConnectionId umdConnectionId);
    DD_RESULT EndTracing(DDConnectionId umdConnectionId, bool isClientInitialized, bool* didDetectCrash);
    DD_RESULT TransferTraceData(DDConnectionId         umdConnectionId,
                                const DDRdfFileWriter* pRdfFileWriter,
                                const DDIOHeartbeat*   pHeartbeat);

private:
    DD_RESULT EndTraceInternal(TraceEnabledClient& client, RmtEventTracer::EndTraceReason endReason, bool isClientInitialized);
    static void CleanupClient(TraceEnabledClient& client);
};

} // namespace DevDriver
