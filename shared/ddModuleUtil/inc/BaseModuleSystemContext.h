/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddModule.h>
#include <ModuleLogger.h>

/// Class used to encapsulate the module specific per system state
class BaseModuleSystemContext
{
public:
    /// Returns the loader interface used to create this object
    const DDModuleLoaderInterface& GetLoader() const { return m_createInfo.loader; }

    /// Attempts to initialize this system context so it can communicate with the system provided in the create info
    virtual DD_RESULT Initialize();

    /// Performs any necessary processing on a system event
    virtual void HandleEvent(
        DD_MODULE_SYSTEM_EVENT eventId,
        const void*            pEventData,
        size_t                 eventDataSize);

    static DD_RESULT QuerySystemInfo(
        DDModuleSystemContext hSystemContext,
        void*                 pUserdata,
        PFN_ddReceiveText     pfnReceiveJson);

protected:
    BaseModuleSystemContext(const DDModuleSystemContextCreateInfo& createInfo);
    virtual ~BaseModuleSystemContext();

    DD_RESULT QuerySystemInfoImpl(void* pUserdata, PFN_ddReceiveText pfnReceiveJson);

    DDModuleSystemContextCreateInfo m_createInfo;
    ModuleLogger                    m_logger;
};
