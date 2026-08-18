/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_settings_rpc_types.h>
#include <dd_settings_base.h>
#include <dd_mutex.h>

#include <g_SettingsRpcService2.h>

#include <util/hashMap.h>
#include <util/vector.h>
#include <ddPlatform.h>

namespace DevDriver
{

class SettingsRpcService: public SettingsRpc::ISettingsRpcService
{
private:
    AllocCb m_allocCb;

    HashMap<const char*, SettingsBase*> m_settingsComponents;
    Mutex                               m_settingsComponentsMutex;

    // User-overrides for all settings components.
    uint8_t* m_pAllUserOverridesData;
    HashMap<const char*, Vector<DDSettingsValueRef>> m_allUserOverrides;

public:
    SettingsRpcService();
    ~SettingsRpcService();

    // Register a settings component to the settings rpc service. Also apply user-overrides to the registered
    // component if available.
    void RegisterSettingsComponent(SettingsBase* pSettingsComponent);

    // Removes a settings component from the settings rpc service.
    void UnRegisterSettingsComponent(SettingsBase* pSettingsComponent);

    // Apply all available user-overrides to the settings component pointed to by `pSettingsComponent`.
    void ApplyComponentUserOverrides(SettingsBase* pSettingsComponent);

    // Apply a single user-override identified by `nameHash`.
    bool ApplyUserOverride(
        SettingsBase*         pSettingsComponent,
        DD_SETTINGS_NAME_HASH nameHash,
        void*                 pSetting,
        size_t                settingSize);

    // Get number of user-overrides across all settings components.
    size_t TotalUserOverrideCount() const;

    // Settings RPC implementations
    DD_RESULT SendAllUserOverrides(const void* pParamBuf, size_t paramBufSize) override;
    DD_RESULT QueryAllCurrentValues(const DDByteWriter& writer) override;
    DD_RESULT GetUnsupportedExperiments(const DDByteWriter& writer) override;
};

} // namespace DevDriver
