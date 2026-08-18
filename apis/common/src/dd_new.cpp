/* Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_new.h>
#include <dd_assert.h>

// Per C++ standard, placement new operator simply returns back the passed-in pointer.
void* operator new(size_t, void* pMemory, DevDriver::PlacementNewDummy) noexcept
{
    DD_ASSERT(pMemory != nullptr);
    return pMemory;
}

// This should never be called.
void operator delete(void*, void*, DevDriver::PlacementNewDummy) noexcept
{
    DD_ALWAYS_ASSERT(false);
}
