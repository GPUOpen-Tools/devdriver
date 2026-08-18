//=============================================================================
/* Copyright (C) 2017-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief  Class declarations for server that manages ETW sessions.
//=============================================================================

#pragma once

#include "baseProtocolServer.h"
#include "protocols/etwProtocol.h"

namespace DevDriver
{
    namespace ETWProtocol
    {
        /// @brief The protocol server implementation for the ETW protocol.
        class ETWServer : public BaseProtocolServer
        {
        public:
            /// @brief Constructor.
            /// @param msg_channel A message channel whose allocator will be used to allocate ETW sessions.
            explicit ETWServer(IMsgChannel* msg_channel);

            /// @brief Destructor.
            ~ETWServer() override = default;

            /// @brief No-op.
            void Finalize() override;

            /// @brief Determines whether or not a session should be established.
            /// @param session The session to accept.
            /// @return true if the session should be accepted, false otherwise.
            bool AcceptSession(const SharedPointer<ISession>& session) override;

            /// @brief Creates a new ETW session, associates it with the provided ISession
            /// and begins tracing.
            ///
            /// @param session The session that was established.
            void SessionEstablished(const SharedPointer<ISession>& session) override;

            /// @brief Updates the session's associated ETW session.
            /// @param session The session to update.
            void UpdateSession(const SharedPointer<ISession>& session) override;

            /// @brief Ends the ETW trace for the session then deletes the associated ETW session.
            /// @param session The session to terminate.
            /// @param termination_reason The reason why the session was terminated (unused).
            void SessionTerminated(const SharedPointer<ISession>& session, Result termination_reason) override;

            /// @brief Returns true if ETW is supported on the system.
            /// @return true if ETW is supported on the system, false otherwise.
            static bool QueryETWSupported();

        private:
        };
    }  // namespace ETWProtocol
}  // namespace DevDriver
