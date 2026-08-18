/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <UberTraceClient.h>
#include <ddCommon.h>

using namespace UberTrace;

UberTraceClient::UberTraceClient()
{
}

DD_RESULT UberTraceClient::Connect(const DDRpcClientCreateInfo& info)
{
    return ddRpcClientCreate(&info, &m_hClient);
}

UberTraceClient::~UberTraceClient() { ddRpcClientDestroy(m_hClient); }

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT UberTraceClient::EnableTracing()
{
    // No parameter
    const void* pParamBuffer     = nullptr;
    const size_t paramBufferSize = 0;

    // No return
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x63727461;
    info.serviceVersion.major = 0;
    info.serviceVersion.minor = 2;
    info.serviceVersion.patch = 0;
    info.function             = 0x1;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT UberTraceClient::QueryTraceParams(
    const DDByteWriter& writer
)
{
    // No parameter
    const void* pParamBuffer     = nullptr;
    const size_t paramBufferSize = 0;

    const DDByteWriter* pResponseWriter = &writer;

    DDRpcClientCallInfo info  = {};
    info.service              = 0x63727461;
    info.serviceVersion.major = 0;
    info.serviceVersion.minor = 2;
    info.serviceVersion.patch = 0;
    info.function             = 0x2;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT UberTraceClient::ConfigureTraceParams(
    const void* pParamBuffer,
    size_t      paramBufferSize
)
{
    // No return
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x63727461;
    info.serviceVersion.major = 0;
    info.serviceVersion.minor = 2;
    info.serviceVersion.patch = 0;
    info.function             = 0x3;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT UberTraceClient::RequestTrace()
{
    // No parameter
    const void* pParamBuffer     = nullptr;
    const size_t paramBufferSize = 0;

    // No return
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x63727461;
    info.serviceVersion.major = 0;
    info.serviceVersion.minor = 2;
    info.serviceVersion.patch = 0;
    info.function             = 0x4;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT UberTraceClient::CancelTrace()
{
    // No parameter
    const void* pParamBuffer     = nullptr;
    const size_t paramBufferSize = 0;

    // No return
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x63727461;
    info.serviceVersion.major = 0;
    info.serviceVersion.minor = 2;
    info.serviceVersion.patch = 0;
    info.function             = 0x5;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT UberTraceClient::CollectTrace(
    const DDByteWriter& writer
)
{
    // No parameter
    const void* pParamBuffer     = nullptr;
    const size_t paramBufferSize = 0;

    const DDByteWriter* pResponseWriter = &writer;

    DDRpcClientCallInfo info  = {};
    info.service              = 0x63727461;
    info.serviceVersion.major = 0;
    info.serviceVersion.minor = 2;
    info.serviceVersion.patch = 0;
    info.function             = 0x6;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    const DD_RESULT result = ddRpcClientCall(m_hClient, &info);

    return result;
}
