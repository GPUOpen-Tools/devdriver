/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <ModuleLogger.h>

using namespace DevDriver;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Forwards formatted log messages into the loader's log callback
void ModuleLogger::Vprintf(DD_LOG_LEVEL level, const char* pFmt, va_list args)
{
    m_logger.Vprintf(DD_MAKE_LOG_EVENT(level, m_pLogCategory), pFmt, args);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ModuleLogger::Printf(DD_LOG_LEVEL level, const char* pFmt, ...)
{
    va_list args;
    va_start(args, pFmt);
    Vprintf(level, pFmt, args);
    va_end(args);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ModuleLogger::Error(const char* pFmt, ...)
{
    va_list args;
    va_start(args, pFmt);
    Vprintf(DD_LOG_LEVEL_ERROR, pFmt, args);
    va_end(args);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ModuleLogger::Warn(const char* pFmt, ...)
{
    va_list args;
    va_start(args, pFmt);
    Vprintf(DD_LOG_LEVEL_WARN, pFmt, args);
    va_end(args);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ModuleLogger::Info(const char* pFmt, ...)
{
    va_list args;
    va_start(args, pFmt);
    Vprintf(DD_LOG_LEVEL_INFO, pFmt, args);
    va_end(args);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ModuleLogger::Verbose(const char* pFmt, ...)
{
    va_list args;
    va_start(args, pFmt);
    Vprintf(DD_LOG_LEVEL_VERBOSE, pFmt, args);
    va_end(args);
}
