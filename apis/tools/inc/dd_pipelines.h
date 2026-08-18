/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_api_registry_api.h>
#include <dd_pipelines_api.h>
#include <protocols/ddURIClient.h>
#include <dd_api_registry_api.h>
#include <dd_connection_api.h>
#include <dd_logger_api.h>
#include <dd_mutex.h>
#include <ddNet.h>

#include <unordered_map>
#include <cstdint>
#include <mutex>

namespace DevDriver
{
class Pipelines
{
    using Client = DevDriver::URIProtocol::URIClient;

public:
    Pipelines();
    ~Pipelines();

    DD_RESULT Initialize(DDApiRegistry* pApiRegistry);
    void ClearAfterRouterDisconnect();
    void SetRpcClientInfo(DDNetConnection ddNet);

    DD_RESULT Connect(DDConnectionId umdConnectionId);
    void Disconnect(DDConnectionId umdConnectionId);

    DD_RESULT DumpDriverPipelines(
        DDConnectionId           umdConnectionId,
        DDPipelineRecordCallback pCallback,
        void*                    pUserdata);

    DD_RESULT InjectPipelines(
        DDConnectionId                   umdConnectionId,
        const DDPipelinesCodeObjectData* pObjects,
        size_t                           numObjects);

private:
    DDNetConnection                             m_net;
    DDConnectionApi*                            m_pConnectionApi;
    DDLoggerApi*                                m_pLogger;
    DevDriver::Mutex                            m_clientsMutex;
    std::unordered_map<DDConnectionId, Client*> m_traceEnabledClients;
};
} // namespace DevDriver
