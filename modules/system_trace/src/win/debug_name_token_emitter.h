/* Copyright (C) 2021-2024 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief Class definition for emitting RMT debug name tokens.
//=============================================================================

#pragma once

#include <ModuleLogger.h>
#include <rmt_token_emitter.h>

#include "dd_etw_debug_name_server_session.h"
#include "etw_server.h"

namespace SystemTraceModule
{
    /// @brief Handles emitting debug name RMT tokens.
    ///
    /// Also handles mark implicit resource RMT token emitting.
    class DebugNameTokenEmitter : public RMTTokenEmitter
    {
    public:
        /// @brief Constructor for the DebugNameTokenEmitter class.
        ///
        /// @param [in] msg_channel The message channel.
        /// @param [in] logger      The object used to log messages.
        DebugNameTokenEmitter(DevDriver::IMsgChannel* msg_channel, ModuleLogger& logger);

        /// @brief Destructor.
        ~DebugNameTokenEmitter() override = default;

        /// @brief Initializes this emitter by registering the protocol server and setting up the session.
        /// @param h_provider The Dev Driver event provider.
        /// @return DD_RESULT_SUCCESS if the initialization was successful; an error code otherwise.
        DD_RESULT Initialize(DDEventProvider h_provider) override;

        /// @brief Cleanup function to be called on application shutdown.
        void Cleanup() override;

        /// @brief Method to enable the token emitter.
        ///
        /// @return DD_RESULT_SUCCESS or an error code if the operation is unsuccessful.
        DD_RESULT Enable() override;

        /// @brief Method to disable the token emitter.
        ///
        /// @return DD_RESULT_SUCCESS or an error code if the operation is unsuccessful.
        DD_RESULT Disable() override;

        /// @brief No-op.
        /// @retval DD_RESULT_SUCCESS.
        DD_RESULT Emit() override;

        /// @brief Callback function used to emit a DEBUG_NAME USERDATA token to an RMT stream.
        ///
        /// The creation time is the time when the system first detected that an event of this type is needed.
        /// For DirectX, it will be the time that the ETW event was created.
        /// This will account for the time delay between the ETW event being created and the ETW event being captured
        /// by the ETW event parser.
        ///
        /// @param [in] name               The name of the resource.
        /// @param [in] key                The correlation key used to match the name with a resource.
        /// @param [in] instance           The DebugNameTokenEmitter to handle the event for.
        /// @param [in] creation_timestamp The creation time to use for the token.
        ///
        /// @return DD_RESULT_SUCCESS or an error code if the operation is unsuccessful.
        static DD_RESULT HandleDebugNameEvent(const char* name, ULONGLONG key, void* instance, const uint64_t creation_timestamp);

        /// @brief Callback function used to emit a MARK_IMPLICIT_RESOURCE USERDATA token to an RMT stream.
        ///
        /// The creation time is the time when the system first detected that an event of this type is needed.
        /// For DirectX, it will be the time that the ETW event was created.
        /// This will account for the time delay between the ETW event being created and the ETW event being captured
        /// by the ETW event parser.
        ///
        /// @param [in] resource_id        The resource ID (that matches the ID in the RESOURCE_CREATE token).
        /// @param [in] instance           The DebugNameTokenEmitter to handle the event for.
        /// @param [in] creation_timestamp The creation time to use for the token.
        /// @param [in] heap_type          The HeapType from the ETW Resource event.
        ///
        /// @return DD_RESULT_SUCCESS or an error code if the operation is unsuccessful.
        static DD_RESULT HandleImplicitResourceEvent(ULONGLONG resource_id, void* instance, const uint64_t creation_timestamp, const uint8_t heap_type);

    private:
        DevDriver::ETWProtocol::ETWDebugNameSession session_;  ///< The ETW Session object.
        DevDriver::ETWProtocol::ETWServer           server_;   ///< Server that manages ETW sessions.
    };

};

