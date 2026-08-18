/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include "RouterUtilsService.h"
#include <BaseModuleConnectionContext.h>

namespace RouterUtilsModule
{

class ConnectionContext : public BaseModuleConnectionContext
{
public:
    explicit ConnectionContext(const DDModuleConnectionContextCreateInfo& create_info);
    ~ConnectionContext() override = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////// Base Class Overrides ///////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    DD_RESULT Initialize() override;

private:
    RouterUtilsService m_routerUtilsService;
};

};  // namespace RouterUtilsModule
