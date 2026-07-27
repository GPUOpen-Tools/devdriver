/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef DD_MODULES_API_H
#define DD_MODULES_API_H

#include "dd_common_api.h"
#include "dd_api_registry_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DD_MODULES_API_NAME "DD_MODULES_API"

#define DD_MODULES_API_VERSION_MAJOR 0
#define DD_MODULES_API_VERSION_MINOR 1
#define DD_MODULES_API_VERSION_PATCH 0

/// A DevDriver module is a C/C++ library (static or dynamic) that implements and exports the function
/// `void DDModuleLoad_xxx(DDApiRegistry* pRegistry);`, where "xxx" is the filename of the compiled module.
///
/// ```c++
/// DD_DECLARE_MODULE_LOAD_API(foo);
///
/// DD_RESULT DDModuleLoad_foo(DDApiRegistry* pRegistry) {
///    printf("foo module is loaded");
/// }
/// ```
///
/// It is recommended that modules register their APIs in DDModuleLoad_xxx(), but query others' APIs later
/// during module initialization.

#ifdef _MSC_VER
    #define DD_DECLARE_MODULE_LOAD_API(name) \
        extern "C" __declspec(dllexport) DD_RESULT __cdecl DDModuleLoad_ ## name (DDApiRegistry* pApiRegistry)
#else
    #define DD_DECLARE_MODULE_LOAD_API(name) \
        extern "C" __attribute__((visibility("default"))) DD_RESULT DDModuleLoad_ ## name (DDApiRegistry* pApiRegistry)
#endif

typedef struct DDModuleInstance DDModuleInstance;

/// This struct holds module level callback functions that each module can implement.
typedef struct DDModulesCallbacks
{
    /// An opaque pointer to a module instance.
    DDModuleInstance* pInstance;

    /// This function is called after __all__ modules (static and dynamic) have been loaded. The order at
    /// which this function is called for every module is not guaranteed.
    ///
    /// @param pInstance Must be \ref DDModulesCallbacks.pInstance.
    /// @return DD_RESULT_SUCCESS The module has been initialized successfully.
    DD_RESULT (*Initialize)(DDModuleInstance* pInstance);

    /// This function gives a module a chance to clean up their resources before the system shuts down.
    /// The order at which this function is called for every module is not guaranteed.
    ///
    /// @param pInstance Must be \ref DDModulesCallbacks.pInstance.
    void (*Destroy)(DDModuleInstance* pInstance);
} DDModulesCallbacks;

typedef struct DDModulesManagerInstance DDModulesManagerInstance;

/// This struct contains functions for DDModule.
typedef struct DDModulesApi
{
    /// A opaque pointer to an internal modules manager instance.
    DDModulesManagerInstance* pInstance;

    /// Add an implementation of \ref DDModulesCallbacks.
    ///
    /// @param pInstance Must be \ref DDModulesApi.pInstance.
    /// @param pCallback A pointer to a \ref DDModulesCallbacks object. This callback object must persist
    /// before the module is unloaded at the end of the program.
    /// @return DD_RESULT_SUCCESS If a callback implementation is added successfully.
    /// @return DD_RESULT_COMMON_INVALID_PARAMETER If either \param pInstance or \param pCallback is NULL.
    DD_RESULT (*AddModulesCallbacks)(DDModulesManagerInstance* pInstance, DDModulesCallbacks* pCallback);
} DDModulesApi;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
