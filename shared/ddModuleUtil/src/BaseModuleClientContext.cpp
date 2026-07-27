/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <BaseModuleClientContext.h>

#include <gpuopen.h>
#include <ddPlatform.h>

#include <g_InfoClient.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BaseModuleClientContext::BaseModuleClientContext(
    const DDModuleClientContextCreateInfo& createInfo)
    : m_createInfo(createInfo)
    , m_logger(createInfo.loader)
    , m_moduleStatus(DD_RESULT_UNKNOWN)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BaseModuleClientContext::~BaseModuleClientContext()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleClientContext::Initialize()
{
    m_moduleStatus = DD_RESULT_SUCCESS;

    return m_moduleStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BaseModuleClientContext::HandleStateChanged(
    const DDModuleClientEventStateChanged& eventData)
{
    // Do nothing by default
    DD_API_UNUSED(eventData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleClientContext::QuerySystemInfo(
    void*             pUserdata,
    PFN_ddReceiveText pfnReceiveJson)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if (pfnReceiveJson != nullptr)
    {
        Info::InfoClient client;

        DDRpcClientCreateInfo clientInfo = {};
        clientInfo.clientId = m_createInfo.systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_ROUTER].id;
        clientInfo.hConnection = m_createInfo.connection;

        result = client.Connect(clientInfo);

        if (result == DD_RESULT_SUCCESS)
        {
            DynamicBufferByteWriter dynamicWriter;
            result = client.QueryInfoAll(*dynamicWriter.Writer());
            if (result == DD_RESULT_SUCCESS)
            {
                const char* pJson = dynamicWriter.DataAsString();
                if (pJson != nullptr)
                {
                    pfnReceiveJson(pUserdata, pJson);
                }
                else
                {
                    result = DD_RESULT_PARSING_INVALID_JSON;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleClientContext::QuerySystemInfo(
    DDModuleClientContext hClientContext,
    void*                 pUserdata,
    PFN_ddReceiveText     pfnReceiveJson)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if ((hClientContext != DD_API_INVALID_HANDLE) && (pfnReceiveJson != nullptr))
    {
        BaseModuleClientContext* pContext = reinterpret_cast<BaseModuleClientContext*>(hClientContext);
        result = pContext->QuerySystemInfo(pUserdata, pfnReceiveJson);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleClientContext::QueryStatus(
    DDModuleClientContext hClientContext)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if (hClientContext != DD_API_INVALID_HANDLE)
    {
        BaseModuleClientContext* pContext = reinterpret_cast<BaseModuleClientContext*>(hClientContext);
        result = pContext->QueryStatus();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleClientContext::QueryClientProtocolVersion(
    DDModuleClientContext hClientContext,
    DDApiVersion*         pVersion)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if (hClientContext != DD_API_INVALID_HANDLE)
    {
        BaseModuleClientContext* pContext = reinterpret_cast<BaseModuleClientContext*>(hClientContext);
        result = pContext->QueryClientProtocolVersion(pVersion);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleClientContext::QueryClientProtocolVersion(
    DDApiVersion* pVersion)
{
    DD_API_UNUSED(pVersion);

    // The default implementation here does not provide a version.

    return DD_RESULT_COMMON_UNIMPLEMENTED;
}
