/* Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <stdint.h>

#include "dd_common_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define DD_ENHANCED_CRASH_INFO_API_NAME "DD_ENHANCED_CRASH_INFO_API"

#define DD_ENHANCED_CRASH_INFO_API_VERSION_MAJOR 1
#define DD_ENHANCED_CRASH_INFO_API_VERSION_MINOR 0
#define DD_ENHANCED_CRASH_INFO_API_VERSION_PATCH 0

typedef struct DDEnhancedCrashInfoInstance DDEnhancedCrashInfoInstance;

typedef struct DDEnhancedCrashInfoConfigFlags
{
    uint8_t  captureWaveData   : 1;
    uint8_t  enableSingleMemOp : 1;
    uint8_t  enableSingleAluOp : 1;
    uint8_t  captureVGPRData   : 1;
    uint8_t  captureSGPRData   : 1;
    uint32_t reserved          : 27;
} DDEnhancedCrashInfoConfigFlags;

/// struct for input/output of Enhanced Crash Info config
typedef struct DDEnhancedCrashInfoConfig
{
    uint64_t                       processId;
    DDEnhancedCrashInfoConfigFlags flags;
} DDEnhancedCrashInfoConfig;

typedef struct DDEnhancedCrashInfoApi
{
    /// An opaque pointer to the internal implementation of the EnhancedCrashInfo API.
    DDEnhancedCrashInfoInstance* pInstance;

   /// Queries the current Enhanced Crash Info config.
    ///
    /// @param      pInstance Must be \ref DDEnhancedCrashInfoApi.pInstance.
    /// @param[out] pEnhancedCrashInfoConfig the current config is written through this parameter
    /// @return     DD_RESULT_SUCCESS Query was successful.
    /// @return     DD_RESULT_COMMON_INVALID_PARAMETER If pointers are null or connection is invalid.
    /// @return     Other errors if query failed.
    DD_RESULT (*QueryEnhancedCrashInfoConfig)(DDEnhancedCrashInfoInstance* pInstance,
                                              DDEnhancedCrashInfoConfig*   pEnhancedCrashInfoConfig);

    /// Sets the Enhanced Crash Info config
    ///
    /// @param  pInstance Must be \ref DDEnhancedCrashInfoApi.pInstance.
    /// @param  pEnhancedCrashInfoConfig the configuration to set
    /// @return DD_RESULT_SUCCESS Request was successful.
    /// @return DD_RESULT_DD_GENERIC_UNAVAILABLE If the connection is invalid.
    /// @return Other errors if request failed.
    DD_RESULT (*SetEnhancedCrashInfoConfig)(DDEnhancedCrashInfoInstance*     pInstance,
                                            const DDEnhancedCrashInfoConfig* pEnhancedCrashInfoConfig);
} DDEnhancedCrashInfoApi;

#ifdef __cplusplus
} // extern "C"
#endif

