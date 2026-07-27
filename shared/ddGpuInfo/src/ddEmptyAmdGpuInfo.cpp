/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <ddAmdGpuInfo.h>

namespace DevDriver
{

// Provide an empty version of this function for configurations that haven't otherwise provided it
Result QueryGpuInfo(const AllocCb& allocCb, Vector<AmdGpuInfo>* pGpus)
{
    DD_UNUSED(allocCb);
    DD_UNUSED(pGpus);

    DD_PRINT(LogLevel::Error, "QueryGpuInfo() is not implemented for your platform/configuration.");

    return Result::Unavailable;
}

// Provide an empty version of this function for configurations that haven't otherwise provided it
std::wstring QueryServiceString()
{
    DD_PRINT(LogLevel::Error, "QueryServiceString() is not implemented for your platform/configuration.");
    return L"";
}

} // namespace DevDriver
