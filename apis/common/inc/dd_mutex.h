/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_common_api.h>

#if defined (__linux__)
#include <pthread.h>
#endif

namespace DevDriver
{

class Mutex
{
#if defined(_WIN32)
    static const uint32_t OsMutexSize = 8; // sizeof(SRWLOCK)
#elif defined(__linux__)
    static const uint32_t OsMutexSize = sizeof(pthread_mutex_t);
#endif

private:
    uint8_t m_osMutexData[OsMutexSize];

public:
    Mutex() noexcept;
    ~Mutex() noexcept;

    /// Enters the critical section if it is not contended.  If it is contended, wait until the critical section is
    /// available.
    void Lock();

    /// Enters the critical section if it is not contended.  Does not wait for the critical section to become available
    /// if it is contended.
    ///
    /// @returns True if the critical section was entered, false otherwise.
    bool TryLock();

    /// Leaves the critical section.
    void Unlock();

private:
    Mutex(Mutex&&) = delete;
    Mutex(const Mutex&) = delete;
    Mutex& operator=(Mutex&&) = delete;
    Mutex& operator=(const Mutex&) = delete;
};

/// The class LockGuard is a mutex wrapper that provides a convenient RAII-style mechanism for owning a
/// \ref Mutex for the duration of a scoped block.
class LockGuard
{
private:
    Mutex& m_mutex;

public:
    /// Lock the given mutex.
    explicit LockGuard(Mutex& mutex)
        : m_mutex(mutex)
    {
        m_mutex.Lock();
    }

    /// Release the mutex.
    ~LockGuard()
    {
        m_mutex.Unlock();
    }

private:
    LockGuard(LockGuard&&) = delete;
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(LockGuard&&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
};

class RWLock
{
#if defined(_WIN32)
    static const uint32_t OsRWLockSize = 8; // sizeof(SRWLOCK)
#elif defined(__linux__)
    static const uint32_t OsRWLockSize = sizeof(pthread_rwlock_t);
#endif

private:
    uint8_t m_osLockData[OsRWLockSize];

public:
    enum LockType
    {
        Read = 0, // Shared for multiple read accesses.
        Write     // Exclusive for one read/write access.
    };

    RWLock() noexcept;
    ~RWLock() noexcept;

    void AcquireReadLock();

    void ReleaseReadLock();

    void AcquireWriteLock();

    void ReleaseWriteLock();

private:
    RWLock(RWLock&&) = delete;
    RWLock(const RWLock&) = delete;
    RWLock& operator=(RWLock&&) = delete;
    RWLock& operator=(const RWLock&) = delete;
};

template<RWLock::LockType L>
class RWLockGuard
{
private:
    RWLock& m_rwLock;

public:
    explicit RWLockGuard(RWLock& lock)
        : m_rwLock(lock)
    {
        if (L == RWLock::LockType::Read)
        {
            m_rwLock.AcquireReadLock();
        }
        else
        {
            m_rwLock.AcquireWriteLock();
        }
    }

    ~RWLockGuard()
    {
        if (L == RWLock::LockType::Read)
        {
            m_rwLock.ReleaseReadLock();
        }
        else
        {
            m_rwLock.ReleaseWriteLock();
        }
    }

private:
    RWLockGuard(RWLockGuard&&) = delete;
    RWLockGuard(const RWLockGuard&) = delete;
    RWLockGuard& operator=(RWLockGuard&&) = delete;
    RWLockGuard& operator=(const RWLockGuard&) = delete;
};

} // namespace DevDriver
