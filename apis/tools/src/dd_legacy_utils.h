/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <stdint.h>

#include <ddApi.h>
#include <dd_logger_api.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Utility functions to convert between old DevDriver interfaces and new ones

// Conversion functions for DDLogger:
int32_t WillLogLegacy(void* pUserdata, const DDLogEvent* pEvent);
void    LoggerLogLegacy(void* pUserdata, const DDLogEvent* pEvent, const char* pMessage);

void* AllocCbFuncLegacy(void* pUserdata, size_t size, size_t alignment, int zero);
void  FreeCbFuncLegacy(void* pUserdata, void* pMemory);

#ifdef __cplusplus
}
#endif
