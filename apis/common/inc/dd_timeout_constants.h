/* Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <cstdint>
#include <ddApi.h>

struct TimeoutConstants
{
    uint32_t connectionTimeoutInMs;
    uint32_t retryTimeoutInMs;
    uint32_t communicationTimeoutInMs;
};

extern TimeoutConstants g_timeoutConstants;

DD_RESULT TimeoutConstantsInitialize(const TimeoutConstants* pTimeouts);
