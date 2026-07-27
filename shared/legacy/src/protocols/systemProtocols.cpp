/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "protocols/systemProtocols.h"

namespace DevDriver
{
    namespace ClientManagementProtocol
    {
        bool IsOutOfBandMessage(const MessageBuffer &message)
        {
            // an out of band message is denoted by both the dstClientId and srcClientId
            // being initialized to kBroadcastClientId.
            static_assert(kBroadcastClientId == 0, "Error, kBroadcastClientId is non-zero. IsOutOfBandMessage needs to be fixed");
            return ((message.header.dstClientId | message.header.srcClientId) == kBroadcastClientId);
        }

        bool IsValidOutOfBandMessage(const MessageBuffer &message)
        {
            // an out of band message is only valid if the sequence field is initialized with the correct version
            // and the protocolId is equal to the receiving client's Protocol::ClientManagement value
            return ((message.header.sequence == kMessageVersion) &&
                    (message.header.protocolId == Protocol::ClientManagement));
        }
    }
}
