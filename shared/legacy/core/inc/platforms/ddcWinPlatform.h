/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
#pragma once

#if defined(_KERNEL_MODE)
static_assert(false, "This header is for user mode windows, and it does not work in kernel mode.");
#endif

// Our code expects these defined before including Windows.h.
// However, we need to guard against clients defining them too.
#ifndef _CRT_RAND_S
    #define _CRT_RAND_S
#endif

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
    #define NOMINMAX
#endif

// WIN32_NO_STATUS makes Windows.h not include macro definitions from winnt.h
// which collide with those from ntstatus.h. This avoids compilation errors
// when other files that include ntstatus.h also include this file.
#define WIN32_NO_STATUS
#include <Windows.h>
#undef WIN32_NO_STATUS

    #include <WinIoCtl.h>

#include <intrin.h>

#define DD_RESTRICT __restrict

#define DD_DEBUG_BREAK() __debugbreak()

namespace DevDriver
{
    namespace Platform
    {
        /* platform functions for performing atomic operations */
        typedef LONG Atomic;
        DD_CHECK_SIZE(Atomic, sizeof(int32));

        typedef LONG64 Atomic64;
        DD_CHECK_SIZE(Atomic64, sizeof(int64));

        struct EmptyStruct {};

        struct MutexStorage
        {
            CRITICAL_SECTION criticalSection;
#if !defined(NDEBUG)
            Atomic           lockCount;
#endif
        };
        typedef Handle SemaphoreStorage;
        typedef HANDLE EventStorage;
        typedef HANDLE ThreadHandle;
        typedef DWORD  ThreadReturnType;
        typedef HMODULE LibraryHandle;

        constexpr ThreadHandle kInvalidThreadHandle = NULL;

        // Maximum supported size for thread names, including NULL byte
        // This exists because some platforms have hard limits on thread name size.
        // Windows doesn't seem to have a thread name size limit, but we use this variable to control
        // a formatting buffer as well and we want to keep it reasonably small since it's stack allocated.
        static constexpr size_t kThreadNameMaxLength = 64;

        #define DD_APIENTRY APIENTRY

        namespace Windows
        {
            // Windows specific functions required for in-memory communication
            Handle CreateSharedSemaphore(uint32 initialCount, uint32 maxCount);
            Handle CopySemaphoreFromProcess(ProcessId processId, Handle hObject);
            Result SignalSharedSemaphore(Handle pSemaphore);
            Result WaitSharedSemaphore(Handle pSemaphore, uint32 millisecTimeout);
            void CloseSharedSemaphore(Handle pSemaphore);

            Handle CreateSharedBuffer(Size bufferSizeInBytes);
            void CloseSharedBuffer(Handle hSharedBuffer);

            Handle MapSystemBufferView(Handle hBuffer, Size bufferSizeInBytes);
            Handle MapProcessBufferView(Handle hBuffer, ProcessId processId);
            void UnmapBufferView(Handle hSharedBuffer, Handle hSharedBufferView);

            // Whether or not the user has enabled Windows Developer Mode on their system
            // See: https://github.com/MicrosoftDocs/windows-uwp/blob/docs/hub/apps/get-started/enable-your-device-for-development.md
            bool IsWin10DeveloperModeEnabled();
        }
    }
}
