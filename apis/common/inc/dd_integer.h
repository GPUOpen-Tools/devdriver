/* Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_common_api.h>
#include <dd_assert.h>
#include <cstdint>

namespace DevDriver
{

template<typename T>
uint32_t SafeCastToU32(T x)
{
    DD_ASSERT((x >= 0) && (x <= UINT32_MAX));
    return static_cast<uint32_t>(x);
}

template<typename T>
uint16_t SafeCastToU16(T x)
{
    DD_ASSERT((x >= 0) && (x <= UINT16_MAX));
    return static_cast<uint16_t>(x);
}

/// Find the smallest power of 2 that's greater than or equal to `x`.
/// Zero is returned if:
/// 1) `x` is 0
/// 2) the operation causes integer overflow
inline uint32_t NextSmallestPow2(uint32_t x)
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x;
}

/// Align a 32-bit integer to be multiples of `alignment`. `alignment` must be a power of 2.
/// Return 0 if either:
/// 1) \param x is 0
/// 2) \param alignment is 0
/// 3) the operation causes integer overflow.
inline uint32_t AlignU32(uint32_t x, uint32_t alignment)
{
    DD_ASSERT((alignment & (alignment - 1)) == 0);
    uint32_t aligned = (x + (alignment - 1)) & (~(alignment - 1));
    return aligned;
}

/// Similar to `AlignU32` but for 64-bit integers.
inline uint64_t AlignU64(uint64_t x, uint64_t alignment)
{
    DD_ASSERT((alignment & (alignment - 1)) == 0);
    uint64_t aligned = (x + (alignment - 1)) & (~(alignment - 1));
    return aligned;
}

} // namespace DevDriver
