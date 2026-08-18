/* Copyright (C) 2022-2024 Advanced Micro Devices, Inc. All rights reserved. */

#include <g_DriverUtilsClient.h>
#include <ddCommon.h>

using namespace DriverUtils;

DriverUtilsClient::DriverUtilsClient()
{
}

DD_RESULT DriverUtilsClient::Connect(const DDRpcClientCreateInfo& info)
{
    return ddRpcClientCreate(&info, &m_hClient);
}

DD_RESULT DriverUtilsClient::IsServiceAvailable()
{
    DDApiVersion version = {};
    DD_RESULT result = ddRpcClientGetServiceInfo(m_hClient, 0x24815012, &version);

    if (result == DD_RESULT_SUCCESS)
    {
        DDApiVersion serviceVersion = {};
        serviceVersion.major        = 1;
        serviceVersion.minor        = 3;
        serviceVersion.patch        = 0;

        result = ddIsVersionCompatible(version, serviceVersion) ?
            DD_RESULT_SUCCESS : DD_RESULT_COMMON_VERSION_MISMATCH;
    }

    return result;
}

DD_RESULT DriverUtilsClient::GetServiceInfo(DDApiVersion* pVersion)
{
    return ddRpcClientGetServiceInfo(m_hClient, 0x24815012, pVersion);
}

DriverUtilsClient::~DriverUtilsClient() { ddRpcClientDestroy(m_hClient); }

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT DriverUtilsClient::EnableTracing()
{
    // No parameter
    const void* pParamBuffer     = nullptr;
    const size_t paramBufferSize = 0;

    // No return
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x24815012;
    info.serviceVersion.major = 1;
    info.serviceVersion.minor = 3;
    info.serviceVersion.patch = 0;
    info.function             = 0x1;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT DriverUtilsClient::EnableCrashAnalysisMode()
{
    // No parameter
    const void* pParamBuffer     = nullptr;
    const size_t paramBufferSize = 0;

    // No return
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x24815012;
    info.serviceVersion.major = 1;
    info.serviceVersion.minor = 3;
    info.serviceVersion.patch = 0;
    info.function             = 0x2;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT DriverUtilsClient::QueryPalDriverInfo(
    const DDByteWriter& writer
)
{
    // No parameter
    const void* pParamBuffer     = nullptr;
    const size_t paramBufferSize = 0;

    const DDByteWriter* pResponseWriter = &writer;

    DDRpcClientCallInfo info  = {};
    info.service              = 0x24815012;
    info.serviceVersion.major = 1;
    info.serviceVersion.minor = 3;
    info.serviceVersion.patch = 0;
    info.function             = 0x3;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT DriverUtilsClient::EnableDriverFeatures(
    const void* pParamBuffer,
    size_t      paramBufferSize
)
{
    // No return
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x24815012;
    info.serviceVersion.major = 1;
    info.serviceVersion.minor = 3;
    info.serviceVersion.patch = 0;
    info.function             = 0x4;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT DriverUtilsClient::SetOverlayString(
    const void* pParamBuffer,
    size_t      paramBufferSize
)
{
    // No return
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x24815012;
    info.serviceVersion.major = 1;
    info.serviceVersion.minor = 3;
    info.serviceVersion.patch = 0;
    info.function             = 0x5;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT DriverUtilsClient::SetDbgLogSeverityLevel(
    const void* pParamBuffer,
    size_t      paramBufferSize
)
{
    // No return
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x24815012;
    info.serviceVersion.major = 1;
    info.serviceVersion.minor = 3;
    info.serviceVersion.patch = 0;
    info.function             = 0x6;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT DriverUtilsClient::SetDbgLogOriginationMask(
    const void* pParamBuffer,
    size_t      paramBufferSize
)
{
    // No return
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x24815012;
    info.serviceVersion.major = 1;
    info.serviceVersion.minor = 3;
    info.serviceVersion.patch = 0;
    info.function             = 0x7;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT DriverUtilsClient::ModifyDbgLogOriginationMask(
    const void* pParamBuffer,
    size_t      paramBufferSize
)
{
    // No return
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x24815012;
    info.serviceVersion.major = 1;
    info.serviceVersion.minor = 3;
    info.serviceVersion.patch = 0;
    info.function             = 0x8;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}
