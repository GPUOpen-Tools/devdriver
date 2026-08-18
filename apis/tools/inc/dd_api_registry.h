/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_api_registry_api.h>
#include <dd_mutex.h>

namespace DevDriver
{

class ApiRegistry
{
private:
    uint8_t*   m_pApiPool;
    size_t     m_apiTotalSize;
    Mutex      m_apiPoolMutex;

public:
    ApiRegistry();
    ~ApiRegistry();

    DD_RESULT Add(const char* pApiName, DDVersion version, void* pApiStruct, size_t apiStructSize);
    DD_RESULT Get(const char* pApiName, DDVersion version, void** ppApiStruct);

private:
    ApiRegistry(ApiRegistry&& registry) = delete;
    ApiRegistry(const ApiRegistry& registry) = delete;
    ApiRegistry& operator=(ApiRegistry&& registry) = delete;
    ApiRegistry& operator=(const ApiRegistry& registry) = delete;
};

// API wrappers
DD_RESULT DDApiRegistry_Add(DDApiRegistryInstance* pRegistry, const char* pApiName, DDVersion version, void* pApiStruct, size_t apiStructSize);
DD_RESULT DDApiRegistry_Get(DDApiRegistryInstance* pRegistry, const char* pApiName, DDVersion version, void** ppApiStruct);

} // namespace DevDriver
