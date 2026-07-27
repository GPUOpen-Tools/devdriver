/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
#pragma once

#include <ddPlatform.h>

namespace DevDriver
{

enum IoCtlType
{
    DevDriverDevModeCmdBuffered,
    DevDriverInDirect,
    DevDriverRgdOcaBuffered,
    DevDriverAdapterInfoBuffered,
    DevDriverGpuIdBuffered,
    DevDriverRgdMonitoringRequest,
};

/// Abstract interface to control the developer mode message bus
class IIoCtlDevice
{
public:
    virtual ~IIoCtlDevice() {}

    DD_NODISCARD
    virtual Result Initialize() = 0;

    virtual void Destroy() = 0;

    /// Executes an IoCtl command on the device
    DD_NODISCARD
    virtual Result IoCtl(
        uint32 ioCtlCode,
        size_t bufferSize,
        void*  pBuffer
    ) = 0;

    DD_NODISCARD
    virtual Result InDirectIoCtl(
        size_t inSize,
        void*  pInBuffer,
        size_t outSize,
        void*  pOutBuffer
    ) = 0;

    DD_NODISCARD
    virtual Result IoCtl(
        IoCtlType type,
        size_t    inSize,
        void*     pInBuffer,
        size_t    outSize,
        void*     pOutBuffer)
        {
            DD_UNUSED(type);
            DD_UNUSED(inSize);
            DD_UNUSED(pInBuffer);
            DD_UNUSED(outSize);
            DD_UNUSED(pOutBuffer);
            return Result::Unavailable;
        };

protected:
    IIoCtlDevice() {}
};

}
