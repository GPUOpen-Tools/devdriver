/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_router_utils_api.h>
#include <dd_router_utils.h>

#include <dd_assert.h>
#include <dd_result.h>
#include <dd_logger_api.h>

#include <ddCommon.h>
#include <g_RouterUtilsRpcClient.h>

#include <cstring>
#include <cstdlib>

#define LOG_ERROR(fmt, ...) s_pLogger->Log(s_pLogger->pInstance, DD_LOG_LVL_ERROR, "[DDRouterUtils] " fmt, ## __VA_ARGS__)
#define LOG_INFO(fmt, ...) s_pLogger->Log(s_pLogger->pInstance, DD_LOG_LVL_INFO, "[DDRouterUtils] " fmt, ## __VA_ARGS__)

namespace
{

DDLoggerApi* s_pLogger;

struct RouterUtilsWriter
{
    uint8_t* pRecvBuf     = nullptr;
    size_t   bytesWritten = 0;
    size_t   totalSize    = 0;
};

// DDRouterUtilsApi wrapper functions.

DD_RESULT GetSysInfo(DDRouterUtilsInstance* pInstance, char* pBuf, size_t* pSize)
{
    DevDriver::RouterUtils* pRouterUtils = reinterpret_cast<DevDriver::RouterUtils*>(pInstance);
    return pRouterUtils->QueryAndCacheSysInfo(pBuf, pSize);
}

DD_RESULT GetCounterAndFrequency(DDRouterUtilsInstance* pInstance, uint64_t* pCounter, uint64_t* pFrequency)
{
    DevDriver::RouterUtils* pRouterUtils = reinterpret_cast<DevDriver::RouterUtils*>(pInstance);
    return pRouterUtils->QueryTimestampAndFrequency(pCounter, pFrequency);
}

DD_RESULT QueryPathByProcessId(DDRouterUtilsInstance* pInstance, uint32_t processId, DDAllocator allocator, char** pProcessPath, size_t* pSize)
{
    DevDriver::RouterUtils* pRouterUtils = reinterpret_cast<DevDriver::RouterUtils*>(pInstance);
    return pRouterUtils->QueryPathByProcessId(processId, allocator, pProcessPath, pSize);
}

} // anonymous namespace

namespace DevDriver
{

RouterUtils::RouterUtils()
    : m_isSysInfoCached(false)
    , m_pSysInfoCache(nullptr)
    , m_sysInfoCacheSize(0)
    , m_net(DD_API_INVALID_HANDLE)
{
}

RouterUtils::~RouterUtils()
{
    ClearAfterRouterDisconnect();
}

DD_RESULT RouterUtils::Initialize(DDApiRegistry* pApiRegistry)
{
    DD_RESULT result = pApiRegistry->Get(
        pApiRegistry->pInstance,
        DD_LOGGER_API_NAME,
        DDVersion {
            DD_LOGGER_API_VERSION_MAJOR,
            DD_LOGGER_API_VERSION_MINOR,
            DD_LOGGER_API_VERSION_PATCH},
        reinterpret_cast<void**>(&s_pLogger));

    DD_ASSERT(result == DD_RESULT_SUCCESS);

    if (result == DD_RESULT_SUCCESS)
    {
        DDRouterUtilsApi routerUtilsApi {
            reinterpret_cast<DDRouterUtilsInstance*>(this),
            GetSysInfo,
            GetCounterAndFrequency,
            ::QueryPathByProcessId};

        result = pApiRegistry->Add(
            pApiRegistry->pInstance,
            DD_ROUTER_UTILS_API_NAME,
            DDVersion {
                DD_ROUTER_UTILS_API_VERSION_MAJOR,
                DD_ROUTER_UTILS_API_VERSION_MINOR,
                DD_ROUTER_UTILS_API_VERSION_PATCH},
            &routerUtilsApi,
            sizeof(routerUtilsApi));

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("Failed to register DDRouterUtilsApi. DD_RESULT: %u.", result);
        }
    }

    return result;
}

void RouterUtils::Destroy()
{
    ClearAfterRouterDisconnect();
}

void RouterUtils::ClearAfterRouterDisconnect()
{
    m_net = DD_API_INVALID_HANDLE;
    m_routerConnectionId = 0;

    if (m_pSysInfoCache)
    {
        std::free(m_pSysInfoCache);
        m_pSysInfoCache = nullptr;
    }
    m_sysInfoCacheSize = 0;
    m_isSysInfoCached = false;
}

void RouterUtils::SetRpcClientInfo(DDNetConnection ddNet, uint16_t routerConnectionId)
{
    m_net = ddNet;
    m_routerConnectionId = routerConnectionId;
}

DD_RESULT RouterUtils::QueryAndCacheSysInfo(char* pSysInfoBuf, size_t* pSize)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    m_sysInfoCacheMutex.Lock();

    if (!m_isSysInfoCached)
    {
        RouterUtilsRpc::RouterUtilsRpcClient rpcClient;

        DDRpcClientCreateInfo clientInfo {};
        clientInfo.hConnection = m_net;
        clientInfo.clientId    = m_routerConnectionId;
        result = rpcClient.Connect(clientInfo);

        if (result == DD_RESULT_SUCCESS)
        {
            RouterUtilsWriter writer{};
            DDByteWriter byteWriter {
                &RouterUtils::ByteWriterBegin,
                &RouterUtils::ByteWriterWriteBytes,
                &RouterUtils::ByteWriterEnd,
                &writer};

            result = rpcClient.QuerySystemInfo(byteWriter);
            if (result == DD_RESULT_SUCCESS)
            {
                m_pSysInfoCache    = reinterpret_cast<char*>(writer.pRecvBuf);
                m_sysInfoCacheSize = writer.totalSize;
                m_isSysInfoCached  = true;
            }
            else
            {
                LOG_ERROR("Failed to RPC call 'QueryAndCacheSysInfo'. DD_RESULT: %s.", StringResult(result));
            }
        }
        else
        {
            LOG_ERROR("Failed to connect RPC client. DD_RESULT: %s.", StringResult(result));
        }
    }

    m_sysInfoCacheMutex.Unlock();

    if (result == DD_RESULT_SUCCESS)
    {
        if (pSize)
        {
            if (pSysInfoBuf)
            {
                if (*pSize >= m_sysInfoCacheSize)
                {
                    DevDriver::Platform::Memcpy_s(pSysInfoBuf, *pSize, m_pSysInfoCache, m_sysInfoCacheSize);
                }
                else
                {
                    *pSize = m_sysInfoCacheSize;
                    result = DD_RESULT_COMMON_BUFFER_TOO_SMALL;
                }
            }
            else
            {
                *pSize = m_sysInfoCacheSize;
            }
        }
        else
        {
            result = DD_RESULT_COMMON_INVALID_PARAMETER;
        }
    }

    return result;
}

DD_RESULT RouterUtils::QueryTimestampAndFrequency(uint64_t* pCounter, uint64_t* pFrequency)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    if ((pCounter == nullptr) || (pFrequency == nullptr))
    {
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    RouterUtilsRpc::RouterUtilsRpcClient rpcClient;

    DDRpcClientCreateInfo clientInfo {};
    clientInfo.hConnection = m_net;
    clientInfo.clientId    = m_routerConnectionId;
    result = rpcClient.Connect(clientInfo);

    if (result == DD_RESULT_SUCCESS)
    {
        RouterUtilsWriter writer{};
        DDByteWriter byteWriter {
            &RouterUtils::ByteWriterBegin,
            &RouterUtils::ByteWriterWriteBytes,
            &RouterUtils::ByteWriterEnd,
            &writer};

        result = rpcClient.QueryTimestampAndFrequency(byteWriter);
        if (result == DD_RESULT_SUCCESS)
        {
            DevDriver::Platform::Memcpy_s(pCounter, sizeof(*pCounter), writer.pRecvBuf, sizeof(*pCounter));
            DevDriver::Platform::Memcpy_s(pFrequency, sizeof(*pFrequency), writer.pRecvBuf + sizeof(*pCounter), sizeof(*pFrequency));
        }
        else
        {
            LOG_ERROR("Failed to RPC call 'QueryTimestampAndFrequency'. DD_RESULT: %s.", StringResult(result));
        }

        std::free(writer.pRecvBuf);
    }
    else
    {
        LOG_ERROR("Failed to connect RPC client. DD_RESULT: %s.", StringResult(result));
    }

    return result;
}

DD_RESULT RouterUtils::QueryPathByProcessId(uint32_t processId, DDAllocator allocator, char** pProcessPath, size_t* pSize)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    RouterUtilsRpc::RouterUtilsRpcClient rpcClient;

    DDRpcClientCreateInfo clientInfo {};
    clientInfo.hConnection = m_net;
    clientInfo.clientId    = m_routerConnectionId;
    result = rpcClient.Connect(clientInfo);

    if (result == DD_RESULT_SUCCESS)
    {
        RouterUtilsWriter writer{};
        DDByteWriter byteWriter {
            &RouterUtils::ByteWriterBegin,
            &RouterUtils::ByteWriterWriteBytes,
            &RouterUtils::ByteWriterEnd,
            &writer};

        result = rpcClient.QueryPathByProcessId(&processId, sizeof(uint32_t), byteWriter);
        if (result == DD_RESULT_SUCCESS)
        {
            const size_t bufSize = (size_t)writer.bytesWritten + 1;
            char* pBuffer = (char*)allocator.Realloc(allocator.pInstance, nullptr, 0, bufSize);

            if (pBuffer != nullptr)
            {
                *pProcessPath = pBuffer;

                if (pSize != nullptr)
                {
                    *pSize = bufSize;
                }

                DevDriver::Platform::Memcpy_s(pBuffer, writer.bytesWritten, writer.pRecvBuf, writer.bytesWritten);
                pBuffer[writer.bytesWritten] = '\0';
            }
            else
            {
                *pProcessPath = nullptr;
                result = DD_RESULT_COMMON_OUT_OF_HEAP_MEMORY;
                LOG_ERROR("Failed to allocate buffer for process path (size: %zu).", bufSize);
            }
        }
        else
        {
            LOG_ERROR("Failed to RPC call 'QueryPathByProcessId'. DD_RESULT: %s.", StringResult(result));
        }

        std::free(writer.pRecvBuf);
    }
    else
    {
        LOG_ERROR("Failed to connect RPC client. DD_RESULT: %s.", StringResult(result));
    }

    return result;
}

DD_RESULT RouterUtils::ByteWriterBegin(void* pUserData, const size_t* pTotalDataSize)
{
    RouterUtilsWriter* pWriter = reinterpret_cast<RouterUtilsWriter*>(pUserData);
    if (pTotalDataSize)
    {
        pWriter->totalSize = *pTotalDataSize;
        pWriter->pRecvBuf = (uint8_t*)std::malloc(pWriter->totalSize);
    }
    return DD_RESULT_SUCCESS;
}

DD_RESULT RouterUtils::ByteWriterWriteBytes(void* pUserData, const void* pData, size_t dataSize)
{
    RouterUtilsWriter* pWriter = reinterpret_cast<RouterUtilsWriter*>(pUserData);

    if (pWriter->pRecvBuf == nullptr)
    {
        DD_ASSERT(pWriter->bytesWritten == 0);
        pWriter->totalSize = dataSize;
        pWriter->pRecvBuf  = (uint8_t*)std::malloc(dataSize);
    }

    size_t newSize = pWriter->bytesWritten + dataSize;
    DD_ASSERT(pWriter->bytesWritten <= newSize);
    if (newSize > pWriter->totalSize)
    {
        // If not enough space, allocate a new bigger buffer, and copy the existing data to the new buffer.
        uint8_t* pNewRecvBuf = (uint8_t*)std::malloc(newSize);

        if (pNewRecvBuf != nullptr)
        {
            DevDriver::Platform::Memcpy_s(pNewRecvBuf, newSize, pWriter->pRecvBuf, pWriter->bytesWritten);
            std::free(pWriter->pRecvBuf);
            pWriter->pRecvBuf = pNewRecvBuf;
            pWriter->totalSize = newSize;
        }
        else
        {
            return DD_RESULT_COMMON_OUT_OF_HEAP_MEMORY;
        }
    }

    DevDriver::Platform::Memcpy_s(pWriter->pRecvBuf + pWriter->bytesWritten, pWriter->totalSize - pWriter->bytesWritten, pData, dataSize);
    pWriter->bytesWritten = newSize;

    return DD_RESULT_SUCCESS;
}

void RouterUtils::ByteWriterEnd(void* pUserdata, DD_RESULT result)
{
    (void)pUserdata;
    (void)result;
}

} // namespace DevDriver
