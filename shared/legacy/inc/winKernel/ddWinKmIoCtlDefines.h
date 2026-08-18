/* Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#ifdef DD_PLATFORM_WINDOWS_UM
#include <winioctl.h>
#endif

#define DD_IOCTL_NUTCRACKER_AMDLOG_DEVDRIVER CTL_CODE (40000, 0x904, METHOD_BUFFERED, FILE_ALL_ACCESS)
#define DD_IOCTL_NUTCRACKER_AMDLOG_DEVDRIVER_IN_DIRECT                                                                 \
    CTL_CODE(40000, 0x905, METHOD_IN_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)
#define DD_IOCTL_NUTCRACKER_AMDLOG_RGD_OCA_STATUS                                                                      \
    CTL_CODE(40000, 0x906, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define DD_IOCTL_NUTCRACKER_AMDLOG_ADAPTER_INFO_QUERY                                                                   \
    CTL_CODE(40000, 0x907, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define DD_IOCTL_NUTCRACKER_AMDLOG_GPU_ID_QUERY                                                                   \
    CTL_CODE(40000, 0x908, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define DD_IOCTL_NUTCRACKER_RGD_MONITORING_REQUEST                                                                 \
    CTL_CODE(40000, 0x909, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

#pragma pack(1)

// ---------  IOCTL_NUTCRACKER_DEVDRIVER  defs -------------------------
struct nc_amdlog_devdriver_input
{
    uint32_t dev_mode_cmd;

    uint32_t process_id;
    uint32_t cmd_data_size;
    uint8_t  cmd_data[1]; // Start of command specific data buffer, see ddDevModeControlCmds.h for details
};

#pragma pack()
