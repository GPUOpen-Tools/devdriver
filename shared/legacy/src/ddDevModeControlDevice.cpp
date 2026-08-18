/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <ddDevModeControlDevice.h>
#include <ddDevModeControlCmds.h>

#if defined(DD_PLATFORM_WINDOWS_UM)
    #include <win/ddWinUmIoCtlDevice.h>
    #include <win/ddWinKmIoCtlDevice.h>
#endif

#include <ddPlatform.h>

namespace DevDriver
{

Result DevModeControlDevice::Initialize(DevModeBusType busType)
{
    Result result = Result::Error;

    // If the user asked for an "auto" bus type, each platform picks its own
    // default and then follows the standard logic below (in the switch).
    if (busType == DevModeBusType::Auto)
    {
#if defined(DD_PLATFORM_WINDOWS_UM)
        // Windows UM defaults to kernel mode because it should always be there.
        busType = DevModeBusType::KernelMode;
#elif defined(DD_PLATFORM_WINDOWS_KM)
        // This is not allowed and should not happen.
#error "DevModeControlDevice should not be built for kernel mode"
#else
        // Posix platforms default to a user mode bus
        busType = DevModeBusType::UserMode;
#endif
    }

    // Prevent double initialization
    if (m_pIoCtlDevice == nullptr)
    {
        IIoCtlDevice* pIoCtlDevice = nullptr;

        switch (busType)
        {
            case DevModeBusType::UserMode:
            {
#if defined(DD_PLATFORM_WINDOWS_UM)
                pIoCtlDevice = DD_NEW(WinUmIoCtlDevice, m_allocCb)();
                if (pIoCtlDevice != nullptr)
                {
                    result = Result::Success;
                }
                else
                {
                    result = Result::InsufficientMemory;
                }
#else
                result = Result::Unavailable;
#endif
                break;
            }

            case DevModeBusType::KernelMode:
            {
#if defined(DD_PLATFORM_WINDOWS_UM)
                pIoCtlDevice = DD_NEW(WinKmIoCtlDevice, m_allocCb)();
                if (pIoCtlDevice != nullptr)
                {
                    result = Result::Success;
                }
                else
                {
                    result = Result::InsufficientMemory;
                }
#else
                result = Result::Unavailable;
#endif
                break;
            }

            default:
            {
                DD_ASSERT_ALWAYS();
                break;
            }
        }

        if (result == Result::Success)
        {
            result = pIoCtlDevice->Initialize();
            if (result != Result::Success)
            {
                DD_DELETE(pIoCtlDevice, m_allocCb);
            }
        }

        if (result == Result::Success)
        {
            m_pIoCtlDevice    = pIoCtlDevice;
            m_ioCtlDeviceType = busType;
        }
    }

    return result;
}

void DevModeControlDevice::Destroy()
{
    if (m_pIoCtlDevice != nullptr)
    {
        m_pIoCtlDevice->Destroy();
        DD_DELETE(m_pIoCtlDevice, m_allocCb);
        m_pIoCtlDevice = nullptr;
    }
}

// On Um bus types, we may need to do additional work to map the shared buffers for certain DevMode commands.
// This helper method maps a single buffer between UM clients.
// On failure, pQueue is not modified.
static Result MapSharedBufferUmToUm(QueueInfo* pQueue)
{
    Result result = Result::InvalidParameter;

    if ((pQueue != nullptr) && (pQueue->sharedBuffer.hSharedBufferView != NULL))
    {
        result = Result::Success;
    }

    if (result == Result::Success)
    {
        // @TODO: For some reason, the existing kernel implementation returns the process local shared buffer
        //        handle in the hSharedBufferView field instead of the hSharedBufferObject field. This should be
        //        cleaned up in the future but it's being left as-is for now to avoid regressions.
        const Handle hSharedQueueBuffer = pQueue->sharedBuffer.hSharedBufferView;
        const Handle hSharedQueueView   = Platform::Windows::MapSystemBufferView(hSharedQueueBuffer, pQueue->bufferSize);
        if (hSharedQueueView != NULL)
        {
            // Save the shared queue buffer handle in here so it can be closed after we unmap the buffer in the event
            // of a partial initialization failure.
            pQueue->sharedBuffer.hSharedBufferObject = hSharedQueueBuffer;
            pQueue->sharedBuffer.hSharedBufferView   = hSharedQueueView;
            result = Result::Success;
        }
        else
        {
            DD_PRINT(
                LogLevel::Error, "Failed to map queue for shared buffer communication. GLE = %d",
                GetLastError());
            result = Result::Error;
        }
    }

    return result;
}

// On Um bus types, we may need to do additional work to map the shared buffers for certain DevModeCmds.
// This helper method tears down that work in the event of partial failure.
static Result UnmapSharedBufferUmToUm(QueueInfo* pQueue)
{
    Result result = Result::InvalidParameter;

    if (pQueue != nullptr)
    {
        // We pass an invalid handle as the buffer object here since that parameter isn't relevant for usermode.
        Platform::Windows::UnmapBufferView(kInvalidHandle,
                                           pQueue->sharedBuffer.hSharedBufferView);

        // Close the shared buffer object
        Platform::Windows::CloseSharedBuffer(pQueue->sharedBuffer.hSharedBufferObject);

        pQueue->sharedBuffer.hSharedBufferObject = kInvalidHandle;
        pQueue->sharedBuffer.hSharedBufferView   = kInvalidHandle;

        result = Result::Success;
    }

    return result;
}

Result DevModeControlDevice::HandlePostIoCtlWork(DevModeCmd cmd, void* pBuffer)
{
    Result result = Result::Success;

    if (m_ioCtlDeviceType == DevModeBusType::UserMode)
    {
        if (pBuffer != nullptr)
        {
            switch (cmd)
            {
                // Since this is Um to Um communication, we need to map both the send and receive queue into our address space.
            case DevModeCmd::RegisterClient:
            {
                auto* pRequest = reinterpret_cast<RegisterClientRequest*>(pBuffer);
                result = MapSharedBufferUmToUm(&pRequest->output.sendQueue);
                if (result == Result::Success)
                {
                    MapSharedBufferUmToUm(&pRequest->output.receiveQueue);
                }
                else
                {
                    // We were only able to initialize one of the two queues - unmap the old one and reset it.
                    DD_UNHANDLED_RESULT(UnmapSharedBufferUmToUm(&pRequest->output.receiveQueue));
                }
                break;
            }

            // Similar to above
            case DevModeCmd::RegisterRouter:
            {
                auto* pRequest = reinterpret_cast<RegisterRouterRequest*>(pBuffer);
                result = MapSharedBufferUmToUm(&pRequest->output.sendQueue);
                if (result == Result::Success)
                {
                    MapSharedBufferUmToUm(&pRequest->output.receiveQueue);
                }
                else
                {
                    DD_UNHANDLED_RESULT(UnmapSharedBufferUmToUm(&pRequest->output.receiveQueue));
                }
                break;
            }

            default:
                // Other commands have no post-work to do.
                break;
            }
        }
        else
        {
            DD_ASSERT_ALWAYS();
            result = Result::InvalidParameter;
        }
    }
    else
    {
        // There's nothing to do, so nothing can fail.
    }

    return result;
}

Result DevModeControlDevice::MakeDevModeRequestRaw(
    DevModeCmd cmd,
    size_t     bufferSize,
    void*      pBuffer)
{
    // This function should never be called when we have a null IoCtl device pointer.
    // This method is private, so this is a programmer error that can fail hard.
    DD_ASSERT(m_pIoCtlDevice != nullptr);

    Result result = m_pIoCtlDevice->IoCtl(static_cast<uint32>(cmd), bufferSize, pBuffer);
    if (result == Result::Success)
    {
        result = HandlePostIoCtlWork(cmd, pBuffer);
    }
    return result;
}

}
