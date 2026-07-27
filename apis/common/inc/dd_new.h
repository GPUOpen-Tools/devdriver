/* Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <cstddef>
#include <cstdio>

namespace DevDriver
{

struct PlacementNewDummy
{
    explicit PlacementNewDummy () {}
};

} // namespace DevDriver

// Placement new operator overload. C++ standard doesn't allow overloading of the placement new operator with
// the default signature, but we can overload it with a different signature, thus PlacementNewDummy.
void* operator new(std::size_t size, void* pMemory, DevDriver::PlacementNewDummy dummy) noexcept;

// This delete operator should never be called. It's defined here for pure symmetry purpose (some compilers
// also require a matching delete operator).
//
// For objects that are constructed via the placement new operator, no delete operator should be called on
// them, because the delete operator wouldn't know how to de-allocate the memory. Instead, Objects' destructor
// should be called manually: `pObject->~Object();`.
void operator delete(void*, void*, DevDriver::PlacementNewDummy) noexcept;

namespace DevDriver
{

// A helper to construct `count` number of objects already allocated in memory pointed to by `pMemory`.
template<typename T, typename... Ps>
void PlaceNew(std::size_t count, T* pMemory, Ps... args)
{
    T* pObject = pMemory;
    for (std::size_t i = 0; i < count; ++i)
    {
        new(pObject, DevDriver::PlacementNewDummy{}) T(args...);
        pObject += 1;
    }
}

// A helper to destruct `count` number of objects residing in the memory pointed by `pMemory`.
template<typename T>
void PlaceDelete(std::size_t count, T* pMemory)
{
    T* pObject = pMemory;
    for (std::size_t i = 0; i < count; ++i)
    {
        pObject->~T();
        pObject += 1;
    }
}

} // namespace DevDriver

