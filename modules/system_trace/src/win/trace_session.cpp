//=============================================================================
/* Copyright (C) 2017-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief  Implementation for an ETW trace session.
//=============================================================================

#include "trace_session.h"

#include <ObjBase.h>
#include <tchar.h>
#include <string>

#include "ddPlatform.h"

#define SAFE_DELETE(x)     \
    {                      \
        if (x)             \
        {                  \
            free(x);       \
            (x) = nullptr; \
        }                  \
    }
#define SAFE_DELETE_ARRAY(x) \
    {                        \
        if (x)               \
        {                    \
            free(x);         \
            (x) = nullptr;   \
        }                    \
    }

bool TraceSession::Start(const char* session_name)
{
    bool result = false;
    DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::Start] Start called");

    if (session_handle_ == 0)
    {
        ZeroMemory(&session_, sizeof(session_));
        session_.properties.Wnode.BufferSize = sizeof(session_);

        DevDriver::Platform::Strncpy(session_.name, session_name, sizeof(session_.name));

        // Setting this to "1" means event timestamps will be based on QueryPerformanceCounter.
        session_.properties.Wnode.ClientContext = 1;
        session_.properties.Wnode.Flags         = WNODE_FLAG_TRACED_GUID;
        session_.properties.LogFileMode         = EVENT_TRACE_REAL_TIME_MODE;
        session_.properties.LoggerNameOffset    = sizeof(EVENT_TRACE_PROPERTIES);
        session_.properties.LogFileNameOffset   = 0;

        // Create the trace session.
        ULONG status = StartTrace(&session_handle_, session_.name, &session_.properties);
        DD_PRINT(DevDriver::LogLevel::Info, "[TraceSession::Start] Etw Trace StartTrace() status == %u", status);

        // If we fail to start the trace because one already exists with the same name, attempt to
        // stop the existing trace, then start a new one.
        if (status == ERROR_ALREADY_EXISTS)
        {
            DD_PRINT(DevDriver::LogLevel::Info, "[TraceSession::Start] Etw Trace already exists - Stopping.");
            // Stop the existing trace.
            status = ControlTrace(NULL, session_.name, &session_.properties, EVENT_TRACE_CONTROL_STOP);
            DD_PRINT(DevDriver::LogLevel::Info, "[TraceSession::Start] Etw Trace ControlTrace(Stop) status == %u", status);
            if (status == ERROR_SUCCESS)
            {
                // Start a new trace if we successfully stopped the existing one.
                status = StartTrace(&session_handle_, session_.name, &session_.properties);
                DD_PRINT(DevDriver::LogLevel::Info, "[TraceSession::Start] Etw Trace StartTrace() (second) status == %u", status);
            }
        }

        result = (status == ERROR_SUCCESS);
        DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::Start] Start: %u", status);
    }
    return result;
}

bool TraceSession::EnableProvider(const GUID& in_guid, UCHAR level, ULONGLONG any_keyword, ULONGLONG all_keyword)
{
    bool result = false;
    DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::EnableProvider] EnableProvider called");

    if (session_handle_ != 0)
    {
        ULONG status = EnableTraceEx2(session_handle_, &in_guid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, level, any_keyword, all_keyword, 0, nullptr);
        result       = (status == ERROR_SUCCESS);
    }
    return result;
}

bool TraceSession::EnableProviderByGUID(const LPCWSTR& in_guid, UCHAR level, ULONGLONG any_keyword, ULONGLONG all_keyword)
{
    bool result = false;
    DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::EnableProviderByGUID] EnableProviderByGUID called");
    if (session_handle_ != 0)
    {
        GUID    provider_guid;
        HRESULT converted = CLSIDFromString(in_guid, &provider_guid);

        if (converted == S_OK)
        {
            ULONG status = EnableTraceEx2(session_handle_, &provider_guid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, level, any_keyword, all_keyword, 0, nullptr);
            result       = (status == ERROR_SUCCESS);
            DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::EnableProviderByGUID] Provider enabled: %u", status);
        }
    }

    // Failed to convert the incoming GUID.
    return result;
}

/// @brief The global callback for all incoming ETW events.
/// @param event_record The ETW event structure raised by the provider.
VOID WINAPI EventRecordCallback(PEVENT_RECORD event_record)
{
    reinterpret_cast<ETWConsumerBase*>(event_record->UserContext)->OnEventRecord(event_record);
}

bool TraceSession::Open(ETWConsumerBase* consumer)
{
    bool result = false;
    DD_ASSERT(consumer != nullptr);
    DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::Open] Open called");

    if (session_handle_ != 0 && trace_handle_ == 0)
    {
        ZeroMemory(&trace_log_file_, sizeof(trace_log_file_));
        trace_log_file_.LoggerName          = session_.name;
        trace_log_file_.ProcessTraceMode    = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
        trace_log_file_.EventRecordCallback = &EventRecordCallback;
        trace_log_file_.Context             = consumer;

        trace_handle_ = ::OpenTrace(&trace_log_file_);
        result        = (trace_handle_ != 0);
        DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::Open] Trace session open: %s", result ? "Successful" : "Unsuccessful");
    }
    return result;
}

bool TraceSession::Process()
{
    bool result = false;
    if (session_handle_ != 0 && trace_handle_ != 0)
    {
        DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::Process] Process trace starting");
        ULONG status = ProcessTrace(&trace_handle_, 1, nullptr, nullptr);
        DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::Process] Process trace finished");
        result = (status == ERROR_SUCCESS);
    }
    DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::Process] Trace session processing: %s", result ? "Successful" : "Unsuccessful");
    return result;
}

bool TraceSession::Close()
{
    bool result = false;
    DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::Close] Trace session closing");
    if (trace_handle_ != 0)
    {
        // We should always have a valid session when we stop the trace.
        // If we don't, it probably means someone stopped the trace session before closing the trace.
        DD_ASSERT(session_handle_ != 0);

        ULONG status = ::CloseTrace(trace_handle_);
        result       = (status == ERROR_SUCCESS || status == ERROR_CTX_CLOSE_PENDING);
        DD_ASSERT(result);
        DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::Close] Trace session close: %u", status);
        trace_handle_ = 0;
    }
    return result;
}

bool TraceSession::DisableProvider(const GUID& provider_id)
{
    bool result = false;
    if (session_handle_ != 0)
    {
        ULONG status = EnableTraceEx2(session_handle_, &provider_id, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
        result       = (status == ERROR_SUCCESS);
        DD_ASSERT(result);
    }
    return result;
}

bool TraceSession::DisableProviderByGUID(const LPCWSTR& in_guid)
{
    bool result = false;
    if (session_handle_ != 0)
    {
        GUID    provider_guid;
        HRESULT converted = CLSIDFromString(in_guid, &provider_guid);

        if (converted == S_OK)
        {
            ULONG status = EnableTraceEx2(session_handle_, &provider_guid, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
            result       = (status == ERROR_SUCCESS);
            DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::DisableProviderByGUID] Provider disabled: %u", status);
        }
    }
    return result;
}

bool TraceSession::Stop()
{
    bool result = false;
    DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::Stop] Trace session stopping");
    if (session_handle_ != 0)
    {
        // We should always close the trace before stopping the session.
        DD_ASSERT(trace_handle_ == 0);

        ULONG status = ControlTrace(session_handle_, session_.name, &session_.properties, EVENT_TRACE_CONTROL_STOP);
        result       = (status == ERROR_SUCCESS);
        DD_ASSERT(result);
        DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::Stop] Trace session stop: %u", status);
        session_handle_ = 0;
    }
    return result;
}

LONGLONG TraceSession::PerfFreq() const
{
    return trace_log_file_.LogfileHeader.PerfFreq.QuadPart;
}

bool TraceSession::QueryETWSupport()
{
    bool result = false;

    SessionProperties session_properties = {};
    ZeroMemory(&session_properties, sizeof(session_properties));

    DevDriver::Platform::Strncpy(session_properties.name, "ETW Support Query", sizeof(session_properties.name));

    char trace_suffix[32];
    DevDriver::Platform::Snprintf(trace_suffix, sizeof(trace_suffix), " - (%u)", DevDriver::Platform::GetProcessId());

    DevDriver::Platform::Strncat(session_properties.name, trace_suffix, sizeof(session_properties.name));

    session_properties.properties.Wnode.BufferSize = sizeof(session_properties);

    session_properties.properties.Wnode.ClientContext = 1;
    session_properties.properties.Wnode.Flags         = WNODE_FLAG_TRACED_GUID;
    session_properties.properties.LogFileMode         = EVENT_TRACE_REAL_TIME_MODE;
    session_properties.properties.LoggerNameOffset    = sizeof(EVENT_TRACE_PROPERTIES);
    session_properties.properties.LogFileNameOffset   = 0;

    TRACEHANDLE session_handle;

    // Create the trace session.
    const ULONG start_status = StartTrace(&session_handle, session_properties.name, &session_properties.properties);
    if (start_status == ERROR_SUCCESS)
    {
        const ULONG stop_status = ControlTrace(session_handle, session_properties.name, &session_properties.properties, EVENT_TRACE_CONTROL_STOP);
        if (stop_status == ERROR_SUCCESS)
        {
            result = true;
        }
        else
        {
            DD_PRINT(DevDriver::LogLevel::Verbose, "[TraceSession::IsETWAvailable] Failed to stop ETW support query trace! Status: %u", stop_status);
        }
    }
    else if (start_status != ERROR_ACCESS_DENIED)
    {
        DD_PRINT(
            DevDriver::LogLevel::Verbose, "[TraceSession::IsETWAvailable] StartTrace in ETW support query returned an unexpected status: %u", start_status);
    }

    return result;
}
