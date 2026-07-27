/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_clocks_api.h>
#include <dd_api_registry_api.h>
#include <dd_router_utils_api.h>
#include <dd_logger_api.h>
#include <dd_mutex.h>
#include <ddNet.h>
#include <g_AmdLogUtilsClient.h>

namespace DevDriver
{
class Clocks
{
private:
    DDNetConnection   m_net;
    DDConnectionId    m_amdLogConnectionId;
    DDConnectionId    m_routerConnectionId;
    bool              m_isTargetLinux;
    bool              m_targetPlatformChecked;
    DDRouterUtilsApi* m_pRouterUtilsApi;

public:
    Clocks();
    ~Clocks();

    DD_RESULT Initialize(DDApiRegistry* pApiRegistry);
    void      ClearAfterRouterDisconnect();

    void SetRpcClientInfo(DDNetConnection ddNet, DDConnectionId amdLogConnectionId);

    // DDClocksApi
    DD_RESULT QueryClockModes(
        uint32_t*                    pNumClockModes,
        DDDeviceClocksClockModeInfo* pClockModes,
        DDGpuId                      gpuId);
    DD_RESULT QueryCurrentClockMode(DD_DEVICE_CLOCK_MODE* pClockModeId, DDGpuId gpuId);
    DD_RESULT SetClockMode(DD_DEVICE_CLOCK_MODE clockModeId, DDGpuId gpuId);

    // DDConnectionCallbacks
    void OnRouterConnected(DDConnectionId connectionId);

private:
    DD_RESULT PopulateClockModes(DDGpuId                         gpuId,
                                 DDDeviceClocksClockModeInfo*    pClockModes,
                                 AmdLogUtils::AmdLogUtilsClient* pAmdLogUtilsClient);
    void CheckTargetPlatform();
};

} // namespace DevDriver
