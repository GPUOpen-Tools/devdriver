/* Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_mutex.h>
#include <Windows.h>

namespace DevDriver
{

Mutex::Mutex() noexcept
    : m_osMutexData {}
{
    // We use SRWLock for our mutex implementation based on the following considerations:
    // - According to Microsoft, SRWLock is must faster than CRITICL_SECTION regardless of high or low contention.
    // - SRWLock (8 bytes) takes up significantly lower memory compared to CRITICAL_SECTION (40 bytes).
    // - SRWLock is non-recurisve, whereas CRITICAL_SECTION can be locked recursively.
    static_assert(sizeof(Mutex::m_osMutexData) >= sizeof(SRWLOCK));

    auto pWinLock = reinterpret_cast<SRWLOCK*>(m_osMutexData);
    InitializeSRWLock(pWinLock);
}

Mutex::~Mutex() noexcept
{
    // No destroy function for SRWLock.
}

void Mutex::Lock()
{
    auto pWinLock = reinterpret_cast<SRWLOCK*>(m_osMutexData);
    AcquireSRWLockExclusive(pWinLock);
}

bool Mutex::TryLock()
{
    auto pWinMutex = reinterpret_cast<SRWLOCK*>(m_osMutexData);
    return TryAcquireSRWLockExclusive(pWinMutex);
}

void Mutex::Unlock()
{
    auto pWinMutex = reinterpret_cast<SRWLOCK*>(m_osMutexData);
    ReleaseSRWLockExclusive(pWinMutex);
}

RWLock::RWLock() noexcept
    : m_osLockData {}
{
    static_assert(sizeof(RWLock::m_osLockData) >= sizeof(SRWLOCK));

    auto pWinRWLock = reinterpret_cast<SRWLOCK*>(m_osLockData);
    InitializeSRWLock(pWinRWLock);
}

RWLock::~RWLock() noexcept
{
    // no destroy function for Win32 SRWLock.
}

void RWLock::AcquireReadLock()
{
    auto pWinRWLock = reinterpret_cast<SRWLOCK*>(m_osLockData);
    AcquireSRWLockShared(pWinRWLock);
}

void RWLock::ReleaseReadLock()
{
    auto pWinRWLock = reinterpret_cast<SRWLOCK*>(m_osLockData);
    ReleaseSRWLockShared(pWinRWLock);
}

void RWLock::AcquireWriteLock()
{
    auto pWinRWLock = reinterpret_cast<SRWLOCK*>(m_osLockData);
    AcquireSRWLockExclusive(pWinRWLock);
}

void RWLock::ReleaseWriteLock()
{
    auto pWinRWLock = reinterpret_cast<SRWLOCK*>(m_osLockData);
    ReleaseSRWLockExclusive(pWinRWLock);
}

} // namespace DevDriver

