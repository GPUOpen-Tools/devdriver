/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddPlatform.h>

namespace DevDriver
{
    inline void LogPipeError(DWORD pipeErrorCode)
    {
        const char* pPipeErrorString = nullptr;

        switch (pipeErrorCode)
        {
            case ERROR_IO_INCOMPLETE:      pPipeErrorString = "IO Incomplete";      break;
            case ERROR_BROKEN_PIPE:        pPipeErrorString = "Broken Pipe";        break;
            case ERROR_OPERATION_ABORTED:  pPipeErrorString = "Operation Aborted";  break;
            case ERROR_PIPE_NOT_CONNECTED: pPipeErrorString = "Pipe Not Connected"; break;
            default: /* Leave the error string as nullptr */                        break;
        }

        if (pPipeErrorString != nullptr)
        {
            DD_PRINT(LogLevel::Info, "Pipe Error: %s", pPipeErrorString);
        }
        else
        {
            DD_PRINT(LogLevel::Info, "Pipe Error: Unknown (0x%x)", pipeErrorCode);
        }
    }

    // Construct a pipe name with the required prefix and a user-supplied port.
    // Return Result::Success if the constructed name fits the size `bufSize` - 1,
    // otherwise `pipeNameBuf` is set to empty and Result::InvalidParameter is
    // returned.
    template <size_t bufSize>
    inline Result MakePipeName(char (&pipeNameBuf)[bufSize], uint16 port)
    {
        // This function should never be called with a zero sized destination buffer
        DD_ASSERT(bufSize > 0);

        Result result = Result::InvalidParameter;

        const char* kPipePrefix = "\\\\.\\pipe\\AMD-Developer-Service";

        int32 len = 0;
        if (port == 0)
        {
            len = Platform::Snprintf(pipeNameBuf, bufSize, "%s", kPipePrefix);
        }
        else
        {
            len = Platform::Snprintf(pipeNameBuf, bufSize, "%s-%hu", kPipePrefix, port);
        }

        if ((len > 0) && (static_cast<size_t>(len) <= bufSize))
        {
            result = Result::Success;
        }
        else
        {
            // Mark the output buffer as an "invalid pipe name"
            pipeNameBuf[0] = '\0';
        }

        return result;
    }

    // Check if pipe name is not null and non-empty.
    inline bool IsValidPipeName(const char* pPipeName)
    {
        return (pPipeName != nullptr) && (pPipeName[0] != '\0');
    }
}
