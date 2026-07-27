/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <Windows.h>
#include <sddl.h>

#include <listener/transports/winPipeTransport.h>
#include <listener/routerCore.h>
#include <win/ddWinPipeUtil.h>

namespace DevDriver
{
    static constexpr size_t kReceiveBufferSize = sizeof(MessageBuffer) * 8;
    static constexpr size_t kSendBufferSize    = sizeof(MessageBuffer) * 8;

    static Result WaitOverlapped(HANDLE hPipe, OVERLAPPED* pOverlapped, DWORD *pBytesTransferred, DWORD waitTimeMs)
    {
        DWORD waitResult = WAIT_OBJECT_0;
        Result result = Result::NotReady;

        if (waitTimeMs > 0)
            waitResult = WaitForSingleObject(pOverlapped->hEvent, waitTimeMs);

        if (waitResult == WAIT_OBJECT_0)
        {
            if (GetOverlappedResult(hPipe, pOverlapped, pBytesTransferred, FALSE))
            {
                result = Result::Success;
            }
            else
            {
                const DWORD errorCode = GetLastError();
                if (errorCode == ERROR_IO_INCOMPLETE)
                {
                    // Keep the result set to NotReady
                }
                else
                {
                    LogPipeError(errorCode);

                    if (errorCode == ERROR_OPERATION_ABORTED)
                    {
                        // This can happen when a read operation is queued from one thread and then accessed from a new one.
                        // We return Aborted to inform the calling code about this situation.
                        // Some documentation about ERROR_OPERATION_ABORTED can be found here:
                        // https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/FileIO/canceling-pending-i-o-operations.md
                        result = Result::Aborted;
                    }
                    else
                    {
                        result = Result::Error;
                    }
                }
            }
        }
        else if (waitResult != WAIT_TIMEOUT)
        {
            result = Result::Error;
        }
        return result;
    }

    void SetDebugPriviledges(bool enabled)
    {
        HANDLE              hToken;
        LUID                SeDebugNameValue;
        TOKEN_PRIVILEGES    TokenPrivileges;

        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        {
            if (LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &SeDebugNameValue))
            {
                TokenPrivileges.PrivilegeCount = 1;
                TokenPrivileges.Privileges[0].Luid = SeDebugNameValue;
                TokenPrivileges.Privileges[0].Attributes = enabled ? SE_PRIVILEGE_ENABLED : 0;

                if (AdjustTokenPrivileges(hToken, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr))
                {
                    CloseHandle(hToken);
                }
                else
                {
                    CloseHandle(hToken);
                    DD_PRINT(LogLevel::Error, "Couldn't adjust token privileges!");
                }
            }
            else
            {
                CloseHandle(hToken);
                DD_PRINT(LogLevel::Error, "Couldn't look up privilege value!");
            }
        }
        else
        {
            DD_PRINT(LogLevel::Error, "Couldn't open process token!");
        }
    }

    Result ReadMessage(PipeInfo &threadInfo, OVERLAPPED &oOverlap, MessageContext &messageContext, uint32 timeoutInMs)
    {
        Result result = Result::Error;
        HANDLE hPipe = reinterpret_cast<HANDLE>(threadInfo.pipeHandle);
        DWORD receivedSize = 0;

        if (!threadInfo.ioPending)
        {
            if (ReadFile(hPipe, &messageContext.message, sizeof(MessageBuffer), &receivedSize, &oOverlap))
            {
                result = Result::Success;
            }
            else
            {
                const DWORD errorCode = GetLastError();
                if (errorCode == ERROR_IO_PENDING)
                {
                    threadInfo.ioPending = true;
                }
                else
                {
                    LogPipeError(errorCode);
                }
            }
        }

        if (threadInfo.ioPending)
        {
            result = WaitOverlapped(hPipe, &oOverlap, &receivedSize, timeoutInMs);

            if (result == Result::Aborted)
            {
                threadInfo.ioPending = false;

                result = Result::NotReady;
            }
        }

        if (result == Result::Success)
        {
            threadInfo.ioPending = false;

            result = ValidateMessageBuffer(&messageContext.message, receivedSize);
        }
        else if (result != Result::NotReady)
        {
            threadInfo.ioPending = false;
            result = Result::Error;
        }

        return result;
    }

    void ListeningThreadCallback(void* pUserdata)
    {
        DD_ASSERT(pUserdata != nullptr);

        PipeInfo* pPipeInfo = reinterpret_cast<PipeInfo*>(pUserdata);
        pPipeInfo->pTransport->ListeningThreadFunc(pPipeInfo);
    }

    void ReceivingThreadCallback(void* pUserdata)
    {
        DD_ASSERT(pUserdata != nullptr);

        PipeInfo* pPipeInfo = reinterpret_cast<PipeInfo*>(pUserdata);
        pPipeInfo->pTransport->ReceivingThreadFunc(pPipeInfo);
    }

    void PipeListenerTransport::ListeningThreadFunc(PipeInfo* pPipeInfo)
    {
        DD_ASSERT(pPipeInfo != nullptr);

        OVERLAPPED oOverlap = {};
        oOverlap.Offset = 0;
        oOverlap.OffsetHigh = 0;
        oOverlap.hEvent = CreateEvent(
            nullptr,    // default security attribute
            TRUE,    // manual-reset event
            FALSE,    // initial state = unsignaled
            nullptr);   // unnamed event object

        while (pPipeInfo->active)
        {
            // Wait for the client to connect; if it succeeds,
            // the function returns a nonzero value. If the function
            // returns zero, GetLastError returns ERROR_PIPE_CONNECTED.

            HANDLE hPipe = CreateSecureNamedPipe(false);

            if (hPipe == INVALID_HANDLE_VALUE)
            {
                DD_PRINT(LogLevel::Error, "[winPipeTransport] CreateNamedPipe failed, GLE=%d.", GetLastError());
                return;
            }

            Result result = (ConnectNamedPipe(hPipe, &oOverlap) != FALSE) ? Result::Success : Result::Error;

            // Signal the listener ready event since our listen pipe is now available for connections.
            m_listenerReadyEvent.Signal();

            if (result != Result::Success)
            {
                DWORD dwErr = GetLastError();
                if (dwErr == ERROR_IO_PENDING)
                {
                    DD_PRINT(LogLevel::Verbose, "[winPipeTransport] Waiting for new client");

                    DWORD numBytes = 0;

                    result = Result::NotReady;

                    while ((pPipeInfo->active) & (result == Result::NotReady))
                    {
                        result = WaitOverlapped(hPipe, &oOverlap, &numBytes, kWaitTimeoutInMs);

                        // If we have no new clients to process, attempt to process the delete set.
                        if (result == Result::NotReady)
                        {
                            Platform::LockGuard<Platform::AtomicLock> lock(m_threadPool.lock);
                            const Result deleteResult = ProcessDeleteSet();
                            if (deleteResult != Result::Success)
                            {
                                DD_PRINT(LogLevel::Error, "Failed to process delete set!");
                            }
                        }
                    }
                }
                else if (dwErr == ERROR_PIPE_CONNECTED)
                {
                    result = Result::Success;
                }
            }

            if (result == Result::Success)
            {
                DD_PRINT(LogLevel::Info, "[winPipeTransport] New client connected, starting new thread");
                PipeInfo* pNewThread = new PipeInfo();
                pNewThread->pipeHandle = DD_PTR_TO_HANDLE(hPipe);
                pNewThread->active = true;

                pNewThread->writeEvent = DD_PTR_TO_HANDLE(CreateEvent(
                    nullptr,    // default security attribute
                    TRUE,    // manual-reset event
                    FALSE,    // initial state = unsignaled
                    nullptr));   // unnamed event object

                pNewThread->readEvent = DD_PTR_TO_HANDLE(CreateEvent(
                    nullptr,    // default security attribute
                    TRUE,    // manual-reset event
                    FALSE,    // initial state = unsignaled
                    nullptr));   // unnamed event object

                pNewThread->pTransport = this;
                result = pNewThread->thread.Start(&ReceivingThreadCallback, pNewThread);

                // This is for humans, so we ignore a failure to set the name. The code can't do anything about it anyway.
                pNewThread->thread.SetName("DevDriver WinPipe Receiver");

                if (result != Result::Success)
                {
                    DD_PRINT(LogLevel::Error, "[winPipeTransport] Thread creation failed!");
                    DisconnectNamedPipe(hPipe);
                    CloseHandle(hPipe);
                    delete pNewThread;
                }
            }
            else
            {
                if (result == Result::Error)
                {
                    DD_PRINT(LogLevel::Error, "[winPipeTransport] Connection failed!");
                }
                CloseHandle(hPipe);
            }
                // The client could not connect, so close the pipe.
        }

        // Clean up any child threads before exiting.

        // Start the shutdown process for all active threads
        for (auto &pair : m_threadPool.threadMap)
        {
            DD_ASSERT(pair.second->active);

            pair.second->active = false;
        }

        // Process all remaining threads.
        Platform::LockGuard<Platform::AtomicLock> lock(m_threadPool.lock);
        Result result = ProcessThreadMap();
        if (result == Result::Success)
        {
            result = ProcessDeleteSet();
            if (result != Result::Success)
            {
                DD_PRINT(LogLevel::Error, "Failed to process delete set!");
            }
        }
        else
        {
            DD_PRINT(LogLevel::Error, "Failed to process thread map!");
        }
    }

    void PipeListenerTransport::ReceivingThreadFunc(PipeInfo* pPipeInfo)
    {
        if ((pPipeInfo == nullptr) | (m_pRouter == nullptr))
        {
            DD_PRINT(LogLevel::Error, "ERROR - Pipe Server Failure");
            return;
        }
        else
        {
            Platform::LockGuard<Platform::AtomicLock> lock(m_threadPool.lock);
            m_threadPool.threadMap.emplace(pPipeInfo->pipeHandle, pPipeInfo);
        }

        DD_PRINT(LogLevel::Info, "[winPipeTransport] New client thread started");

        OVERLAPPED oOverlap = {};
        oOverlap.Offset = 0;
        oOverlap.OffsetHigh = 0;
        oOverlap.hEvent = reinterpret_cast<HANDLE>(pPipeInfo->readEvent);

        MessageContext recvContext = {};
        recvContext.connectionInfo.handle = m_transportHandle;
        recvContext.connectionInfo.size = sizeof(HANDLE);
        memcpy_s(&recvContext.connectionInfo.data[0], sizeof(recvContext.connectionInfo.data), &pPipeInfo->pipeHandle, sizeof(HANDLE));

        RoutingCache cache(m_pRouter);
        std::deque<MessageContext> recvQueue;
        std::deque<MessageContext> retryQueue;

        // Loop until done reading
        while (pPipeInfo->active)
        {
            size_t firstNewMessageIndex = recvQueue.size();
            DD_STATIC_CONST uint32 kReceiveDelayInMs = 10;

            // Check for new local messages.
            Result result = ReadMessage(*pPipeInfo, oOverlap, recvContext, kReceiveDelayInMs);
            while (result == Result::Success)
            {
                recvQueue.emplace_back(recvContext);
                result = ReadMessage(*pPipeInfo, oOverlap, recvContext, kNoWait);
            }

            size_t messageNumber = 0;
            for (const auto &message : recvQueue)
            {
                messageNumber++;
                // only requeue messages if it's the first time we've tried to send them
                if ((cache.RouteMessage(message) == Result::NotReady) & (messageNumber > firstNewMessageIndex))
                {
                    retryQueue.emplace_back(message);
                }
            }
            recvQueue.clear();
            recvQueue.swap(retryQueue);

            if (result == Result::Error)
            {
                Platform::LockGuard<Platform::AtomicLock> lock(m_threadPool.lock);
                pPipeInfo->active = false;
                if (m_threadPool.threadMap.erase(pPipeInfo->pipeHandle) > 0)
                {
                    m_threadPool.deleteSet.emplace(pPipeInfo);
                }
            }
        }

        // The loop can exit (pPipeInfo->active flipped to false by another thread, or a read error)
        // while an overlapped ReadFile issued into oOverlap / recvContext is still pending in the
        // kernel. oOverlap and recvContext are stack locals of this function: if we returned now,
        // the stack would be reclaimed while the kernel still owns those addresses, and a later
        // completion (e.g. the client disconnecting during teardown) would write the I/O status
        // (STATUS_PIPE_DISCONNECTED) and byte count into whatever reused that stack memory - a
        // classic escaped-OVERLAPPED use-after-free. Cancel the pending read and drain it to
        // terminal completion so the kernel is finished with oOverlap/recvContext before we return.
        // pPipeInfo->pipeHandle is still valid here (the PipeInfo is only freed later, by the
        // listener thread in ProcessThreadMap/ProcessDeleteSet after this thread is joined).
        if (pPipeInfo->ioPending)
        {
            HANDLE hPipe = reinterpret_cast<HANDLE>(pPipeInfo->pipeHandle);

            // CancelIoEx is asynchronous: success only means cancellation was requested, not that
            // the kernel is done with oOverlap/recvContext. ERROR_NOT_FOUND is benign (the read
            // already completed and left the cancelable set); any other failure means our
            // handle/ownership assumptions are broken.
            if (!CancelIoEx(hPipe, &oOverlap))
            {
                const DWORD cancelError = GetLastError();
                if (cancelError != ERROR_NOT_FOUND)
                {
                    DD_PRINT(LogLevel::Error, "[winPipeTransport] CancelIoEx failed during drain, GLE=%d.", cancelError);
                }
            }

            // Drain to terminal completion before returning, so the kernel has finished writing to
            // oOverlap/recvContext (both stack locals) before this frame unwinds. bWait=TRUE blocks
            // on the exclusively owned manual-reset event until the operation reaches terminal
            // completion, so a single call suffices: success, ERROR_OPERATION_ABORTED (cancelled),
            // or a terminal pipe error (disconnected/broken) all mean the kernel is done with
            // oOverlap/recvContext. ERROR_IO_INCOMPLETE cannot occur under a blocking wait.
            DWORD bytesTransferred = 0;
            GetOverlappedResult(hPipe, &oOverlap, &bytesTransferred, TRUE);
            pPipeInfo->ioPending = false;
        }
    }

    Result PipeListenerTransport::ProcessDeleteSet()
    {
        Result result = Result::Success;

        // Attempt to stop all the connection threads in the delete set.
        while (m_threadPool.deleteSet.empty() == false)
        {
            auto pipeIter = m_threadPool.deleteSet.begin();
            PipeInfo* pPipeInfo = *pipeIter;

            // The thread should be inactive when we attempt to stop it.
            DD_ASSERT(pPipeInfo->active == false);

            if (pPipeInfo->thread.IsJoinable())
            {
                result = pPipeInfo->thread.Join(kLogicFailureTimeout);
            }

            // If the thread has been stopped successfully, then free its resources.
            if (result == Result::Success)
            {
                DisconnectNamedPipe(reinterpret_cast<HANDLE>(pPipeInfo->pipeHandle));
                CloseHandle(reinterpret_cast<HANDLE>(pPipeInfo->pipeHandle));
                CloseHandle(reinterpret_cast<HANDLE>(pPipeInfo->readEvent));
                CloseHandle(reinterpret_cast<HANDLE>(pPipeInfo->writeEvent));
                delete pPipeInfo;

                m_threadPool.deleteSet.erase(pipeIter);
            }
            else
            {
                DD_WARN_REASON("Failed to join listener connection thread!");
                break;
            }
        }

        return result;
    }

    Result PipeListenerTransport::ProcessThreadMap()
    {
        Result result = Result::Success;

        // Attempt to stop all the connection threads in the thread map.
        while (m_threadPool.threadMap.empty() == false)
        {
            auto pipeIter = m_threadPool.threadMap.begin();
            PipeInfo* pPipeInfo = pipeIter->second;

            // The thread should have already been moved to the inactive state before this function is called.
            DD_ASSERT(pPipeInfo->active == false);

            if (pPipeInfo->thread.IsJoinable())
            {
                result = pPipeInfo->thread.Join(kLogicFailureTimeout);
            }

            // If the thread has been stopped successfully, then free its resources.
            if (result == Result::Success)
            {
                DisconnectNamedPipe(reinterpret_cast<HANDLE>(pPipeInfo->pipeHandle));
                CloseHandle(reinterpret_cast<HANDLE>(pPipeInfo->pipeHandle));
                CloseHandle(reinterpret_cast<HANDLE>(pPipeInfo->readEvent));
                CloseHandle(reinterpret_cast<HANDLE>(pPipeInfo->writeEvent));
                delete pPipeInfo;

                m_threadPool.threadMap.erase(pipeIter);
            }
            else
            {
                DD_WARN_REASON("Failed to join listener connection thread!");
                break;
            }
        }

        return result;
    }

    HANDLE PipeListenerTransport::CreateSecureNamedPipe(bool firstPipeInstance)
    {
        DWORD openFlags = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED; // Can read/write and uses overlapped i/o

        if (firstPipeInstance)
        {
            openFlags |= FILE_FLAG_FIRST_PIPE_INSTANCE; // Ensure this is the ONLY instance of this pipe
                                                        // at creation time since the router expects to
                                                        // have exclusive access to it.
        }

        SECURITY_ATTRIBUTES  sa = {};

#ifdef DD_AUTHENTICATED_USER_SECURITY
        // Create a security descriptor that allows all users to access the pipe.
        // Here is the breakdown of the SDDL string:
        // "D"   Indicates that this is a Discretionary Access Control List (DACL)
        // "A"   Specifies that this is an Allow ACE, meaning it grants permissions.
        // "OI"  Object Inherit
        // "CI"  Container Inherit
        // "GR"  Generic Read
        // "GW"  Generic Write
        // "AU"  Authenticated Users
        if (ConvertStringSecurityDescriptorToSecurityDescriptor(
                "D:(A;OICI;GRGW;;;AU)",
                SDDL_REVISION_1,
                &sa.lpSecurityDescriptor,
                NULL))
        {
            sa.nLength              = sizeof(sa);
            sa.bInheritHandle       = TRUE;
        }
        else
        {
            sa.lpSecurityDescriptor = NULL;
            DD_WARN_REASON("Failed to create security descriptor.");
        }
#endif

        HANDLE hPipe = CreateNamedPipeA(m_pipeName,                                            // Pipe name
                                        openFlags,
                                        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // Message oriented and blocking reads/writes
                                        PIPE_UNLIMITED_INSTANCES,                              // Max. instances
                                        kSendBufferSize,                                       // Output buffer size
                                        kReceiveBufferSize,                                    // Input buffer size
                                        0,                                                     // Client time-out
                                        sa.lpSecurityDescriptor != NULL ? &sa : nullptr);      // Default security attribute

        if (sa.lpSecurityDescriptor)
        {
            LocalFree(sa.lpSecurityDescriptor);
        }

        return hPipe;
    }

    PipeListenerTransport::PipeListenerTransport(const HostInfo& hostInfo) :
        m_listening(false),
        m_listenerThread(),
        m_listenerReadyEvent(false),
        m_pRouter(nullptr)
    {
        // The hostname field should always be nullptr for Windows pipes
        DD_ASSERT(hostInfo.pHostname == nullptr);

        MakePipeName(m_pipeName, hostInfo.port);
    }

    PipeListenerTransport::~PipeListenerTransport()
    {
        if (m_listening)
            Disable();
    }

    Result PipeListenerTransport::ReceiveMessage(ConnectionInfo &connectionInfo, MessageBuffer &message, uint32 timeoutInMs)
    {
        DD_UNUSED(connectionInfo);
        DD_UNUSED(message);
        DD_UNUSED(timeoutInMs);
        return Result::Error;
    }

    Result PipeListenerTransport::TransmitMessage(const ConnectionInfo & connectionInfo, const MessageBuffer & message)
    {
        Result result = Result::Error;

        // Make sure we don't attempt to write a message that contains an invalid payload size
        if (message.header.payloadSize <= kMaxPayloadSizeInBytes)
        {
            DD_ASSERT(connectionInfo.handle == m_transportHandle);
            DD_ASSERT(connectionInfo.size == sizeof(HANDLE));

            const HANDLE& hPipe = *reinterpret_cast<const HANDLE*>(&connectionInfo.data[0]);
            PipeInfo* pPipeInfo = nullptr;

            m_threadPool.lock.Lock();
            auto find = m_threadPool.threadMap.find(DD_PTR_TO_HANDLE(hPipe));
            if (find != m_threadPool.threadMap.end())
            {
                pPipeInfo = find->second;
            }
            m_threadPool.lock.Unlock();

            if (pPipeInfo != nullptr)
            {
                DWORD cbWritten = 0;
                DWORD totalMessageSize = sizeof(MessageHeader) + message.header.payloadSize;
                OVERLAPPED oOverlap = {};

                Platform::LockGuard<Platform::AtomicLock> lock(pPipeInfo->lock);
                HANDLE hEvent = reinterpret_cast<HANDLE>(pPipeInfo->writeEvent);

                oOverlap.hEvent = hEvent;

                BOOL fSuccess = WriteFile(hPipe,
                    &message,     // buffer to write from
                    totalMessageSize, // number of bytes to write
                    &cbWritten,   // number of bytes written
                    &oOverlap);        // not overlapped I/O

                if (!fSuccess)
                {
                    DWORD dwErr = GetLastError();
                    if (dwErr == ERROR_IO_PENDING)
                    {
                        Result waitResult = WaitOverlapped(hPipe, &oOverlap, &cbWritten, kLogicFailureTimeout);
                        if (waitResult == Result::Success)
                        {
                            fSuccess = TRUE;
                        }
                        else
                        {
                            DD_WARN_REASON("Wait on pipe write failed.");
                        }
                    }
                }

                if (fSuccess != FALSE)
                {
                    result = Result::Success;
                }
                else
                {

                    m_threadPool.lock.Lock();
                    pPipeInfo->active = false;
                    if (m_threadPool.threadMap.erase(pPipeInfo->pipeHandle) > 0)
                    {
                        m_threadPool.deleteSet.emplace(pPipeInfo);
                    }
                    m_threadPool.lock.Unlock();
                }

            }
        }

        return result;
    }

    Result PipeListenerTransport::TransmitBroadcastMessage(const MessageBuffer & message)
    {
        DD_UNUSED(message);
        return Result::Error;
    }

    Result PipeListenerTransport::Enable(RouterCore *pRouter, TransportHandle handle)
    {
        Result result = Result::Error;
        SetDebugPriviledges(true);

        HANDLE hPipe = INVALID_HANDLE_VALUE;
        if (IsValidPipeName(m_pipeName))
        {
            hPipe = CreateSecureNamedPipe(true);
        }

        if (hPipe != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hPipe);

            m_transportHandle = handle;
            m_pRouter = pRouter;

            // Clear the event so we can use it to identify when the listener thread is ready.
            m_listenerReadyEvent.Clear();

            m_listenerThread.active = true;
            m_listenerThread.pTransport = this;
            result = m_listenerThread.thread.Start(&ListeningThreadCallback, &m_listenerThread);

            // This is for humans, so we ignore a failure to set the name. The code can't do anything about it anyway.
            m_listenerThread.thread.SetName("DevDriver WinPipe Listener");

            // If we successfully start the thread, wait until the listen pipe becomes available
            // before we continue.
            if (result == Result::Success)
            {
                result = m_listenerReadyEvent.Wait(kLogicFailureTimeout);
                if (result == Result::Success)
                {
                    // The listener thread is ready to accept connections. Set our status bool.
                    m_listening = true;
                }
                else
                {
                    DD_WARN_REASON("[winPipeTransport] Listener thread never created listener pipe!");

                    // Something went wrong inside the listener thread. It should have exited, so make sure it did.
                    const Result joinResult = m_listenerThread.thread.Join(kLogicFailureTimeout);
                    if (joinResult != Result::Success)
                    {
                        DD_WARN_REASON("[winPipeTransport] Failed to join listener thread after initialization error!");
                    }
                }
            }
            else
            {
                DD_WARN_REASON("[winPipeTransport] Listener thread creation failed!");
            }

            // Initialized failed, unwind some of our state changes.
            if (result != Result::Success)
            {
                m_transportHandle = 0;
                m_pRouter = nullptr;
                SetDebugPriviledges(false);
            }
        }
        else if (GetLastError() == ERROR_ACCESS_DENIED)
        {
            result = Result::Unavailable;
        }

        return result;
    }

    Result PipeListenerTransport::Disable()
    {
        Result result = Result::Error;
        if (m_listening)
        {
            // The thread should always be joinable before we disable the transport.
            DD_ASSERT(m_listenerThread.thread.IsJoinable());

            m_listenerThread.active = false;
            result = m_listenerThread.thread.Join(kLogicFailureTimeout);

            if (result == Result::Success)
            {
                m_transportHandle = 0;
                m_listening = false;
                SetDebugPriviledges(false);
            }
            else
            {
                DD_PRINT(LogLevel::Error, "Failed to disable pipe transport: %s", ResultToString(result));
                DD_ASSERT_ALWAYS();
            }
        }
        // todo: unregister all clients
        return result;
    }
} // DevDriver
