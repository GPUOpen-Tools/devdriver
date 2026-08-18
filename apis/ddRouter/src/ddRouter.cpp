/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <routerContext.h>

using namespace DevDriver;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Convert a `DD_RESULT` into a human recognizable string.
const char* ddRouterResultToString(
    DD_RESULT result) /// API result
{
    return ddApiResultToString(result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Get version of the library to check interface compatibility
DDApiVersion ddRouterQueryVersion()
{
    DDApiVersion version = {};

    version.major = DD_ROUTER_API_MAJOR_VERSION;
    version.minor = DD_ROUTER_API_MINOR_VERSION;
    version.patch = DD_ROUTER_API_PATCH_VERSION;

    return version;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Get human-readable representation of the library version
const char* ddRouterQueryVersionString()
{
    return DD_ROUTER_API_VERSION_STRING;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT ddRouterCreate(
    const DDRouterCreateInfo* pCreateInfo,
    DDRouter*                 phRouter)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if ((pCreateInfo != nullptr) && (phRouter != nullptr))
    {
        Router* pRouter = nullptr;
        result          = Router::Create(*pCreateInfo, &pRouter);

        if (result == DD_RESULT_SUCCESS)
        {
            *phRouter = ToHandle(pRouter);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ddRouterDestroy(DDRouter hRouter)
{
    if (hRouter != DD_API_INVALID_HANDLE)
    {
        Router* pRouter = FromHandle(hRouter);
        AllocCb alloc   = pRouter->DDAlloc();

        // Use a local copy of Alloc so that we're not calling a method on data as it's deallocated.
        DD_DELETE(pRouter, alloc);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Loads a builtin module
DD_RESULT
ddRouterLoadBuiltinModule(DDRouter hRouter, const DDModuleInterface* pInterface, DDModuleLoadedInfo* pModuleInfo)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if ((hRouter != DD_API_INVALID_HANDLE) && (pInterface != nullptr))
    {
        Router* pRouter = FromHandle(hRouter);
        result          = pRouter->LoadBuiltinModule(pInterface, pModuleInfo);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Loads a dynamic module from the provided path
DD_RESULT ddRouterLoadDynamicModule(
    DDRouter            hRouter,
    const char*         pModulePath, // Path from which to load the module
    DDModuleLoadedInfo* pModuleInfo)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if ((hRouter != DD_API_INVALID_HANDLE) && (pModulePath != nullptr))
    {
        Router* pRouter = FromHandle(hRouter);
        result          = pRouter->LoadDynamicModule(pModulePath, pModuleInfo);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Unloads a module
DD_RESULT ddRouterUnloadModule(DDRouter hRouter, DDModuleContext hModule)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if (hRouter != DD_API_INVALID_HANDLE)
    {
        Router* pRouter = FromHandle(hRouter);
        result          = pRouter->UnloadModule(hModule);
    }

    return result;
}
