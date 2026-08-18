//=============================================================================
/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief Class definitions for the encapsulation of the profiling module for each client state
//=============================================================================

#pragma once

#include <BaseModuleConnectionContext.h>
#include <ddCommon.h>
#include <ddEventServer.h>
#include <gpuopen.h>

#include <rmt_token_emitter.h>

namespace SystemTraceModule
{
    /// @brief Class used to encapsulate the profiling module specific per client state
    class ModuleConnectionContext : public BaseModuleConnectionContext
    {
    public:
        explicit ModuleConnectionContext(const DDModuleConnectionContextCreateInfo& create_info);

        /// @brief Destructor.
        ~ModuleConnectionContext() override;

        /// @brief Helper function to unwrap the ModuleConnectionContext.
        /// @param context The context handle for which the associated ModuleConnectionContext should be returned.
        /// @return The ModuleConnectionContext associated with the handle.
        static ModuleConnectionContext* HandleToPtr(DDModuleConnectionContext context)
        {
            DD_ASSERT(context != nullptr);
            return reinterpret_cast<ModuleConnectionContext*>(context);
        }

        /// @brief Provides the allocator used for this connection context.
        /// @return The allocator used for this connection context.
        const DevDriver::AllocCb& GetAllocCb() const
        {
            return dd_alloc_cb_;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /////////////// Base Class Overrides ///////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Initializes this context.
        /// @return DD_RESULT_SUCCESS if the context was initialized successfully.
        DD_RESULT Initialize() override;

    private:
        DevDriver::AllocCb dd_alloc_cb_;  ///< The allocator callback used for this context.

        /// @brief Called when the event server provider is enabled.
        void OnProviderEnabled();

        /// @brief Called when the event server provider is enabled.
        void OnProviderDisabled();

        void ProcessEvents();

        /// @brief Stops the event processing loop, then joins the event processing thread.
        void CleanupEventThread();

        DDEventProvider             h_provider_;         ///< The event provider for the RMT token emitter.
        DevDriver::Platform::Thread event_thread_;       ///< A thread running a loop to process router events and emit RMT tokens.
        bool                        exit_requested_;     ///< Starts as false, but will stop the event processing loop when set to true.
        RMTTokenEmitter*            rmt_token_emitter_;  ///< Object used to emit RMT tokens.
    };

};
