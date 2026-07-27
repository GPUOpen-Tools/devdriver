/* Copyright (C) 2022-2024 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddRpcClient.h>

namespace Info
{

class InfoClient
{
public:
    InfoClient();
    ~InfoClient();

    DD_RESULT Connect(const DDRpcClientCreateInfo& info);
    DD_RESULT IsServiceAvailable();
    DD_RESULT GetServiceInfo(DDApiVersion* pVersion);

    /// Queries the list of all available info sources
    DD_RESULT QuerySources(
        const DDByteWriter& writer
    );

    /// Queries information from a specific info source
    DD_RESULT QueryInfo(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer
    );

    /// Queries information from all available info sources
    DD_RESULT QueryInfoAll(
        const DDByteWriter& writer
    );

    /// Queries the path of an application by its process id. The returned path string is UTF-8 encoded, and doesn't end with null-terminator.
    DD_RESULT QueryPathByProcessId(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer
    );

private:
    DDRpcClient m_hClient = DD_API_INVALID_HANDLE;
};

} // namespace Info
