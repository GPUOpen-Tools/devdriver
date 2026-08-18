/* Copyright (C) 2021-2024 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddRpcServer.h>

namespace SettingsRpc
{

class ISettingsRpcService
{
public:
    virtual ~ISettingsRpcService() {}

    // Send user overrides of all components to the driver.
    virtual DD_RESULT SendAllUserOverrides(
        const void* pParamBuffer,
        size_t      paramBufferSize
    ) = 0;

    // Query current setting values of all components from the driver.
    virtual DD_RESULT QueryAllCurrentValues(
        const DDByteWriter& writer
    ) = 0;

    // Query currently unsupported experiments for all components from the driver.
    virtual DD_RESULT GetUnsupportedExperiments(
        const DDByteWriter& writer
    ) = 0;

protected:
    ISettingsRpcService() {}
};

DD_RESULT RegisterService(DDRpcServer hServer, ISettingsRpcService* pService);

void UnRegisterService(DDRpcServer hServer);

} // namespace SettingsRpc
