/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <BaseModuleConnectionContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BaseModuleConnectionContext::BaseModuleConnectionContext(const DDModuleConnectionContextCreateInfo& createInfo)
    : m_createInfo(createInfo),
    m_logger(createInfo.loader)
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BaseModuleConnectionContext::~BaseModuleConnectionContext()
{
    // Nothing to do
}
