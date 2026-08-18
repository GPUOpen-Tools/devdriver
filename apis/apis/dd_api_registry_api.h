/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef DD_API_REGISTRY_API_H
#define DD_API_REGISTRY_API_H

#include "dd_common_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DD_API_REGISTRY_API_VERSION_MAJOR 0
#define DD_API_REGISTRY_API_VERSION_MINOR 1
#define DD_API_REGISTRY_API_VERSION_PATCH 0

typedef struct DDApiRegistryInstance DDApiRegistryInstance;

/// A struct containing functions and data members for ApiRegistry.
typedef struct DDApiRegistry
{
    /// The current version of this API.
    DDVersion version;

    /// A opaque pointer to an internal API registry instance.
    DDApiRegistryInstance* pInstance;

    /// Add an API struct to the registry. This function stores a copy of the API struct in the registry.
    ///
    /// @param pRegistry Must be \ref DDApiRegistry.pInstance.
    /// @param pApiName A pointer to the API name. The registry only stores the pointer, so the caller of this
    /// function needs to make sure the name string data exists throughout the whole time the API is registered.
    /// @param version The version of the API.
    /// @param pApiStruct A pointer to an instantiation of the API struct.
    /// @param apiStructSize The size of the API struct.
    ///
    /// @return DD_RESULT_SUCCESS The API has been registered successfully.
    /// @return DD_RESULT_COMMON_ALREADY_EXISTS Registration failed because the API with the same name already
    /// exists in the registry.
    /// @return DD_RESULT_COMMON_BUFFER_TOO_SMALL Registration failed because the internal API pool is too small to
    /// accept more data.
    DD_RESULT (*Add)(DDApiRegistryInstance* pInstance, const char* pApiName, DDVersion version, void* pApiStruct, size_t apiStructSize);

    /// Get the API by its name.
    ///
    /// @param pRegistry Must be \ref DDApiRegistry.pInstance.
    /// @param pApiName A pointer to the API name string data.
    /// @param version The version of the API to query.
    /// @param[out] ppOutApiStruct On success, it's set to a pointer to the copy of the API struct stored in the
    /// registry. On failure, it's set to NULL.
    ///
    /// @return DD_RESULT_SUCCESS The API with the correct version is returned.
    /// @return DD_RESULT_COMMON_DOES_NOT_EXIST The queried API doesn't exist in the registry.
    /// @return DD_RESULT_COMMON_VERSION_MISMATCH The version of the existing API doesn't satisfy the queried version.
    /// @return DD_RESULT_COMMON_INVALID_PARAMETER If ppOutApiStruct is a null pointer.
    DD_RESULT (*Get)(DDApiRegistryInstance* pInstance, const char* pApiName, DDVersion version, void** ppOutApiStruct);
} DDApiRegistry;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
