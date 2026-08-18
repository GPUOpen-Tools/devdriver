/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_allocator_api.h>
#include <dd_api_registry_api.h>
#include <dd_mutex.h>

#include <ddNet.h>

namespace DevDriver
{

class RouterUtils
{
private:
    Mutex  m_sysInfoCacheMutex;
    bool   m_isSysInfoCached;
    char*  m_pSysInfoCache;
    size_t m_sysInfoCacheSize;

    DDNetConnection    m_net;
    uint16_t           m_routerConnectionId;

public:
    RouterUtils();
    ~RouterUtils();

    DD_RESULT Initialize(DDApiRegistry* pApiRegistry);
    void      Destroy();

    void ClearAfterRouterDisconnect();

    void SetRpcClientInfo(DDNetConnection ddNet, uint16_t routerConnectionId);

    DD_RESULT QueryAndCacheSysInfo(char* pSysInfoBuf, size_t* pSize);
    DD_RESULT QueryTimestampAndFrequency(uint64_t* pCounter, uint64_t* pFrequency);
    DD_RESULT QueryPathByProcessId(uint32_t processId, DDAllocator allocator, char** pProcessPath, size_t* pSize);

private:
    static DD_RESULT ByteWriterBegin(void* pUserData, const size_t* pTotalDataSize);
    static DD_RESULT ByteWriterWriteBytes(void* pUserdata, const void* pData, size_t dataSize);
    static void      ByteWriterEnd(void* pUserdata, DD_RESULT result);
};

} // namespace DevDriver
