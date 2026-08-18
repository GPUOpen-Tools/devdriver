/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddModule.h>

#include <ddCommon.h>

/// Utility class used to facilitate logging inside module code
class ModuleLogger
{
public:
    ModuleLogger(
        const DDModuleLoaderInterface& loader)
        : ModuleLogger("Default Base Module", loader)
    {
    }

    ModuleLogger(
        const char*                    pLogCategory,
        const DDModuleLoaderInterface& loader)
        : m_logger(loader.logger),
          m_pLogCategory(pLogCategory)
    {
    }

    void Vprintf(DD_LOG_LEVEL level, const char* pFmt, va_list args);
    void Printf(DD_LOG_LEVEL level, const char* pFmt, ...);
    void Error(const char* pFmt, ...);
    void Warn(const char* pFmt, ...);
    void Info(const char* pFmt, ...);
    void Verbose(const char* pFmt, ...);

private:
    LoggerUtil              m_logger;
    const char*             m_pLogCategory;
};
