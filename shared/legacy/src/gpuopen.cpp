/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "gpuopen.h"

namespace DevDriver
{

ClientMetadata::ClientMetadata(uint64 value)
{
    // If we're going to alias as a 64-bit value, make sure the struct is still just 64-bits)
    static_assert(sizeof(uint64) == sizeof(ClientMetadata),
                  "Size of ClientMetadata is no longer 64-bits, alias constructor needs updating");

    // Bits 0-15 are the ProtocolFlags
    protocols.value = static_cast<uint32>(value & 0xFFFF);

    // Bits 32-39 are the Component
    clientType = static_cast<Component>((value & 0xFF00000000) >> 32);

    // Bits 40-47 are reserved, ignore them and zero initialize
    reserved = 0;

    // Bits 48-63 are the StatusFlags
    status = static_cast<StatusFlags>((value & 0xFFFF000000000000) >> 48);
}

bool ClientMetadata::Matches(const ClientMetadata& right) const
{
    bool result = true;

    // The Matches function treats this struct as a filter, so a ClientMetadata with all default (zero) values
    // by definition always matches.
    if (IsDefault() == false)
    {
        // Component is an enum, so the comparison needs to be equality
        const bool clientTypeMatches =
            (clientType != Component::Unknown)
            ? (clientType == right.clientType)
            : true;

        // ProtocolFlags is a bit field, so we can do a bitwise comparison
        const bool protocolMatches =
            (protocols.value != 0)
            ? (protocols.value & right.protocols.value) == protocols.value
            : true;
        // StatusFlags is a bit field, so we can do a bitwise comparison
        const bool statusMatches =
            (status != 0)
            ? (status & right.status) == status
            : true;
        result = clientTypeMatches & protocolMatches & statusMatches;
    }

    return result;
}

bool ClientMetadata::MatchesAny(const ClientMetadata& right) const
{
    bool result = true;

    // The MatchesAny function treats this struct as a filter, so a ClientMetadata with all default (zero) values
    // by definition always matches.
    if (IsDefault() == false)
    {
        // Component is an enum, so the comparison needs to be equality
        const bool clientTypeMatches = (clientType == right.clientType);
        // ProtocolFlags is a bit field, so we can do a bitwise comparison
        const bool protocolMatches = (protocols.value & right.protocols.value) != 0;
        // StatusFlags is a bit field, so we can do a bitwise comparison
        const bool statusMatches = (status & right.status) != 0;
        result = clientTypeMatches | protocolMatches | statusMatches;
    }

    return result;
}

Result ValidateMessageBuffer(const void* pMsgBuffer, size_t msgBufferSize)
{
    Result result = Result::Error;

    // Ensure that we've been passed valid parameters
    if ((pMsgBuffer != nullptr) && (msgBufferSize > 0))
    {
        // A valid message buffer must be no larger than the full size message buffer structure
        // and it must also be large enough to contain a valid header.
        if ((msgBufferSize <= sizeof(MessageBuffer)) && (msgBufferSize >= sizeof(MessageHeader)))
        {
            // Calculate the total size of the message from the data encoded in the buffer.
            const MessageHeader* pHeader = reinterpret_cast<const MessageHeader*>(pMsgBuffer);
            const size_t encodedMessageSize = (sizeof(MessageHeader) + pHeader->payloadSize);

            // The encoded message size should match our expected size exactly
            if (encodedMessageSize == msgBufferSize)
            {
                result = Result::Success;
            }
        }
    }
    else
    {
        result = Result::InvalidParameter;
    }

    return result;
}

} // namespace DevDriver
