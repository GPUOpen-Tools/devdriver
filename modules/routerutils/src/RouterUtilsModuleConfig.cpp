/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "RouterUtilsModuleConnectionContext.h"
#include <BaseModuleClientContext.h>

namespace RouterUtilsModule
{

DDModuleNativeApi QueryNativeApi()
{
    // No native API support
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const DDModuleExtensionInterface* QueryExtension(DDModuleExtensionId id)
{
    DD_API_UNUSED(id);

    // No extension support
    return nullptr;
}

};  // namespace RouterUtilsModule
