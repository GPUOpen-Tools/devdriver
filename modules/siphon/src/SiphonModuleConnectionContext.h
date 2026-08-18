/* Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <BaseModuleConnectionContext.h>
#include <ddApi.h>
#include <dd_settings_api.h>
#include <g_DriverSiphonService.h>

#include <vector>
#include <string>
#include <unordered_map>

namespace SiphonModule
{

class ModuleConnectionContext
    : public BaseModuleConnectionContext
    , public DriverSiphon::IDriverSiphonService
{
    struct SettingsBlob
    {
        std::string defaultDllPath;
        std::string currentDllPath;
        std::vector<uint8_t> blob;
    };

private:
    // The driver store path on Windows. This is the default place to load
    // client drivers. We will fail to load the correct drivers if driver
    // store is disabled.
    std::string m_windowsDriverInstallDir;

    SettingsBlob m_settingsBlobs[DD_SETTINGS_DRIVER_TYPE_COUNT];

#ifdef _WIN32
    // Keyed by DDGpuId (raw u32All); value is the HKLM registry subkey for the KMD class entry.
    std::unordered_map<uint32_t, std::string> m_kmdRegistryKeyByGpuId;
#endif

public:
    explicit ModuleConnectionContext(const DDModuleConnectionContextCreateInfo& createInfo);
    ~ModuleConnectionContext() override;

    DD_RESULT LoadSettingsBlobsFromAllDrivers();
    DD_RESULT RegisterRpcService();

    // === BaseModuleConnectionContext ===
    DD_RESULT Initialize() override;

    // === IDriverSiphonService ===
    DD_RESULT QuerySettingsBlobsAll(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer) override;

    // === IDriverSiphonService ===
    DD_RESULT QuerySettingsRegistryOverrides(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer) override;

    // === IDriverSiphonService ===
    DD_RESULT ClearSettingsRegistryOverride(
        const void* pParamBuffer,
        size_t      paramBufferSize) override;

    // === IDriverSiphonService ===
    DD_RESULT WriteKernelSettingOverride(
        const void* pParamBuffer,
        size_t      paramBufferSize) override;

    // === IDriverSiphonService ===
    DD_RESULT TriggerKernelPnpReload(
        const void* pParamBuffer,
        size_t      paramBufferSize) override;

private:
#ifdef _WIN32
    /// Methods to query the installation directory on Windows.
    void InitDriverInstallDir();
    void InitKmdRegistryKeyMap();
    std::string GetDriverInstallPathFromRegistry(const char* pDriverName);
#endif

    std::string GetDefaultDriverPath(DD_SETTINGS_DRIVER_TYPE driverType);
    std::string GetXglPathFromEnvVar();

    DD_RESULT LoadSettingsBlobFromDefaultPath(DD_SETTINGS_DRIVER_TYPE driverType);

    DD_RESULT LoadSettingsBlobs(
        const char*           driverPath,
        std::vector<uint8_t>& settingsBlobs);

    DD_RESULT WriteQuerySettingsBlobsAllResult(
        const DDByteWriter&   writer,
        const std::string&    driverPath,
        std::vector<uint8_t>& settingsBlobs);

    DD_RESULT WriteQuerySettingsBlobsAllResultWithReload(
        const DDByteWriter&     writer,
        const char*             pDriverPathOverride,
        size_t                  driverPathOverrideSize, // including null-terminator
        DD_SETTINGS_DRIVER_TYPE driverType,
        bool                    reload);

    DD_RESULT DecodePrivateSettingsBlobs(uint8_t* pBlobsAll, size_t blobsAllSize);
};

} // namespace SiphonModule
