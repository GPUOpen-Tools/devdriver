/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddPlatform.h>
#include <listener/transports/abstractListenerTransport.h>
#include <deque>
#include <unordered_set>
#include <condition_variable>
#include <unordered_map>
#include <listener/transportThread.h>
#include <msgTransport.h>

namespace DevDriver
{
    class RouterCore;
    class PipeListenerTransport;

    struct PipeInfo
    {
        PipeListenerTransport* pTransport;
        Platform::AtomicLock   lock;
        Platform::Thread       thread;
        volatile bool          active;
        Handle                 pipeHandle;
        Handle                 writeEvent;
        Handle                 readEvent;
        bool                   ioPending;
    };

    class PipeListenerTransport : public IListenerTransport
    {
    public:
        friend void ListeningThreadCallback(void* pUserdata);
        friend void ReceivingThreadCallback(void* pUserdata);

        PipeListenerTransport(const HostInfo& hostInfo);
        ~PipeListenerTransport() override;

        Result ReceiveMessage(ConnectionInfo &connectionInfo, MessageBuffer &message, uint32 timeoutInMs) override;
        Result TransmitMessage(const ConnectionInfo &connectionInfo, const MessageBuffer &message) override;
        Result TransmitBroadcastMessage(const MessageBuffer &message) override;

        Result Enable(RouterCore *pRouter, TransportHandle handle) override;
        Result Disable() override;

        TransportHandle GetHandle() override { return m_transportHandle; };
        bool ForwardingConnection() override { return false; };
        const char* GetTransportName() override { return "Local Pipe"; };
    protected:
        char            m_pipeName[kMaxStringLength];
        TransportHandle m_transportHandle;

        bool m_listening;

        struct
        {
            std::unordered_map<Handle, PipeInfo*> threadMap;
            std::unordered_set<PipeInfo*>         deleteSet;
            Platform::AtomicLock                  lock;
        } m_threadPool;

        PipeInfo        m_listenerThread;
        Platform::Event m_listenerReadyEvent;
        RouterCore*     m_pRouter;

        void   ListeningThreadFunc(PipeInfo* pPipeInfo);
        void   ReceivingThreadFunc(PipeInfo* pPipeInfo);
        Result ProcessDeleteSet();
        Result ProcessThreadMap();
        HANDLE CreateSecureNamedPipe(bool firstPipeInstance);

        DD_STATIC_CONST uint32 kWaitTimeoutInMs = 100;
    };
} // DevDriver
