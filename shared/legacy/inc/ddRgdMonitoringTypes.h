/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_common_api.h>
#include "ddPlatform.h"

namespace DevDriver
{

constexpr size_t MAX_PROCESS_NAME_SIZE = 256;

constexpr uint32_t RgdMonitoringVersionMajor = 1;
constexpr uint32_t RgdMonitoringVersionMinor = 0;
constexpr uint32_t RgdMonitoringVersionPatch = 0;

/// GPU dump options flags for RGD monitoring
/// Maps 1:1 with D3D12DDI_GPU_DUMP_OPTIONS_0121 from d3d12umddi.h
enum RgdGpuDumpOptions : uint32_t
{
    RgdGpuDumpOptionNoOverhead      = 0x1,   // D3D12DDI_GPU_DUMP_OPTION_NO_OVERHEAD
    RgdGpuDumpOptionMediumOverhead  = 0x2,   // D3D12DDI_GPU_DUMP_OPTION_MEDIUM_OVERHEAD
    RgdGpuDumpOptionHighOverhead    = 0x4,   // D3D12DDI_GPU_DUMP_OPTION_HIGH_OVERHEAD
    RgdGpuDumpOptionNoData          = 0x8,   // D3D12DDI_GPU_DUMP_OPTION_NO_DATA
    RgdGpuDumpOptionShaderRegisters = 0x10,  // D3D12DDI_GPU_DUMP_OPTION_SHADER_REGISTERS
    RgdGpuDumpOptionResources       = 0x20,  // D3D12DDI_GPU_DUMP_OPTION_RESOURCES
    RgdGpuDumpOptionEventMarkers    = 0x40,  // D3D12DDI_GPU_DUMP_OPTION_EVENT_MARKERS
};

/// Request structure for RGD monitoring IOCTL
/// Shared between UMD and RGD tool (kernel-mode component)
DD_NETWORK_STRUCT(RgdMonitoringRequest, 8)
{
    DDVersion version;                            // Version of this structure (12 bytes with padding)
    char      processName[MAX_PROCESS_NAME_SIZE]; // Executable name (256 bytes)
    uint32_t  options;                            // RgdGpuDumpOptions flags (4 bytes)
    uint8     padding[240];                       // Pad to 512 bytes (12+256+4+240=512)
};

DD_CHECK_SIZE(RgdMonitoringRequest, 512);

} // DevDriver
