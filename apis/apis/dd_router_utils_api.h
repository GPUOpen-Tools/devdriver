/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef DD_ROUTER_UTILS_API_H
#define DD_ROUTER_UTILS_API_H

#include "dd_common_api.h"
#include "dd_allocator_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DD_ROUTER_UTILS_API_NAME "DD_ROUTER_UTILS_API"

#define DD_ROUTER_UTILS_API_VERSION_MAJOR 0
#define DD_ROUTER_UTILS_API_VERSION_MINOR 2
#define DD_ROUTER_UTILS_API_VERSION_PATCH 0

typedef struct DDRouterUtilsInstance DDRouterUtilsInstance;

typedef struct DDRouterUtilsApi
{
    /// An opaque pointer to internal implementation of the router utils api.
    DDRouterUtilsInstance* pInstance;

    /// Retrieve the system information of the target machine where \ref DDRouter is running. This function
    /// caches the retrieved data and returns the cached data in subsequent calls. Note, system info is
    /// only available after the connection to \ref DDRouter has been established.
    ///
    /// @param pInstance Must be \ref DDRouterUtilsApi.pInstance.
    /// @param[out] pBuf A pointer to a buffer to receive system information data. This pointer can be NULL.
    /// @param[in,out] pSize A pointer to the size value. If \param pBuf is non-NULL, the pointed-to value
    /// represents the size of \param pBuf. Otherwise, the required size is written.
    /// @return DD_RESULT_SUCCESS The required size of system information data is written.
    /// @return DD_RESULT_SUCCESS The system information data is written to the buffer pointed to by \param pBuf.
    /// @return DD_RESULT_COMMON_BUFFER_TOO_SMALL The value pointed to by \param pSize is too small.
    /// @return DD_RESULT_COMMON_INVALID_PARAMETER \param pSize is NULL.
    /// @return DD_RESULT_DD_GENERIC_NOT_READY If router connection isn't ready.
    /// @return Other errors if the query failed.
    DD_RESULT (*GetSysInfo)(DDRouterUtilsInstance* pInstance, char* pBuf, size_t* pSize);

    /// Query the time stamp and frequency on the target machine. Time stamp is a monotonically
    /// increasing value representing the number of ticks since the machine boot. Frequency
    /// represents number of ticks per second.
    ///
    /// @param pInstance Must be \ref DDRouterUtilsApi.pInstance.
    /// @param[out] pTimestamp A pointer to a variable to receive time stamp value.
    /// @param[out] pFrequency A pointer to a variable to receive frequency value.
    /// @return DD_RESULT_SUCCESS Time stamp and frequency are set successfully.
    /// @return DD_RESULT_COMMON_INVALID_PARAMETER A NULL pointer is passed.
    /// @return DD_RESULT_DD_GENERIC_NOT_READY If router connection isn't ready.
    /// @return Other errors if the query failed.
    DD_RESULT (*GetTimestampAndFrequency)(DDRouterUtilsInstance* pInstance, uint64_t* pTimestamp, uint64_t* pFrequency);

    /// Queries the full path of a process on the target machine.
    /// @param pInstance Must be \ref DDRouterUtilsApi.pInstance.
    /// @param processId The identifier of the process to query.
    /// @param allocator The allocator to use to allocate the buffer.
    /// @param [out] pProcessPath The process path.
    /// @param [out] pSize Size in bytes of the allocated buffer, for use with \ref DDAllocator.Free. May be NULL.
    /// @return DD_RESULT_SUCCESS Process path was queried successfully.
    /// @return DD_RESULT_DD_GENERIC_NOT_READY If router connection isn't ready.
    /// @return Other errors if the query failed.
    DD_RESULT (*QueryPathByProcessId)(DDRouterUtilsInstance* pInstance, uint32_t processId, DDAllocator allocator, char** pProcessPath, size_t* pSize);

} DDRouterUtilsApi;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
