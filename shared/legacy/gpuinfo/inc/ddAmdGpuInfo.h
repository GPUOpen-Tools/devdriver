/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddPlatform.h>
#include <util/vector.h>
#include <ddAdapterShared.h>

#include <string>

namespace DevDriver
{

/// An amalgamation of information about a single GPU
/// This GPU will have identified as AMD when initially queried
/// There is an InfoService node in ListenerCore that mirrors this struct into Json
struct AmdGpuInfo
{
    char name[128];             // Name of the AMD GPU
    char driverInstallDir[256]; // Path to the driver installation directory

    struct PciLocation
    {
        uint32 bus;
        uint32 device;
        uint32 function;
    };
    // This can be used to uniquely identify a GPU in a system
    PciLocation pci;

    struct AsicInfo
    {
        uint32 gpuIndex;       // Index of gpu as enumerated
        uint64 gpuCounterFreq; // ???

        uint32 numShaderEngines;                                     // The number of shader engines
        uint32 numShaderArraysPerEngine;                             // The number of shader arrays per shader engine
        uint32 cuMask[kMaxShaderEngines][kMaxShaderArraysPerEngine]; // A mask describing which CUs are enabled
        uint32 numCus;                                               // The number of compute units.

        struct Ids
        {
            uint32 gfxEngineId;   // Coarse-grain GFX engine ID (R800, SI, etc.)
            uint32 family;        // Hardware family ID. Driver-defined identifier for a particular family of devices.
            uint32 eRevId;        // Hardware revision ID. Driver-defined identifier for a particular device and
                                  // sub-revision in the hardware family designated by the familyId.
                                  // See AMDGPU_TAHITI_RANGE, AMDGPU_FIJI_RANGE, etc. as defined in amdgpu_asic.h.
            uint32 revisionId;    // PCI revision ID. 8-bit value as reported in the device structure in the PCI config
                                  // space.  Identifies a revision of a specific PCI device ID.
            uint32 deviceId;      // PCI device ID. 16-bit value device ID as reported in the PCI config space.
            uint32 subsystemId;   // The PCI ID or ACPI ID of the adapter's hardware subsystem.
            uint32 vendorId;      // The PCI ID or ACPI ID of the adapter's hardware vendor.
            uint8  luid[8];       // The locally unique identifier for the adapter.
        };
        Ids ids;
    };
    AsicInfo asic;

    struct ClocksFreqRange
    {
        uint64 min; // The minimum clock frequency for a component in Hz
        uint64 max; // The maxmimum clock frequency for a component in Hz
    };
    ClocksFreqRange engineClocks;

    static constexpr uint64 kMaxExcludedVaRanges = 0x20;
    struct MemoryInfo
    {
        LocalMemoryType type;
        uint32          memOpsPerClock;
        uint32          busBitWidth;

        ClocksFreqRange clocksHz;

        struct HeapInfo
        {
            uint64 physAddr;
            uint64 size;
        };
        HeapInfo localHeap;
        HeapInfo invisibleHeap;

        uint64 hbccSize; // Size of High Bandwidth Cache Controller (HBCC) memory segment.
                         // HBCC memory segment comprises of system and local video memory, where HW/KMD
                         // will ensure high performance by migrating pages accessed by hardware to local.
                         // This HBCC memory segment is only available on certain platforms.

        struct VaRange
        {
            uint64 base;
            uint64 size;
        };
        VaRange excludedVaRanges[kMaxExcludedVaRanges];

        // Compute the memory bandwidth in bytes for a partially-filled out adapter
        // This is called as part of QueryGpuInfo.
        uint64 BandwidthInBytes() const
        {
            // Bit-Bandwidth is computed as the multiple of several properties:
            return static_cast<uint64>(busBitWidth)      // Bits per MemOp
                   * static_cast<uint64>(memOpsPerClock) // MemOps per MemClock
                   * clocksHz.max                        // MemClocks per second
                   / 8;                                  // Convert Bits to Bytes
        }
    };
    MemoryInfo memory;

    struct BigSwVersion
    {
        uint32 Major;
        uint32 Minor;
        uint32 Misc;
    };
    BigSwVersion bigSwVersion;

    struct LibDrmVersion
    {
        uint32_t Major; // drm major version
        uint32_t Minor; // drm minor version
    };
    LibDrmVersion drmVersion;

    AmdGpuInfo() { memset(this, 0, sizeof(*this)); }
};

// Query information about all AMD adapters in the system
Result QueryGpuInfo(const AllocCb& allocCb, Vector<AmdGpuInfo>* pGpus);

// Query service name from KMD for ETW traces
std::wstring QueryServiceString();

// The AmdGpuInfo::kMaxExcludedVaRanges is used in system info utils, so we can't just modify it.
// Since it is going away, there is no need to make the change to remove it.
static_assert(kMaxExcludedVaRanges == AmdGpuInfo::kMaxExcludedVaRanges, "Shared definition must match!");

} // namespace DevDriver
