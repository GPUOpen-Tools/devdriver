/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <BaseModuleSystemContext.h>
#include <g_InfoClient.h>

// =======================================================================================
BaseModuleSystemContext::BaseModuleSystemContext(
    const DDModuleSystemContextCreateInfo& createInfo)
    : m_createInfo(createInfo)
    , m_logger(createInfo.loader)
{
}

// =======================================================================================
BaseModuleSystemContext::~BaseModuleSystemContext()
{
}

// =======================================================================================
DD_RESULT BaseModuleSystemContext::Initialize()
{
    // Return success by default
    return DD_RESULT_SUCCESS;
}

// =======================================================================================
void BaseModuleSystemContext::HandleEvent(
    DD_MODULE_SYSTEM_EVENT eventId,
    const void*            pEventData,
    size_t                 eventDataSize)
{
    // Do nothing by default
    DD_API_UNUSED(eventId);
    DD_API_UNUSED(pEventData);
    DD_API_UNUSED(eventDataSize);
}

// =======================================================================================
DD_RESULT BaseModuleSystemContext::QuerySystemInfo(
    DDModuleSystemContext hSystemContext,
    void*                 pUserdata,
    PFN_ddReceiveText     pfnReceiveJson)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if ((hSystemContext != DD_API_INVALID_HANDLE) && (pfnReceiveJson != nullptr))
    {
        BaseModuleSystemContext* pContext = reinterpret_cast<BaseModuleSystemContext*>(hSystemContext);
        result = pContext->QuerySystemInfoImpl(pUserdata, pfnReceiveJson);
    }

    return result;
}

// =======================================================================================
DD_RESULT BaseModuleSystemContext::QuerySystemInfoImpl(
    void* pUserdata,
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
