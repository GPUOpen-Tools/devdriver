/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_api_registry.h>
#include <ddPlatform.h>
#include <string.h>

namespace DevDriver
{
static constexpr size_t API_DATA_POOL_SIZE = 4 * 1024;

struct ApiData
{
    // A pointer to the API name string.
    const char* pName;
    // API version numbers.
    DDVersion   version;
    // The size of the API struct.
    size_t      size;
    // The aligned size of the entire `ApiData` including trailing `data`.
    size_t      alignedSize;
    // The starting address of the API struct data.
    uint8_t     data[1];
};

ApiRegistry::ApiRegistry()
    : m_apiTotalSize(0)
{
    m_pApiPool = new uint8_t[API_DATA_POOL_SIZE];
}

ApiRegistry::~ApiRegistry()
{
    delete[] m_pApiPool;
}

DD_RESULT ApiRegistry::Add(const char* pApiName, DDVersion version, void* pApiStruct, size_t apiStructSize)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    LockGuard lock(m_apiPoolMutex);

    // Check if an API with the same name has already been registered.

    size_t apiPoolPos = 0;
    while (apiPoolPos < m_apiTotalSize)
    {
        const ApiData* pApiData = reinterpret_cast<const ApiData*>(m_pApiPool + apiPoolPos);
        if (strcmp(pApiData->pName, pApiName) == 0)
        {
            result = DD_RESULT_COMMON_ALREADY_EXISTS;
            break;
        }
        apiPoolPos += pApiData->alignedSize;
    }

    if (result == DD_RESULT_SUCCESS)
    {
        // The API doesn't already exist in the pool, insert it.

        const uint32_t ALIGN_SIZE = sizeof(uint64_t); // Align to 8 bytes on 32-bit and 64-bit machines.

        const size_t apiDataSize = (offsetof(ApiData, data) + apiStructSize);
        const size_t apiDataAlignedSize = (apiDataSize + ALIGN_SIZE - 1) & ~(ALIGN_SIZE - 1);

        if ((apiDataAlignedSize + m_apiTotalSize) <= API_DATA_POOL_SIZE)
        {
            ApiData* pApiData = reinterpret_cast<ApiData*>(m_pApiPool + apiPoolPos);
            pApiData->pName = pApiName;
            pApiData->version = version;
            pApiData->size = apiStructSize;
            pApiData->alignedSize = apiDataAlignedSize;
            Platform::Memcpy_s(pApiData->data, apiStructSize, pApiStruct, apiStructSize);
            m_apiTotalSize += apiDataAlignedSize;
        }
        else
        {
            result = DD_RESULT_COMMON_BUFFER_TOO_SMALL;
        }
    }

    return result;
}

DD_RESULT ApiRegistry::Get(const char* pApiName, DDVersion version, void** ppApiStruct)
{
    DD_RESULT result = DD_RESULT_COMMON_DOES_NOT_EXIST;

    if (ppApiStruct != nullptr)
    {
        *ppApiStruct = nullptr;

        size_t apiPoolPos = 0;

        LockGuard lock(m_apiPoolMutex);

        while (apiPoolPos < m_apiTotalSize)
        {
            ApiData* pApiData = reinterpret_cast<ApiData*>(m_pApiPool + apiPoolPos);
            if (strcmp(pApiData->pName, pApiName) == 0)
            {
                if ((pApiData->version.major == version.major) && (pApiData->version.minor >= version.minor))
                {
                    *ppApiStruct = pApiData->data;
                    result = DD_RESULT_SUCCESS;
                }
                else
                {
                    result = DD_RESULT_COMMON_VERSION_MISMATCH;
                }
                break;
            }
            apiPoolPos += pApiData->alignedSize;
        }
    }
    else
    {
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    return result;
}

DD_RESULT DDApiRegistry_Add(
    DDApiRegistryInstance* pRegistry,
    const char* pApiName,
    DDVersion version,
    void* pApiStruct,
    size_t apiStructSize)
{
    ApiRegistry* pReg = reinterpret_cast<ApiRegistry*>(pRegistry);
    return pReg->Add(pApiName, version, pApiStruct, apiStructSize);
}

DD_RESULT DDApiRegistry_Get(
    DDApiRegistryInstance* pRegistry,
    const char* pApiName,
    DDVersion version,
    void** ppApiStruct)
{
    ApiRegistry* pReg = reinterpret_cast<ApiRegistry*>(pRegistry);
    return pReg->Get(pApiName, version, ppApiStruct);
}

} // namespace DevDriver
