/* Copyright (C) 2023-2024 Advanced Micro Devices, Inc. All rights reserved. */

#include <g_RouterUtilsRpcClient.h>
#include <ddCommon.h>

using namespace RouterUtilsRpc;

RouterUtilsRpcClient::RouterUtilsRpcClient()
{
}

DD_RESULT RouterUtilsRpcClient::Connect(const DDRpcClientCreateInfo& info)
{
    return ddRpcClientCreate(&info, &m_hClient);
}

DD_RESULT RouterUtilsRpcClient::IsServiceAvailable()
{
    DDApiVersion version = {};
    DD_RESULT result = ddRpcClientGetServiceInfo(m_hClient, 0x70f462cb, &version);

    if (result == DD_RESULT_SUCCESS)
    {
        DDApiVersion serviceVersion = {};
        serviceVersion.major        = 0;
        serviceVersion.minor        = 1;
        serviceVersion.patch        = 0;

        result = ddIsVersionCompatible(version, serviceVersion) ?
            DD_RESULT_SUCCESS : DD_RESULT_COMMON_VERSION_MISMATCH;
    }

    return result;
}

DD_RESULT RouterUtilsRpcClient::GetServiceInfo(DDApiVersion* pVersion)
{
    return ddRpcClientGetServiceInfo(m_hClient, 0x70f462cb, pVersion);
}

RouterUtilsRpcClient::~RouterUtilsRpcClient() { ddRpcClientDestroy(m_hClient); }

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RouterUtilsRpcClient::QuerySystemInfo(
    const DDByteWriter& writer
)
{
    // No parameter
    const void* pParamBuffer     = nullptr;
    const size_t paramBufferSize = 0;

    const DDByteWriter* pResponseWriter = &writer;

    DDRpcClientCallInfo info  = {};
    info.service              = 0x70f462cb;
    info.serviceVersion.major = 0;
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
DD_RESULT RouterUtilsRpcClient::QueryPathByProcessId(
    const void*         pParamBuffer,
    size_t              paramBufferSize,
    const DDByteWriter& writer
)
{
    const DDByteWriter* pResponseWriter = &writer;

    DDRpcClientCallInfo info  = {};
    info.service              = 0x70f462cb;
    info.serviceVersion.major = 0;
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
DD_RESULT RouterUtilsRpcClient::QueryTimestampAndFrequency(
    const DDByteWriter& writer
)
{
    // No parameter
    const void* pParamBuffer     = nullptr;
    const size_t paramBufferSize = 0;

    const DDByteWriter* pResponseWriter = &writer;

    DDRpcClientCallInfo info  = {};
    info.service              = 0x70f462cb;
    info.serviceVersion.major = 0;
    info.serviceVersion.minor = 1;
    info.serviceVersion.patch = 0;
    info.function             = 0x3;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RouterUtilsRpcClient::QueryDeviceClocks(
    const void*         pParamBuffer,
    size_t              paramBufferSize,
    const DDByteWriter& writer
)
{
    const DDByteWriter* pResponseWriter = &writer;

    DDRpcClientCallInfo info  = {};
    info.service              = 0x70f462cb;
    info.serviceVersion.major = 0;
    info.serviceVersion.minor = 1;
    info.serviceVersion.patch = 0;
    info.function             = 0x4;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RouterUtilsRpcClient::QueryCurrentClockMode(
    const void*         pParamBuffer,
    size_t              paramBufferSize,
    const DDByteWriter& writer
)
{
    const DDByteWriter* pResponseWriter = &writer;

    DDRpcClientCallInfo info  = {};
    info.service              = 0x70f462cb;
    info.serviceVersion.major = 0;
    info.serviceVersion.minor = 1;
    info.serviceVersion.patch = 0;
    info.function             = 0x5;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RouterUtilsRpcClient::SetClockMode(
    const void* pParamBuffer,
    size_t      paramBufferSize
)
{
    // No return
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x70f462cb;
    info.serviceVersion.major = 0;
    info.serviceVersion.minor = 1;
    info.serviceVersion.patch = 0;
    info.function             = 0x6;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}
