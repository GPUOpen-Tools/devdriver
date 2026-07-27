/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <util/vector.h>

namespace DevDriver
{

template <>
bool Vector<char>::Append(const char* pStr)
{
    return Append(pStr, Platform::Strlen_s(pStr, SIZE_MAX));
}

} // DevDriver
