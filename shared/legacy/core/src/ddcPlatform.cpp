/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <ddPlatform.h>

#if DD_PLATFORM_IS_UM
    #include <cstddef>
    #include <stdio.h>
#endif

#include <stb_sprintf.h>

namespace DevDriver
{
    void check_expr_is_bool(bool)
    {
    }

    const char* ResultToString(Result result)
    {
        switch (result)
        {
            //// Generic Result Code  ////
            case Result::Success:            return "Success";
            case Result::Error:              return "Error";
            case Result::NotReady:           return "NotReady";
            case Result::VersionMismatch:    return "VersionMismatch";
            case Result::Unavailable:        return "Unavailable";
            case Result::Rejected:           return "Rejected";
            case Result::EndOfStream:        return "EndOfStream";
            case Result::Aborted:            return "Aborted";
            case Result::InsufficientMemory: return "InsufficientMemory";
            case Result::InvalidParameter:   return "InvalidParameter";
            case Result::InvalidClientId:    return "InvalidClientId";
            case Result::ConnectionExists:   return "ConnectionExists";
            case Result::FileNotFound:       return "FileNotFound";
            case Result::FunctionNotFound:   return "FunctionNotFound";
            case Result::InterfaceNotFound:  return "InterfaceNotFound";
            case Result::EntryExists:        return "EntryExists";
            case Result::FileAccessError:    return "FileAccessError";
            case Result::FileIoError:        return "FileIoError";
            case Result::LimitReached:       return "LimitReached";
            case Result::MemoryOverLimit:    return "MemoryOverLimit";

            //// URI PROTOCOL  ////
            case Result::UriServiceRegistrationError:  return "UriServiceRegistrationError";
            case Result::UriStringParseError:          return "UriStringParseError";
            case Result::UriInvalidParameters:         return "UriInvalidParameters";
            case Result::UriInvalidPostDataBlock:      return "UriInvalidPostDataBlock";
            case Result::UriInvalidPostDataSize:       return "UriInvalidPostDataSize";
            case Result::UriFailedToAcquirePostBlock:  return "UriFailedToAcquirePostBlock";
            case Result::UriFailedToOpenResponseBlock: return "UriFailedToOpenResponseBlock";
            case Result::UriRequestFailed:             return "UriRequestFailed";
            case Result::UriPendingRequestError:       return "UriPendingRequestError";
            case Result::UriInvalidChar:               return "UriInvalidChar";
            case Result::UriInvalidJson:               return "UriInvalidJson";

            //// Settings URI Service  ////
            case Result::SettingsUriInvalidComponent:        return "SettingsUriInvalidComponent";
            case Result::SettingsUriInvalidSettingName:      return "SettingsUriInvalidSettingName";
            case Result::SettingsUriInvalidSettingValue:     return "SettingsUriInvalidSettingValue";
            case Result::SettingsUriInvalidSettingValueSize: return "SettingsUriInvalidSettingValueSize";

            //// Info URI Service ////
            case Result::InfoUriSourceNameInvalid:       return "InfoUriSourceNameInvalid";
            case Result::InfoUriSourceCallbackInvalid:   return "InfoUriSourceCallbackInvalid";
            case Result::InfoUriSourceAlreadyRegistered: return "InfoUriSourceAlreadyRegistered";
            case Result::InfoUriSourceWriteFailed:       return "InfoUriSourceWriteFailed";

            //// Settings Service  ////
            case Result::SettingsInvalidComponent:        return "SettingsInvalidComponent";
            case Result::SettingsInvalidSettingName:      return "SettingsInvalidSettingName";
            case Result::SettingsInvalidSettingValue:     return "SettingsInvalidSettingValue";
            case Result::SettingsInsufficientValueSize:   return "SettingsInsufficientValueSize";
            case Result::SettingsInvalidSettingValueSize: return "SettingsInvalidSettingValueSize";
        }

        DD_PRINT(LogLevel::Warn, "Result code %u is not handled", static_cast<uint32>(result));
        return "Unrecognized DevDriver::Result";
    }

    Result BoolToResult(bool value)
    {
        return (value ? Result::Success : Result::Error);
    }

    void MarkUnhandledResultImpl(
        Result      result,
        const char* pExpr,
        const char* pFile,
        int         lineNumber,
        const char* pFunc)
    {
#if defined(DD_OPT_ASSERTS_ENABLE)
        if (result != Result::Success)
        {
            DD_PRINT(DevDriver::LogLevel::Error,
                "%s (%d): Unchecked Result in %s: \"%s\" == \"%s\" (0x%X)\n",
                pFile,
                lineNumber,
                pFunc,
                pExpr,
                ResultToString(result),
                result);
        }
#else
        DD_UNUSED(result);
        DD_UNUSED(pExpr);
        DD_UNUSED(pFile);
        DD_UNUSED(lineNumber);
        DD_UNUSED(pFunc);
#endif
    }

    namespace Platform
    {
        void* GenericAlloc(void* pUserdata, size_t size, size_t alignment, bool zero)
        {
            DD_UNUSED(pUserdata);
            return AllocateMemory(size, alignment, zero);
        }

        void GenericFree(void* pUserdata, void* pMemory)
        {
            DD_UNUSED(pUserdata);
            FreeMemory(pMemory);
        }

        AllocCb GenericAllocCb =
        {
            nullptr,
            &GenericAlloc,
            &GenericFree
        };

        // Write not more than dataSize characters into pDst, including the NULL terminator.
        // Returns the number of characters that would have been written if the buffer is large enough, including the NULL terminator.
        int32 Snprintf(char* pDst, size_t dstSize, const char* pFmt, ...)
        {
            va_list args;
            va_start(args, pFmt);

            const int32 ret = Vsnprintf(pDst, dstSize, pFmt, args);

            va_end(args);

            if (ret >= 0)
            {
                // ret is the minimum size of the buffer required to hold this formatted string - including a NULL terminator
                if (static_cast<size_t>(ret) > dstSize)
                {
                    // It's common practice to call this function with an empty buffer to query the size.
                    // This warning is just to help track down bugs, so silence it when the buffer in question is empty.
                    if (dstSize != 0)
                    {
                        DD_PRINT(LogLevel::Warn,
                            "Snprintf truncating output from %zu to %zu",
                            ret,
                            dstSize
                        );
                    }
                }
            }
            else
            {
                // A negative value means that some error occurred
                DD_PRINT(LogLevel::Warn,
                    "An unknown io error occured in Vsnprintf: %d (0x%x)",
                    ret,
                    ret);
            }

            return ret;
        }

        int32 Vsnprintf(char* pDst, size_t dstSize, const char* format, va_list args)
        {
            DD_ASSERT(dstSize < INT32_MAX);
            int32 ret = stbsp_vsnprintf(pDst, int(dstSize), format, args);

            // If the return value looks like a valid length, add one to account for a NULL byte.
            if (ret >= 0)
            {
                ret += 1;
            }
            else
            {
                // A negative value means that some error occurred
                // We don't print anything here because our logging requires Vsnprintf
            }

            return ret;
        }

        /////////////////////////////////////////////////////
        // Print to consoles and debuggers
        void DebugPrint(LogLevel lvl, const char* pFormat, ...)
        {
            // Use the typical pattern of snprintf-style functions:
            //      1. Call a first time to get the required buffer size
            //      2. Call a second time to do the actual formatting
            // We'll need two va_lists for this, since each execution of vsnprintf consumes its va_list

            va_list args;
            // 1. Dry run format to get the required buffer size
            va_list dryRunArgs;
            va_start(dryRunArgs, pFormat);
            va_copy(args, dryRunArgs);

            // This is the buffer requirement including the NULL terminator,
            int32 requiredSize = Platform::Vsnprintf(nullptr, 0, pFormat, dryRunArgs);
            va_end(dryRunArgs);

            // if requiredSize < 0 then Platform::Vsnprintf failed.
            if (requiredSize >= 0)
            {
                // This buffer has a fixed amount stack-allocated for the common case
                // It's pretty rare that we need to spill onto the heap.
                char    stackBuffer[128];
                char*   heapBuffer = nullptr;
                // default to using the stack buffer unless we need more space
                char*   bufferToUse = stackBuffer;

                // add 1 for trailing newline character
                requiredSize++;

                if (requiredSize > static_cast<int32>(sizeof(stackBuffer)))
                {
                    heapBuffer = static_cast<char*>(AllocateMemory(requiredSize, 1, true));

                    if (heapBuffer == nullptr)
                    {
                        // early exit if memory alloc failed
                        return;
                    }

                    bufferToUse = heapBuffer;
                }

                // 2. Do the actual formatting
                Platform::Vsnprintf(bufferToUse, requiredSize, pFormat, args);
                va_end(args);

                bufferToUse[requiredSize - 2] = '\n';
                bufferToUse[requiredSize - 1] = '\0';

#if DD_PLATFORM_IS_UM
                printf("[DevDriver] %s", bufferToUse);
#else
                // On Kernel mode platforms, printf() isn't available, so we skip it and let PlatformDebugPrint handle output
#endif

                // Platforms may have additional logging to do - e.g. system logging frameworks like OutputDebugStringA().
                PlatformDebugPrint(lvl, bufferToUse);

                if (heapBuffer != nullptr)
                {
                    FreeMemory(heapBuffer);
                }
            }
        }

        ThreadReturnType Thread::ThreadShim(void* pShimParam)
        {
            DD_ASSERT(pShimParam != nullptr);

            Thread* pThread = reinterpret_cast<Thread*>(pShimParam);
            DD_ASSERT(pThread->pFnFunction != nullptr);
            DD_ASSERT(pThread->hThread     != kInvalidThreadHandle);

            // Execute the caller's thread function
            pThread->pFnFunction(pThread->pParameter);

            // Posix platforms do not have a simple way to timeout a thread join.
            // To get around this, we wrap user-supplied callbacks and explicitly signal when the
            // user callback returns.
            // Thread::Join() can then wait on this event to know if the thread exited normally.
            // If it returns without timing out, we can call the posix join without having to
            // worry about blocking indefinitely.
            // This behavior is toggle-able across all platforms until we have a more native solution.
            pThread->onExit.Signal();

            return ThreadReturnType(0);
        }

        void Thread::Reset()
        {
            pFnFunction = nullptr;
            pParameter  = nullptr;
            hThread     = kInvalidThreadHandle;

            onExit.Clear();
        }

        Result Thread::SetName(const char* pFmt, ...)
        {
            Result result = Result::Error;

            DD_WARN(hThread != kInvalidThreadHandle);
            if (hThread != kInvalidThreadHandle)
            {
                // Limit the size of the thread name to the platform defined maximum.
                char threadNameBuffer[kThreadNameMaxLength];
                memset(threadNameBuffer, 0, sizeof(threadNameBuffer));

                va_list args;
                va_start(args, pFmt);
                const int32 ret = Vsnprintf(threadNameBuffer, ArraySize(threadNameBuffer), pFmt, args);
                va_end(args);

                if (ret < 0)
                {
                    result = Result::Error;
                }
                else
                {
                    result = SetNameRaw(threadNameBuffer);
                }
            }

            return result;
        }

        Thread::~Thread()
        {
            if (IsJoinable())
            {
                DD_ASSERT_REASON("A Thread object left scope without calling Join()");
            }
        }

        // Random::Random() is implemented per platform, and seeded with the
        // time.

        Random::Random(uint64 seed)
        {
            Reseed(seed);
        }

        Random::~Random() = default;

        // Standard Linear Congruential Generator.
        // It's basically rand() but consistent across platforms.
        uint32 Random::Generate()
        {
            // Keep the naming consistent with math notation.
            constexpr auto m = kModulus;
            constexpr auto a = kMultiplier;
            constexpr auto c = kIncrement;

            m_prevState = (m_prevState * a + c) % m;

            // Return a subset of the bits
            uint32 parts[3] = {};
            parts[0] = (m_prevState >>  0) & 0xffff;
            parts[1] = (m_prevState >> 16) & 0xffff;
            parts[2] = (m_prevState >> 32) & 0xffff;
            return  (parts[2] << 15) | (parts[1] >> 1);
        }

        void Random::Reseed(uint64 seed)
        {
            // Seeds must be smaller than the modulus.
            // If we silently do the wrapping, a seed of 1 and (kModulus + 1) will generate the same sequence.
            // This is bad but not the end of the world.
            DD_WARN(seed < kModulus);
            m_prevState = seed % kModulus;
        }

        Library::Library()
            : m_hLib(nullptr)
        {
        }

        Library::~Library()
        {
            Close();
        }

        bool Library::IsLoaded() const
        {
            return (m_hLib != nullptr);
        }

        void Library::Swap(Library* pLibrary)
        {
            m_hLib = pLibrary->m_hLib;
            pLibrary->m_hLib = nullptr;
        }

        AtomicLock::AtomicLock()
            : m_lock(0)
        {
        }

        AtomicLock::~AtomicLock() = default;

        void AtomicLock::Lock()
        {
            // TODO - implement timeout
            while (TryLock() == false)
            {
                while (AtomicGet(&m_lock) != 0)
                {
                    // Spin until the mutex is unlocked again
                }
            }
        }

        bool AtomicLock::IsLocked()
        {
            return (AtomicGet(&m_lock) != 0);
        }
    }

    // The minimum alignment that system allocators are expected to adhere to.
#if !DD_PLATFORM_IS_KM
    constexpr size_t kMinSystemAlignment = alignof(max_align_t);
#else
    // In the kernel, we have to hardcode this to 16 bytes because of header issues...
    constexpr size_t kMinSystemAlignment = 16;
#endif

    void* AllocCb::Alloc(size_t size, size_t alignment, bool zero) const
    {
        // Allocators are not expected to ever align smaller than the system minimum.
        // (This is usually sizeof(void*), but always check against this constant)
        if (alignment < kMinSystemAlignment)
        {
            alignment = kMinSystemAlignment;
        }

        return pfnAlloc(pUserdata, size, alignment, zero);
    }

    void* AllocCb::Alloc(size_t size, bool zero) const
    {
        return Alloc(size, kMinSystemAlignment, zero);
    }

    void AllocCb::Free(void* pMemory) const
    {
        pfnFree(pUserdata, pMemory);
    }

    const void* VoidPtrInc(
        const void* pPtr,
        size_t      numBytes)
    {
        return (static_cast<const uint8*>(pPtr) + numBytes);
    }

    void* VoidPtrInc(
        void*  pPtr,
        size_t numBytes)
    {
        return (static_cast<uint8*>(pPtr) + numBytes);
    }

    const void* VoidPtrDec(
        const void* pPtr,
        size_t      numBytes)
    {
        return (static_cast<const uint8*>(pPtr) - numBytes);
    }

    void* VoidPtrDec(
        void*  pPtr,
        size_t numBytes)
    {
        return (static_cast<uint8*>(pPtr) - numBytes);
    }
}
