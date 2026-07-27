/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddIoCtlDevice.h>
#include <ddPlatform.h>

namespace DevDriver
{

class WinUmIoCtlDevice final : public IIoCtlDevice
{
public:
    WinUmIoCtlDevice() {}
    ~WinUmIoCtlDevice()
    {
        Destroy();
    }

    DD_NODISCARD
    Result Initialize() override;

    void Destroy() override;

    /// IIoCtlDevice Interface
    DD_NODISCARD
    virtual Result IoCtl(
        uint32 ioCtlCode,
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

private:
    // No members
};
} // DevDriver
