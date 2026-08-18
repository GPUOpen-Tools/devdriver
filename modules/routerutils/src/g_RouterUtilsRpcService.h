/* Copyright (C) 2023-2024 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddRpcServer.h>

namespace RouterUtilsRpc
{

class IRouterUtilsRpcService
{
public:
    virtual ~IRouterUtilsRpcService() {}

    // Query the system info of the target machine. System info stays unchanged until the next system reboot.
    virtual DD_RESULT QuerySystemInfo(
        const DDByteWriter& writer
    ) = 0;

    // Queries the path of an application running on the target machine by its process id. The returned path string is UTF-8 encoded, and doesn't end with null-terminator.
    virtual DD_RESULT QueryPathByProcessId(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer
    ) = 0;

    // Queries a time stamp and frequency on the target machine. Time stamp is a monotonically increasing value representing the number of ticks since the last machine boot. Frequency represents the number of ticks per second.
    virtual DD_RESULT QueryTimestampAndFrequency(
        const DDByteWriter& writer
    ) = 0;

    // Queries the list of supported clock modes
    virtual DD_RESULT QueryDeviceClocks(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer
    ) = 0;

    // Queries which clock mode is currently active
    virtual DD_RESULT QueryCurrentClockMode(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer
    ) = 0;

    // Requests that the current clock mode be changed to the provided one
    virtual DD_RESULT SetClockMode(
        const void* pParamBuffer,
        size_t      paramBufferSize
    ) = 0;

protected:
    IRouterUtilsRpcService() {}
};

DD_RESULT RegisterService(DDRpcServer hServer, IRouterUtilsRpcService* pService);

void UnRegisterService(DDRpcServer hServer);

} // namespace RouterUtilsRpc
