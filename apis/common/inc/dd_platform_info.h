/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_common_api.h>
#include <dd_result.h>

namespace DevDriver
{
class PlatformInfo
{
public:
    /// Initialize the global PlatformInfo object.
    ///
    /// This call is thread-safe. Only the first call of this function does the actual initialization. It is highly
    /// recommended that this function is called once at the start of the program before calling the individual
    /// retrieval functions below.
    ///
    /// @return DD_RESULT_SUCCESS if initialization succeeded.
    /// @return other errors if initialization failed. All fields in PlatformInfo object will be set to default values.
    static ResultEx Init();

    /// Get the page size.
    ///
    /// This function will first initialize the global PlatformInfo object if it hasn't been initialized. It is
    /// recommended that `Init()` is called once at the start of the program before calling this function.
    static uint32_t GetPageSize();

    /// Get the cache line size.
    ///
    /// This function will first initialize the global PlatformInfo object if it hasn't been initialized. It is
    /// recommended that `Init()` is called once at the start of the program before calling this function.
    static uint32_t GetCacheLineSize();

    /// Get L1 cache size.
    ///
    /// This function will first initialize the global PlatformInfo object if it hasn't been initialized. It is
    /// recommended that `Init()` is called once at the start of the program before calling this function.
    static uint32_t GetL1CacheSize();

    /// Get L2 cache size.
    ///
    /// This function will first initialize the global PlatformInfo object if it hasn't been initialized. It is
    /// recommended that `Init()` is called once at the start of the program before calling this function.
    static uint32_t GetL2CacheSize();

    /// Get L3 cache size.
    ///
    /// This function will first initialize the global PlatformInfo object if it hasn't been initialized. It is
    /// recommended that `Init()` is called once at the start of the program before calling this function.
    static uint32_t GetL3CacheSize();
};

} // namespace DevDriver
