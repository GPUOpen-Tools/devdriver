/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "RouterUtilsService.h"

#include <system_info_writer.h>

#include <ddCommon.h>
#include <util/ddJsonWriter.h>
#include <util/ddStructuredReader.h>

namespace RouterUtilsModule
{

static constexpr char const* kSourceVersionLabel = "version";
static constexpr uint32_t    kSourceVersion      = 4;

void RouterUtilsService::QueryAndCacheSystemInfoAsync()
{
    m_sysInfoThread.Start(&RouterUtilsService::QueryAndCacheSystemInfo, this);
}

void RouterUtilsService::QueryAndCacheSystemInfo(void* pUserdata)
{
    RouterUtilsService* pService = (RouterUtilsService*)pUserdata;
    DevDriver::RWLockGuard<DevDriver::RWLock::LockType::Write> lock(pService->m_sysInfoLock);

    DevDriver::JsonWriter json_writer(&pService->m_sysInfo);
    json_writer.BeginMap();
    {
        // Add "version" number.
        json_writer.KeyAndValue(kSourceVersionLabel, kSourceVersion);

        json_writer.KeyAndBeginMap("system");
        system_info_utils::SystemInfoWriter::WriteSystemInfo(&json_writer);
        json_writer.EndMap();
    }
    json_writer.EndMap();

    if (DevDriverToDDResult(json_writer.End()) != DD_RESULT_SUCCESS)
    {
        pService->m_sysInfo.Clear();
    }
}

DD_RESULT RouterUtilsService::QuerySystemInfo(const DDByteWriter& writer)
{
    DevDriver::RWLockGuard<DevDriver::RWLock::LockType::Read> lock(m_sysInfoLock);
    DD_RESULT result = !m_sysInfo.IsEmpty() ? DD_RESULT_SUCCESS : DD_RESULT_UNKNOWN;

    if (result == DD_RESULT_SUCCESS)
    {
        ByteWriterWrapper wrapper(writer);
        result = wrapper.Begin(m_sysInfo.Size());
        if (result == DD_RESULT_SUCCESS)
        {
            result = wrapper.Write(m_sysInfo.Data(), m_sysInfo.Size());
        }
        wrapper.End(result);
    }

    return result;
}

};  // namespace RouterUtilsModule

