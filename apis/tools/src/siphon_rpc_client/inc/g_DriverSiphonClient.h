/* Copyright (C) 2022-2025 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddRpcClient.h>

namespace DriverSiphon
{

class DriverSiphonClient
{
public:
    DriverSiphonClient();
    ~DriverSiphonClient();

    DD_RESULT Connect(const DDRpcClientCreateInfo& info);
    DD_RESULT IsServiceAvailable();
    DD_RESULT GetServiceInfo(DDApiVersion* pVersion);

    /// Queries all Settings blobs from client drivers.
    DD_RESULT QuerySettingsBlobsAll(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer
    );

    /// Queries all the driver settings in the registry.
    DD_RESULT QuerySettingsRegistryOverrides(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer
    );

    /// Clears a setting override.
    DD_RESULT ClearSettingsRegistryOverride(
        const void* pParamBuffer,
        size_t      paramBufferSize
    );

    /// Writes a KMD registry override for a single setting on a specific GPU.
    DD_RESULT WriteKernelSettingOverride(
        const void* pParamBuffer,
        size_t      paramBufferSize
    );

    /// Performs a PnP disable/re-enable cycle on a GPU to reload the KMD.
    DD_RESULT TriggerKernelPnpReload(
        const void* pParamBuffer,
        size_t      paramBufferSize
    );

private:
    DDRpcClient m_hClient = DD_API_INVALID_HANDLE;
};

} // namespace DriverSiphon
