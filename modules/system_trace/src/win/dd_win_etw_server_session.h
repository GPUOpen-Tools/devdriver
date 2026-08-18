//=============================================================================
/* Copyright (C) 2017-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief  Class definitions for an ETW session.
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
        /// @brief Base name for the ETW trace session.
        constexpr const char* kTraceSessionName = "RDS Trace Session";

        /// @brief GUID for the DX kernel ETW provider.
        static constexpr PCWSTR kDxgKernelProviderGUID = L"{802ec45a-1e99-4b83-9920-87c98277ba9d}";

        /// @brief The keyword filter to use for the DxgKernel ETW provider.
        ///
        /// The Direct3D12 Parser has several events it looks for, which are the keys in kObjectTypeMap.
        /// Each event has a set of keywords, and the ones we're interested in are Base and Resource, since this is sufficient to capture the events we need.
        /// The Base and Resource keywords have a bitmask of 0x1 and 0x40 respectively for this provider.
        /// We want to have events that have either of these keywords, so we bitwise or them together to create a new mask that will include events from either.
        static constexpr ULONGLONG kDxgKernelKeywordFilter = 0x1 | 0x40;

        /// @brief A process id that we trace when we are only interested in catching association context events.
        ///
        ///  All traces that we do will catch these - we name and use this to filter out other events.
        /// These associate events are sent on queue creation - probably - so we need to always listen for them.
        constexpr ProcessId kAssocationContextProcessId = 0;

        /// @brief The current state of an ETW session.
        enum class SessionState
        {
            kIdle = 0,
            kTracing,
            kStreaming,
            kTransmitMessage
        };

        /// @brief A session that handles ETW events.
        class ETWSession : public ETWConsumerBase
        {
        public:
            /// @brief Constructor.
            /// @param session The parent session.
            /// @param alloc_cb An allocator callback for this session to use.
            ETWSession(const SharedPointer<ISession>& session, const AllocCb& alloc_cb)
                : session_(session)
                , alloc_cb_(alloc_cb)
                , state_(SessionState::kIdle)
                , payload_()
                , trace_(alloc_cb)
                , num_events_(0)
                , trace_session_()
                , trace_parser_()
                , trace_in_progress_(false)
            {
                DD_UNUSED(alloc_cb_);
            };

            /// @brief Destructor.
            ~ETWSession() override
            {
                if (trace_in_progress_)
                {
                    trace_session_.Stop();
                    trace_in_progress_ = false;
                    trace_thread_.join();
                    trace_.Clear();
                }
            };

            /// @brief Attempts to advance the session to the next state.
            ///
            /// If the session is idle, looks for a begin trace message from the session. If one is
            /// received, attempt to start a new trace and then transition to the tracing state.
            ///
            /// If the session is transmit message, transmit the pending payload and transition to
            /// the next state.
            ///
            /// If the session state is tracing, looks for an end trace message from the session. If one is
            /// received, attempt to end the current trace, discarding the trace data if requested.
            ///
            /// If the session is streaming attempt to send each event in the current trace. If all
            /// events are sent, send a sentinel.
            void UpdateSession()
            {
                DD_ASSERT(this == reinterpret_cast<ETWSession*>(session_->GetUserData()));

                switch (state_)
                {
                case SessionState::kIdle:
                {
                    ETWPayload payload;
                    uint32     bytes_received = 0;
                    Result     result         = session_->Receive(sizeof(payload), &payload, &bytes_received, kNoWait);
                    if (result == Result::Success)
                    {
                        DD_ASSERT(sizeof(payload) == bytes_received);
                        trace_.Clear();
                        num_events_ = 0;

                        if (payload.command == ETWMessage::BeginTrace)
                        {
                            // End the previous trace - the only data that we want from it is Association Contexts.
                            Result prev_trace_result = EndTrace();
                            // This can only fail if  there wasn't a previous trace.
                            // We never expect this to happen, so we assert in debug builds and
                            // ignore it in release builds.
                            DD_ASSERT(prev_trace_result == Result::Success);
                            DD_UNUSED(prev_trace_result);

                            DD_PRINT(LogLevel::Info, "[ETWSession] Trace request received");
                            payload_.command                   = ETWMessage::BeginResponse;
                            payload_.startTraceResponse.result = BeginTrace(payload.startTrace.processId);
                            DD_ASSERT(payload_.startTraceResponse.result == Result::Success);
                            TransmitAndChangeState();
                        }
                    }
                    break;
                }
                case SessionState::kTransmitMessage:
                {
                    TransmitAndChangeState();
                    break;
                }
                case SessionState::kTracing:
                {
                    ETWPayload payload;
                    uint32     bytes_received = 0;
                    Result     result         = session_->Receive(sizeof(payload), &payload, &bytes_received, kNoWait);
                    if (result == Result::Success)
                    {
                        DD_ASSERT(sizeof(payload) == bytes_received);
                        if (payload.command == ETWMessage::EndTrace)
                        {
                            payload_.command                  = ETWMessage::EndResponse;
                            payload_.stopTraceResponse.result = EndTrace();

                            if (payload.stopTrace.discard == 0)
                            {
                                payload_.stopTraceResponse.numEventsCaptured = static_cast<uint32>(num_events_);
                            }
                            else
                            {
                                DD_PRINT(LogLevel::Warn, "[ETWSession::UpdateSession] Discarding trace as requested");
                                payload_.stopTraceResponse.numEventsCaptured = 0;
                                trace_.Clear();
                            }

                            // Starting with RS5, we need to constantly listen for ETW events for AssociateContext events.
                            // After we finish tracing for the RGP file, we restart tracing in case more events are sent.
                            BeginTrace(kAssocationContextProcessId);

                            TransmitAndChangeState();
                        }
                    }
                    break;
                }
                case SessionState::kStreaming:
                {
                    while (trace_.Size() > 0)
                    {
                        ETWPayload* payload     = trace_.PeekFront();
                        Result      send_result = session_->Send(sizeof(ETWPayload), payload, kNoWait);
                        if (send_result == Result::Success)
                        {
                            trace_.PopFront();
                        }
                        else if (send_result == Result::NotReady)
                        {
                            break;
                        }
                    }
                    if ((trace_.Size() == 0) && (num_events_ > 0))
                    {
                        payload_.command                  = ETWMessage::TraceDataSentinel;
                        payload_.traceDataSentinel.result = Result::Success;
                        TransmitAndChangeState();
                    }
                    break;
                }
                default:
                    DD_UNREACHABLE();
                    break;
                }
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
                    DD_PRINT(LogLevel::Info, "[ETWSession::BeginTrace] Beginning trace");

                    // Append the target process id to the default trace session name to allow ETW traces to run in
                    // parallel across different processes.
                    char trace_name[128];
                    Platform::Strncpy(trace_name, kTraceSessionName, sizeof(trace_name));

                    char trace_suffix[32];
                    Platform::Snprintf(trace_suffix, sizeof(trace_suffix), " - (%u)", process_id);

                    Platform::Strncat(trace_name, trace_suffix, sizeof(trace_name));

                    const bool started = trace_session_.Start(trace_name);
                    if (started)
                    {
                        DD_PRINT(LogLevel::Info, "[ETWSession::BeginTrace] Trace session started");

                        if (trace_parser_.Start(process_id))
                        {
                            DD_PRINT(LogLevel::Info, "[ETWSession::BeginTrace] Trace parser started for process %u", process_id);
                            if (trace_session_.EnableProviderByGUID(kDxgKernelProviderGUID, 0, kDxgKernelKeywordFilter))
                            {
                                DD_PRINT(LogLevel::Info, "[ETWSession::BeginTrace] DXGK provider enabled");
                                if (trace_session_.Open(this))
                                {
                                    DD_PRINT(LogLevel::Info, "[ETWSession::BeginTrace] Trace session opened");
                                    trace_thread_ = std::thread(&TraceSession::Process, &trace_session_);
                                    if (trace_thread_.joinable())
                                    {
                                        DD_PRINT(LogLevel::Info, "[ETWSession::BeginTrace] Trace thread started");
                                        trace_in_progress_ = true;

                                        return Result::Success;
                                    }
                                }
                            }
                        }
                    }
                }
                DD_PRINT(LogLevel::Info, "[ETWSession::BeginTrace] Begin failed");
                return Result::Error;
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
                    DD_PRINT(LogLevel::Info, "[ETWSession::EndTrace] Ending trace");
                    if (trace_session_.Close())
                    {
                        DD_PRINT(LogLevel::Info, "[ETWSession::EndTrace] Trace session closed");
                        if (trace_session_.DisableProviderByGUID(kDxgKernelProviderGUID))
                        {
                            DD_PRINT(LogLevel::Info, "[ETWSession::EndTrace] DXGK provider disabled");
                            if (trace_session_.Stop())
                            {
                                DD_PRINT(LogLevel::Info, "[ETWSession::EndTrace] Trace session stopped");

                                trace_in_progress_ = false;
                                trace_thread_.join();
                                trace_.Clear();
                                num_events_ = trace_parser_.FinishTrace(trace_);
                                DD_PRINT(LogLevel::Info, "[ETWSession::EndTrace] Finished parsing %u events", num_events_);

                                return Result::Success;
                            }
                        }
                    }
                }
                DD_PRINT(LogLevel::Info, "[ETWSession::EndTrace] End failed");
                return Result::Error;
            }

            /// @brief Callback to be called when an ETW event arrives.
            ///
            /// Uses the trace parser to parse the incoming event.
            /// @param event The event that was recorded.
            void OnEventRecord(PEVENT_RECORD event) override
            {
                trace_parser_.ParseEvent(event);
            }

        private:
            SharedPointer<ISession> session_;            ///< The owning session.
            AllocCb                 alloc_cb_;           ///< An allocator callback.
            SessionState            state_;              ///< The current state of this session.
            ETWPayload              payload_;            ///< Payload sent to the owning session when TransmitAndChangeState() is called.
            Queue<ETWPayload>       trace_;              ///< The result of the last trace.
            size_t                  num_events_;         ///< The number of events parsed during a trace.
            TraceSession            trace_session_;      ///< The ETW tracing session.
            std::thread             trace_thread_;       ///< The thread that tracing is performed on.
            EtwParser               trace_parser_;       ///< Parses ETW events.
            bool                    trace_in_progress_;  ///< true if a trace is in progress, false otherwise.

            /// @brief Broadcasts the pending payload and then performs the correct state transition based
            /// off of the sent command.
            void TransmitAndChangeState()
            {
                if (session_->Send(sizeof(payload_), &payload_, kNoWait) == Result::Success)
                {
                    switch (payload_.command)
                    {
                    case ETWMessage::BeginResponse:
                    {
                        if (payload_.startTraceResponse.result == Result::Success)
                        {
                            state_ = SessionState::kTracing;
                        }
                        else
                        {
                            state_ = SessionState::kIdle;
                        }
                        break;
                    }
                    case ETWMessage::EndResponse:
                    {
                        if (payload_.stopTraceResponse.result == Result::Success && payload_.stopTraceResponse.numEventsCaptured != 0)
                        {
                            state_ = SessionState::kStreaming;
                        }
                        else
                        {
                            state_ = SessionState::kIdle;
                        }
                        break;
                    }
                    case ETWMessage::TraceDataSentinel:
                    {
                        state_ = SessionState::kIdle;
                        break;
                    }
                    default:
                    {
                        state_ = SessionState::kIdle;
                        DD_UNREACHABLE();
                        break;
                    }
                    }
                }
            }
        };
    }  // namespace ETWProtocol
}  // namespace DevDriver
