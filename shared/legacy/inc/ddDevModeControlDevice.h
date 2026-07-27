/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
#pragma once

#include <gpuopen.h>
#include <ddPlatform.h>
#include <ddDevModeControl.h>

namespace DevDriver
{

class IIoCtlDevice;

/// Provides a control interface for configuring the developer mode bus.
class DevModeControlDevice
{
public:
    DevModeControlDevice(const AllocCb& allocCb)
        : m_allocCb(allocCb)
    {
    }

    ~DevModeControlDevice()
    {
    }

    // Lifetime management functions
    DD_NODISCARD
    Result Initialize(DevModeBusType busType);

    void Destroy();

    /// Platform-agnostic call into the devmode device.
    ///
    /// This is a convience overload to prevent errors with DevModeCmds.
    /// Prefer this to the non-templated overload.
    template <typename Request>
    DD_NODISCARD
    Result MakeDevModeRequest(
        Request* pInOutBuffer
    )
    {
        Result result = Result::InvalidParameter;

        // Buffer must be valid and contain a DevModeResponseHeader.
        if (IsInitialized() && (pInOutBuffer != nullptr))
        {
            // Make sure the header is the right offset. This is a compile-time check.
            static_assert(offsetof(Request, header) == 0, "Expected header field at offset 0");

            // Make sure the header is the right type. This is a compile-time check.
            const DevModeResponseHeader& header = pInOutBuffer->header;
            DD_UNUSED(header);

            result = MakeDevModeRequestRaw(
                Request::kCmd,
                sizeof(*pInOutBuffer),
                pInOutBuffer
            );
        }

        return result;
    }

private:
    /// Platform-agnostic call into the devmode device.
    ///
    /// Prefer calling the typed-wrapper instead of this whenever possible.
    DD_NODISCARD
    Result MakeDevModeRequestRaw(
        DevModeCmd cmd,
        size_t     bufferSize,
        void*      pBuffer
    );

    DD_NODISCARD
    Result HandlePostIoCtlWork(DevModeCmd cmd, void* pBuffer);

    bool IsInitialized() const
    {
        // This should not happen. Auto is resolved to UserMode or KernelMode at init time.
        DD_ASSERT(m_ioCtlDeviceType != DevModeBusType::Auto);
        return ((m_pIoCtlDevice != nullptr) && (m_ioCtlDeviceType != DevModeBusType::Unknown));
    }

    // Allocation callbacks
    AllocCb        m_allocCb;

    // Device to issue ioctl commands through. May be user-mode or kernel mode depending on the platform.
    IIoCtlDevice*  m_pIoCtlDevice = nullptr;

    // Type of IoCtlDevice stored in m_pIoCtlDevice
    // This may be Unknown, UserMode, or KernelMode, but will never be Auto. Unknown represents and uninitialized object.
    DevModeBusType m_ioCtlDeviceType = DevModeBusType::Unknown;
};

}
