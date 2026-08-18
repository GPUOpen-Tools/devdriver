/* Copyright (C) 2021-2024 Advanced Micro Devices, Inc. All rights reserved. */

#include <g_SettingsRpcClient.h>
#include <ddCommon.h>

using namespace SettingsRpc;

SettingsRpcClient::SettingsRpcClient()
{
}

DD_RESULT SettingsRpcClient::Connect(const DDRpcClientCreateInfo& info)
{
    return ddRpcClientCreate(&info, &m_hClient);
}

DD_RESULT SettingsRpcClient::IsServiceAvailable()
{
    DDApiVersion version = {};
    DD_RESULT result = ddRpcClientGetServiceInfo(m_hClient, 0x15375127, &version);

    if (result == DD_RESULT_SUCCESS)
    {
        DDApiVersion serviceVersion = {};
        serviceVersion.major        = 2;
        serviceVersion.minor        = 1;
        serviceVersion.patch        = 0;

        result = ddIsVersionCompatible(version, serviceVersion) ?
            DD_RESULT_SUCCESS : DD_RESULT_COMMON_VERSION_MISMATCH;
    }

    return result;
}

DD_RESULT SettingsRpcClient::GetServiceInfo(DDApiVersion* pVersion)
{
    return ddRpcClientGetServiceInfo(m_hClient, 0x15375127, pVersion);
}

SettingsRpcClient::~SettingsRpcClient() { ddRpcClientDestroy(m_hClient); }

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT SettingsRpcClient::SendAllUserOverrides(
    const void* pParamBuffer,
    size_t      paramBufferSize
)
{
    // No return
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x15375127;
    info.serviceVersion.major = 2;
    info.serviceVersion.minor = 1;
    info.serviceVersion.patch = 0;
    info.function             = 0x1;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT SettingsRpcClient::QueryAllCurrentValues(
    const DDByteWriter& writer
)
{
    // No parameter
    const void* pParamBuffer     = nullptr;
    const size_t paramBufferSize = 0;

    const DDByteWriter* pResponseWriter = &writer;

    DDRpcClientCallInfo info  = {};
    info.service              = 0x15375127;
    info.serviceVersion.major = 2;
    info.serviceVersion.minor = 1;
    info.serviceVersion.patch = 0;
    info.function             = 0x2;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT SettingsRpcClient::GetUnsupportedExperiments(
    const DDByteWriter& writer
)
{
    // No parameter
    const void* pParamBuffer     = nullptr;
    const size_t paramBufferSize = 0;

    const DDByteWriter* pResponseWriter = &writer;

    DDRpcClientCallInfo info  = {};
    info.service              = 0x15375127;
    info.serviceVersion.major = 2;
    info.serviceVersion.minor = 1;
    info.serviceVersion.patch = 0;
    info.function             = 0x3;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}
