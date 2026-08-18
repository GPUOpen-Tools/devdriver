/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddRpcClient.h>

namespace UberTrace
{

class UberTraceClient
{
public:
    UberTraceClient();
    ~UberTraceClient();

    DD_RESULT Connect(const DDRpcClientCreateInfo& info);

    /// Attempts to enable tracing
    DD_RESULT EnableTracing();

    /// Queries the current set of trace parameters
    DD_RESULT QueryTraceParams(
        const DDByteWriter& writer
    );

    /// Configures the current set of trace parameters
    DD_RESULT ConfigureTraceParams(
        const void* pParamBuffer,
        size_t      paramBufferSize
    );

    /// Requests execution of a trace
    DD_RESULT RequestTrace();

    /// Cancels a previously requested trace
    DD_RESULT CancelTrace();

    /// Collects the data created by a previously executed trace
    DD_RESULT CollectTrace(
        const DDByteWriter& writer
    );

private:
    DDRpcClient m_hClient = DD_API_INVALID_HANDLE;
};

} // namespace UberTrace
