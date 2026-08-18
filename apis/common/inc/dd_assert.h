/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#ifdef DD_ASSERT
#undef DD_ASSERT
#endif

#if defined(_MSC_VER)
    #if defined(_KERNEL_MODE)
        #include <wdm.h>
        #define DD_ALWAYS_ASSERT(condition) NT_ASSERT(condition)
    #else
        #define DD_ALWAYS_ASSERT(condition) do { if (!(condition)) __debugbreak(); } while(0)
    #endif
#else
    #include <signal.h>
    #define DD_ALWAYS_ASSERT(condition) do { if (!(condition)) raise(SIGTRAP); } while(0)
#endif

#ifdef DD_OPT_ASSERTS_ENABLE
    #define DD_ASSERT(condition) DD_ALWAYS_ASSERT(condition)
#else
    #define DD_ASSERT(condition) (void)(condition)
#endif
