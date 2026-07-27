/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_uber_trace_api.h>
#include <dd_api_registry_api.h>
#include <dd_logger_api.h>
#include <dd_mutex.h>

#include <ddNet.h>
#include <UberTraceClient.h>

#include <unordered_map>
#include <cstdint>

namespace DevDriver
{

class UberTrace
{
private:
    struct TraceEnabledClient
    {
        ::UberTrace::UberTraceClient m_uberTraceClient;
    };

    DDNetConnection m_net;

    DevDriver::Mutex                                       m_clientsMutex;
    std::unordered_map<DDConnectionId, TraceEnabledClient> m_traceEnabledClients;
    DDLoggerApi*                                           m_pLogger;

public:
    UberTrace();
    ~UberTrace();

    DD_RESULT Initialize(DDApiRegistry* pApiRegistry);
    void      ClearAfterRouterDisconnect();

    void SetRpcClientInfo(DDNetConnection ddNet);

    DD_RESULT Connect(DDConnectionId umdConnectionId);
    void      Disconnect(DDConnectionId umdConnectionId);

    DD_RESULT EnableTracing(DDConnectionId umdConnectionId);

    DD_RESULT ConfigureTraceParams(DDConnectionId umdConnectionId, const char* pData, size_t dataSize);
    DD_RESULT RequestTrace(DDConnectionId umdConnectionId);
    DD_RESULT CancelTrace(DDConnectionId umdConnectionId);

    DD_RESULT CollectTrace(DDConnectionId umdConnectionId, uint32_t timeoutInMs, const DDByteWriter* pWriter);
};

} // namespace DevDriver
