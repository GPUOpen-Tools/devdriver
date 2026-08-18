/* Copyright (C) 2021-2024 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Develoepr Tools Team
/// @file
/// @brief Class implementation for emitting RMT debug name tokens.
//=============================================================================

#include "debug_name_token_emitter.h"

#include <util/rmtTokens.h>

namespace SystemTraceModule
{
    DebugNameTokenEmitter::DebugNameTokenEmitter(DevDriver::IMsgChannel* msg_channel, ModuleLogger& logger)
        : RMTTokenEmitter(msg_channel, logger)
        , server_(msg_channel)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DD_RESULT DebugNameTokenEmitter::Initialize(DDEventProvider provider)
    {
        h_provider_ = provider;
        DD_PRINT(DevDriver::LogLevel::Debug, "[DebugNameTokenEmitter::Initialize] provider = 0x%lx", provider);

        DevDriver::IMsgChannel* msg_channel = GetMsgChannel();
        DD_RESULT               result      = DevDriverToDDResult(msg_channel->RegisterProtocolServer(&server_));
        session_.GetParser().SetLogger(&logger_);
        session_.GetParser().SetTokenEmitterInstance(this);
        session_.GetParser().SetDebugNameEmitterCallback(DebugNameTokenEmitter::HandleDebugNameEvent);
        session_.GetParser().SetImplicitResourceEmitterCallback(DebugNameTokenEmitter::HandleImplicitResourceEvent);

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void DebugNameTokenEmitter::Cleanup()
    {
        DevDriver::IMsgChannel* msg_channel = GetMsgChannel();
        DD_UNHANDLED_RESULT(msg_channel->UnregisterProtocolServer(&server_));
    }

    DD_RESULT DebugNameTokenEmitter::Enable()
    {
        // Note: Currently, resource name related ETW events for all running processes are included in the RMT.
        // The ETW parser uses a process ID of 0 to indicate parsing is complete.  Any other process ID indicates
        // parsing is active.
        DevDriver::ProcessId process_id = 1;
        DD_PRINT(DevDriver::LogLevel::Debug, "[DebugNameTokenEmitter::Enable] Begin trace");

        // Reset the timestamp event timer.
        ResetTimer();

        return (session_.BeginTrace(process_id) == DevDriver::Result::Success) ? DD_RESULT::DD_RESULT_SUCCESS : DD_RESULT::DD_RESULT_DD_GENERIC_ABORTED;
    }

    DD_RESULT DebugNameTokenEmitter::Disable()
    {
        return (session_.EndTrace() == DevDriver::Result::Success) ? DD_RESULT::DD_RESULT_SUCCESS : DD_RESULT::DD_RESULT_DD_GENERIC_ABORTED;
    }

    DD_RESULT DebugNameTokenEmitter::Emit()
    {
        // No-op.
        return DD_RESULT_SUCCESS;
    }

    DD_RESULT DebugNameTokenEmitter::HandleDebugNameEvent(const char* name, ULONGLONG key, void* instance, const uint64_t creation_timestamp)
    {
        DD_RESULT        result = DD_RESULT_SUCCESS;
        DevDriver::uint8 delta  = 0;

        DebugNameTokenEmitter* debug_name_token_emitter_instance = static_cast<DebugNameTokenEmitter*>(instance);
        DD_ASSERT(debug_name_token_emitter_instance != nullptr);

        const uint64_t current_timestamp = DevDriver::Platform::QueryTimestamp();
        const uint64_t time_delay        = current_timestamp - creation_timestamp;

        result = debug_name_token_emitter_instance->CalculateTimeDelta(&delta);

        switch (result)
        {
        case DD_RESULT_SUCCESS:
        {
            if (time_delay == 0)
            {
                DevDriver::RMT_MSG_USERDATA_DEBUG_NAME token(delta, name, static_cast<uint32_t>(key));
                result = ddEventServerEmit(debug_name_token_emitter_instance->h_provider_, kRMTToken, token.Size(), token.Data());
            }
            else
            {
                DevDriver::RMT_MSG_USERDATA_DEBUG_NAME_V2 token(delta, name, static_cast<uint32_t>(key), time_delay);
                result = ddEventServerEmit(debug_name_token_emitter_instance->h_provider_, kRMTToken, token.Size(), token.Data());
            }
#ifdef LOG_TOKENS
            static_cast<DebugNameTokenEmitter*>(instance)->logger_.Info(
                "[DebugNameTokenEmitter::HandleDebugNameEvent] Emit debug name token '%s' with correlation ID 0x%x. Delta = %i",
                name,
                static_cast<uint32_t>(key),
                delta);
#endif
            break;
        }

        case DD_RESULT_DD_EVENT_EMIT_PROVIDER_DISABLED:
        case DD_RESULT_DD_EVENT_EMIT_EVENT_DISABLED:
#ifdef LOG_TOKENS
            static_cast<DebugNameTokenEmitter*>(instance)->logger_.Info(
                "[DebugNameTokenEmitter::HandleDebugNameEvent] Emitter disabled for debug name token '%s' with correlation ID 0x%x. Delta = %i",
                name,
                static_cast<uint32_t>(key),
                delta);
#endif
            break;

        default:
#ifdef LOG_TOKENS
            static_cast<DebugNameTokenEmitter*>(instance)->logger_.Warn(
                "[DebugNameTokenEmitter::HandleDebugNameEvent] Error emitting debug name token '%s' with correlation ID 0x%x. Delta = %i, ",
                name,
                static_cast<uint32_t>(key),
                delta);
#endif
            break;
        }

        return result;
    }

    DD_RESULT DebugNameTokenEmitter::HandleImplicitResourceEvent(ULONGLONG key, void* instance, const uint64_t creation_timestamp, const uint8_t heap_type)
    {
        DD_RESULT result = DD_RESULT_SUCCESS;

        DebugNameTokenEmitter* debug_name_token_emitter_instance = static_cast<DebugNameTokenEmitter*>(instance);
        DD_ASSERT(debug_name_token_emitter_instance != nullptr);

        const uint64_t current_timestamp = DevDriver::Platform::QueryTimestamp();
        const uint64_t time_delay        = current_timestamp - creation_timestamp;

        DevDriver::uint8 delta = 0;
        result                 = debug_name_token_emitter_instance->CalculateTimeDelta(&delta);

        switch (result)
        {
        case DD_RESULT_SUCCESS:
        {
            if (time_delay == 0)
            {
                DevDriver::RMT_MSG_USERDATA_MARK_IMPLICIT_RESOURCE token(delta, static_cast<uint32_t>(key));
                result = ddEventServerEmit(debug_name_token_emitter_instance->h_provider_, kRMTToken, token.Size(), token.Data());
            }
            else
            {
                DevDriver::RMT_MSG_USERDATA_MARK_IMPLICIT_RESOURCE_V2 token(delta, static_cast<uint32_t>(key), time_delay, heap_type);
                result = ddEventServerEmit(debug_name_token_emitter_instance->h_provider_, kRMTToken, token.Size(), token.Data());
            }
#ifdef LOG_TOKENS
            static_cast<DebugNameTokenEmitter*>(instance)->logger_.Info(
                "[DebugNameTokenEmitter::HandleImplicitResourceEvent] Emit mark implicit resource token with resource ID 0x%x, heap_type = %i",
                static_cast<uint32_t>(key),
                static_cast<uint32_t>(heap_type));
#endif
            break;
        }

        case DD_RESULT_DD_EVENT_EMIT_PROVIDER_DISABLED:
        case DD_RESULT_DD_EVENT_EMIT_EVENT_DISABLED:
#ifdef LOG_TOKENS
            static_cast<DebugNameTokenEmitter*>(instance)->logger_.Info(
                "[DebugNameTokenEmitter::HandleImplicitResourceEvent] Emitter disabled for mark implicit resource token with resource ID 0x%x",
                static_cast<uint32_t>(key));
#endif
            break;

        default:
#ifdef LOG_TOKENS
            static_cast<DebugNameTokenEmitter*>(instance)->logger_.Info(
                "[DebugNameTokenEmitter::HandleImplicitResourceEvent] Error emitting mark implicit resource token with resource ID 0x%x.  Result code = "
                "0x%x",
                static_cast<uint32_t>(key),
                result);
#endif
            break;
        }
        return result;
    }
};
