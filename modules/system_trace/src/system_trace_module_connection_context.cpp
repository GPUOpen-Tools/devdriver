//=============================================================================
/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation for the encapsulation of the profiling module for each client state
//=============================================================================

#include <string>

#include <system_trace_module_connection_context.h>
#include <msgChannel.h>

namespace SystemTraceModule
{
    /// @brief Provider ID for DevTools events.
    static constexpr uint32_t kDevToolsRouterEventProviderId = 0x21777465;

    static constexpr char const* kDevToolsRouterEventProviderName = "DevTools Router Event Provider";

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ModuleConnectionContext::ModuleConnectionContext(const DDModuleConnectionContextCreateInfo& create_info)
        : BaseModuleConnectionContext(create_info)
        , dd_alloc_cb_({&m_createInfo.loader.apiAllocCb, &ddApiAlloc, &ddApiFree})
        , h_provider_(DD_API_INVALID_HANDLE)
        , exit_requested_(false)
        , rmt_token_emitter_(nullptr)
    {
        // Attempt to create an RMT token emitter
        //
        // This function may return nullptr in cases where the underlying platform does not support RMT token generation.
        rmt_token_emitter_ = RMTTokenEmitter::Create(FromHandle(create_info.hConnection), dd_alloc_cb_, m_logger);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ModuleConnectionContext::~ModuleConnectionContext()
    {
        if (rmt_token_emitter_ != nullptr)
        {
            CleanupEventThread();

            ddEventServerDestroyProvider(h_provider_);

            rmt_token_emitter_->Cleanup();
            RMTTokenEmitter::Destroy(rmt_token_emitter_, dd_alloc_cb_);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DD_RESULT ModuleConnectionContext::Initialize()
    {
        DD_RESULT result = DD_RESULT_SUCCESS;

        // Initializing the event provider only makes sense when we have RMT tokens to emit
        if (rmt_token_emitter_ != nullptr)
        {
            DDEventProviderCreateInfo provider_info = {};
            provider_info.hServer                   = m_createInfo.hEventServer;
            provider_info.id                        = kDevToolsRouterEventProviderId;
            provider_info.numEvents                 = kCount;
            DevDriver::Platform::Strncpy(provider_info.name, kDevToolsRouterEventProviderName, sizeof(provider_info.name));

            provider_info.stateChangeCb.pUserdata   = this;
            provider_info.stateChangeCb.pfnEnabled  = [](void* user_data) { reinterpret_cast<ModuleConnectionContext*>(user_data)->OnProviderEnabled(); };
            provider_info.stateChangeCb.pfnDisabled = [](void* user_data) { reinterpret_cast<ModuleConnectionContext*>(user_data)->OnProviderDisabled(); };

            result = ddEventServerCreateProvider(&provider_info, &h_provider_);

            if (result == DD_RESULT_SUCCESS)
            {
                result = rmt_token_emitter_->Initialize(h_provider_);
            }

            if (result != DD_RESULT_SUCCESS)
            {
                // On some platforms, the RMT token emitter will fail to initialize and this is expected.
                // Clean up all resources and log a message, but allow the initialization process to continue as well.

                rmt_token_emitter_->Cleanup();
                RMTTokenEmitter::Destroy(rmt_token_emitter_, dd_alloc_cb_);
                rmt_token_emitter_ = nullptr;

                ddEventServerDestroyProvider(h_provider_);
                h_provider_ = nullptr;

                DD_PRINT(DevDriver::LogLevel::Error,
                         "[ModuleConnectionContext::Initialize] Failed to initialize RMT token emitter: %s.",
                         ddApiResultToString(result));

                result = DD_RESULT_SUCCESS;
            }
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void ModuleConnectionContext::OnProviderEnabled()
    {
        if (rmt_token_emitter_ != nullptr)
        {
            exit_requested_ = false;

            rmt_token_emitter_->Enable();

            DD_UNHANDLED_RESULT(event_thread_.Start([](void* user_data) { reinterpret_cast<ModuleConnectionContext*>(user_data)->ProcessEvents(); }, this));
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void ModuleConnectionContext::OnProviderDisabled()
    {
        if (rmt_token_emitter_ != nullptr)
        {
            rmt_token_emitter_->Disable();

            CleanupEventThread();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void ModuleConnectionContext::ProcessEvents()
    {
        // This thread function should only run when we have a valid rmt token emitter
        DD_ASSERT(rmt_token_emitter_ != nullptr);

        while (!exit_requested_)
        {
            const DD_RESULT result = rmt_token_emitter_->Emit();

            if (result != DD_RESULT_SUCCESS)
            {
                DD_PRINT(DevDriver::LogLevel::Error,
                         "[ModuleConnectionContext::ProcessEvents] Failed to emit RMT token with error: %s.",
                         ddApiResultToString(result));
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void ModuleConnectionContext::CleanupEventThread()
    {
        // We should only be dealing with the event thread when there's a valid rmt token emitter
        DD_ASSERT(rmt_token_emitter_ != nullptr);

        exit_requested_ = true;

        if (event_thread_.IsJoinable())
        {
            DD_UNHANDLED_RESULT(event_thread_.Join(DevDriver::kLogicFailureTimeout));
        }
    }

};
