/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddDefs.h>
#include <dd_clocks_api.h>
#include <dd_enhanced_crash_info_api.h>

////////////////////////////////////////////////////////////////////////////////
// Design and Compatibility Guidelines
//
// In order to maintain tool compatibility, please prefer creating
// new events and structures rather than modifying existing event structures.
// If an existing structure must be modified then its version number must
// be incremented.
////////////////////////////////////////////////////////////////////////////////

namespace DevDriver
{

// Enumeration of KMD event IDs
enum class KmdEventType : uint32
{
    UnknownNutcrackerEvent = 0,
    ProcessCreate          = 1,
    ProcessDestroy         = 2,
    PageTableUpdate        = 3,
    RmtToken               = 4,
    PageFault              = 5,
    Tdr                    = 6,
    AmdLogEvent            = 7,
    ShaderWaves            = 8,
    SeInfo                 = 9,
    MmrRegisters           = 10,
    WaveRegisters          = 11,
    VgpSgpRegisters        = 12,
    Count                  = 13,
};

// Event data structure, which is an opaque container for event specific data
struct KmdEventData
{
    KmdEventType eventType; // Event type
    size_t       dataSize;  // Size, in bytes, of the data pointed to by pData
    const void*  pData;     // Pointer to event-specific data, see the structures below for details.

    // Nutcracker-specific fields.  These fields are assumed to use the nutcracker value/defintion
    uint32 sourceArea; // Source area as defined by nutcracker
    uint32 eventId;    // Bitpacked event ID that includes class, sub-class and event ID as defined by nutcracker.
};

// Event-specific data for the KmdEventType::ProcessCreate & KmdEventType::ProcessDestroy events
struct KmdProcessEventData
{
    uint32 processId;
};

// Structure describing a page table entry (PTE) destination address range
struct KmdPteRange
{
    uint32 numOfPages;
    uint64 destinationAddress;
    bool   isLocalAllocation;
};

static const uint32 MaxPteRanges = 16;

// Event-specific data for the KmdEventType::PageTableUpdate event
struct KmdPageTableUpdateEventData
{
    uint32 processId;
    uint8  adapterIndex;
    uint64 sourceVirtualAddress;
    uint8  pageSizeInKb;
    uint64 allocationSize;

    struct
    {
        uint8 isHbccData           : 1;   // Data is coming from HBCC callback
        uint8 isDiscard            : 1;
        uint8 isUpdate             : 1;
        uint8 isTransfer           : 1;
        uint8 reserved             : 4;
    } Flags;

    uint8       numPteRanges;
    KmdPteRange pteRanges[MaxPteRanges];
};

struct KmdPageFaultEventData
{
    static const uint32 kMaxPageFaultProcessNameLength = 64;

    uint32 vmId;
    uint32 processId;
    uint64 pageFaultAddress;
    uint32 processNameLength;
    uint8  processName[kMaxPageFaultProcessNameLength];
};

/// GPU ID Determined from (BusID << 16) | (DeviceID << 8) | FunctionID
typedef uint32_t DDGpuId;
typedef struct QueryClockMode
{
    DDGpuId              gpuId;
    DD_DEVICE_CLOCK_MODE mode;
    uint32_t             status;
} QueryClockMode;

typedef struct QueryClockFrequency
{
    DDClockModeInfo input;
    DDClockFreqs    output;
    uint32_t        status;
} QueryClockFrequency;

typedef struct SetClockModeInfo
{
    DDClockModeInfo input;
    uint32_t        status;
} SetClockModeInfo;

#pragma pack(push, 1)

// Shader Engine debug registers
struct KmdSeRegsInfo
{
    uint32_t  version;
    uint32    spiDebugBusy;
    uint32    sqDebugStsGlobal;
    uint32    sqDebugStsGlobal2;
};

// Graphics Register Bus Manager status registers
struct KmdGrbmStatusSeRegs
{
    uint32_t  version;
    uint32    grbmStatusSe0;
    uint32    grbmStatusSe1;
    uint32    grbmStatusSe2;
    uint32    grbmStatusSe3;
    // SE4 and SE5 are NV31 specific, 2x does not have this
    uint32    grbmStatusSe4;
    uint32    grbmStatusSe5;
};

// Shader Engine information
struct KmdSeInfoEventData
{
    uint32_t      version;
    DDGpuId       gpuId;
    // Number of Shader Engines described.
    // This is the length of the seRegsInfos array
    uint32_t      numSe;
    KmdSeRegsInfo seRegsInfos[1];
};

enum KmdHangType
{
    pageFault    = 0,
    nonPageFault = 1,
    Unknown      = 2,
};

struct KmdWaveRegister
{
    uint32_t offset;
    uint32_t data;
};

struct KmdWaveRegistersEventData
{
    // Version field for compatibility check within the driver
    uint32_t version;

    // Specify which shader wave this sequence of registers belongs to.
    // This corresponds to the `shaderId` in one of the `WaveInfo` events.
    uint32_t shaderId;

    // The number of registers in this event
    uint32_t numRegisters;

    // Dynamically sized array of Wave registers
    KmdWaveRegister registerInfos[1];
};

// Information about a specific wave
struct KmdWaveInfo
{
    uint32_t  version;

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

// Event Data containing the hung waves and active waves
// as well as the Se status registers
struct KmdShaderWavesEventData
{
    uint32_t                version;
    DDGpuId                 gpuId;
    KmdHangType             typeOfHang;
    KmdGrbmStatusSeRegs     grbmStatusSeRegs;
    uint32_t                numberOfHungWaves;
    uint32_t                numberOfActiveWaves;
    // Array of hung waves followed by active waves
    // KmdWaveInfo * [numberOfHungWaves]
    // KmdWaveInfo * [numberOfActiveWaves]
    KmdWaveInfo             waveInfos[1];
};

// Information about a specific memory mapped register
struct KmdMmrRegisterInfo
{
    uint32_t offset;
    uint32_t data;
};

// Event containing the current value of a number of memory mapped registers
struct KmdMmrRegistersEventData
{
    uint32_t           version;
    DDGpuId            gpuId;
    // Number of KmdMmrRegisterInfo in the registerInfos array
    uint32_t           numRegisters;
    KmdMmrRegisterInfo registerInfos[1];
};

struct KmdSettingsBlobQuery
{
    uint32_t version;
    DDGpuId  gpuId;
    uint64_t dataSize;
    uint8_t* pData;
    bool     success;
    uint8_t  reserved[7];
};

struct KmdSettingsValuesQuery
{
    uint32_t version;
    DDGpuId  gpuId;
    uint64_t dataSize;
    uint8_t* pData;
    uint8_t  reserved[8];
};

#pragma pack(pop)

} // DevDriver
