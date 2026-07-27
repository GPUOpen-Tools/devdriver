/* Copyright (C) 2022-2025 Advanced Micro Devices, Inc. All rights reserved. */

#include <g_DriverSiphonService.h>

namespace DriverSiphon
{

static DD_RESULT RegisterFunctions(
    DDRpcServer hServer,
    IDriverSiphonService* pService)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    // Register "QuerySettingsBlobsAll"
    if (result == DD_RESULT_SUCCESS)
    {
        DDRpcServerRegisterFunctionInfo info = {};
        info.serviceId                       = 0x5aa7a7be;
        info.id                              = 0x1;
        info.pName                           = "QuerySettingsBlobsAll";
        info.pDescription                    = "Queries all Settings blobs from client drivers.";
        info.pFuncUserdata                   = pService;
        info.pfnFuncCb                       = [](
            const DDRpcServerCallInfo* pCall) -> DD_RESULT
        {
            auto* pService = reinterpret_cast<IDriverSiphonService*>(pCall->pUserdata);

            // Execute the service implementation
            return pService->QuerySettingsBlobsAll(pCall->pParameterData, pCall->parameterDataSize, *pCall->pWriter);
        };

        result = ddRpcServerRegisterFunction(hServer, &info);
    }

    // Register "QuerySettingsRegistryOverrides"
    if (result == DD_RESULT_SUCCESS)
    {
        DDRpcServerRegisterFunctionInfo info = {};
        info.serviceId                       = 0x5aa7a7be;
        info.id                              = 0x2;
        info.pName                           = "QuerySettingsRegistryOverrides";
        info.pDescription                    = "Queries all the driver settings in the registry.";
        info.pFuncUserdata                   = pService;
        info.pfnFuncCb                       = [](
            const DDRpcServerCallInfo* pCall) -> DD_RESULT
        {
            auto* pService = reinterpret_cast<IDriverSiphonService*>(pCall->pUserdata);

            // Execute the service implementation
            return pService->QuerySettingsRegistryOverrides(pCall->pParameterData, pCall->parameterDataSize, *pCall->pWriter);
        };

        result = ddRpcServerRegisterFunction(hServer, &info);
    }

    // Register "ClearSettingsRegistryOverride"
    if (result == DD_RESULT_SUCCESS)
    {
        DDRpcServerRegisterFunctionInfo info = {};
        info.serviceId                       = 0x5aa7a7be;
        info.id                              = 0x3;
        info.pName                           = "ClearSettingsRegistryOverride";
        info.pDescription                    = "Clears a setting override.";
        info.pFuncUserdata                   = pService;
        info.pfnFuncCb                       = [](
            const DDRpcServerCallInfo* pCall) -> DD_RESULT
        {
            auto* pService = reinterpret_cast<IDriverSiphonService*>(pCall->pUserdata);

            // Execute the service implementation
            return pService->ClearSettingsRegistryOverride(pCall->pParameterData, pCall->parameterDataSize);
        };

        result = ddRpcServerRegisterFunction(hServer, &info);
    }

    // Register "WriteKernelSettingOverride"
    if (result == DD_RESULT_SUCCESS)
    {
        DDRpcServerRegisterFunctionInfo info = {};
        info.serviceId                       = 0x5aa7a7be;
        info.id                              = 0x4;
        info.pName                           = "WriteKernelSettingOverride";
        info.pDescription                    = "Writes a KMD registry override for a single setting on a specific GPU.";
        info.pFuncUserdata                   = pService;
        info.pfnFuncCb                       = [](
            const DDRpcServerCallInfo* pCall) -> DD_RESULT
        {
            auto* pService = reinterpret_cast<IDriverSiphonService*>(pCall->pUserdata);

            // Execute the service implementation
            return pService->WriteKernelSettingOverride(pCall->pParameterData, pCall->parameterDataSize);
        };

        result = ddRpcServerRegisterFunction(hServer, &info);
    }

    // Register "TriggerKernelPnpReload"
    if (result == DD_RESULT_SUCCESS)
    {
        DDRpcServerRegisterFunctionInfo info = {};
        info.serviceId                       = 0x5aa7a7be;
        info.id                              = 0x5;
        info.pName                           = "TriggerKernelPnpReload";
        info.pDescription                    = "Performs a PnP disable/re-enable cycle on a GPU to reload the KMD.";
        info.pFuncUserdata                   = pService;
        info.pfnFuncCb                       = [](
            const DDRpcServerCallInfo* pCall) -> DD_RESULT
        {
            auto* pService = reinterpret_cast<IDriverSiphonService*>(pCall->pUserdata);

            // Execute the service implementation
            return pService->TriggerKernelPnpReload(pCall->pParameterData, pCall->parameterDataSize);
        };

        result = ddRpcServerRegisterFunction(hServer, &info);
    }

    return result;
}

DD_RESULT RegisterService(
    DDRpcServer hServer,
    IDriverSiphonService* pService
)
{
    DDRpcServerRegisterServiceInfo info = {};
    info.id                             = 0x5aa7a7be;
    info.version.major                  = 1;
    info.version.minor                  = 2;
    info.version.patch                  = 0;
    info.pName                          = "DriverSiphon";
    info.pDescription                   = "A service that provides support for siphoning data out of client drivers.";

    // Register the service
    DD_RESULT result = ddRpcServerRegisterService(hServer, &info);

    // Register individual functions
    if (result == DD_RESULT_SUCCESS)
    {
        result = RegisterFunctions(hServer, pService);

        if (result != DD_RESULT_SUCCESS)
        {
            // Unregister the service if registering functions fails
            ddRpcServerUnregisterService(hServer, info.id);
        }
    }

    return result;
}

void UnRegisterService(DDRpcServer hServer)
{
    DDRpcServerRegisterServiceInfo info = {};
    info.id                             = 0x5aa7a7be;
    info.version.major                  = 1;
    info.version.minor                  = 2;
    info.version.patch                  = 0;
    info.pName                          = "DriverSiphon";
    info.pDescription                   = "A service that provides support for siphoning data out of client drivers.";

    // Unregister the service if registering functions fails
    ddRpcServerUnregisterService(hServer, info.id);
}

} // namespace DriverSiphon
