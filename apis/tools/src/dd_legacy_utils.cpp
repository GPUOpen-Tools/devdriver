/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "dd_legacy_utils.h"
#include <ddPlatform.h>

DD_LOG_LVL GetApiLogLvl(DD_LOG_LEVEL lvl)
{
    DD_LOG_LVL outLvl = DD_LOG_LVL_VERBOSE;
    switch (lvl)
    {
        case DD_LOG_LEVEL_INFO: outLvl = DD_LOG_LVL_INFO; break;
        case DD_LOG_LEVEL_WARN: outLvl = DD_LOG_LVL_WARN; break;
        case DD_LOG_LEVEL_ERROR: outLvl = DD_LOG_LVL_ERROR; break;
        case DD_LOG_LEVEL_ALWAYS:
        case DD_LOG_LEVEL_VERBOSE:
        case DD_LOG_LEVEL_DEBUG:
        default: outLvl = DD_LOG_LVL_VERBOSE; break;
    }

    return outLvl;
}

// Conversion functions for DDLogger:
int32_t WillLogLegacy(void* pUserdata, const DDLogEvent* pEvent)
{
    int32_t ret = 0;
    if ((pUserdata != nullptr) && (pEvent->level != DD_LOG_LEVEL_NEVER))
    {
        ret = 1;
    }

    return ret;
}

void LoggerLogLegacy(void* pUserdata, const DDLogEvent* pEvent, const char* pMessage)
{
    DDLoggerApi* pLog = static_cast<DDLoggerApi*>(pUserdata);

    if (pLog != nullptr)
    {
        pLog->Log(pLog->pInstance, GetApiLogLvl(pEvent->level), pMessage);
    }
}

void* AllocCbFuncLegacy(void* pUserdata, size_t size, size_t alignment, int zero)
{
    DD_UNUSED(pUserdata);
    return DevDriver::Platform::AllocateMemory(size, alignment, zero);
}

void FreeCbFuncLegacy(void* pUserdata, void* pMemory)
{
    DD_UNUSED(pUserdata);
    DevDriver::Platform::FreeMemory(pMemory);
}
