/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <util/ddMetroHash.h>

namespace DevDriver
{
namespace MetroHash
{

// Compacts a 128-bit hash into a 64-bit one by XOR'ing the low and high 64-bits together.
uint64 Compact64(
    const Hash* pHash)
{
    return (static_cast<uint64>(pHash->dwords[3] ^ pHash->dwords[1]) |
           (static_cast<uint64>(pHash->dwords[2] ^ pHash->dwords[0]) << 32));
}

// Compacts a 64-bit hash checksum into a 32-bit one by XOR'ing each 32-bit chunk together.
uint32 Compact32(
    const Hash* pHash)
{
    return pHash->dwords[3] ^ pHash->dwords[2] ^ pHash->dwords[1] ^ pHash->dwords[0];
}

uint64 HashCStr64(const char* pString, size_t maxLength)
{
    return MetroHash64(reinterpret_cast<const uint8*>(pString), DevDriver::Platform::Strlen_s(pString, maxLength));
}

} // MetroHash
} // DevDriver
