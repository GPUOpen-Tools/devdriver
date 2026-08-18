/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "RouterUtilsService.h"
#include <ddCommon.h>

#include <Psapi.h>
#include <Windows.h>

namespace RouterUtilsModule
{

RouterUtilsService::RouterUtilsService(DDLoggerInfo)
    : RouterUtilsRpc::IRouterUtilsRpcService {},
      m_sysInfo(DevDriver::Platform::GenericAllocCb)
{
    QueryAndCacheSystemInfoAsync();
}

RouterUtilsService::~RouterUtilsService()
{
    m_sysInfoThread.Join();
}

DD_RESULT RouterUtilsService::QueryPathByProcessId(
    const void* pParamBuffer,
    size_t paramBufferSize,
    const DDByteWriter& writer)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    uint32_t processId = 0;

    if (paramBufferSize >= sizeof(uint32_t))
    {
        DevDriver::Platform::Memcpy_s(&processId, sizeof(processId), pParamBuffer, sizeof(processId));
    }
    else
    {
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    if (result == DD_RESULT_SUCCESS)
    {
        HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (processHandle != NULL)
        {
            const int retryCount    = 4;
            DWORD     appPathBufLen = MAX_PATH; // Length of pAppPathBuf in wchar_t.
            wchar_t*  pAppPathBuf   = (wchar_t*)realloc(nullptr, appPathBufLen * sizeof(wchar_t));
            DWORD     appPathLen    = 0;  // Lenght of the actual path in wchar_t, not including null-terminator.

            DD_ASSERT(pAppPathBuf != nullptr);

            for (int i = 0; i < retryCount; ++i)
            {
                appPathLen = GetModuleFileNameExW(processHandle, NULL, pAppPathBuf, appPathBufLen);
                if (appPathLen > 0)
                {
                    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                    {
                        result = DD_RESULT_COMMON_BUFFER_TOO_SMALL;

                        appPathBufLen *= 2;
                        wchar_t* pNewAppPathBuf = (wchar_t*)realloc(pAppPathBuf, appPathBufLen * sizeof(wchar_t));
                        if (pNewAppPathBuf != nullptr)
                        {
                            pAppPathBuf = pNewAppPathBuf;
                        }
                        else
                        {
                            result = DD_RESULT_COMMON_OUT_OF_HEAP_MEMORY;
                            break;
                        }
                    }
                    else
                    {
                        result = DD_RESULT_SUCCESS;
                        break;
                    }
                }
                else
                {
                    result = DD_RESULT_COMMON_UNKNOWN;
                }
            }
            if (result == DD_RESULT_SUCCESS)
            {
                const int u8PathBufSize = WideCharToMultiByte(CP_UTF8, 0, pAppPathBuf, appPathLen, NULL, 0, NULL, NULL);
                char*     u8PathBuf     = (char*)malloc(u8PathBufSize);
                const int bytesWritten  = WideCharToMultiByte(
                    CP_UTF8, 0, pAppPathBuf, appPathLen, u8PathBuf, u8PathBufSize, NULL, NULL);

                if (bytesWritten == u8PathBufSize)
                {
                    ByteWriterWrapper wrapper(writer);
                    result = wrapper.Begin(u8PathBufSize);
                    if (result == DD_RESULT_SUCCESS)
                    {
                        result = wrapper.Write(u8PathBuf, bytesWritten);
                    }
                    wrapper.End(result);
                }
                else
                {
                    result = DD_RESULT_COMMON_UNKNOWN;
                }
                free(u8PathBuf);
            }
            free(pAppPathBuf);
            CloseHandle(processHandle);
        }
    }

    return result;
}

DD_RESULT RouterUtilsService::QueryTimestampAndFrequency(const DDByteWriter& writer)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    uint64_t counter   = 0;
    uint64_t frequency = 0;

    LARGE_INTEGER ticks {};
    if (QueryPerformanceCounter(&ticks))
    {
        counter = ticks.QuadPart;
    }
    else
    {
        result = DD_RESULT_COMMON_UNKNOWN;
    }

    if (result == DD_RESULT_SUCCESS)
    {
        LARGE_INTEGER freq {};
        if (QueryPerformanceFrequency(&freq))
        {
            frequency = freq.QuadPart;
        }
        else
        {
            result = DD_RESULT_COMMON_UNKNOWN;
        }
    }

    ByteWriterWrapper wrapper(writer);
    result = wrapper.Begin(sizeof(frequency) + sizeof(counter));
    if (result == DD_RESULT_SUCCESS)
    {
        result = wrapper.Write(&counter, sizeof(counter));
        result = wrapper.Write(&frequency, sizeof(frequency));
    }
    wrapper.End(result);

    return result;
}

DD_RESULT RouterUtilsService::QueryDeviceClocks(const void*, size_t, const DDByteWriter&)
{
    return DD_RESULT_COMMON_UNIMPLEMENTED;
}

DD_RESULT RouterUtilsService::QueryCurrentClockMode(const void*, size_t, const DDByteWriter&)
{
    return DD_RESULT_COMMON_UNIMPLEMENTED;
}

DD_RESULT RouterUtilsService::SetClockMode(const void*, size_t)
{
    return DD_RESULT_COMMON_UNIMPLEMENTED;
}
};  // namespace RouterUtilsModule
