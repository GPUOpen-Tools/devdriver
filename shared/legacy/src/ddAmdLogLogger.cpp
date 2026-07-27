/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "ddAmdLogLogger.h"
#include <util/vector.h>

#if defined(DD_PLATFORM_WINDOWS_UM)
    #include <win/ddWinKmIoCtlDevice.h>
    #include "appmodel.h"
#endif

namespace DevDriver
{

AmdLogLogger::AmdLogLogger(const AllocCb& allocCb, const AmdLogEscapeCb& escapeCb)
    : m_pIoCtlDevice(nullptr)
    , m_allocCb(allocCb)
    , m_escapeCb(escapeCb)
    , m_isUWPApp(false)
    , m_isInit(false)
{
}

#if defined(DD_PLATFORM_WINDOWS_UM)

bool IsUWPApp()
{
    UINT32 length = 0;
    LONG   rc     = GetPackageFamilyName(GetCurrentProcess(), &length, NULL);
    return (rc != APPMODEL_ERROR_NO_PACKAGE);
}

AmdLogLogger::~AmdLogLogger()
{
    if (m_pIoCtlDevice)
    {
        m_pIoCtlDevice->Destroy();
        DD_DELETE(m_pIoCtlDevice, m_allocCb);
        m_pIoCtlDevice = nullptr;
    }
}

Result AmdLogLogger::Init()
{
    Result result = Result::Success;
    m_isUWPApp    = IsUWPApp();

    if (m_isUWPApp == false)
    {
        m_pIoCtlDevice = DD_NEW(WinKmIoCtlDevice, m_allocCb)();

        if (m_pIoCtlDevice != nullptr)
        {
            result = m_pIoCtlDevice->Initialize();
            if (result != Result::Success)
            {
                DD_DELETE(m_pIoCtlDevice, m_allocCb);
                m_pIoCtlDevice = nullptr;
            }
        }
        else
        {
            result = Result::InsufficientMemory;
        }
    }

    m_isInit = (result == Result::Success);

    return result;
}

Result AmdLogLogger::WriteAmdlogData(uint32_t      logFlags,
                                     uint32_t      sourceId,
                                     uint32_t      eventId,
                                     DDGpuId       gpuId,
                                     void*         pData,
                                     size_t        dataSize)
{
    Result          result    = Result::Success;
    AmdLogEventInfo eventInfo = {};
    eventInfo.flags           = logFlags;
    eventInfo.sourceId        = sourceId;
    eventInfo.eventId         = eventId;
    eventInfo.gpuId           = gpuId;
    eventInfo.pData           = pData;
    eventInfo.dataSize        = dataSize;

    if (m_isInit == false)
    {
        result = Init();
    }

    if (result == Result::Success)
    {
        result = WriteDataInternal(&eventInfo);
    }

    return result;
}

Result AmdLogLogger::WriteDataInternal(AmdLogEventInfo* pEventInfo)
{
    Result result = Result::Error;

    DD_ASSERT(m_isInit == true);

    if (pEventInfo != nullptr)
    {
        if ((m_isUWPApp == false) && (m_pIoCtlDevice != nullptr))
        {
            result = m_pIoCtlDevice->InDirectIoCtl(pEventInfo->dataSize,
                                                   pEventInfo->pData,
                                                   sizeof(AmdLogEventInfo),
                                                   pEventInfo);
        }
        else if (m_escapeCb.pfnCallback)
        {
            result = m_escapeCb.pfnCallback(0, m_escapeCb.pUserData, pEventInfo, sizeof(AmdLogEventInfo));
        }
    }

    return result;
}

#else

AmdLogLogger::~AmdLogLogger()
{

}

Result AmdLogLogger::Init()
{
    return Result::Success;
}

Result AmdLogLogger::WriteAmdlogData(uint32_t      logFlags,
                                     uint32_t      sourceId,
                                     uint32_t      eventId,
                                     DDGpuId       gpuId,
                                     void*         pData,
                                     size_t        dataSize)
{
    DD_UNUSED(logFlags);
    DD_UNUSED(sourceId);
    DD_UNUSED(eventId);
    DD_UNUSED(gpuId);
    DD_UNUSED(dataSize);
    DD_UNUSED(pData);
    return Result::Unavailable;
}

Result AmdLogLogger::WriteDataInternal(AmdLogEventInfo* pEventInfo)
{
    DD_UNUSED(pEventInfo);
    return Result::Unavailable;
}

#endif
}
