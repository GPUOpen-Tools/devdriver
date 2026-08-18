/* Copyright (C) 2022-2025 Advanced Micro Devices, Inc. All rights reserved. */

#include <g_DriverSiphonClient.h>
#include <ddCommon.h>

using namespace DriverSiphon;

DriverSiphonClient::DriverSiphonClient()
{
}

DD_RESULT DriverSiphonClient::Connect(const DDRpcClientCreateInfo& info)
{
    return ddRpcClientCreate(&info, &m_hClient);
}

DD_RESULT DriverSiphonClient::IsServiceAvailable()
{
    DDApiVersion version = {};
    DD_RESULT result = ddRpcClientGetServiceInfo(m_hClient, 0x5aa7a7be, &version);

    if (result == DD_RESULT_SUCCESS)
    {
        DDApiVersion serviceVersion = {};
        serviceVersion.major        = 1;
        serviceVersion.minor        = 2;
        serviceVersion.patch        = 0;

        result = ddIsVersionCompatible(version, serviceVersion) ?
            DD_RESULT_SUCCESS : DD_RESULT_COMMON_VERSION_MISMATCH;
    }

    return result;
}

DD_RESULT DriverSiphonClient::GetServiceInfo(DDApiVersion* pVersion)
{
    return ddRpcClientGetServiceInfo(m_hClient, 0x5aa7a7be, pVersion);
}

DriverSiphonClient::~DriverSiphonClient() { ddRpcClientDestroy(m_hClient); }

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT DriverSiphonClient::QuerySettingsBlobsAll(
    const void*         pParamBuffer,
    size_t              paramBufferSize,
    const DDByteWriter& writer
)
{
    const DDByteWriter* pResponseWriter = &writer;

    DDRpcClientCallInfo info  = {};
    info.service              = 0x5aa7a7be;
    info.serviceVersion.major = 1;
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
DD_RESULT DriverSiphonClient::QuerySettingsRegistryOverrides(
    const void*         pParamBuffer,
    size_t              paramBufferSize,
    const DDByteWriter& writer
)
{
    const DDByteWriter* pResponseWriter = &writer;

    DDRpcClientCallInfo info  = {};
    info.service              = 0x5aa7a7be;
    info.serviceVersion.major = 1;
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
DD_RESULT DriverSiphonClient::ClearSettingsRegistryOverride(
    const void* pParamBuffer,
    size_t      paramBufferSize
)
{
    // No return
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x5aa7a7be;
    info.serviceVersion.major = 1;
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
DD_RESULT DriverSiphonClient::WriteKernelSettingOverride(
    const void* pParamBuffer,
    size_t      paramBufferSize
)
{
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x5aa7a7be;
    info.serviceVersion.major = 1;
    info.serviceVersion.minor = 2;
    info.serviceVersion.patch = 0;
    info.function             = 0x4;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;

    return ddRpcClientCall(m_hClient, &info);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT DriverSiphonClient::TriggerKernelPnpReload(
    const void* pParamBuffer,
    size_t      paramBufferSize
)
{
    EmptyByteWriter<DD_RESULT_DD_RPC_FUNC_RESPONSE_REJECTED> writer;
    const DDByteWriter* pResponseWriter = writer.Writer();

    DDRpcClientCallInfo info  = {};
    info.service              = 0x5aa7a7be;
    info.serviceVersion.major = 1;
    info.serviceVersion.minor = 2;
    info.serviceVersion.patch = 0;
    info.function             = 0x5;
    info.pParamBuffer         = pParamBuffer;
    info.paramBufferSize      = paramBufferSize;
    info.pResponseWriter      = pResponseWriter;
    info.receiveTimeoutMillis = 500;

    return ddRpcClientCall(m_hClient, &info);
}
