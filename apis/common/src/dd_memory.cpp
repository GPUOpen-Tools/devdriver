/* Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_memory.h>
#include <dd_integer.h>
#include <dd_platform_info.h>

namespace DevDriver
{

DD_RESULT ScratchBuffer::Initialize(uint32_t totalSize, uint32_t initialCommitSize)
{
    if ((totalSize == 0) || (totalSize < initialCommitSize))
    {
        return DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    m_pageSize = PlatformInfo::GetPageSize();

    const uint32_t pageSizeAlignedTotalSize = AlignU32(totalSize, m_pageSize);
    const uint32_t pageSizeAlignedInitialCommitSize = AlignU32(initialCommitSize, m_pageSize);

    // Reserve virtual memory.
    DD_RESULT result = ReserveMemory(pageSizeAlignedTotalSize, reinterpret_cast<void**>(&m_pBuffer));
    if (result == DD_RESULT_SUCCESS)
    {
        m_totalSize = pageSizeAlignedTotalSize;
    }

    // Commit a part of virtual memory to physical memory.
    if (result == DD_RESULT_SUCCESS)
    {
        result = CommitMemory(pageSizeAlignedInitialCommitSize);
        if (result == DD_RESULT_SUCCESS)
        {
            m_committedSize = pageSizeAlignedInitialCommitSize;
        }
    }

    if (result != DD_RESULT_SUCCESS)
    {
        m_pBuffer = nullptr;
    }

    return result;
}

void ScratchBuffer::Destroy()
{
    FreeMemory(m_pBuffer, m_totalSize);
    m_totalSize = 0;
    m_committedSize = 0;
    m_pageSize = 0;
    m_top = 0;
    m_pBuffer = nullptr;
}

void* ScratchBuffer::Push(uint32_t size)
{
    if (size > (m_totalSize - m_top))
    {
        return nullptr;
    }

    DD_RESULT result = DD_RESULT_SUCCESS;
    void* pResultMem = nullptr;

    uint32_t alignedSizeToCommit = 0;
    if (size > (m_committedSize - m_top))
    {
        const uint32_t extraSizeNeeded = size - (m_committedSize - m_top);
        alignedSizeToCommit = AlignU32(extraSizeNeeded, m_pageSize);
        result = CommitMemory(alignedSizeToCommit);
    }

    if (result == DD_RESULT_SUCCESS)
    {
        m_committedSize += alignedSizeToCommit;
        pResultMem = m_pBuffer + m_top;
        m_top += size;
    }

    return pResultMem;
}

void ScratchBuffer::Pop(uint32_t size)
{
    DD_ASSERT(size <= m_top);
    m_top -= size;
}

void ScratchBuffer::Clear()
{
    m_top = 0;
}

} // namespace DevDriver
