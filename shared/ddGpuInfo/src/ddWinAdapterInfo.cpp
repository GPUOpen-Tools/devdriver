/* Copyright (C) 2025-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <ddAdapterInfo.h>
#include <win/ddWinKmIoCtlDevice.h>

namespace DevDriver
{

Result QueryAdapterInfo(Vector<ddAmdAdapterInfo>& gpus)
{
    WinKmIoCtlDevice ioctlDevice = WinKmIoCtlDevice();
    Result           result      = ioctlDevice.Initialize();
    ddAdapters       adapters    = {};

    if (result == Result::Success)
    {
        result = ioctlDevice.IoCtl(DevDriverGpuIdBuffered, sizeof(adapters), &adapters, sizeof(adapters), &adapters);
        // Return early if we fail or the version is wrong
        if (result != Result::Success)
        {
            return result;
        }

        if (adapters.version != DD_ADAPTERS_VERSION)
        {
            return Result::VersionMismatch;
        }
    }

    if (result == Result::Success)
    {
        for (uint32 i = 0; i < adapters.numAdapters; ++i)
        {
            ddAmdAdapterInfo adapterInfo = {};
            adapterInfo.asic.gpuIndex    = i;
            adapterInfo.gpuId            = adapters.gpuIds[i];
            adapterInfo.size             = sizeof(ddAmdAdapterInfo);
            result = ioctlDevice.IoCtl(DevDriverAdapterInfoBuffered,
                                       sizeof(adapterInfo),
                                       &adapterInfo,
                                       sizeof(adapterInfo),
                                       &adapterInfo);
            // Return early if we fail or if the version doesn't match
            if (result != Result::Success)
            {
                return result;
            }

            if (adapterInfo.version != DD_ADAPTER_INFO_VERSION)
            {
                return Result::VersionMismatch;
            }

            gpus.PushBack(adapterInfo);
        }
    }

    return result;
}

} // namespace DevDriver
