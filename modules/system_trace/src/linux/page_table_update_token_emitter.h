//=============================================================================
/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief Class definition for emitting RMT page table update tokens.
//=============================================================================

#pragma once

#include <rmt_token_emitter.h>

#include "rmt_ftrace.h"

namespace SystemTraceModule
{
    class PageTableUpdateTokenEmitter : public RMTTokenEmitter
    {
    public:
        PageTableUpdateTokenEmitter(DevDriver::IMsgChannel* msg_channel, DevDriver::AllocCb alloc_cb, ModuleLogger& logger);
        ~PageTableUpdateTokenEmitter();

        DD_RESULT Initialize(DDEventProvider h_provider) override;
        void      Cleanup() override;

        /// @brief Method to enable the token emitter.
        ///
        /// @return DD_RESULT_SUCCESS or an error code if the operation is unsuccessful.
        DD_RESULT Enable() override;

        /// @brief Method to disable the token emitter.
        ///
        /// @return DD_RESULT_SUCCESS or an error code if the operation is unsuccessful.
        DD_RESULT Disable() override;

        DD_RESULT Emit() override;

    private:
        DD_RESULT ProcessAndEmit(const DevDriver::Vector<rmt_ftrace::PageTableUpdateEvent, 128>& ptu_events);

        rmt_ftrace::FTraceContext ftrace_;
        DevDriver::AllocCb        dd_alloc_cb_;
    };

}
