/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "protocolSession.h"

namespace DevDriver
{
    Result ISession::SendPayload(const SizedPayloadContainer& payload, uint32 timeoutInMs)
    {
        return Send(payload.payloadSize, payload.payload, timeoutInMs);
    }

    Result ISession::ReceivePayload(SizedPayloadContainer* pPayload, uint32 timeoutInMs)
    {
        DD_ASSERT(pPayload != nullptr);
        return Receive(sizeof(pPayload->payload), pPayload->payload, &pPayload->payloadSize, timeoutInMs);
    }

    ISession::ISession()
    {
    }
} // DevDriver
