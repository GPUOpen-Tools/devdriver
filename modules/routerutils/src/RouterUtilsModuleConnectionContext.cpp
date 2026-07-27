/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "RouterUtilsModuleConnectionContext.h"

namespace RouterUtilsModule
{

ConnectionContext::ConnectionContext(const DDModuleConnectionContextCreateInfo& create_info)
    : BaseModuleConnectionContext(create_info)
    , m_routerUtilsService(create_info.loader.logger)
{
}

DD_RESULT ConnectionContext::Initialize()
{
    return RouterUtilsRpc::RegisterService(m_createInfo.hRpcServer, &m_routerUtilsService);
}

};  // namespace RouterUtilsModule
