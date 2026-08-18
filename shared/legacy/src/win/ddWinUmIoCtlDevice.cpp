/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

// This header defines the GUID, but we need to give it a translation unit to live in.
#define INITGUID

#include <gpuopen.h>
#include <ddDevModeControl.h>
#include <win/ddWinUmIoCtlDevice.h>

namespace DevDriver
{
    // Every IoCtl request has at least this header structure.
    static constexpr size_t      kMinIoCtlMessageSize = sizeof(DevModeResponseHeader);

    Result WinUmIoCtlDevice::Initialize()
    {
        return Result::Success;
    }

    void WinUmIoCtlDevice::Destroy()
    {
        // Nothing to do
    }

    Result WinUmIoCtlDevice::IoCtl(
        uint32 ioCtlCode,
        size_t bufferSize,
        void*  pBuffer
    )
    {
        // The WinUm implementation doesn't use this when making requests to the utility driver.
        DD_UNUSED(ioCtlCode);

        Result result = Result::InvalidParameter;

        if ((bufferSize >= kMinIoCtlMessageSize) && (pBuffer != nullptr))
        {
            result = Result::Success;
        }
        else
        {
            DD_WARN(bufferSize >= kMinIoCtlMessageSize);
            DD_WARN(pBuffer != nullptr);
        }

        // Get a handle to the named pipe
        HANDLE hPipe = NULL;
        if (result == Result::Success)
        {
            // Loop until we get a valid pipe handle
            while (1)
            {
                hPipe = CreateFileA(
                    kWinIoCtlPipeName,            // lpFileName
                    GENERIC_READ | GENERIC_WRITE, // dwDesiredAccess
                    0,                            // dwShareMode           - This pipe cannot be opened again until we close the handle
                    nullptr,                      // lpSecurityAttributes  - default security attributs
                    OPEN_EXISTING,                // dwCreationDisposition - Open an already existing one. If one does not exist, it fails to open.
                    0,                            // dwFlagsAndAttributes  - default attributes
                    NULL);                        // hTemplateFile

                // If we have a valid pipe, we're done!
                if (hPipe != INVALID_HANDLE_VALUE)
                {
                    result = Result::Success;
                    break;
                }

                // The handle is *not* valid! Find out why.
                DWORD lastError = GetLastError();

                // We expect to find a busy pipe, since multiple processes might try to use this pipe at the same time.
                // Otherwise, it's an error.
                if (lastError != ERROR_PIPE_BUSY)
                {
                    const char* pLastErrorStr = nullptr;
                    switch (lastError)
                    {
                        case ERROR_FILE_NOT_FOUND: pLastErrorStr = "ERROR_FILE_NOT_FOUND"; break;
                        default:                   pLastErrorStr = nullptr;                break;
                    }

                    if (pLastErrorStr != nullptr)
                    {
                        DD_PRINT(LogLevel::Error, "[WinUmIoCtlDevice] Could not open pipe: %s", pLastErrorStr);
                    }
                    else
                    {
                        DD_PRINT(LogLevel::Error,
                                 "[WinUmIoCtlDevice] Could not open pipe. GetLastError() = %d (0x%x)",
                                 lastError,
                                 lastError);
                    }
                    result = Result::Error;

                    break;
                }
                // The pipe is busy, so sleep and try again.
                else if (!WaitNamedPipeA(kWinIoCtlPipeName, kLogicFailureTimeout))
                {
                    DD_PRINT(LogLevel::Error,
                             "[WinUmIoCtlDevice] Pipe was busy: %.1f second wait timed out.",
                             kLogicFailureTimeout * 1e-3);
                    result = Result::Error;
                    break;
                }
                else
                {
                    // Everything is OK, try again.
                    continue;
                }
            }
        }

        // Wave our wand at the named pipe and do the magic thing. Probably doesn't work without this block.
        if (result == Result::Success)
        {
            DWORD dwMode = PIPE_READMODE_MESSAGE;
            BOOL fSuccess = SetNamedPipeHandleState(
                hPipe,    // hNamedPipe
                &dwMode,  // lpMode
                nullptr,  // lpMaxCollectionCount
                nullptr); // lpCollectDataTimeout

            if (!fSuccess)
            {
                DWORD lastError = GetLastError();
                DD_PRINT(LogLevel::Error,
                         "[WinUmIoCtlDevice] SetNamedPipeHandleState failed. GetLastError() = %d (0x%x)",
                         lastError,
                         lastError);
                result = Result::Error;
            }
        }

        // Transact a read and write through the pipe.
        // The response data will come back to the same buffer, so we just need to error check and return.
        if (result == Result::Success)
        {
            DWORD bytesRead = 0;
            ULONG serverPID = 0;
            GetNamedPipeServerProcessId(hPipe, &serverPID);
            DD_PRINT(LogLevel::Debug, "[WinUmIoCtlDevice] Connected to server PID %u", serverPID);

            // Try to open a named pipe; wait for it, if necessary.
            // Our input and output buffers are the same - we expect the response next to the input data.
            DD_ASSERT(bufferSize <= UINT32_MAX);
            DWORD dwBufferSize = static_cast<uint32>(bufferSize);
            BOOL fSuccess = TransactNamedPipe(
                hPipe,        // pipe name
                pBuffer,      // message
                dwBufferSize, // message length
                pBuffer,      // buffer to receive reply
                dwBufferSize, // size of buffer
                &bytesRead,   // number of bytes read
                nullptr       // lpOverlapped
            );
            // Now that we got the data, close the handle. We won't use it any more
            CloseHandle(hPipe);

            if ((!fSuccess) || (bytesRead < kMinIoCtlMessageSize))
            {
                DD_WARN(!fSuccess);
                DD_WARN(bytesRead < kMinIoCtlMessageSize);

                DWORD lastError = GetLastError();
                DD_PRINT(LogLevel::Error, "[WinUmIoCtlDevice] TransactNamedPipe to pipe failed. GetLastError() = %d (0x%x)", lastError, lastError);
                result = Result::Error;
            }
        }

        return result;
    }

    Result WinUmIoCtlDevice::InDirectIoCtl(
        size_t inSize,
        void*  pInBuffer,
        size_t outSize,
        void*  pOutBuffer
    )
    {
        DD_UNUSED(inSize);
        DD_UNUSED(pInBuffer);
        DD_UNUSED(outSize);
        DD_UNUSED(pOutBuffer);

        return Result::Unavailable;
    }

} // DevDriver
