/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once
#include "ddPlatform.h"
#include "ddAmdLogInterface.h"

namespace DevDriver
{
class IIoCtlDevice;

// This callback allows the Logger to use the UMDs escape code paths rather than implement it
// directly in DevDriver.
typedef Result (*pfnAmdlogEscapeCb)(uint32_t gpuIdx,    // [in] GPU Index
                                    void*    pUserdata, // [in] Userdata pointer
                                    void*    pData,     // [in] Pointer to the log info
                                    size_t   dataSize); // [in] Size of the data

// Helper structure for pfnAmdlogEscapeCb
struct AmdLogEscapeCb
{
    void*             pUserData;   // [in] Userdata pointer
    pfnAmdlogEscapeCb pfnCallback; // [in] Pointer to a data callback function
};

class AmdLogLogger
{
public:
    AmdLogLogger(const AllocCb& allocCb, const AmdLogEscapeCb& escapeCb);
    ~AmdLogLogger();

    Result WriteAmdlogData(uint32_t logFlags,
                           uint32_t sourceId,
                           uint32_t eventId,
                           DDGpuId  gpuId,
                           void*    pData,
                           size_t   dataSize);

    /// @deprecated Will be removed following PAL promotion
    Result WriteAmdlogData(uint32_t logFlags, uint32_t eventId, DDGpuId gpuId, void* pData, size_t dataSize)
    {
        DD_UNUSED(logFlags);
        DD_UNUSED(eventId);
        DD_UNUSED(gpuId);
        DD_UNUSED(pData);
        DD_UNUSED(dataSize);
        return Result::Unavailable;
    }

    /// @deprecated Will be removed following PAL promotion
    Result WriteAmdlogString(uint32_t logFlags, DDGpuId gpuId, const char* pFormat, ...)
    {
        DD_UNUSED(logFlags);
        DD_UNUSED(gpuId);
        DD_UNUSED(pFormat);
        return Result::Unavailable;
    }

protected:

    Result Init();
    Result WriteDataInternal(AmdLogEventInfo* pEventInfo);

    IIoCtlDevice*          m_pIoCtlDevice;
    AllocCb                m_allocCb;
    AmdLogEscapeCb         m_escapeCb;
    bool                   m_isUWPApp;
    bool                   m_isInit;
};

} // DevDriver
