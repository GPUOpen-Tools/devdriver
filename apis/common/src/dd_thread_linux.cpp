/* Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <ddPlatform.h>
#include <dd_assert.h>
#include <dd_result.h>
#include <dd_thread.h>
#include <cstdlib>
#include <cstring>
#include <pthread.h>

namespace DevDriver
{

struct ThreadIdentifier
{
    pthread_t id;
};

Thread::~Thread()
{
    // Thread should be joined before being destroyed.
    DD_ASSERT(m_pThreadId == nullptr);
    if (m_pThreadId != nullptr)
    {
        std::free(m_pThreadId);
    }
}

DD_RESULT Thread::Start(ThreadFunction pThreadFn, void* pUserdata)
{
    if (pThreadFn == nullptr)
    {
        return DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    if (m_pThreadId != nullptr)
    {
        // Previously started thread is still running.
        return DD_RESULT_COMMON_ALREADY_EXISTS;
    }

    m_pThreadId = (ThreadIdentifier*)std::malloc(sizeof(*m_pThreadId));
    if (m_pThreadId == nullptr)
    {
        return DD_RESULT_COMMON_OUT_OF_HEAP_MEMORY;
    }

    DD_RESULT result = DD_RESULT_SUCCESS;

    m_pThreadFn = pThreadFn;
    m_pUserdata = pUserdata;

    int err = pthread_create(&m_pThreadId->id, nullptr, ThreadFnShim, this);
    result = ResultFromErrno(err);

    if (result != DD_RESULT_SUCCESS)
    {
        std::free(m_pThreadId);
        m_pThreadId = nullptr;
    }

    return result;
}

DD_RESULT Thread::Join()
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    if (m_pThreadId != nullptr)
    {
        int err = pthread_join(m_pThreadId->id, nullptr);
        result = ResultFromErrno(err);

        m_pThreadFn = nullptr;
        m_pUserdata = nullptr;

        std::free(m_pThreadId);
        m_pThreadId = nullptr;
    }

    return result;
}

DD_RESULT Thread::SetDebugName(const char* pName)
{
    if (pName == nullptr)
    {
        return DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    if (m_pThreadId == nullptr)
    {
        return DD_RESULT_COMMON_DOES_NOT_EXIST;
    }

    const size_t NameBufferSize = 16;
    char nameBuf[NameBufferSize] {};
    Platform::Strncpy(nameBuf, pName, NameBufferSize);
    int err = pthread_setname_np(m_pThreadId->id, nameBuf);
    return ResultFromErrno(err);
}

void* Thread::ThreadFnShim(void* pThread)
{
    DevDriver::Thread* pThisThread = static_cast<DevDriver::Thread*>(pThread);
    pThisThread->m_pThreadFn(pThisThread->m_pUserdata);
    return nullptr;
}

} // namespace DevDriver
