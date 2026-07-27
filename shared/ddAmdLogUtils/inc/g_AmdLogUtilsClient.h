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

    /// Query the current settings for all supported kernel components
    DD_RESULT QueryAllCurrentKernelValues(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer
    );

    /// Query the default JSON settings blob for all supported kernel components
    DD_RESULT QueryKernelSettingsBlobsAll(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer
    );

    /// Send the RGD config parameters for KMD to use in OCA dumps
    DD_RESULT SendRgdOcaConfig(
        const void* pParamBuffer,
        size_t      paramBufferSize
    );

    /// Send the OCA high overhead config parameters for KMD to use in OCA dumps
    DD_RESULT SetOcaHighOverheadConfig(
        const void* pParamBuffer,
        size_t      paramBufferSize
    );

private:
    DDRpcClient m_hClient = DD_API_INVALID_HANDLE;
};

} // namespace AmdLogUtils
