/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_amd_log_utils_api.h>
#include <dd_api_registry_api.h>
#include <dd_logger_api.h>
#include <ddNet.h>
#include <g_AmdLogUtilsClient.h>

namespace DevDriver
{
class AmdLogUtilsManager
{
private:
    DDNetConnection m_net;
    DDConnectionId  m_amdLogConnectionId;

public:
    AmdLogUtilsManager();
    ~AmdLogUtilsManager();

    DD_RESULT Initialize(DDApiRegistry* pApiRegistry);
    void      ClearAfterRouterDisconnect();

    void SetRpcClientInfo(DDNetConnection ddNet, DDConnectionId amdLogConnectionId);

    // DDAmdLogUtilsApi
    DD_RESULT IsServiceAvailable();
    DD_RESULT GetServiceInfo(DDApiVersion* pVersion);
    DD_RESULT SendRgdOcaConfig(const void* pConfigData, size_t configDataSize);
    DD_RESULT SetOcaHighOverheadConfig(const void* pConfigData, size_t configDataSize);
};

} // namespace DevDriver
