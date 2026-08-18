//=============================================================================
/* Copyright (C) 2017-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief  Class declarations for an ETW trace session.
//=============================================================================

#pragma once

#include <Windows.h>

#include <Tdh.h>
#include <evntcons.h>
#include <evntprov.h>
#include <evntrace.h>

#include "ddPlatform.h"

/// @brief The base class for a real-time ETW consumer.
class ETWConsumerBase
{
public:
    /// @brief Callback to be called when an ETW event arrives.
    /// @param event_record The event that was recorded.
    virtual void OnEventRecord(PEVENT_RECORD event_record) = 0;

    /// @brief Destructor.
    virtual ~ETWConsumerBase() = default;
};

/// An ETW trace session that can be enable providers and start a real-time event trace.
class TraceSession
{
public:
    /// @brief Constructor.
    TraceSession() = default;

    /// @brief Constructor.
    ~TraceSession() = default;

    /// @brief Start the trace session.
    /// @return True when the session was started correctly, and false if it failed.
    bool Start(const char* session_name);

    /// Enable the ETW provider with the incoming GUID.
    /// @param in_guid The GUID for the provider to enable.
    /// @param level The level of detail to provide in each logged event.
    /// @param any_keyword A bitmask to determine the set of events to provide.
    /// @param all_keyword A bitmask to restrict the set of event categories to provide.
    /// @return True if enabling the provider was successful, and false if it failed.
    bool EnableProvider(const GUID& in_guid, UCHAR level, ULONGLONG any_keyword = 0, ULONGLONG all_keyword = 0);

    /// @brief Enable the ETW provider with the incoming GUID string.
    /// @param in_guid The GUID string for the provider to enable.
    /// @param level The level of detail to provide in each logged event.
    /// @param any_keyword A bitmask to determine the set of events to provide.
    /// @param all_keyword A bitmask to restrict the set of event categories to provide.
    /// @return True if enabling the provider was successful, and false if it failed.
    bool EnableProviderByGUID(const LPCWSTR& in_guid, UCHAR level, ULONGLONG any_keyword = 0, ULONGLONG all_keyword = 0);

    /// @brief On a trace with the provided consumer.
    /// @param consumer The consumer to use when opening a new tracing session.
    /// @return True when the trace was opened successfully, and false if it failed.
    bool Open(ETWConsumerBase* consumer);

    /// @brief Process all new incoming events from the trace session.
    /// @return True when processing the trace was successful, and false when it fails.
    bool Process();

    /// @brief Close an active trace session.
    /// @return True when the trace was closed successfully, and false if it failed.
    bool Close();

    /// @brief Disable a trace provider by GUID.
    /// @param provider_id The GUID for the provider to disable.
    /// @return True when the provider was disabled successfully, and false if it failed.
    bool DisableProvider(const GUID& provider_id);

    /// @brief Disable a trace provider by GUID.
    /// @param in_guid The GUID for the provider to disable.
    /// @return True when the provider was disabled successfully, and false if it failed.
    bool DisableProviderByGUID(const LPCWSTR& in_guid);

    /// @brief Stop the tracing session from processing events.
    /// @return True when the trace session was stopped successfully, and false if it failed.
    bool Stop();

    /// @brief Retrieve the trace session's timestamp frequency.
    /// @return The trace session's timestamp frequency.
    LONGLONG PerfFreq() const;

    /// @brief Queries if ETW is supported on the system.
    /// @return True if ETW is supported.
    static bool QueryETWSupport();

private:
    /// @brief Information about an ETW trace session.
    struct SessionProperties
    {
        EVENT_TRACE_PROPERTIES properties;  ///< Properties of the ETW session.
        char                   name[128];   ///< Storage for the ETW session name.
    };

    SessionProperties   session_;             ///< The information about the ETW trace session.
    EVENT_TRACE_LOGFILE trace_log_file_;      ///< The trace logfile to stream data to.
    TRACEHANDLE         session_handle_ = 0;  ///< The ETW trace session handle.
    TRACEHANDLE         trace_handle_   = 0;  ///< The handle for the active ETW trace.
};
