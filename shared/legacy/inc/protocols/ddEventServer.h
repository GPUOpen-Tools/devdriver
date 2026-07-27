/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <baseProtocolServer.h>
#include <util/hashMap.h>
#include <util/vector.h>
#include <util/queue.h>
#include <protocols/ddEventProtocol.h>

namespace DevDriver
{

namespace EventProtocol
{

class BaseEventProvider;
struct EventChunk;
class EventServerSession;

constexpr size_t kEventProviderMaxNameLen = 256;

struct EventProviderInfo
{
    EventProviderId id;
    char            name[kEventProviderMaxNameLen];
    bool            enabled;
    bool            registered;
};

class EventServer final : public BaseProtocolServer
{
    friend class BaseEventProvider;
    friend class EventServerSession;

public:
    using SessionMapIterator = HashMap<EventProviderId, BaseEventProvider*, 16u>::Iterator;

public:
    explicit EventServer(IMsgChannel* pMsgChannel);
    ~EventServer();
    EventServer(const EventServer&) = delete;
    EventServer(EventServer&&) = delete;
    EventServer& operator=(const EventServer&) = delete;
    EventServer& operator=(EventServer&&) = delete;

    bool AcceptSession(const SharedPointer<ISession>& pSession) override;
    void SessionEstablished(const SharedPointer<ISession>& pSession) override;
    void UpdateSession(const SharedPointer<ISession>& pSession) override;
    void SessionTerminated(const SharedPointer<ISession>& pSession, Result terminationReason) override;

    Result RegisterProvider(BaseEventProvider* pProvider);
    Result UnregisterProvider(BaseEventProvider* pProvider);

private:
    struct PendingConnection
    {
        DevDriver::SharedPointer<DevDriver::ISession> pSession;
    };

    Result BuildQueryProvidersResponse(BlockId* pBlockId);
    Result ApplyProviderUpdate(const ProviderUpdateHeader* pUpdate);
    Result AssignSessionToProvider(EventServerSession* pEventSession, EventProviderId providerId);
    void   UnassignSessionFromProvider(EventServerSession* pEventSession, EventProviderId providerId);

    Vector<EventServerSession*, 16u>::Iterator FindPendingSessionById(SessionId id);
    SessionMapIterator FindProviderBySessionId(SessionId sessionId);

    HashMap<EventProviderId, BaseEventProvider*, 16u> m_eventProviders;
    Vector<EventServerSession*, 16u>                  m_pendingSessions;
    Platform::AtomicLock                              m_updateMutex;
};

} // EventProtocol
} // DevDriver
