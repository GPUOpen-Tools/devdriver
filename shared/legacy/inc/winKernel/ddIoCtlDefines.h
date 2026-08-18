/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddDevModeControl.h>
#include "ddWinKmIoCtlDefines.h"

namespace DevDriver
{

// -------------------------------------------------------------------------------------------------
// Windows only definitions and declarations
// -------------------------------------------------------------------------------------------------

// Returns the ioctl code for amdlog, to be used for Windows ioctl calls to amdlog
inline DWORD GetIoCtlCode(IoCtlType ioctlType)
{
#if defined(__clang__)
// error: signed shift result (0x7C07FC000) requires 36 bits to represent, but 'long' only has 32
// bits [-Wshift-overflow]
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshift-overflow"
#endif
    DWORD ioCtlRet = static_cast<DWORD>(DD_IOCTL_NUTCRACKER_AMDLOG_DEVDRIVER);
    switch (ioctlType)
    {
        case DevDriverInDirect:
            ioCtlRet = static_cast<DWORD>(DD_IOCTL_NUTCRACKER_AMDLOG_DEVDRIVER_IN_DIRECT);
            break;
        case DevDriverRgdOcaBuffered:
            ioCtlRet = static_cast<DWORD>(DD_IOCTL_NUTCRACKER_AMDLOG_RGD_OCA_STATUS);
            break;
        case DevDriverAdapterInfoBuffered:
            ioCtlRet = static_cast<DWORD>(DD_IOCTL_NUTCRACKER_AMDLOG_ADAPTER_INFO_QUERY);
            break;
        case DevDriverGpuIdBuffered:
            ioCtlRet = static_cast<DWORD>(DD_IOCTL_NUTCRACKER_AMDLOG_GPU_ID_QUERY);
            break;
        case DevDriverRgdMonitoringRequest:
            ioCtlRet = static_cast<DWORD>(DD_IOCTL_NUTCRACKER_RGD_MONITORING_REQUEST);
            break;
        case DevDriverDevModeCmdBuffered:
        default:
            ioCtlRet = static_cast<DWORD>(DD_IOCTL_NUTCRACKER_AMDLOG_DEVDRIVER);
            break;

    }
    return ioCtlRet;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
}

} // DevDriver
