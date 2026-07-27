//=============================================================================
/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation for base RTT token emitter.
//=============================================================================

#include "rmt_token_emitter.h"

#include <util/rmtTokens.h>

#if STM_ENABLE_KERNEL_TRACING
#if defined(DD_PLATFORM_WINDOWS_UM)
#include "win/debug_name_token_emitter.h"
#else
#include "linux/page_table_update_token_emitter.h"
#endif
#endif

namespace SystemTraceModule
{
#if defined(DD_PLATFORM_WINDOWS_UM)
    /// @brief Environment variable that will disable support for ETW when it is set.
    ///
    /// This allows external applications to work around ETW-related incompatibilities when necessary.
    static constexpr char kDisableETWEnvVarName[] = "DEVTOOLS_DISABLE_ETW";
#endif

    RMTTokenEmitter* RMTTokenEmitter::Create(DevDriver::IMsgChannel* msg_channel, const DevDriver::AllocCb& alloc_cb, ModuleLogger& logger)
    {
        RMTTokenEmitter* emitter = nullptr;

#if STM_ENABLE_KERNEL_TRACING
#if defined(DD_PLATFORM_WINDOWS_UM)
        // Support for ETW is always enabled on Windows as long as the "disable" environment variable is not set.
        char*  pEnvVal    = nullptr;
        size_t envValSize = 0;
        _dupenv_s(&pEnvVal, &envValSize, kDisableETWEnvVarName);
        const bool is_etw_enabled = (pEnvVal == nullptr);
        free(pEnvVal);
        if (is_etw_enabled)
        {
            emitter = DD_NEW(DebugNameTokenEmitter, alloc_cb)(msg_channel, logger);
        }
#elif defined(DD_PLATFORM_LINUX_UM)
        emitter = DD_NEW(PageTableUpdateTokenEmitter, alloc_cb)(msg_channel, alloc_cb, logger);
#else
        static_assert(false, "Unsupported platform for RmtTokenEmitter");
#endif
#else
        DD_UNUSED(msg_channel);
        DD_UNUSED(alloc_cb);
        DD_UNUSED(logger);
#endif

        return emitter;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void RMTTokenEmitter::Destroy(RMTTokenEmitter* emitter, const DevDriver::AllocCb& alloc_cb)
    {
        DD_DELETE(emitter, alloc_cb);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    RMTTokenEmitter::RMTTokenEmitter(DevDriver::IMsgChannel* msg_channel, ModuleLogger& logger)
        : h_provider_(DD_API_INVALID_HANDLE)
        , logger_(logger)
        , msg_channel_(msg_channel)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DevDriver::IMsgChannel* RMTTokenEmitter::GetMsgChannel()
    {
        return msg_channel_;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DD_RESULT RMTTokenEmitter::CalculateTimeDelta(DevDriver::uint8* out_delta)
    {
        DD_ASSERT(out_delta != nullptr);

        *out_delta = 0;

        DD_RESULT result = DD_RESULT_SUCCESS;

        const DevDriver::EventTimestamp timestamp = event_timer_.CreateTimestamp();

        if (timestamp.type == DevDriver::EventTimestampType::Full)
        {
            DevDriver::RMT_MSG_TIMESTAMP token(timestamp.full.timestamp, timestamp.full.frequency);
            result = ddEventServerEmit(h_provider_, kRMTToken, token.Size(), token.Data());
        }
        else if (timestamp.type == DevDriver::EventTimestampType::LargeDelta)
        {
            DevDriver::RMT_MSG_TIME_DELTA token(timestamp.largeDelta.delta, timestamp.largeDelta.numBytes);
            result = ddEventServerEmit(h_provider_, kRMTToken, token.Size(), token.Data());
        }
        else
        {
            *out_delta = timestamp.smallDelta.delta;
        }

        return result;
    }

    void RMTTokenEmitter::ResetTimer()
    {
        event_timer_.Reset();
    }
}
