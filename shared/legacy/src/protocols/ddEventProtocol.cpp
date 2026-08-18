/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <protocols/ddEventProtocol.h>

namespace DevDriver
{
namespace EventProtocol
{
    // ============================================================================================================
    QueryProvidersResponseHeader::QueryProvidersResponseHeader(uint32 numProviders)
        : numProviders(numProviders)
    {
    }

    // ============================================================================================================
    ProviderDescriptionHeader::ProviderDescriptionHeader(
        uint32 providerId,
        uint32 numEvents,
        uint32 eventDescriptionDataSize,
        bool isEnabled,
        uint8_t version)
        : providerId(providerId)
        , numEvents(numEvents)
        , eventDescriptionDataSize(eventDescriptionDataSize)
        , isEnabled(isEnabled)
        , version(version)
    {
    }

    // ============================================================================================================
    size_t ProviderDescriptionHeader::GetNextProviderDescriptionOffset() const
    {
        return (GetEventDescriptionOffset() + eventDescriptionDataSize);
    }

    // ============================================================================================================
    ProviderUpdateHeader::ProviderUpdateHeader(uint32 providerId, uint32 eventDataSize, bool isEnabled)
        : providerId(providerId)
        , eventDataSize(eventDataSize)
        , isEnabled(isEnabled)
    {
    }

    // ============================================================================================================
    size_t ProviderUpdateHeader::GetNextProviderUpdateOffset() const
    {
        return (GetEventDataOffset() + eventDataSize);
    }

    // ============================================================================================================
    QueryProvidersRequestPayload::QueryProvidersRequestPayload()
        : header(EventMessage::QueryProvidersRequest)
    {
    }

    // ============================================================================================================
    AllocateProviderUpdatesRequest::AllocateProviderUpdatesRequest(uint32 dataSize)
        : header(EventMessage::AllocateProviderUpdatesRequest)
        , dataSize(dataSize)
    {
    }

    // ============================================================================================================
    ApplyProviderUpdatesRequest::ApplyProviderUpdatesRequest()
        : header(EventMessage::ApplyProviderUpdatesRequest)
    {
    }

    void* EventDataUpdatePayload::GetEventDataBuffer()
    {
        return eventData;
    }

    const void* EventDataUpdatePayload::GetEventDataBuffer() const
    {
        return eventData;
    }

    size_t EventDataUpdatePayload::GetEventDataBufferSize() const
    {
        return sizeof(eventData);
    }

    size_t EventDataUpdatePayload::GetEventDataSize() const
    {
        return static_cast<size_t>(header.eventDataSize);
    }

    void EventDataUpdatePayload::SetEventDataSize(uint16 eventDataSize)
    {
        header.eventDataSize = eventDataSize;
    }

    SubscribeToProviderRequest::SubscribeToProviderRequest(EventProviderId id)
        : header(EventMessage::SubscribeToProviderRequest)
        , providerId(id)
    {
    }

    UnsubscribeFromProviderRequest::UnsubscribeFromProviderRequest()
        : header(EventMessage::UnsubscribeFromProviderRequest)
    {
    }
} // namespace EventProtocol
} // namespace DevDriver
