/* Copyright (C) 2023-2024 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddRpcClient.h>

namespace RouterUtilsRpc
{

class RouterUtilsRpcClient
{
public:
    RouterUtilsRpcClient();
    ~RouterUtilsRpcClient();

    DD_RESULT Connect(const DDRpcClientCreateInfo& info);
    DD_RESULT IsServiceAvailable();
    DD_RESULT GetServiceInfo(DDApiVersion* pVersion);

    /// Query the system info of the target machine. System info stays unchanged until the next system reboot.
    DD_RESULT QuerySystemInfo(
        const DDByteWriter& writer
    );

    /// Queries the path of an application running on the target machine by its process id. The returned path string is UTF-8 encoded, and doesn't end with null-terminator.
    DD_RESULT QueryPathByProcessId(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer
    );

    /// Queries a time stamp and frequency on the target machine. Time stamp is a monotonically increasing value representing the number of ticks since the last machine boot. Frequency represents the number of ticks per second.
    DD_RESULT QueryTimestampAndFrequency(
        const DDByteWriter& writer
    );

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

private:
    DDRpcClient m_hClient = DD_API_INVALID_HANDLE;
};

} // namespace RouterUtilsRpc
