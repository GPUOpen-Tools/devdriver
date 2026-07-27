/* Copyright (C) 2021-2024 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddRpcClient.h>

namespace SettingsRpc
{

class SettingsRpcClient
{
public:
    SettingsRpcClient();
    ~SettingsRpcClient();

    DD_RESULT Connect(const DDRpcClientCreateInfo& info);
    DD_RESULT IsServiceAvailable();
    DD_RESULT GetServiceInfo(DDApiVersion* pVersion);

    /// Send user overrides of all components to the driver.
    DD_RESULT SendAllUserOverrides(
        const void* pParamBuffer,
        size_t      paramBufferSize
    );

    /// Query current setting values of all components from the driver.
    DD_RESULT QueryAllCurrentValues(
        const DDByteWriter& writer
    );

    /// Query currently unsupported experiments for all components from the driver.
    DD_RESULT GetUnsupportedExperiments(
        const DDByteWriter& writer
    );

private:
    DDRpcClient m_hClient = DD_API_INVALID_HANDLE;
};

} // namespace SettingsRpc
