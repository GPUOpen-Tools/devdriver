/* Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved. */
#pragma once

#include <cstring>
#include <cerrno>

/**
 * @file rdf_platform.h
 * @brief Cross-platform wrappers for memory and string functions
 *
 * This header provides safe wrappers of common c functions.
 */

namespace rdf {
namespace platform {

/**
 * @brief Safe memcpy wrapper
 * @param dest Destination buffer
 * @param destSize Size of the destination buffer in bytes
 * @param src Source buffer
 * @param count Number of bytes to copy
 * @return 0 on success, non-zero on error
 */
inline int memcpy_s(void* dest, size_t destSize, const void* src, size_t count)
{
#if defined(_MSC_VER)
    return ::memcpy_s(dest, destSize, src, count);
#else
    if (dest == nullptr || src == nullptr)
    {
        return EINVAL;
    }

    if (count > destSize)
    {
        return ERANGE;
    }

    // Check for overlap (undefined behavior for memcpy, except when dest == src)
    const char* srcBytes = static_cast<const char*>(src);
    char* destBytes = static_cast<char*>(dest);

    if (destBytes != srcBytes &&
        ((destBytes < srcBytes + count) && (srcBytes < destBytes + count)))
    {
        // Overlapping buffers - this is undefined behavior for memcpy
        return EINVAL;
    }

    ::memcpy(dest, src, count);
    return 0;
#endif
}

/**
 * @brief Safe memmove wrapper
 * @param dest Destination buffer
 * @param destSize Size of the destination buffer in bytes
 * @param src Source buffer
 * @param count Number of bytes to move
 * @return 0 on success, non-zero on error
 */
inline int memmove_s(void* dest, size_t destSize, const void* src, size_t count)
{
#if defined(_MSC_VER)
    return ::memmove_s(dest, destSize, src, count);
#else
    if (dest == nullptr || src == nullptr)
    {
        return EINVAL;
    }

    if (count > destSize)
    {
        return ERANGE;
    }

    ::memmove(dest, src, count);
    return 0;
#endif
}

} // namespace platform
} // namespace rdf
