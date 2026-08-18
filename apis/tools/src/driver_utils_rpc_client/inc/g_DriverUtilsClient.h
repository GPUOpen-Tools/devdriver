/* Copyright (C) 2022-2024 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddRpcClient.h>

namespace DriverUtils
{

class DriverUtilsClient
{
public:
    DriverUtilsClient();
    ~DriverUtilsClient();

    DD_RESULT Connect(const DDRpcClientCreateInfo& info);
    DD_RESULT IsServiceAvailable();
    DD_RESULT GetServiceInfo(DDApiVersion* pVersion);

    /// Informs driver we are collecting trace data
    DD_RESULT EnableTracing();

    /// Informs driver to enable crash analysis mode
    DD_RESULT EnableCrashAnalysisMode();

    /// Queries the driver for extended client info
    DD_RESULT QueryPalDriverInfo(
        const DDByteWriter& writer
    );

    /// Informs driver to enable different features: Tracing, CrashAnalysis, RT Shader Data Tokens, Debug Vmid
    DD_RESULT EnableDriverFeatures(
        const void* pParamBuffer,
        size_t      paramBufferSize
    );

    /// Sends a string to PAL to display in the driver overlay
    DD_RESULT SetOverlayString(
        const void* pParamBuffer,
        size_t      paramBufferSize
    );

    /// Set driver DbgLog's severity level
    DD_RESULT SetDbgLogSeverityLevel(
        const void* pParamBuffer,
        size_t      paramBufferSize
    );

    /// Set driver DbgLog's origination mask
    DD_RESULT SetDbgLogOriginationMask(
        const void* pParamBuffer,
        size_t      paramBufferSize
    );

    /// Modify driver DbgLog's origination mask
    DD_RESULT ModifyDbgLogOriginationMask(
        const void* pParamBuffer,
        size_t      paramBufferSize
    );

private:
    DDRpcClient m_hClient = DD_API_INVALID_HANDLE;
};

} // namespace DriverUtils
