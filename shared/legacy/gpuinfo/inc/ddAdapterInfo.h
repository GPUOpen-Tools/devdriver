/* Copyright (C) 2025-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <ddAdapterShared.h>
#include <util/vector.h>

namespace DevDriver
{
#pragma pack(push, 1)

static constexpr uint32_t DD_ADAPTERS_VERSION     = 1; // Version of the ddAdapters structure
static constexpr uint32_t DD_ADAPTER_INFO_VERSION = 2; // Version of the ddAmdAdapterInfo structure

static constexpr uint32_t DD_MAX_SERVICE_NAME_LENGTH = 256; // Max length of the service name string
static constexpr uint32_t DD_MAX_DRIVER_DIR_LENGTH   = 256; // Max length of the driver install directory string
static constexpr uint32_t DD_MAX_NAME_LENGTH         = 128; // Max length of the GPU name string

// This struct mirrors the data in AmdGpuInfo, but is used to pass data between the driver and tools.
struct ddAmdAdapterInfo
{
    uint32_t     version;
    uint32_t     size;                                       // Size of this structure in bytes
    uint32_t     success;                                    // Set to 1 on success, 0 otherwise
    char         name[DD_MAX_NAME_LENGTH];                   // Name of the AMD GPU
    char         driverInstallDir[DD_MAX_DRIVER_DIR_LENGTH]; // Path to the driver installation directory
    char         serviceName[DD_MAX_SERVICE_NAME_LENGTH];    // The service string from KMD for ETW traces.

    struct PciLocation
    {
        uint32_t bus;
        uint32_t device;
        uint32_t function;
    };
    // This can be used to uniquely identify a GPU in a system
    PciLocation pci;
    uint32_t    gpuId;    // (Bus << 16) | (Device << 8) | Function

    struct AsicInfo
    {
        uint32_t gpuIndex;       // Index of gpu as enumerated
        uint64_t gpuCounterFreq;

        uint32_t numShaderEngines;                                     // The number of shader engines
        uint32_t numShaderArraysPerEngine;                             // The number of shader arrays per shader engine
        uint32_t cuMask[kMaxShaderEngines][kMaxShaderArraysPerEngine]; // A mask describing which CUs are enabled
        uint32_t numCus;                                               // The number of compute units.

        struct Ids
        {
            uint32_t gfxEngineId; // Coarse-grain GFX engine ID (R800, SI, etc.)
            uint32_t family;      // Hardware family ID. Driver-defined identifier for a particular family of devices.
            uint32_t eRevId;      // Hardware revision ID. Driver-defined identifier for a particular device and
                                  // sub-revision in the hardware family designated by the familyId.
                                  // See AMDGPU_TAHITI_RANGE, AMDGPU_FIJI_RANGE, etc. as defined in amdgpu_asic.h.
            uint32_t revisionId;  // PCI revision ID. 8-bit value as reported in the device structure in the PCI config
                                  // space.  Identifies a revision of a specific PCI device ID.
            uint32_t deviceId;    // PCI device ID. 16-bit value device ID as reported in the PCI config space.
            uint32_t subsystemId; // The PCI ID or ACPI ID of the adapter's hardware subsystem.
            uint32_t vendorId;    // The PCI ID or ACPI ID of the adapter's hardware vendor.
            uint8_t  luid[8];     // The locally unique identifier for the adapter.
        };
        Ids ids;
    };
    AsicInfo asic;

    struct ClocksFreqRange
    {
        uint64_t min; // The minimum clock frequency for a component in Hz
        uint64_t max; // The maxmimum clock frequency for a component in Hz
    };
    ClocksFreqRange engineClocks;

    struct MemoryInfo
    {
        LocalMemoryType type;
        uint32_t        memOpsPerClock;
        uint32_t        busBitWidth;

        ClocksFreqRange clocksHz;

        struct HeapInfo
        {
            uint64_t physAddr;
            uint64_t size;
        };
        HeapInfo localHeap;
        HeapInfo invisibleHeap;

        uint64_t hbccSize; // Size of High Bandwidth Cache Controller (HBCC) memory segment.
                           // HBCC memory segment comprises of system and local video memory, where HW/KMD
                           // will ensure high performance by migrating pages accessed by hardware to local.
                           // This HBCC memory segment is only available on certain platforms.

        struct VaRange
        {
            uint64_t base;
            uint64_t size;
        };
        VaRange excludedVaRanges[kMaxExcludedVaRanges];
    };
    MemoryInfo memory;

    struct BigSwVersion
    {
        uint32_t Major;
        uint32_t Minor;
        uint32_t Misc;
    };
    BigSwVersion bigSwVersion;

    struct LibDrmVersion
    {
        uint32_t Major; // drm major version
        uint32_t Minor; // drm minor version
    };
    LibDrmVersion drmVersion;

    bool cpuHostAperEnabled;              // CPU host aperture
    bool isResizeableBarControlSupported; // Smart access memory

    size_t ToBuffer(uint8_t* pBuffer) const
    {
        if (pBuffer == nullptr)
        {
            return 0;
        }
        uint32_t copySize = 0;

        Platform::Memcpy_s(pBuffer + copySize, sizeof(version), &version, sizeof(version));
        copySize += sizeof(version);

        Platform::Memcpy_s(pBuffer + copySize, sizeof(size), &size, sizeof(size));
        copySize += sizeof(size);

        Platform::Memcpy_s(pBuffer + copySize, sizeof(success), &success, sizeof(success));
        copySize += sizeof(success);

        Platform::Memcpy_s(pBuffer + copySize, sizeof(name), name, sizeof(name));
        copySize += sizeof(name);

        Platform::Memcpy_s(pBuffer + copySize, sizeof(driverInstallDir), driverInstallDir, sizeof(driverInstallDir));
        copySize += sizeof(driverInstallDir);

        Platform::Memcpy_s(pBuffer + copySize, sizeof(serviceName), serviceName, sizeof(serviceName));
        copySize += sizeof(serviceName);

        Platform::Memcpy_s(pBuffer + copySize, sizeof(pci), &pci, sizeof(pci));
        copySize += sizeof(pci);

        Platform::Memcpy_s(pBuffer + copySize, sizeof(gpuId), &gpuId, sizeof(gpuId));
        copySize += sizeof(gpuId);

        Platform::Memcpy_s(pBuffer + copySize, sizeof(asic), &asic, sizeof(asic));
        copySize += sizeof(asic);

        Platform::Memcpy_s(pBuffer + copySize, sizeof(engineClocks), &engineClocks, sizeof(engineClocks));
        copySize += sizeof(engineClocks);

        Platform::Memcpy_s(pBuffer + copySize, sizeof(memory), &memory, sizeof(memory));
        copySize += sizeof(memory);

        Platform::Memcpy_s(pBuffer + copySize, sizeof(bigSwVersion), &bigSwVersion, sizeof(bigSwVersion));
        copySize += sizeof(bigSwVersion);

        Platform::Memcpy_s(pBuffer + copySize, sizeof(drmVersion), &drmVersion, sizeof(drmVersion));
        copySize += sizeof(drmVersion);

        Platform::Memcpy_s(pBuffer + copySize, sizeof(cpuHostAperEnabled), &cpuHostAperEnabled, sizeof(cpuHostAperEnabled));
        copySize += sizeof(cpuHostAperEnabled);

        Platform::Memcpy_s(pBuffer + copySize, sizeof(isResizeableBarControlSupported), &isResizeableBarControlSupported, sizeof(isResizeableBarControlSupported));
        copySize += sizeof(isResizeableBarControlSupported);

        return copySize;
    }

    void FromBuffer(const uint8_t* pBuffer)
    {
        if (pBuffer == nullptr)
        {
            return;
        }

        uint32_t offset = 0;

        Platform::Memcpy_s(&version, sizeof(version), pBuffer + offset, sizeof(version));
        offset += sizeof(version);

        Platform::Memcpy_s(&size, sizeof(size), pBuffer + offset, sizeof(size));
        offset += sizeof(size);

        Platform::Memcpy_s(&success, sizeof(success), pBuffer + offset, sizeof(success));
        offset += sizeof(success);

        Platform::Memcpy_s(name, sizeof(name), pBuffer + offset, sizeof(name));
        offset += sizeof(name);

        Platform::Memcpy_s(driverInstallDir, sizeof(driverInstallDir), pBuffer + offset, sizeof(driverInstallDir));
        offset += sizeof(driverInstallDir);

        Platform::Memcpy_s(serviceName, sizeof(serviceName), pBuffer + offset, sizeof(serviceName));
        offset += sizeof(serviceName);

        Platform::Memcpy_s(&pci, sizeof(pci), pBuffer + offset, sizeof(pci));
        offset += sizeof(pci);

        Platform::Memcpy_s(&gpuId, sizeof(gpuId), pBuffer + offset, sizeof(gpuId));
        offset += sizeof(gpuId);

        Platform::Memcpy_s(&asic, sizeof(asic), pBuffer + offset, sizeof(asic));
        offset += sizeof(asic);

        Platform::Memcpy_s(&engineClocks, sizeof(engineClocks), pBuffer + offset, sizeof(engineClocks));
        offset += sizeof(engineClocks);

        Platform::Memcpy_s(&memory, sizeof(memory), pBuffer + offset, sizeof(memory));
        offset += sizeof(memory);

        Platform::Memcpy_s(&bigSwVersion, sizeof(bigSwVersion), pBuffer + offset, sizeof(bigSwVersion));
        offset += sizeof(bigSwVersion);

        Platform::Memcpy_s(&drmVersion, sizeof(drmVersion), pBuffer + offset, sizeof(drmVersion));
        offset += sizeof(drmVersion);

        Platform::Memcpy_s(&cpuHostAperEnabled, sizeof(cpuHostAperEnabled), pBuffer + offset, sizeof(cpuHostAperEnabled));
        offset += sizeof(cpuHostAperEnabled);

        Platform::Memcpy_s(&isResizeableBarControlSupported, sizeof(isResizeableBarControlSupported), pBuffer + offset, sizeof(isResizeableBarControlSupported));
        offset += sizeof(isResizeableBarControlSupported);
    }
};

// The max num drivers, this mirrors MAX_NUM_DRIVERS in inc\nutcracker_amdlog_configuration.h
constexpr uint32_t kMaxDrivers = 32;

// This the payload to the Adapter IDs IOCTL.
struct ddAdapters
{
    uint32_t version;
    uint32_t numAdapters;         // The number of adapters in the system
    uint32_t gpuIds[kMaxDrivers]; // GPU ID Determined from (BusID << 16) | (DeviceID << 8) | FunctionID

    size_t ToBuffer(uint8_t* buffer) const
    {
        if (buffer == nullptr)
        {
            return 0;
        }
        size_t copySize = sizeof(ddAdapters);
        Platform::Memcpy_s(buffer, copySize, this, copySize);
        return copySize;
    }

    void FromBuffer(const uint8_t* buffer)
    {
        if (buffer == nullptr)
        {
            return;
        }

        Platform::Memcpy_s(this, sizeof(ddAdapters), buffer, sizeof(ddAdapters));
    }
};
#pragma pack(pop)

Result QueryAdapterInfo(Vector<ddAmdAdapterInfo>& gpus);

} // namespace DevDriver
