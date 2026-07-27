/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <protocols/ddEventProtocol.h>
#include <protocols/ddEventServer.h>
#include <ddTransferManager.h>

namespace DevDriver
{
namespace EventProtocol
{

enum class SessionState
{
    ReceivePayload = 0,
    ProcessPayload,
    SendPayload,
};

class EventServerSession
{
public:
    EventServerSession(
        const AllocCb& allocCb,
        SharedPointer<ISession> pSession,
        EventServer* pServer,
        TransferProtocol::TransferManager* pTransferManager);

    EventServerSession(const EventServerSession&)            = delete;
    EventServerSession& operator=(const EventServerSession&) = delete;

    ~EventServerSession();

    void UpdateSession();

    SessionId GetSessionId();

    Result AllocateEventChunk(EventChunk** ppChunk);
    void FreeEventChunk(EventChunk* pChunk);
    void EnqueueEventChunks(size_t numChunks, EventChunk** ppChunks);

    void SetProviderId(EventProviderId providerId);

private:
    struct EventChunkInfo
    {
        EventChunk* pChunk;
        size_t      bytesSent;
    };

    // Protocol message handlers
    SessionState HandleQueryProvidersRequest(SizedPayloadContainer& container);
    SessionState HandleAllocateProviderUpdatesRequest(SizedPayloadContainer& container);
    SessionState HandleApplyProviderUpdatesRequest(SizedPayloadContainer& container);
    SessionState HandleSubscribeToProviderRequest(SizedPayloadContainer& container);
    SessionState HandleUnsubscribeFromProviderRequest();

    EventChunk* DequeueEventChunk();
    bool IsTargetMemoryUsageExceeded() const;
    void TrimEventChunkMemory();

    void SendEventData();

    EventServer*                                 m_pServer;
    SharedPointer<ISession>                      m_pSession;
    AllocCb                                      m_allocCb;
    SizedPayloadContainer                        m_payloadContainer;
    SessionState                                 m_state;
    TransferProtocol::TransferManager*           m_pTransferManager;
    SharedPointer<TransferProtocol::ServerBlock> m_pUpdateBlock;
    SizedPayloadContainer                        m_eventPayloadContainer;
    bool                                         m_eventPayloadPending;
    EventChunkInfo                               m_eventChunkInfo;
    EventProviderId                              m_assignedProviderId;

    Platform::AtomicLock                         m_eventPoolMutex;
    Vector<EventChunk*>                          m_eventChunkPool;
    Platform::AtomicLock                         m_eventQueueMutex;
    Vector<EventChunk*>                          m_eventChunkQueue;
    uint64                                       m_nextTrimTime;
};

} // namespace EventProtocol
} // namespace DevDriver
