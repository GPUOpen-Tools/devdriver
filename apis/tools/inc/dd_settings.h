/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_api_registry_api.h>
#include <dd_common_api.h>
#include <dd_dynamic_buffer.h>
#include <dd_settings_api.h>

#include <ddNet.h>
#include <vector>

namespace DevDriver
{

class Settings
{
private:
    DDNetConnection m_net;
    DDProtocolId    m_netProtocolId;
    uint16_t        m_routerConnectionId;
    uint16_t        m_amdLogConnectionId;

public:
    Settings();
    ~Settings() = default;

    DD_RESULT Initialize(DDApiRegistry* pApiRegistry);

    void SetRpcClientInfo(DDNetConnection ddNet, uint16_t routerConnectionId, uint16_t amdLogConnectionId);
    void ClearAfterRouterDisconnect();

    void SetProtocolIdForTest(DDProtocolId id) { m_netProtocolId = id; }

    DD_RESULT QuerySettingsBlobsAll(
        DD_SETTINGS_DRIVER_TYPE driverType,
        const char*             pDriverPathOverride,
        size_t                  driverPathOverrideSize,
        bool                    reload,
        DynamicBuffer*          pOutBuf);

    DD_RESULT SendAllUserOverrides(
        size_t                              numComponents,
        const DDSettingsComponentValueRefs* pAllOverrides,
        uint16_t                            umdConnectionId);

    DD_RESULT QueryAllCurrentValues(DynamicBuffer* pOutBuf, uint16_t umdConnectionId);

    DD_RESULT GetUnsupportedExperiments(DynamicBuffer* pOutBuf, uint16_t umdConnectionId);

    DD_RESULT QueryRegistryOverrides(DD_SETTINGS_DRIVER_TYPE              driverType,
                                     const char*                          pBlobs,
                                     std::vector<DDSettingsRegistryInfo>& output);

    DD_RESULT ClearRegistryOverride(DD_SETTINGS_DRIVER_TYPE       driverType,
                                    const DDSettingsRegistryInfo* pRegistrySetting);

};

} // namespace DevDriver
