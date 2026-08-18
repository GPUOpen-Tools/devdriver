//=============================================================================
/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief Base class definition for emitting RMT tokens.
//=============================================================================

#pragma once

#include <ModuleLogger.h>
#include <ddCommon.h>
#include <ddEventServer.h>
#include <msgChannel.h>
#include <util/ddEventTimer.h>

namespace SystemTraceModule
{
    /// @brief Enumeration of DevTools events.
    enum DevtoolsRouterEvent
    {
        kUnknown  = 0,  ///< Unknown DevToolsRouter event.
        kRMTToken = 1,  ///< Event emitted when a new RMT token is generated.
        kCount          ///< Total number of DevToolsRouter events.
    };

    /// @brief Base class for processing DevToolsRouter events and emitting corresponding tokens.
    class RMTTokenEmitter
    {
    public:
        /// @brief Creates an RMTTokenEmitter derived object.
        ///
        /// @param [in] msg_channel The message channel.
        /// @param [in] alloc_cb    The memory allocation callback function.
        /// @param [in] logger      The object used to log messages.
        ///
        /// @return A pointer to the token emitter object.
        static RMTTokenEmitter* Create(DevDriver::IMsgChannel* msg_channel, const DevDriver::AllocCb& alloc_cb, ModuleLogger& logger);

        /// @brief Destroys a token emitter that was created using Create().
        /// @param emitter The emitter to destroy.
        /// @param alloc_cb The memory allocation callback function.
        static void Destroy(RMTTokenEmitter* emitter, const DevDriver::AllocCb& alloc_cb);

        /// @brief Constructor for the RMTTokenEmitter class.
        ///
        /// @param [in] msg_channel The message channel.
        /// @param [in] logger      The object used to log messages.
        RMTTokenEmitter(DevDriver::IMsgChannel* msg_channel, ModuleLogger& logger);

        /// @brief Destructor.
        virtual ~RMTTokenEmitter() = default;

        /// @brief Performs the initialization for this emitter.
        /// @param h_provider The object that this emitter should push RMT events to.
        /// @return DD_RESULT_SUCCESS or an error code if the operation is unsuccessful.
        virtual DD_RESULT Initialize(DDEventProvider h_provider) = 0;

        /// @brief Cleanup to be called on application shutdown.
        virtual void Cleanup() = 0;

        /// @brief Pure virtual method to enable the token emitter.
        ///
        /// @return DD_RESULT_SUCCESS or an error code if the operation is unsuccessful.
        virtual DD_RESULT Enable() = 0;

        /// @brief Pure virtual method to disable the token emitter.
        ///
        /// @return DD_RESULT_SUCCESS or an error code if the operation is unsuccessful.
        virtual DD_RESULT Disable() = 0;

        /// @brief Pure virtual method to process pending events and then emit the relevant
        /// tokens.
        ///
        /// @return DD_RESULT_SUCCESS or an error code if the operation is unsuccessful.
        virtual DD_RESULT Emit() = 0;

        /// @brief Resets the timer used to calculate timestamps offsets.
        void ResetTimer();

    protected:
        /// @brief Returns the message channel used by this emitter.
        /// @return The message channel used by this emitter.
        DevDriver::IMsgChannel* GetMsgChannel();

        /// @brief This function calculates the delta since the last timestamp/time delta
        /// token and returns it in out_delta.
        ///
        /// If the delta exceeds the time
        /// delta/timestamp threshold then it will emit one of those tokens and
        /// return zero in out_delta.
        ///
        /// @param out_delta The output for the delta since the last timestamp/time delta token.
        /// @return DD_RESULT_SUCCESS if the calculation is successful.
        DD_RESULT CalculateTimeDelta(DevDriver::uint8* out_delta);

        DDEventProvider h_provider_;  ///< The object where RMT events are pushed to.
        ModuleLogger    logger_;      ///< The object used to log messages.

    private:
        DevDriver::IMsgChannel* msg_channel_;  ///< The message channel.
        DevDriver::EventTimer   event_timer_;  ///< The timer used to create event timestamps.
    };

}
