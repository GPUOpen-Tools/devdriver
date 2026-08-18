/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddIoCtlDevice.h>
#include <winKernel/ddIoCtlDefines.h>
#include <ddPlatform.h>

namespace DevDriver
{

class WinKmIoCtlDevice final : public IIoCtlDevice
{
public:
    WinKmIoCtlDevice() {}
    ~WinKmIoCtlDevice()
    {
        Destroy();
    }

    DD_NODISCARD
    Result Initialize() override;

    void Destroy() override;

    // ioctl interface to devdriver (via amdlog)
    // ddCommandCode: DevDriver command number (see DevModeCmd type)
    // bufferSize   : Size of command-specific buffer to provide and receive command data
    // pBuffer      : Pointer to command-specific buffer to provide and receive command data
    DD_NODISCARD
    virtual Result IoCtl(
        uint32 ddCommandCode,
        size_t bufferSize,
        void*  pBuffer
    ) override;

    DD_NODISCARD
    virtual Result InDirectIoCtl(
        size_t inSize,
        void*  pInBuffer,
        size_t outSize,
        void*  pOutBuffer
    ) override;

    DD_NODISCARD
    Result IoCtl(
        IoCtlType type,
        size_t    inSize,
        void*     pInBuffer,
        size_t    outSize,
        void*     pOutBuffer
    ) override;

private:
    HANDLE m_hDevice = INVALID_HANDLE_VALUE;
};
} // DevDriver
