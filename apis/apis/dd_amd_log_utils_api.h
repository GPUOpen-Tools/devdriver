/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef DD_AMD_LOG_UTILS_API_H
#define DD_AMD_LOG_UTILS_API_H

#include <stdint.h>

#include "dd_common_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define DD_AMD_LOG_UTILS_API_NAME "DD_AMD_LOG_UTILS_API"

#define DD_AMD_LOG_UTILS_API_VERSION_MAJOR 0
#define DD_AMD_LOG_UTILS_API_VERSION_MINOR 1
#define DD_AMD_LOG_UTILS_API_VERSION_PATCH 0

typedef struct DDAmdLogUtilsInstance DDAmdLogUtilsInstance;

typedef struct DDAmdLogUtilsApi
{
    /// An opaque pointer to the internal implementation of the AmdLogUtils API.
    DDAmdLogUtilsInstance* pInstance;

    /// Checks if the AmdLogUtils service is available.
    ///
    /// @param pInstance Must be \ref DDAmdLogUtilsApi.pInstance.
    /// @return DD_RESULT_SUCCESS Service is available.
    /// @return DD_RESULT_COMMON_INVALID_PARAMETER If pointer is null.
    /// @return Other errors if service is unavailable.
    DD_RESULT (*IsServiceAvailable)(DDAmdLogUtilsInstance* pInstance);

    /// Gets the service version information.
    ///
    /// @param pInstance Must be \ref DDAmdLogUtilsApi.pInstance.
    /// @param pVersion Output parameter to receive the service version.
    /// @return DD_RESULT_SUCCESS Query was successful.
    /// @return DD_RESULT_COMMON_INVALID_PARAMETER If pointers are null.
    /// @return Other errors if query failed.
    DD_RESULT (*GetServiceInfo)(DDAmdLogUtilsInstance* pInstance,
                                DDApiVersion*          pVersion);

} DDAmdLogUtilsApi;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
