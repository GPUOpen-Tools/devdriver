/* Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <ddApi.h>
#include <ddDefs.h>
#include <ddModule.h>

namespace SiphonModule
{

// ============================================================================
DDModuleNativeApi QueryNativeApi()
{
    return DDModuleNativeApi{};
}

// ============================================================================
const DDModuleExtensionInterface* QueryExtension(
    DDModuleExtensionId id)
{
    // No mercury extension
    DD_UNUSED(id);
    return nullptr;
}

} // namespace SiphonModule
