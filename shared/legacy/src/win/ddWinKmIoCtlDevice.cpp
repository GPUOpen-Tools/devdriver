/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <win/ddWinKmIoCtlDevice.h>
#include <winKernel/ddIoCtlDefines.h>
#include <winKernel/ddWinKmIoCtlDefines.h>

namespace DevDriver
{

Result WinKmIoCtlDevice::Initialize()
{
    Result result = Result::InvalidParameter;

    if (m_hDevice == INVALID_HANDLE_VALUE)
    {
        DD_PRINT(LogLevel::Debug,
                 "Initializing WinKmIoCtlDevice with driver file \"%s\"",
                 kDriverFileName);
        m_hDevice = CreateFileA(
            kDriverFileName,              // lpFileName - i.e. our driver file.
            GENERIC_READ | GENERIC_WRITE, // dwDesiredAccess
            0,                            // dwShareMode
            NULL,                         // lpSecurityAttributes
            OPEN_EXISTING,                // dwCreationDisposition
            FILE_ATTRIBUTE_NORMAL,        // dwFlagsAndAttributes
            NULL                          // hTemplateFile
        );

        if (m_hDevice != INVALID_HANDLE_VALUE)
        {
            result = Result::Success;
        }
        else
        {
            // CreateFile failed
            const DWORD error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND)
            {
                result = Result::Unavailable;
                // This is expected to happen whenever the km utility driver is not installed.
                // This may also happen if the device isn't installed properly (e.g. yellow triangle).
                // TODO: re-enable this when the km utility driver ships.
                DD_PRINT(LogLevel::Never,
                    "Initializing WinKmIoCtlDevice with driver file \"%s\" FAILED: ERROR_FILE_NOT_FOUND",
                    kDriverFileName);
            }
            else if (error == ERROR_ACCESS_DENIED)
            {
                result = Result::Rejected;
                DD_PRINT(LogLevel::Error,
                    "Initializing WinKmIoCtlDevice with driver file \"%s\" FAILED: ERROR_ACCESS_DENIED",
                    kDriverFileName);
            }
            else
            {
                result = Result::Error;
                DD_PRINT(LogLevel::Error,
                         "Initializing WinKmIoCtlDevice with driver file \"%s\" FAILED: GetLastError() = 0x%x",
                         kDriverFileName,
                         error);
            }
        }
    }
    else
    {
        // Initialize was likely called without first calling Destroy.
        // This is a programming error
        result = Result::Error;
        DD_ASSERT_REASON("WinKmIoCtlDevice::Initialize() was called on an initialized device");
    }

    return result;
}

void WinKmIoCtlDevice::Destroy()
{
    if (m_hDevice != INVALID_HANDLE_VALUE)
    {
        const BOOL result = CloseHandle(m_hDevice);

        if (result)
        {
            m_hDevice = INVALID_HANDLE_VALUE;
        }
        else
        {
            DD_WARN_REASON("Failed to close kernel bus device handle");
        }
    }
}

Result WinKmIoCtlDevice::IoCtl(
    uint32 ddCommandCode,
    size_t bufferSize,
    void*  pBuffer)
{
    Result result = Result::InvalidParameter;

    // Validate the input parameters
    if ((bufferSize <= UINT32_MAX) &&
        (pBuffer    != nullptr))
    {
        nc_amdlog_devdriver_input *pRequest;
        DWORD requestSize;

        // the command-specific data is located after the nc_amdlog_devdriver_input structure
        requestSize = static_cast<DWORD>(sizeof(nc_amdlog_devdriver_input) + bufferSize);

        pRequest = reinterpret_cast<nc_amdlog_devdriver_input *>(malloc(requestSize));

        if (pRequest)
        {
            pRequest->process_id    = GetCurrentProcessId();
            pRequest->dev_mode_cmd  = ddCommandCode;
            pRequest->cmd_data_size = static_cast<uint32_t>(bufferSize);

            // copy the caller's command data into the ioctl packet
            Platform::Memcpy_s(pRequest->cmd_data, requestSize - sizeof(nc_amdlog_devdriver_input) + 1, pBuffer, bufferSize);

            DWORD winIoCtlCode = GetIoCtlCode(IoCtlType::DevDriverDevModeCmdBuffered);

            if (DeviceIoControl(m_hDevice,
                                winIoCtlCode,
                                pRequest,
                                requestSize,
                                pRequest,
                                requestSize,
                                nullptr,
                                nullptr))
            {
                result = Result::Success;
            }
            else
            {
                result = Result::Error;
            }

            // copy the command-specific ioctl response data to the caller
            Platform::Memcpy_s(pBuffer, bufferSize, pRequest->cmd_data, bufferSize);

            free(pRequest);
        }
        else
        {
            result = Result::Error;
        }
    }

    return result;
}

// This routine is used to pass a pointer from user space to the kernel safely.
//      pInBuffer should contain the pointer from user space
//      pOutBuffer can be an additional input struct
Result WinKmIoCtlDevice::InDirectIoCtl(
    size_t inSize,
    void*  pInBuffer,
    size_t outSize,
    void*  pOutBuffer)
{
    Result result = Result::InvalidParameter;

    // Validate the input parameters
    if ((inSize <= UINT32_MAX)   &&
        (pInBuffer != nullptr)   &&
        (pOutBuffer != nullptr)  &&
        (outSize <= UINT32_MAX))
    {
        DWORD winIoCtlCode = GetIoCtlCode(IoCtlType::DevDriverInDirect);

        if (DeviceIoControl(m_hDevice,
                            winIoCtlCode,
                            pInBuffer,
                            (DWORD)inSize,
                            pOutBuffer,
                            (DWORD)outSize,
                            nullptr,
                            nullptr))
        {
            result = Result::Success;
        }
        else
        {
            result = Result::Error;
        }
    }

    return result;
}

Result WinKmIoCtlDevice::IoCtl(
    IoCtlType type,
    size_t    inSize,
    void*     pInBuffer,
    size_t    outSize,
    void*     pOutBuffer)
{
    Result result = Result::InvalidParameter;

    // Validate the input parameters
    if ((inSize <= UINT32_MAX)   &&
        (pInBuffer != nullptr)   &&
        (pOutBuffer != nullptr)  &&
        (outSize <= UINT32_MAX))
    {
        DWORD winIoCtlCode = GetIoCtlCode(type);

        if (DeviceIoControl(m_hDevice,
                            winIoCtlCode,
                            pInBuffer,
                            (DWORD)inSize,
                            pOutBuffer,
                            (DWORD)outSize,
                            nullptr,
                            nullptr))
        {
            result = Result::Success;
        }
        else
        {
            result = Result::Error;
        }
    }

    return result;
}
} // DevDriver
