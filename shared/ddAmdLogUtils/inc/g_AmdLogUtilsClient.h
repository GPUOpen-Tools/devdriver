/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddRpcClient.h>

namespace AmdLogUtils
{

class AmdLogUtilsClient
{
public:
    AmdLogUtilsClient();
    ~AmdLogUtilsClient();

    DD_RESULT Connect(const DDRpcClientCreateInfo& info);
    DD_RESULT IsServiceAvailable();
    DD_RESULT GetServiceInfo(DDApiVersion* pVersion);

    /// Queries the list of supported clock modes
    DD_RESULT QueryDeviceClocks(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer
    );

    /// Queries which clock mode is currently active
    DD_RESULT QueryCurrentClockMode(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer
    );

    /// Requests that the current clock mode be changed to the provided one
    DD_RESULT SetClockMode(
        const void* pParamBuffer,
        size_t      paramBufferSize
    );

    /// Query the current Enhanced Crash Info configuration
    DD_RESULT QueryEnhancedCrashInfoConfig(
        const DDByteWriter& writer
    );

    /// Set the Enhanced Crash Info configuration
    DD_RESULT SetEnhancedCrashInfoConfig(
        const void* pParamBuffer,
        size_t      paramBufferSize
    );

    // NOTE: QueryAllCurrentKernelValues and QueryKernelSettingsBlobsAll are guarded below because
    // they expose internal kernel settings queries via the AmdLog service that must not be exposed
    // in open-source builds. If this file is regenerated from rpc-gen, these guards must be
    // reapplied. See featureList.json SettingsInternal feature.

    // NOTE: SendRgdOcaConfig and SetOcaHighOverheadConfig are guarded below because they represent
    // internal driver-to-driver RPC protocol (RGD -> KMD via AmdLog service) that must not be
    // exposed in open-source builds. If this file is regenerated from rpc-gen, these guards must
    // be reapplied. See featureList.json OcaConfig feature.

private:
    DDRpcClient m_hClient = DD_API_INVALID_HANDLE;
};

} // namespace AmdLogUtils
