/* Copyright (C) 2017-2024 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief  Class definitions for an ETW name session.
//=============================================================================

#pragma once

#include <thread>

#include "gpuopen.h"
#include "protocols/etwProtocol.h"
#include "util/queue.h"

#include "d3d12_etw_event_parser.h"
#include "trace_session.h"

namespace DevDriver
{
    namespace ETWProtocol
    {
        /// @brief Base name for the ETW trace session
        constexpr const char* kTraceDebugNameSessionName = "Debug Name Trace Session";

        /// @brief The keyword filter to use for the Direct3D12 ETW provider.
        ///
        /// The Direct3D12 Parser uses three prefixes to filter events: kEtwDx12DebugObjectEventNameString, kEtwDx12ResourceEventString and kEtwDx12HeapEventString.
        /// Each event has a set of keywords, and the ones we're interested in are Names and Resources, since this is sufficient to capture the events we need.
        /// The Names and Resources keywords have a bitmask of 0x1 and 0x4 respectively for this provider.
        /// We want to have events that have either of these keywords, so we bitwise or them together to create a new mask that will include events from either.
        static constexpr ULONGLONG kDirect3D12KeywordFilter = 0x1 | 0x4;

        /// @brief A session that handles debug name events.
        class ETWDebugNameSession : public ETWConsumerBase
        {
        public:
            /// @brief Constructor.
            ETWDebugNameSession() = default;

            /// @brief Destructor.
            ~ETWDebugNameSession() override
            {
                if (trace_in_progress_)
                {
                    trace_session_.Close();
                    trace_session_.Stop();
                    trace_in_progress_ = false;
                    trace_thread_.join();
                }
            };

            /// @brief No-op.
            void UpdateSession()
            {
            }

            /// @brief Opens the trace session and begins a trace.
            ///
            /// This will error if there is already a trace in progress.
            /// @param process_id The target process id.
            /// @retval Result::Success The trace was successfully stopped.
            /// @retval Result::Error There was an error stopping the trace.
            Result BeginTrace(ProcessId process_id)
            {
                if (!trace_in_progress_)
                {
                    DD_PRINT(LogLevel::Info, "[ETWDebugNameSession::BeginTrace] Beginning trace");

                    // Append the target process id to the default trace session name to allow ETW traces to run in
                    // parallel across different processes.
                    char trace_name[128];
                    Platform::Strncpy(trace_name, kTraceDebugNameSessionName, sizeof(trace_name));

                    char trace_suffix[32];
                    Platform::Snprintf(trace_suffix, sizeof(trace_suffix), " - (%u)", process_id);

                    Platform::Strncat(trace_name, trace_suffix, sizeof(trace_name));

                    const bool started = trace_session_.Start(trace_name);
                    if (started)
                    {
                        DD_PRINT(LogLevel::Info, "[ETWDebugNameSession::BeginTrace] Trace session started");

                        if (trace_parser_.Start(process_id))
                        {
                            DD_PRINT(LogLevel::Info, "[ETWDebugNameSession::BeginTrace] Trace parser started for process %u", process_id);
                            UCHAR level = 5;
                            if (trace_session_.EnableProviderByGUID(kDirect3D12ProviderGuid, level, kDirect3D12KeywordFilter))
                            {
                                DD_PRINT(LogLevel::Info, "[ETWDebugNameSession::BeginTrace] Direct3D12 provider enabled");
                                if (trace_session_.Open(this))
                                {
                                    DD_PRINT(LogLevel::Info, "[ETWDebugNameSession::BeginTrace] Trace session opened");
                                    trace_thread_ = std::thread(&TraceSession::Process, &trace_session_);
                                    if (trace_thread_.joinable())
                                    {
                                        DD_PRINT(LogLevel::Info, "[ETWDebugNameSession::BeginTrace] Trace thread started");
                                        trace_in_progress_ = true;
                                        return Result::Success;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        DD_PRINT(LogLevel::Info, "[ETWDebugNameSession::BeginTrace] Unable to start session.");
                        return Result::Error;
                    }
                }
                DD_PRINT(LogLevel::Info, "[ETWDebugNameSession::BeginTrace] Already in progress.");
                return Result::Success;
            }

            /// @brief Finishes the current trace by closing the trace session and joining the trace processing thread.
            ///
            /// This will error if a trace is not in progress.
            /// @retval Result::Success The trace was successfully stopped.
            /// @retval Result::Error There was an error stopping the trace.
            Result EndTrace()
            {
                if (trace_in_progress_)
                {
                    DD_PRINT(LogLevel::Info, "[ETWDebugNameSession::EndTrace] Ending trace");
                    if (trace_session_.Close())
                    {
                        DD_PRINT(LogLevel::Info, "[ETWDebugNameSession::EndTrace] Trace session closed");

                        if (trace_session_.DisableProviderByGUID(kDirect3D12ProviderGuid))
                        {
                            DD_PRINT(LogLevel::Info, "[ETWDebugNameSession::EndTrace] Direct3D12 provider disabled");
                            if (trace_session_.Stop())
                            {
                                DD_PRINT(LogLevel::Info, "[ETWSession::EndTrace] Trace session stopped");

                                trace_in_progress_ = false;
                                trace_thread_.join();
                                trace_parser_.FinishTrace();

                                return Result::Success;
                            }
                        }
                    }
                }
                DD_PRINT(LogLevel::Info, "[ETWDebugNameSession::EndTrace] End failed");
                return Result::Error;
            }

            /// @brief Callback to be called when an ETW event arrives.
            ///
            /// Uses the trace parser to parse the incoming event.
            /// @param event The event that was recorded.
            void OnEventRecord(PEVENT_RECORD event) override
            {
                DD_PRINT(LogLevel::Debug, "[ETWDebugNameSession::OnEventRecord]");
                trace_parser_.ParseEvent(event);
            }

            /// @brief Provides the event parser used by this session.
            /// @return The event parser used by this session.
            EtwParser& GetParser()
            {
                return trace_parser_;
            }

        private:
            TraceSession trace_session_;              ///< The ETW tracing session.
            std::thread  trace_thread_;               ///< The thread that tracing is performed on.
            EtwParser    trace_parser_;               ///< Parses ETW events.
            bool         trace_in_progress_ = false;  ///< true if a trace is in progress, false otherwise.
        };
    }  // namespace ETWProtocol
}  // namespace DevDriver
