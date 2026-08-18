/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddModule.h>
#include <ModuleLogger.h>

/// Class used to encapsulate the module specific per Connection state
class BaseModuleConnectionContext
{
public:
    /// Returns the loader interface used to create this object
    const DDModuleLoaderInterface& GetLoader() const { return m_createInfo.loader; }

    /// Attempts to initialize this Connection context so it can communicate with the Connection provided in the create info
    virtual DD_RESULT Initialize() = 0;

protected:
    BaseModuleConnectionContext(const DDModuleConnectionContextCreateInfo& createInfo);
    virtual ~BaseModuleConnectionContext();

    DDModuleConnectionContextCreateInfo m_createInfo;
    ModuleLogger                        m_logger;
};
