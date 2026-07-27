/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_common_api.h>
#include "ddPlatform.h"

namespace DevDriver
{
constexpr uint32_t AmdLogVersionMajor = 1;
constexpr uint32_t AmdLogVersionMinor = 0;

struct AmdLogEventVersion
{
    uint16_t major;
    uint16_t minor;
};

enum struct AmdLogEventInfoFlags : uint32
{
    Default     = 0,
    RealTime    = 1,
};

/// @deprecated Do not use
enum AmdlogEventId
{
    IfVersion,
    String,
    Count
};

DD_NETWORK_STRUCT(AmdLogEventInfo, 8)
{
    uint32_t sourceId;
    uint32_t eventId;
    uint32_t flags;
    DDGpuId  gpuId;
    void*    pData;
    size_t   dataSize;
};

enum RgdState
{
    RgdStateMonitoringNotEnabled = 0,
    RgdStateMonitoringEnabledNotLaunched,
    RgdStateEarlyConnection,
    RgdStateConnectionPostDeviceInit,
    RgdStateDisconnectedPostDeviceInitNoCrash,
    RgdStateDisconnectedPostDeviceInitTraceError,
    RgdStateDisconnectedEarlyNoCrash,
    RgdStateDisconnectedTraceCaptured
};

constexpr size_t MAX_STRING_SIZE = 256;

DD_NETWORK_STRUCT(RgdOcaClientUpdate, 256)
{
    char     RgdFilePath[MAX_STRING_SIZE];
    char     AppName[MAX_STRING_SIZE];
    RgdState state;
    uint8    padding[252];
};

DD_NETWORK_STRUCT(OcaCaptureConfig, 16)
{
    bool     enablePfResetAction;
    bool     enableStablePstate;
    bool     enableStallOnFault;
    bool     captureWaveData;
    bool     captureVGPRData;
    bool     captureSGPRData;
    bool     enableSingleMemOp;
    bool     enableSingleAluOp;
    uint8    padding[8];
};

}
