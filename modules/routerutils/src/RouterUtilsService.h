/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include "g_RouterUtilsRpcService.h"
#include <dd_clocks_api.h>
#include <util/ddStructuredReader.h>
#include <util/vector.h>
#include <dd_mutex.h>
#include <dd_thread.h>

#if defined(DD_TARGET_PLATFORM_LINUX) || defined(DD_TARGET_PLATFORM_ANDROID)
#include <amdgpu.h>
#include <unordered_map>
#endif

#include <thread>

namespace RouterUtilsModule
{

class RouterUtilsService : public RouterUtilsRpc::IRouterUtilsRpcService
{
private:

#if defined(DD_TARGET_PLATFORM_LINUX) || defined(DD_TARGET_PLATFORM_ANDROID)
    struct AmdGpuDevice
    {
        amdgpu_context_handle hGpuContext;
        amdgpu_device_handle  hGpuDevice;
        uint32_t              majorVersion;
        uint32_t              minorVersion;
    };

    std::unordered_map<DDGpuId, AmdGpuDevice> m_gpuDevices;
#endif

    DevDriver::Thread       m_sysInfoThread;
    DevDriver::Vector<char> m_sysInfo;
    DevDriver::RWLock       m_sysInfoLock;

public:
    RouterUtilsService(DDLoggerInfo logger);

    ~RouterUtilsService() override;

private:
    void QueryAndCacheSystemInfoAsync();
    static void QueryAndCacheSystemInfo(void* pUserdata);

public:
    ////////////////////////////////
    // RPC Function Implementations

    DD_RESULT QuerySystemInfo(const DDByteWriter& writer) override;

    DD_RESULT QueryPathByProcessId(
        const void* pParamBuffer,
        size_t paramBufferSize,
        const DDByteWriter& writer) override;

    DD_RESULT QueryTimestampAndFrequency(const DDByteWriter& writer) override;

    DD_RESULT QueryDeviceClocks(
        const void* pParamBuffer, size_t paramBufferSize, const DDByteWriter& writer) override;

    DD_RESULT QueryCurrentClockMode(
        const void* pParamBuffer, size_t paramBufferSize, const DDByteWriter& writer) override;

    DD_RESULT SetClockMode(const void* pParamBuffer, size_t paramBufferSize) override;
};

} // namespace RouterUtilsModule
