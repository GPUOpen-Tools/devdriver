/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddModule.h>
#include <ModuleLogger.h>

class BaseModuleDataContext;

/// Class used to encapsulate the module specific per client state
class BaseModuleClientContext
{
public:
    /// Returns the loader interface used to create this object
    const DDModuleLoaderInterface& GetLoader() const { return m_createInfo.loader; }

    /// Attempts to initialize this client context so it can communicate with the client provided in the create info
    virtual DD_RESULT Initialize();

    /// Performs any required processing when the remote client state changes
    virtual void HandleStateChanged(
        const DDModuleClientEventStateChanged& eventData); /// Information about the state change

    static DD_RESULT QuerySystemInfo(
        DDModuleClientContext hClientContext,
        void*                 pUserdata,
        PFN_ddReceiveText     pfnReceiveJson);

    static DD_RESULT QueryStatus(
        DDModuleClientContext hClientContext);

    static DD_RESULT QueryClientProtocolVersion(
        DDModuleClientContext hClientContext,
        DDApiVersion*         pVersion);

protected:
    BaseModuleClientContext(const DDModuleClientContextCreateInfo& createInfo);
    virtual ~BaseModuleClientContext();

    /// Queries per-system information from the remote system
    DD_RESULT QuerySystemInfo(
        void*             pUserdata,
        PFN_ddReceiveText pfnReceiveJson);

    DD_RESULT QueryStatus() const { return m_moduleStatus; }

    virtual DD_RESULT QueryClientProtocolVersion(DDApiVersion* pVersion);

    DDModuleClientContextCreateInfo m_createInfo;
    ModuleLogger                    m_logger;
    DD_RESULT                       m_moduleStatus;
};
