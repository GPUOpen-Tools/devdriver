/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <util/hashFunc.h>

namespace DevDriver
{

uint32 DefaultHashFunc<const char*>::operator()(const char* pKey) const
{
    // We cannot pass NULL strings to strlen() and friends, so guard against it anyway.
    uint32 hash = 0;
    DD_ASSERT(pKey != nullptr);
    if (pKey != nullptr)
    {
        hash = MetroHash::MetroHash32(reinterpret_cast<const uint8*>(pKey), Platform::Strlen_s(pKey, SIZE_MAX));
    }

    return hash;
}

bool DefaultEqualFunc<const char*>::operator()(const char* pKey1, const char* pKey2) const
{
    DD_ASSERT(pKey1 != nullptr);
    DD_ASSERT(pKey2 != nullptr);
    return (strcmp(pKey1, pKey2) == 0);
}

} // namespace DevDriver
