/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "ddDriverIoctlDevice.h"
#include <util/vector.h>

#if defined(DD_PLATFORM_WINDOWS_UM)
    #include <win/ddWinKmIoCtlDevice.h>
    #include "appmodel.h"
#endif

namespace DevDriver
{

DriverIoctlDevice::DriverIoctlDevice(const AllocCb& allocCb)
    : m_pIoCtlDevice(nullptr)
    , m_allocCb(allocCb)
    , m_isInit(false)
{
}

#if defined(DD_PLATFORM_WINDOWS_UM)

DriverIoctlDevice::~DriverIoctlDevice()
{
    if (m_pIoCtlDevice != nullptr)
    {
        m_pIoCtlDevice->Destroy();
        DD_DELETE(m_pIoCtlDevice, m_allocCb);
        m_pIoCtlDevice = nullptr;
    }
}

Result DriverIoctlDevice::Init()
{
    Result result = Result::Success;

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

    m_isInit = (result == Result::Success);
    return result;
}

Result DriverIoctlDevice::SendRgdMonitoringRequest(
    const char* pProcessName,
    uint32_t    options)
{
    Result result = Result::Success;

    if (m_isInit == false)
    {
        result = Init();
    }

    if (result == Result::Success)
    {
        RgdMonitoringRequest request = {};
        request.version.major = RgdMonitoringVersionMajor;
        request.version.minor = RgdMonitoringVersionMinor;
        request.version.patch = RgdMonitoringVersionPatch;
        Platform::Strncpy(request.processName, pProcessName, MAX_PROCESS_NAME_SIZE);
        request.options = options;

        result = SendMonitoringRequestInternal(&request);
    }

    return result;
}

Result DriverIoctlDevice::SendMonitoringRequestInternal(
    RgdMonitoringRequest* pRequest)
{
    Result result = Result::Error;

    DD_ASSERT(m_isInit == true);

    if (pRequest != nullptr)
    {
        if (m_pIoCtlDevice != nullptr)
        {
            result = m_pIoCtlDevice->IoCtl(
                DevDriver::DevDriverRgdMonitoringRequest,
                sizeof(RgdMonitoringRequest),
                pRequest,
                sizeof(RgdMonitoringRequest),
                pRequest);
        }
    }

    return result;
}

#else  // Non-Windows platforms

DriverIoctlDevice::~DriverIoctlDevice()
{
}

Result DriverIoctlDevice::Init()
{
    return Result::Unavailable;
}

Result DriverIoctlDevice::SendRgdMonitoringRequest(
    const char* processName,
    uint32_t options)
{
    DD_UNUSED(processName);
    DD_UNUSED(options);
    return Result::Unavailable;
}

Result DriverIoctlDevice::SendMonitoringRequestInternal(
    RgdMonitoringRequest* pRequest)
{
    DD_UNUSED(pRequest);
    return Result::Unavailable;
}

#endif

} // DevDriver
