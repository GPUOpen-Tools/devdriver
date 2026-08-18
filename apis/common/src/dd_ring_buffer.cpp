/* Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_assert.h>
#include <dd_integer.h>
#include <dd_ring_buffer.h>

namespace DevDriver
{

void RingBuffer::SetBuffer(const MirroredBuffer* pBuffer)
{
    m_pBuf = reinterpret_cast<uint8_t*>(pBuffer->pBuffer);
    m_bufSize = pBuffer->bufferSize;

    DD_ASSERT((m_bufSize & (m_bufSize - 1)) == 0);
    m_bufSizeMask = m_bufSize - 1;
}

RingBuffer::Work RingBuffer::AcquireForWrite(uint32_t size)
{
    Work work {};

    m_wholeBufferMutex.Lock();

    if (size > 0)
    {
        DD_ASSERT(m_write >= m_read);

        uint64_t occupiedBufSize = m_write - m_read;
        DD_ASSERT(occupiedBufSize <= m_bufSize);

        uint64_t availableBufSize = m_bufSize - occupiedBufSize;
        if (availableBufSize >= size)
        {
            work.offset = static_cast<uint32_t>(m_write & m_bufSizeMask);
            work.size = size;
            m_write += size;
        }
    }

    return work;
}

RingBuffer::Work RingBuffer::AcquireForRead(uint32_t maxSize)
{
    Work work {};

    m_wholeBufferMutex.Lock();

    DD_ASSERT(m_write >= m_read);
    uint64_t writtenDataSize = m_write - m_read;
    DD_ASSERT(writtenDataSize <= m_bufSize);
    work.offset = static_cast<uint32_t>(m_read & m_bufSizeMask);
    uint64_t acquiredSize = (writtenDataSize > maxSize ? maxSize : writtenDataSize);
    work.size = static_cast<uint32_t>(acquiredSize);
    m_read += work.size;

    return work;
}

RingBuffer::Work RingBuffer::AcquireForReadAll()
{
    Work work {};

    m_wholeBufferMutex.Lock();

    DD_ASSERT(m_write >= m_read);
    uint64_t writtenDataSize = m_write - m_read;
    DD_ASSERT(writtenDataSize <= m_bufSize);
    work.offset = static_cast<uint32_t>(m_read & m_bufSizeMask);
    work.size = static_cast<uint32_t>(writtenDataSize);
    m_read += work.size;

    return work;
}

void RingBuffer::Release()
{
    m_wholeBufferMutex.Unlock();
}

} // namespace DevDriver
