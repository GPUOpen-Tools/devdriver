/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include "ddPlatform.h"
#include "ddRgdMonitoringTypes.h"

namespace DevDriver
{

class IIoCtlDevice;

/// IOCTL device for RGD monitoring communication
/// Follows the pattern established by DevModeControlDevice
class DriverIoctlDevice
{
public:
    DriverIoctlDevice(const AllocCb& allocCb);
    ~DriverIoctlDevice();

    /// Send RGD monitoring request to kernel driver
    /// @param processName  Executable name (not full path)
    /// @param options      RgdGpuDumpOptions flags (can be OR'd)
    /// @return             Success if IOCTL sent successfully, error code otherwise
    Result SendRgdMonitoringRequest(
        const char* processName,
        uint32_t    options);

protected:
    Result Init();
    Result SendMonitoringRequestInternal(RgdMonitoringRequest* pRequest);

    IIoCtlDevice*  m_pIoCtlDevice;   // Platform-specific IOCTL device
    AllocCb        m_allocCb;        // Allocator callbacks
    bool           m_isInit;         // Initialization state flag
};

} // DevDriver
