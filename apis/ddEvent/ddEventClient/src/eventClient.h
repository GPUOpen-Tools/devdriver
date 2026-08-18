/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddEventClient.h>
#include <legacy/legacyEventClient.h>

namespace Event
{

class EventClient
{
public:
    EventClient(DDNetConnection hConnection, const DDEventDataCallback& dataCb);
    ~EventClient();

    DD_RESULT Connect(
        DDClientId clientId);

    DD_RESULT ReadEventData(
        uint32_t timeoutInMs);

    DD_RESULT EnableProviders(
        size_t          numProviderIds,
        const uint32_t* pProviderIds);

    DD_RESULT DisableProviders(
        size_t          numProviderIds,
        const uint32_t* pProviderIds);

    DD_RESULT SubscribeToProvider(uint32_t providerId);

private:
    DD_RESULT BulkUpdateProviders(
        size_t          numProviderIds,
        const uint32_t* pProviderIds,
        bool            enabled);

    void ReceiveEventData(const void* pData, size_t dataSize);

    DevDriver::EventProtocol::EventClient m_legacyClient;
    DDEventDataCallback                   m_dataCb;
    uint8_t                               m_eventProviderVersion;
};

} // namespace Event
