/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef DD_ROUTER_HEADER
#define DD_ROUTER_HEADER

#include <ddRouterApi.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Get version of the loaded library to check interface compatibility
DDApiVersion ddRouterQueryVersion(
    void);

/// Get human-readable representation of the loaded library version
const char* ddRouterQueryVersionString(
    void);

/// Convert a `DD_RESULT` into a human recognizable string.
/// Use this with `printf`-style functions to provide useful error messages:
/// ```c
///     const DD_RESULT result = ddRouterContextCreate(&info, &router);
///     if (result != DD_ROUTER_SUCCESS) {
///         printf("An error occurred: %s", ddRouterResultToString(result));
///     } else {
///         printf("%s" "Success!");
///     }
/// ```
const char* ddRouterResultToString(
    DD_RESULT result);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Creates a router and sets up a developer mode message bus on the system.
///
/// The output parameter is a handle to the router and this handle must provided to all other function calls.
DD_RESULT ddRouterCreate(
    const DDRouterCreateInfo* pCreateInfo, ///< [in]  Creation info
    DDRouter*                 phRouter);   ///< [out] Handle to the new router

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Destroys a router
///
/// This shuts down the developer mode message bus on the system
void ddRouterDestroy(
    DDRouter hRouter); ///< Handle to the router being destroyed

/// Loads a built-in module
DD_RESULT ddRouterLoadBuiltinModule(
    DDRouter                 hRouter,      /// [in] Handle to the DDRouter
    const DDModuleInterface* pInterface,   /// [in] Pointer to a builtin module interface
    DDModuleLoadedInfo*      pModuleInfo); /// [out] [Optional] Returned loaded module info structure

/// Loads a dynamic module from the provided path
DD_RESULT ddRouterLoadDynamicModule(
    DDRouter            hRouter,      /// [in] Handle to the DDRouter
    const char*         pModulePath,  /// [in] Path to a dynamic module on the filesystem
    DDModuleLoadedInfo* pModuleInfo); /// [out] [Optional] Returned loaded module info structure

/// Unloads a module
DD_RESULT ddRouterUnloadModule(
    DDRouter        hRouter,  /// [in] Handle to the DDRouter
    DDModuleContext hModule); /// [in/out] Pointer to the handle associated with the module to unload

#ifdef __cplusplus
} // extern "C"
#endif

#endif
