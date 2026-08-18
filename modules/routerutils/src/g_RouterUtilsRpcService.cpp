/* Copyright (C) 2023-2024 Advanced Micro Devices, Inc. All rights reserved. */

#include <g_RouterUtilsRpcService.h>

namespace RouterUtilsRpc
{

static DD_RESULT RegisterFunctions(
    DDRpcServer hServer,
    IRouterUtilsRpcService* pService)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    // Register "QuerySystemInfo"
    if (result == DD_RESULT_SUCCESS)
    {
        DDRpcServerRegisterFunctionInfo info = {};
        info.serviceId                       = 0x70f462cb;
        info.id                              = 0x1;
        info.pName                           = "QuerySystemInfo";
        info.pDescription                    = "Query the system info of the target machine. System info stays unchanged until the next system reboot.";
        info.pFuncUserdata                   = pService;
        info.pfnFuncCb                       = [](
            const DDRpcServerCallInfo* pCall) -> DD_RESULT
        {
            auto* pService = reinterpret_cast<IRouterUtilsRpcService*>(pCall->pUserdata);

            // Execute the service implementation
            return pService->QuerySystemInfo(*pCall->pWriter);
        };

        result = ddRpcServerRegisterFunction(hServer, &info);
    }

    // Register "QueryPathByProcessId"
    if (result == DD_RESULT_SUCCESS)
    {
        DDRpcServerRegisterFunctionInfo info = {};
        info.serviceId                       = 0x70f462cb;
        info.id                              = 0x2;
        info.pName                           = "QueryPathByProcessId";
        info.pDescription                    = "Queries the path of an application running on the target machine by its process id. The returned path string is UTF-8 encoded, and doesn't end with null-terminator.";
        info.pFuncUserdata                   = pService;
        info.pfnFuncCb                       = [](
            const DDRpcServerCallInfo* pCall) -> DD_RESULT
        {
            auto* pService = reinterpret_cast<IRouterUtilsRpcService*>(pCall->pUserdata);

            // Execute the service implementation
            return pService->QueryPathByProcessId(pCall->pParameterData, pCall->parameterDataSize, *pCall->pWriter);
        };

        result = ddRpcServerRegisterFunction(hServer, &info);
    }

    // Register "QueryTimestampAndFrequency"
    if (result == DD_RESULT_SUCCESS)
    {
        DDRpcServerRegisterFunctionInfo info = {};
        info.serviceId                       = 0x70f462cb;
        info.id                              = 0x3;
        info.pName                           = "QueryTimestampAndFrequency";
        info.pDescription                    = "Queries a time stamp and frequency on the target machine. Time stamp is a monotonically increasing value representing the number of ticks since the last machine boot. Frequency represents the number of ticks per second.";
        info.pFuncUserdata                   = pService;
        info.pfnFuncCb                       = [](
            const DDRpcServerCallInfo* pCall) -> DD_RESULT
        {
            auto* pService = reinterpret_cast<IRouterUtilsRpcService*>(pCall->pUserdata);

            // Execute the service implementation
            return pService->QueryTimestampAndFrequency(*pCall->pWriter);
        };

        result = ddRpcServerRegisterFunction(hServer, &info);
    }

    // Register "QueryDeviceClocks"
    if (result == DD_RESULT_SUCCESS)
    {
        DDRpcServerRegisterFunctionInfo info = {};
        info.serviceId                       = 0x70f462cb;
        info.id                              = 0x4;
        info.pName                           = "QueryDeviceClocks";
        info.pDescription                    = "Queries the list of supported clock modes";
        info.pFuncUserdata                   = pService;
        info.pfnFuncCb                       = [](
            const DDRpcServerCallInfo* pCall) -> DD_RESULT
        {
            auto* pService = reinterpret_cast<IRouterUtilsRpcService*>(pCall->pUserdata);

            // Execute the service implementation
            return pService->QueryDeviceClocks(pCall->pParameterData, pCall->parameterDataSize, *pCall->pWriter);
        };

        result = ddRpcServerRegisterFunction(hServer, &info);
    }

    // Register "QueryCurrentClockMode"
    if (result == DD_RESULT_SUCCESS)
    {
        DDRpcServerRegisterFunctionInfo info = {};
        info.serviceId                       = 0x70f462cb;
        info.id                              = 0x5;
        info.pName                           = "QueryCurrentClockMode";
        info.pDescription                    = "Queries which clock mode is currently active";
        info.pFuncUserdata                   = pService;
        info.pfnFuncCb                       = [](
            const DDRpcServerCallInfo* pCall) -> DD_RESULT
        {
            auto* pService = reinterpret_cast<IRouterUtilsRpcService*>(pCall->pUserdata);

            // Execute the service implementation
            return pService->QueryCurrentClockMode(pCall->pParameterData, pCall->parameterDataSize, *pCall->pWriter);
        };

        result = ddRpcServerRegisterFunction(hServer, &info);
    }

    // Register "SetClockMode"
    if (result == DD_RESULT_SUCCESS)
    {
        DDRpcServerRegisterFunctionInfo info = {};
        info.serviceId                       = 0x70f462cb;
        info.id                              = 0x6;
        info.pName                           = "SetClockMode";
        info.pDescription                    = "Requests that the current clock mode be changed to the provided one";
        info.pFuncUserdata                   = pService;
        info.pfnFuncCb                       = [](
            const DDRpcServerCallInfo* pCall) -> DD_RESULT
        {
            auto* pService = reinterpret_cast<IRouterUtilsRpcService*>(pCall->pUserdata);

            // Execute the service implementation
            return pService->SetClockMode(pCall->pParameterData, pCall->parameterDataSize);
        };

        result = ddRpcServerRegisterFunction(hServer, &info);
    }

    return result;
}

DD_RESULT RegisterService(
    DDRpcServer hServer,
    IRouterUtilsRpcService* pService
)
{
    DDRpcServerRegisterServiceInfo info = {};
    info.id                             = 0x70f462cb;
    info.version.major                  = 0;
    info.version.minor                  = 1;
    info.version.patch                  = 0;
    info.pName                          = "RouterUtilsRpc";
    info.pDescription                   = "Utility service used to query information from the target machine where the router is running.";

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
    info.id                             = 0x70f462cb;
    info.version.major                  = 0;
    info.version.minor                  = 1;
    info.version.patch                  = 0;
    info.pName                          = "RouterUtilsRpc";
    info.pDescription                   = "Utility service used to query information from the target machine where the router is running.";

    // Unregister the service if registering functions fails
    ddRpcServerUnregisterService(hServer, info.id);
}

} // namespace RouterUtilsRpc
