/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <ddPlatform.h>
#include <ddDevModeQueue.h>

namespace DevDriver
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Internal use functions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

DD_ALIGNED_STRUCT(SharedQueueHeader, 4)
{
    // Read and Write Locks
    Platform::AtomicLock readLock;
    Platform::AtomicLock writeLock;

    // Current indices into shared buffer
    Platform::Atomic readIndex;
    Platform::Atomic writeIndex;

    // Counters
    Platform::Atomic popCount;
    Platform::Atomic pushCount;
    Platform::Atomic failCount;
};
DD_CHECK_SIZE(SharedQueueHeader, 28);

//---------------------------------------------------------------------
// PopQueueMessage
//
// Pop message from the specified queue
//
inline static Result PopQueueMessage(QueueInfo &messageQueue, const uint32 timeout, MessageBuffer &message)
{
    SharedQueueHeader* DD_RESTRICT pSharedBuffer = reinterpret_cast<SharedQueueHeader*>(messageQueue.sharedBuffer.hSharedBufferView);

    // wait until the read semaphore has been signaled + decrement
    Result result = Platform::Windows::WaitSharedSemaphore(messageQueue.hSemRead, timeout);
    if (result == Result::Success)
    {
        pSharedBuffer->readLock.Lock();
        uint32 index = static_cast<uint32>(Platform::AtomicGet(&pSharedBuffer->readIndex));

        // calculate the location of the message in the buffer
        uint32 baseOffset = messageQueue.messageOffset + messageQueue.queueMessageSize * index;
        MessageBuffer* DD_RESTRICT pTempMessage = reinterpret_cast<MessageBuffer*>(reinterpret_cast<uint8*>(pSharedBuffer) + baseOffset);

        result = Result::Error;

        if (pTempMessage->header.payloadSize <= kMaxPayloadSizeInBytes)
        {
            const uint32 encodedMsgSize = sizeof(MessageHeader) + pTempMessage->header.payloadSize;
            if ((encodedMsgSize <= messageQueue.queueMessageSize) && (encodedMsgSize <= sizeof(MessageBuffer)))
            {
                DD_PRINT(LogLevel::Debug, "Reading src: %u dst: %u from queue position %u", pTempMessage->header.srcClientId, pTempMessage->header.dstClientId, index);

                // copy the buffer out
                Platform::Memcpy_s(&message, sizeof(MessageBuffer), pTempMessage, encodedMsgSize);

                Platform::AtomicSet(&pSharedBuffer->readIndex, static_cast<int32>((index + 1) % messageQueue.queueLength));
                Platform::AtomicIncrement(&pSharedBuffer->popCount);

                result = Result::Success;
            }
        }

        pSharedBuffer->readLock.Unlock();

        // signal the write semaphore so a blocked thread can write
        Platform::Windows::SignalSharedSemaphore(messageQueue.hSemWrite);
    }

    return result;
}

//---------------------------------------------------------------------
// PushQueueMessage
//
// Push message into the specified queue
//
inline static Result PushQueueMessage(QueueInfo &messageQueue, const uint32 timeout, const MessageBuffer &message)
{
    SharedQueueHeader* DD_RESTRICT pSharedBuffer = reinterpret_cast<SharedQueueHeader*>(messageQueue.sharedBuffer.hSharedBufferView);

#ifdef DEVDRIVER_LOSSY_RATIO
    uint32 randVal;
    if (!rand_s(&randVal))
    {
        if ((randVal / (float)UINT_MAX) < DEVDRIVER_LOSSY_RATIO)
        {
            AtomicIncrement(&pSharedBuffer->failCount);
            return Result::Error;
        }
    }
#endif

    Result result = Result::Error;

    if (message.header.payloadSize <= kMaxPayloadSizeInBytes)
    {
        // wait until the write semaphore has been signaled and decrement
        result = Platform::Windows::WaitSharedSemaphore(messageQueue.hSemWrite, timeout);
        if (result == Result::Success)
        {
            pSharedBuffer->writeLock.Lock();
            uint32 index = static_cast<uint32>(Platform::AtomicGet(&pSharedBuffer->writeIndex));

            DD_PRINT(LogLevel::Debug, "Writing src: %u dst: %u into queue position %u", message.header.srcClientId, message.header.dstClientId, index);

            // calculate the location of the message in the buffer
            uint32 baseOffset = messageQueue.messageOffset + messageQueue.queueMessageSize * index;
            MessageBuffer* DD_RESTRICT pDestBuffer = reinterpret_cast<MessageBuffer*>(reinterpret_cast<uint8*>(pSharedBuffer) + baseOffset);

            const uint32 msgSize = sizeof(MessageHeader) + message.header.payloadSize;
            if (msgSize <= messageQueue.queueMessageSize)
            {
                // copy the buffer in
                Platform::Memcpy_s(pDestBuffer, messageQueue.queueMessageSize, &message, msgSize);
                Platform::AtomicSet(&pSharedBuffer->writeIndex, static_cast<int32>((index + 1) % messageQueue.queueLength));
                Platform::AtomicIncrement(&pSharedBuffer->pushCount);
            }
            else
            {
                result = Result::Error;
            }

            pSharedBuffer->writeLock.Unlock();

            // signal the read semaphore
            Platform::Windows::SignalSharedSemaphore(messageQueue.hSemRead);
        }
        else
        {
            Platform::AtomicIncrement(&pSharedBuffer->failCount);
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Public functions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------
// SharedQueue Constructor
//
SharedQueue::SharedQueue()
{
    memset(&m_sendQueue, 0, sizeof(QueueInfo));
    memset(&m_receiveQueue, 0, sizeof(QueueInfo));
}

//---------------------------------------------------------------------
// SharedQueue Destructor
//
SharedQueue::~SharedQueue()
{
    Destroy();
}

//---------------------------------------------------------------------
// SharedQueue::QueryTransmitCount
//
// Query a message transport and return how many packets it has transmitted successfully
uint32 SharedQueue::QueryTransmitCount() const
{
    const SharedQueueHeader* DD_RESTRICT pSharedBuffer =
        reinterpret_cast<const SharedQueueHeader*>(m_sendQueue.sharedBuffer.hSharedBufferView);
    return static_cast<uint32>(Platform::AtomicGet(&pSharedBuffer->popCount));
}

//---------------------------------------------------------------------
// SharedQueue::QueryReceiveCount
//
// Query a message transport and return how many packets it has received successfully
//
uint32 SharedQueue::QueryReceiveCount() const
{
    const SharedQueueHeader* DD_RESTRICT pSharedBuffer =
        reinterpret_cast<const SharedQueueHeader*>(m_receiveQueue.sharedBuffer.hSharedBufferView);
    return static_cast<uint32>(Platform::AtomicGet(&pSharedBuffer->pushCount));
}

//---------------------------------------------------------------------
// SharedQueue::QueryTransmitFailureCount
//
// Query a message transport and return how many packets it failed to transmit
//
uint32 SharedQueue::QueryTransmitFailureCount() const
{
    const SharedQueueHeader* DD_RESTRICT pSharedBuffer =
        reinterpret_cast<const SharedQueueHeader*>(m_sendQueue.sharedBuffer.hSharedBufferView);
    return static_cast<uint32>(Platform::AtomicGet(&pSharedBuffer->failCount));
}

//---------------------------------------------------------------------
// SharedQueue::QueryReceiveFailureCount
//
// Query a message transport and return how many packets it failed to receive
//
uint32 SharedQueue::QueryReceiveFailureCount() const
{
    const SharedQueueHeader* DD_RESTRICT pSharedBuffer =
        reinterpret_cast<const SharedQueueHeader*>(m_receiveQueue.sharedBuffer.hSharedBufferView);
    return static_cast<uint32>(Platform::AtomicGet(&pSharedBuffer->failCount));
}

//---------------------------------------------------------------------
// CreateMessageQueue
//
// Create a message handler used to communicate
// between two processes
Result CreateMessageQueue(QueueInfo &messageQueue, const uint32 queueLength, const uint32 queueMessageSize)
{
    Result status = Result::Error;
#ifndef DD_PLATFORM_WINDOWS_KM
    Handle hSemWrite = Platform::Windows::CreateSharedSemaphore(queueLength, queueLength);
    Handle hSemRead = Platform::Windows::CreateSharedSemaphore(0, queueLength);
    if ((hSemWrite != kNullPtr) & (hSemRead != kNullPtr))
    {
        messageQueue.hSemWrite = hSemWrite;
        messageQueue.hSemRead = hSemRead;
        messageQueue.queueLength = queueLength;
        messageQueue.queueMessageSize = queueMessageSize;
        status = Result::Success;
    }
    else
    {
        if (hSemWrite != kNullPtr)
        {
            Platform::Windows::CloseSharedSemaphore(hSemWrite);
        }
        if (hSemRead != kNullPtr)
        {
            Platform::Windows::CloseSharedSemaphore(hSemRead);
        }
    }

    DD_WARN(status == Result::Success);
#else
    DD_UNUSED(messageQueue);
    DD_UNUSED(queueLength);
    DD_UNUSED(queueMessageSize);
    DD_NOT_IMPLEMENTED();
#endif

    return status;
}

size_t SharedQueue::GetHeaderSize()
{
    return sizeof(SharedQueueHeader);
}

//---------------------------------------------------------------------
// DestroyMessageQueue
//
// Destroy a previously created message handler
void DestroyMessageQueue(QueueInfo &messageQueue)
{
    // release the kernel handles for the shared semaphore objects
    if (messageQueue.hSemWrite != kNullPtr)
    {
        Platform::Windows::CloseSharedSemaphore(messageQueue.hSemWrite);
        messageQueue.hSemWrite = kNullPtr;
    }

    if (messageQueue.hSemRead != kNullPtr)
    {
        Platform::Windows::CloseSharedSemaphore(messageQueue.hSemRead);
        messageQueue.hSemRead = kNullPtr;
    }

    // If hSharedBufferObject is not null we are using the fake KMD codepath. This means we have to unmap and
    // destroy the shared buffer as part of object destruction. If hSharedBufferObject is set to kNullPtr,
    // we are talking to a real KMD and this is unnecessary - unmapping and destroying the backing memory
    // is handled by the KMD itself.
    if (messageQueue.sharedBuffer.hSharedBufferObject != kNullPtr)
    {
        // Unmap the kernel view of the shared buffer
        if (messageQueue.sharedBuffer.hSharedBufferView != kNullPtr)
        {
            Platform::Windows::UnmapBufferView(messageQueue.sharedBuffer.hSharedBufferObject,
                messageQueue.sharedBuffer.hSharedBufferView);
        }
        // Destroy the shared buffer
        Platform::Windows::CloseSharedBuffer(messageQueue.sharedBuffer.hSharedBufferObject);
    }
    messageQueue.sharedBuffer.hSharedBufferView = kNullPtr;
    messageQueue.sharedBuffer.hSharedBufferObject = kNullPtr;
}

//---------------------------------------------------------------------
// CreateTransport
//
// Initialize the message transport and the message queue handles that have to be passed to the server
Result SharedQueue::Initialize(uint32 queueLength, uint32 queueMessageSize)
{
    Result status = Result::Error;
#ifndef DD_PLATFORM_WINDOWS_KM
    {
        status = Result::Success;

        // Send message handler
        if (status == Result::Success)
        {
            status = CreateMessageQueue(m_sendQueue, queueLength, queueMessageSize);
        }
        // Recieve message handler
        if (status == Result::Success)
        {
            status = CreateMessageQueue(m_receiveQueue, queueLength, queueMessageSize);
        }
    }
#else
    DD_UNUSED(queueLength);
    DD_UNUSED(queueMessageSize);
    DD_NOT_IMPLEMENTED();
#endif
    return status;
}

//---------------------------------------------------------------------
// DestroyTransport
//
// Destroy a message channel and thread
void SharedQueue::Destroy()
{
    // Destroy the message handlers
    DestroyMessageQueue(m_sendQueue);
    DestroyMessageQueue(m_receiveQueue);
}

//---------------------------------------------------------------------
// ReceiveMessage
//
// Receive a message from a given message handler
Result SharedQueue::ReceiveMessage(MessageBuffer &messageBuffer, const uint32 timeout)
{
    Result result = Result::Error;
    if (m_receiveQueue.sharedBuffer.hSharedBufferView != kNullPtr)
    {
        result = PopQueueMessage(m_receiveQueue, timeout, messageBuffer);
    }
    return result;
}

//---------------------------------------------------------------------
// TransmitMessage
//
// Transmits a message to a given message handler
Result SharedQueue::TransmitMessage(const MessageBuffer &messageBuffer, const uint32 timeout)
{
    Result result = Result::Error;
    if (m_sendQueue.sharedBuffer.hSharedBufferView != kNullPtr)
    {
        result = PushQueueMessage(m_sendQueue, timeout, messageBuffer);
    }
    return result;
}

}
