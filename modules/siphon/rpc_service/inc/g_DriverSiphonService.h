/* Copyright (C) 2022-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddRpcServer.h>

namespace DriverSiphon
{

class IDriverSiphonService
{
public:
    virtual ~IDriverSiphonService() {}

    // Queries all Settings blobs from client drivers.
    virtual DD_RESULT QuerySettingsBlobsAll(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer
    ) = 0;

    // Queries all the driver settings in the registry.
    virtual DD_RESULT QuerySettingsRegistryOverrides(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer
    ) = 0;

    // Clears a setting override.
    virtual DD_RESULT ClearSettingsRegistryOverride(
        const void* pParamBuffer,
        size_t      paramBufferSize
    ) = 0;

    // Writes a KMD registry override for a single setting on a specific GPU.
    // Windows-only; returns DD_RESULT_DD_GENERIC_UNAVAILABLE on other platforms.
    virtual DD_RESULT WriteKernelSettingOverride(
        const void* pParamBuffer,
        size_t      paramBufferSize
    ) = 0;

    // Performs a PnP disable/re-enable cycle on a specific GPU to reload the KMD
    // and pick up any registry overrides written since the last driver initialisation.
    // Windows-only; returns DD_RESULT_DD_GENERIC_UNAVAILABLE on other platforms.
    virtual DD_RESULT TriggerKernelPnpReload(
        const void* pParamBuffer,
        size_t      paramBufferSize
    ) = 0;

protected:
    IDriverSiphonService() {}
};

DD_RESULT RegisterService(DDRpcServer hServer, IDriverSiphonService* pService);

void UnRegisterService(DDRpcServer hServer);

} // namespace DriverSiphon
