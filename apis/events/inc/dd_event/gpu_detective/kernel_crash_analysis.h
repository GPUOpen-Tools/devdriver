/* Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include "../common.h"
#include "ddPlatform.h"

#include <stdint.h>
#include <string.h>

using namespace DevDriver;

namespace KernelCrashAnalysisEvents
{
#pragma pack(push, 1)

constexpr uint32_t VersionMajor = 0;
constexpr uint32_t VersionMinor = 1;

constexpr uint32_t ProviderId = 0xE43C9C8E;

/// Unique id represeting each event. Each variable name of the enum value corresponds to the
/// struct with the same name.
enum class EventId : uint8_t
{
    PageFault         = DDCommonEventId::FirstEventIdForIndividualProvider,
    ShaderWaves       = DDCommonEventId::FirstEventIdForIndividualProvider + 1,
    SeInfo            = DDCommonEventId::FirstEventIdForIndividualProvider + 2,
    MmrRegisters      = DDCommonEventId::FirstEventIdForIndividualProvider + 3,
    WaveRegisters     = DDCommonEventId::FirstEventIdForIndividualProvider + 4,
    SgprVgprRegisters = DDCommonEventId::FirstEventIdForIndividualProvider + 5,
};

/// Data generated from kernel driver when a VM Page Fault happens.
struct PageFault
{
    uint32_t vmId;

    /// Process ID (PID) of the offending process.
    uint32_t processId;

    /// Page fault virtual address.
    uint64_t pageFaultAddress;

    /// Length of the process name.
    uint16_t processNameLength;

    /// The name of the offending process, encoded in UTF-8.
    uint8_t processName[64];

    void FromBuffer(const uint8_t* buffer)
    {
        Platform::Memcpy_s(&vmId, sizeof(vmId), buffer, sizeof(vmId));
        buffer += sizeof(vmId);

        Platform::Memcpy_s(&processId, sizeof(processId), buffer, sizeof(processId));
        buffer += sizeof(processId);

        Platform::Memcpy_s(&pageFaultAddress, sizeof(pageFaultAddress), buffer, sizeof(pageFaultAddress));
        buffer += sizeof(pageFaultAddress);

        Platform::Memcpy_s(&processNameLength, sizeof(processNameLength), buffer, sizeof(processNameLength));
        buffer += sizeof(processNameLength);

        Platform::Memcpy_s(processName, sizeof(processName), buffer, processNameLength);
    }

    /// Fill the pre-allocated `buffer` with the data of this struct. The size of
    /// the buffer has to be at least `sizeof(PageFault)` big.
    ///
    /// Return the actual amount of bytes copied into `buffer`.
    uint32_t ToBuffer(uint8_t* buffer) const
    {
        uint32_t copySize = 0;

        Platform::Memcpy_s(buffer + copySize, sizeof(vmId), &vmId, sizeof(vmId));
        copySize += sizeof(vmId);

        Platform::Memcpy_s(buffer + copySize, sizeof(processId), &processId, sizeof(processId));
        copySize += sizeof(processId);

        Platform::Memcpy_s(buffer + copySize, sizeof(pageFaultAddress), &pageFaultAddress, sizeof(pageFaultAddress));
        copySize += sizeof(pageFaultAddress);

        Platform::Memcpy_s(buffer + copySize, sizeof(processNameLength), &processNameLength, sizeof(processNameLength));
        copySize += sizeof(processNameLength);

        Platform::Memcpy_s(buffer + copySize, processNameLength, processName, processNameLength);
        copySize += processNameLength;

        return copySize;
    }
};

// offset and data of a single memory mapped register
struct MmrRegisterInfo
{
    uint32_t offset;
    uint32_t data;
};

// Note: Must exactly match KmdMmrRegistersEventData in KmdEventDefs.h
struct MmrRegistersData
{
    uint32_t version;

    // GPU identifier for these register events
    uint32_t gpuId;

    // number of MMrRegisterInfo structures which follow
    uint32_t numRegisters;

    // array of MMrRegisterInfo
    // actual array length is `numRegisters`
    MmrRegisterInfo registerInfos[1];

    static size_t CalculateStructureSize(uint32_t numRegisterInfoForCalculation)
    {
        numRegisterInfoForCalculation = Platform::Max(1U, numRegisterInfoForCalculation);
        return sizeof(MmrRegistersData) +
               sizeof(MmrRegisterInfo) * (numRegisterInfoForCalculation - 1);
    }

    static size_t CalculateBufferSize(uint32_t numRegisterInfoForCalculation)
    {
        return sizeof(MmrRegistersData) +
               sizeof(MmrRegisterInfo) * (numRegisterInfoForCalculation - 1);
    }

    static uint32_t GetNumMmrRegistersFromBuffer(const uint8_t *pBuffer)
    {
        pBuffer += offsetof(MmrRegistersData, numRegisters);
        return *reinterpret_cast<const uint32_t*>(pBuffer);
    }

    size_t FromBuffer(const uint8_t* pBuffer)
    {
        uint32_t numRegistersInBuffer = GetNumMmrRegistersFromBuffer(pBuffer);
        size_t   copySize             = CalculateBufferSize(numRegistersInBuffer);
        Platform::Memcpy_s(this, CalculateStructureSize(numRegistersInBuffer), pBuffer, copySize);
        return copySize;
    }

    size_t ToBuffer(uint8_t* pBuffer)
    {
        size_t copySize = CalculateBufferSize(numRegisters);
        Platform::Memcpy_s(pBuffer, copySize, this, copySize);
        return copySize;
    }
};

// Graphics Register Bus Manager status registers
struct GrbmStatusSeRegs
{
    uint32_t    version;
    uint32_t    grbmStatusSe0;
    uint32_t    grbmStatusSe1;
    uint32_t    grbmStatusSe2;
    uint32_t    grbmStatusSe3;
    // SE4 and SE5 are NV31 specific, 2x does not have this
    uint32_t    grbmStatusSe4;
    uint32_t    grbmStatusSe5;
};

// Note: Must exactly match KmdWaveInfo in KmdEventDefs.h
struct WaveInfo
{
    uint32_t    version;

    union
    {
        struct
        {
            unsigned int    waveId  : 5;
            unsigned int            : 3;
            unsigned int    simdId  : 2;
            unsigned int    wgpId   : 4;
            unsigned int            : 2;
            unsigned int    saId    : 1;
            unsigned int            : 1;
            unsigned int    seId    : 4;
            unsigned int    reserved: 10;
        };
        uint32_t        shaderId;
    };
};

// NOTE: HangType must match the Hangtype enum in kmdEventDefs.h
enum HangType : uint32_t
{
    pageFault     = 0,
    nonPageFault  = 1,
    Unknown       = 2,
};

// Note: Must exactly match KmdShaderWavesEventData in kmdEventDefs.h
struct ShaderWaves
{
    // structure version
    uint32_t         version;

    // GPU identifier for these register events
    uint32_t         gpuId;

    HangType         typeOfHang;
    GrbmStatusSeRegs grbmStatusSeRegs;

    uint32_t         numberOfHungWaves;
    uint32_t         numberOfActiveWaves;

    // aray of hung waves followed by active waves
    // KmdWaveInfo * [numberOfHungWaves]
    // KmdWaveInfo * [numberOfActiveWaves]
    WaveInfo         waveInfos[1];

    static size_t CalculateStructureSize(uint32_t numWaveInfoForCalculation)
    {
        numWaveInfoForCalculation = Platform::Max(1U, numWaveInfoForCalculation);
        return sizeof(ShaderWaves) +
               sizeof(WaveInfo) * (numWaveInfoForCalculation - 1);
    }

    static size_t CalculateBufferSize(uint32_t numWaveInfoForCalculation)
    {
        return sizeof(ShaderWaves) +
               sizeof(WaveInfo) * (numWaveInfoForCalculation - 1);
    }

    static uint32_t GetTotalNumWavesFromBuffer(const uint8_t *pBuffer)
    {
        uint32_t actualNumberOfHungWaves;
        uint32_t actualNumberOfActiveWaves;

        pBuffer += offsetof(ShaderWaves, numberOfHungWaves);
        actualNumberOfHungWaves   = *reinterpret_cast<const uint32_t*>(pBuffer);

        pBuffer += sizeof(numberOfHungWaves);
        actualNumberOfActiveWaves = *reinterpret_cast<const uint32_t*>(pBuffer);

        return actualNumberOfHungWaves + actualNumberOfActiveWaves;
    }

    size_t FromBuffer(const uint8_t* pBuffer)
    {
        uint32_t numWavesInBuffer = GetTotalNumWavesFromBuffer(pBuffer);
        size_t   copySize         = CalculateBufferSize(numWavesInBuffer);
        Platform::Memcpy_s(this, CalculateStructureSize(numWavesInBuffer), pBuffer, copySize);
        return copySize;
    }

    size_t ToBuffer(uint8_t* pBuffer)
    {
        size_t copySize = CalculateBufferSize(numberOfHungWaves + numberOfActiveWaves);
        Platform::Memcpy_s(pBuffer, copySize, this, copySize);
        return copySize;
    }
};

struct SeRegsInfo
{
    uint32_t version;
    uint32_t spiDebugBusy;
    uint32_t sqDebugStsGlobal;
    uint32_t sqDebugStsGlobal2;
};

struct SeInfo
{
    // structure version
    uint32_t   version;

    // GPU identifier for these register events
    uint32_t   gpuId;

    // number of SeRegsInfo structures in seRegsInfos array
    uint32_t   numSe;
    SeRegsInfo seRegsInfos[1];

    static size_t CalculateStructureSize(uint32_t numSeRegsInfoForCalculation)
    {
        numSeRegsInfoForCalculation = Platform::Max(1U, numSeRegsInfoForCalculation);
        return sizeof(SeInfo) +
               sizeof(SeRegsInfo) * (numSeRegsInfoForCalculation - 1);
    }

    static size_t CalculateBufferSize(uint32_t numSeRegsInfoForCalculation)
    {
        return sizeof(SeInfo) +
               sizeof(SeRegsInfo) * (numSeRegsInfoForCalculation - 1);
    }

    static uint32_t GetTotalSeRegsInfosFromBuffer(const uint8_t *pBuffer)
    {
        pBuffer += offsetof(SeInfo, numSe);
        return *reinterpret_cast<const uint32_t*>(pBuffer);
    }

    size_t FromBuffer(const uint8_t* pBuffer)
    {
        uint32_t numSeInBuffer = GetTotalSeRegsInfosFromBuffer(pBuffer);
        size_t   copySize      = CalculateBufferSize(numSeInBuffer);
        Platform::Memcpy_s(this, CalculateStructureSize(numSeInBuffer), pBuffer, copySize);
        return copySize;
    }

    size_t ToBuffer(uint8_t* pBuffer)
    {
        size_t copySize = CalculateBufferSize(numSe);
        Platform::Memcpy_s(pBuffer, copySize, this, copySize);
        return copySize;
   }
};

// offset and data of a single shader wave register
struct WaveRegisterInfo
{
    uint32_t offset;
    uint32_t data;
};

// Note: Must exactly match KmdWaveRegistersEventData in KmdEventDefs.h
struct WaveRegistersData
{
    uint32_t version;

    uint32_t shaderId;

    // number of WaveRegisterInfo structures which follow
    uint32_t numRegisters;

    // array of WaveRegisterInfo
    // actual array length is `numRegisters`
    WaveRegisterInfo registerInfos[1];

    static size_t CalculateStructureSize(uint32_t numRegisterInfoForCalculation)
    {
        numRegisterInfoForCalculation = Platform::Max(1U, numRegisterInfoForCalculation);
        return sizeof(WaveRegistersData) +
               sizeof(WaveRegisterInfo) * (numRegisterInfoForCalculation - 1);
    }

    static size_t CalculateBufferSize(uint32_t numRegisterInfoForCalculation)
    {
        return sizeof(WaveRegistersData) +
               sizeof(WaveRegisterInfo) * (numRegisterInfoForCalculation - 1);
    }

    static uint32_t GetNumWaveRegistersFromBuffer(const uint8_t *pBuffer)
    {
        pBuffer += offsetof(WaveRegistersData, numRegisters);
        return *reinterpret_cast<const uint32_t*>(pBuffer);
    }

    size_t FromBuffer(const uint8_t* pBuffer)
    {
        uint32_t numRegistersInBuffer = GetNumWaveRegistersFromBuffer(pBuffer);
        size_t   copySize             = CalculateBufferSize(numRegistersInBuffer);
        Platform::Memcpy_s(this, CalculateStructureSize(numRegistersInBuffer), pBuffer, copySize);
        return copySize;
    }

    size_t ToBuffer(uint8_t* pBuffer)
    {
        size_t copySize = CalculateBufferSize(numRegisters);
        Platform::Memcpy_s(pBuffer, copySize, this, copySize);
        return copySize;
    }
};

constexpr uint32_t kMaxGPRRegs = 256;  // Maximum VGPR currently is 256

// This struct is a counterpart to nc_radeon_gpu_detective_bgd_gpr_data in nutcracker_radeon_gpu_detective_bgd_def.h
struct GprRegistersData
{
    uint32_t    version;
    uint32_t    shaderId;
    uint32_t    workItem; // Also known as thread id, only applicable for VGPR
    bool        isVgpr;

    uint32_t    regToRead; // This defines up to what value can be valid data (VGPR 256, SGPR 128)
    uint32_t    reg[kMaxGPRRegs];

    void FromBuffer(const uint8_t* buffer)
    {
        Platform::Memcpy_s(&version, sizeof(version), buffer, sizeof(version));
        buffer += sizeof(version);

        Platform::Memcpy_s(&shaderId, sizeof(shaderId), buffer, sizeof(shaderId));
        buffer += sizeof(shaderId);

        Platform::Memcpy_s(&workItem, sizeof(workItem), buffer, sizeof(workItem));
        buffer += sizeof(workItem);

        Platform::Memcpy_s(&isVgpr, sizeof(isVgpr), buffer, sizeof(isVgpr));
        buffer += sizeof(isVgpr);

        Platform::Memcpy_s(&regToRead, sizeof(regToRead), buffer, sizeof(regToRead));
        buffer += sizeof(regToRead);

        Platform::Memcpy_s(&reg, sizeof(reg), buffer, sizeof(reg));
    }

    /// Fill the pre-allocated `buffer` with the data of this struct. The size of
    /// the buffer has to be at least `sizeof(GprRegistersData)` big.
    ///
    /// Return the actual amount of bytes copied into `buffer`.
    uint32_t ToBuffer(uint8_t* buffer) const
    {
        uint32_t copySize = 0;

        Platform::Memcpy_s(buffer + copySize, sizeof(version), &version, sizeof(version));
        copySize += sizeof(version);

        Platform::Memcpy_s(buffer + copySize, sizeof(shaderId), &shaderId, sizeof(shaderId));
        copySize += sizeof(shaderId);

        Platform::Memcpy_s(buffer + copySize, sizeof(workItem), &workItem, sizeof(workItem));
        copySize += sizeof(workItem);

        Platform::Memcpy_s(buffer + copySize, sizeof(isVgpr), &isVgpr, sizeof(isVgpr));
        copySize += sizeof(isVgpr);

        Platform::Memcpy_s(buffer + copySize, sizeof(regToRead), &regToRead, sizeof(regToRead));
        copySize += sizeof(regToRead);

        Platform::Memcpy_s(buffer + copySize, sizeof(reg), &reg, sizeof(reg));
        copySize += sizeof(reg);

        return copySize;
    }
};

#pragma pack(pop)
} // namespace KernelCrashAnalysisEvents
