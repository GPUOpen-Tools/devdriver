/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_memory_trace_api.h>
#include <dd_router_utils_api.h>
#include <dd_api_registry_api.h>
#include <dd_connection_api.h>
#include <dd_logger_api.h>
#include <dd_mutex.h>
#include <ddNet.h>
#include <ddModule.h>
#include <RmtEventTracer.h>
#include <unordered_map>

// Forward declaration.
struct DDRdfFileWriter;

namespace DevDriver
{

class MemoryTrace
{
    using TraceClientMap = std::unordered_map<uint32_t, RmtEventTracer*>;

private:
    DDNetConnection                 m_net;
    DDRouterUtilsApi*               m_pRouterUtilsApi;
    DDConnectionApi*                m_pConnectionApi;
    Vector<uint8_t>                 m_sysInfoBuffer;
    const DDModuleSystemClientInfo* m_pSystemClients;
    RWLock                          m_clientsMutex;
    TraceClientMap                  m_traceEnabledClients;
    DDLoggerApi*                    m_pLogger;

    DDClientId GetGfxKernelId() const;
    DDClientId GetAmdLogId() const;
    DDClientId GetRouterId() const;

public:
    MemoryTrace();
    ~MemoryTrace();

    void OnDriverConnected(const DDConnectionInfo* pConnInfo);
    DD_RESULT Initialize(DDApiRegistry* pApiRegistry);
    DD_RESULT UpdateSysInfoBuffer();
    void      ClearAfterRouterDisconnect();
    void      SetRpcClientInfo(DDNetConnection ddNet);
    void      SetSystemClients(const DDModuleSystemClientInfo* pSysClients);
    DD_RESULT EnableTracing(DDConnectionId umdConnectionId, ProcessId processId, bool useKmd);
    DD_RESULT DisableTracing(DDConnectionId umdConnectionId);
    DD_RESULT EndTracing(DDConnectionId umdConnectionId, bool isClientInitialized);
    DD_RESULT DumpTrace(DDConnectionId umdConnectionId, bool isClientInitialized);
    DD_RESULT AbortTrace(DDConnectionId umdConnectionId, bool isClientInitialized);
    DD_RESULT InsertSnapshot(DDConnectionId umdConnectionId, const char* pSnapshotName);
    DD_RESULT ClearTrace(DDConnectionId umdConnectionId);
    DD_RESULT QueryStatus(DDConnectionId umdConnectionId, DDMemoryTraceStatus* pStatus);
    DD_RESULT TransferTraceData(DDConnectionId         umdConnectionId,
                                const DDRdfFileWriter* pFileWriter,
                                const DDIOHeartbeat*   pIoCb,
                                bool                   useCompression);
};

} // namespace DevDriver
