//=============================================================================
/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation for server that manages ETW sessions.
//=============================================================================

#include "etw_server.h"

#include <thread>

#include "ddPlatform.h"
#include "msgChannel.h"
#include "util/queue.h"
#include "util/vector.h"

#include "dd_win_etw_server_session.h"

namespace DevDriver
{
    namespace ETWProtocol
    {
        ETWServer::ETWServer(IMsgChannel* msg_channel)
            : BaseProtocolServer(msg_channel, Protocol::ETW, kVersion, kVersion)
        {
            DD_ASSERT(m_pMsgChannel != nullptr);
        }

        void ETWServer::Finalize()
        {
        }

        bool ETWServer::AcceptSession(const SharedPointer<ISession>& session)
        {
            DD_UNUSED(session);
            return true;
        }

        void ETWServer::SessionEstablished(const SharedPointer<ISession>& session)
        {
            // Allocate session data for the newly established session
            ETWSession* session_object = DD_NEW(ETWSession, m_pMsgChannel->GetAllocCb())(session, m_pMsgChannel->GetAllocCb());
            if (session_object != nullptr)
            {
                // Starting with RS5, we need to constantly listen for ETW events for AssociateContext events.
                DD_UNHANDLED_RESULT(session_object->BeginTrace(kAssocationContextProcessId));
                session->SetUserData(session_object);
            }
            else
            {
                DD_ASSERT_REASON("Out of memory - DD_NEW returned NULL.");
            }
        }

        void ETWServer::UpdateSession(const SharedPointer<ISession>& session)
        {
            ETWSession* session_object = reinterpret_cast<ETWSession*>(session->GetUserData());
            if (session_object != nullptr)
            {
                session_object->UpdateSession();
            }
        }

        void ETWServer::SessionTerminated(const SharedPointer<ISession>& session, Result termination_reason)
        {
            DD_UNUSED(termination_reason);
            ETWSession* session_object = reinterpret_cast<ETWSession*>(session->SetUserData(nullptr));

            if (session_object != nullptr)
            {
                DD_UNHANDLED_RESULT(session_object->EndTrace());
                DD_DELETE(session_object, m_pMsgChannel->GetAllocCb());
            }
        }

        bool ETWServer::QueryETWSupported()
        {
            return TraceSession::QueryETWSupport();
        }
    }  // namespace ETWProtocol
}  // namespace DevDriver
