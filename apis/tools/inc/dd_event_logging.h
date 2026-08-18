/* Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_event_logging_api.h>
#include <dd_connection_api.h>

#include <dd_api_registry_api.h>
#include <dd_logger_api.h>
#include <dd_mutex.h>
#include <dd_router_utils_api.h>

#include <ddRdf.h>
#include <ddNet.h>
#include <ddModule.h>
#include <ddEventParser.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace DevDriver
{

class RdfEventStreamer;

class EventLogging
{
    DDNetConnection  m_net;
    DDConnectionApi* m_pConnectionApi;
    DevDriver::Mutex m_clientsMutex;
    DDLoggerApi*     m_pLogger;
    std::unordered_map<DDConnectionId, RdfEventStreamer*> m_traceEnabledClients;
    DDEventReceiveEventCallback m_receiveCallback;

public:
    EventLogging();
    ~EventLogging();

    DD_RESULT Initialize(DDApiRegistry* pApiRegistry);
    void      ClearAfterRouterDisconnect();

    void      SetRpcClientInfo(DDNetConnection ddNet);

    DD_RESULT EnableTracing(DDConnectionId connectionId, DDProcessId processId, uint32_t providerId, bool noUmdConnection);
    DD_RESULT RegisterEventReceiveCb(const DDEventReceiveEventCallback* pReceiveCallback);
    void      ReceiveStreamerEvent(DDEventParserEventInfo eventInfo, const void* pEventDataPayload, size_t eventDataPayloadSize);
    void      DisableTracing(DDConnectionId connectionId);
    DD_RESULT EndTracing(DDConnectionId connectionId, bool isClientInitialized);
    DD_RESULT TransferTraceData(DDConnectionId         connectionId,
                                const DDRdfFileWriter* pRdfFileWriter,
                                const DDIOHeartbeat*   pHeartbeat);

private:
    DD_RESULT EndTraceInternal(DevDriver::RdfEventStreamer* pStreamer, bool isClientInitialized);

    std::vector<char> m_eventPayloadBuffer;
};

} // namespace DevDriver
