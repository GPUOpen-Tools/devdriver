/* Copyright (C) 2025-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddPlatform.h>

namespace DevDriver
{

/// Enumerates all of the types of local video memory which could be associated with a GPU
enum struct LocalMemoryType : uint32
{
    Unknown = 0,

    Ddr2,
    Ddr3,
    Ddr4,
    Gddr5,
    Gddr6,
    Hbm,
    Hbm2,
    Hbm3,
    Lpddr4,
    Lpddr5,
    Ddr5,

    Count
};

// Get memory ops per clock for a given LocalMemoryType
inline uint32 MemoryOpsPerClock(LocalMemoryType type)
{
    switch (type)
    {
        case LocalMemoryType::Unknown: return 0;
        case LocalMemoryType::Count:   return 0;

        case LocalMemoryType::Ddr2:    return 2;
        case LocalMemoryType::Ddr3:    return 2;
        case LocalMemoryType::Ddr4:    return 2;
        case LocalMemoryType::Gddr5:   return 4;
        case LocalMemoryType::Gddr6:   return 16;
        case LocalMemoryType::Hbm:     return 2;
        case LocalMemoryType::Hbm2:    return 2;
        case LocalMemoryType::Hbm3:    return 2;
        case LocalMemoryType::Lpddr4:  return 2;
        case LocalMemoryType::Lpddr5:  return 4;
        case LocalMemoryType::Ddr5:    return 4;
    }

    return 0;
};

// Get a printable string for a memory type.
// LocalMemoryType::Unknown or invalid enums return an empty string
inline const char* ToString(LocalMemoryType type)
{
    switch (type)
    {
        case LocalMemoryType::Unknown: return "";
        case LocalMemoryType::Count:   return "";

        case LocalMemoryType::Ddr2:    return "Ddr2";
        case LocalMemoryType::Ddr3:    return "Ddr3";
        case LocalMemoryType::Ddr4:    return "Ddr4";
        case LocalMemoryType::Gddr5:   return "Gddr5";
        case LocalMemoryType::Gddr6:   return "Gddr6";
        case LocalMemoryType::Hbm:     return "Hbm";
        case LocalMemoryType::Hbm2:    return "Hbm2";
        case LocalMemoryType::Hbm3:    return "Hbm3";
        case LocalMemoryType::Lpddr4:  return "Lpddr4";
        case LocalMemoryType::Lpddr5:  return "Lpddr5";
        case LocalMemoryType::Ddr5:    return "Ddr5";
    }

    return nullptr;
};

static constexpr uint32 kMaxShaderEngines         = 16;     // The maximum number of shader engines on an asic
static constexpr uint32 kMaxShaderArraysPerEngine = 16;     // The maximum shader arrays per shader engine on an asic
static constexpr uint64 kMaxExcludedVaRanges      = 0x20;

// Counts the number of 1 bits
inline uint32 CountSetBits(uint32 value)
{
    uint32 numberOfOnes = 0;
    for (uint8 digit = 0; digit < 32; digit++)
    {
        if ((value & (1 << digit)) != 0)
        {
            ++numberOfOnes;
        }
    }

    return numberOfOnes;
}

}
