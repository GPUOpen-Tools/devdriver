/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <new>
#include <dd_pipelines.h>
#include <dd_pipelines_api.h>

#include <ddCommon.h>
#include <dd_dynamic_buffer.h>
#include <protocols/ddPipelineUriService.h>

namespace
{
DD_RESULT ConnectWrapper(DDPipelinesInstance* pInstance, DDConnectionId umdConnectionId)
{
    DevDriver::Pipelines* pPipelines = reinterpret_cast<DevDriver::Pipelines*>(pInstance);
    return pPipelines->Connect(umdConnectionId);
}

void DisconnectWrapper(DDPipelinesInstance* pInstance, DDConnectionId umdConnection)
{
    DevDriver::Pipelines* pPipelines = reinterpret_cast<DevDriver::Pipelines*>(pInstance);
    pPipelines->Disconnect(umdConnection);
}

DD_RESULT DumpDriverPipelinesWrapper(DDPipelinesInstance*     pInstance,
                                     DDConnectionId           umdConnection,
                                     DDPipelineRecordCallback callback,
                                     void*                    userPointer)
{
    DevDriver::Pipelines* pPipelines = reinterpret_cast<DevDriver::Pipelines*>(pInstance);
    return pPipelines->DumpDriverPipelines(umdConnection, callback, userPointer);
}

DD_RESULT InjectPipelinesWrapper(DDPipelinesInstance*             pInstance,
                                 DDConnectionId                   umdConnectionId,
                                 const DDPipelinesCodeObjectData* pObjects,
                                 size_t numObjects)
{
    DevDriver::Pipelines* pPipelines = reinterpret_cast<DevDriver::Pipelines*>(pInstance);
    return pPipelines->InjectPipelines(umdConnectionId, pObjects, numObjects);
}

/// Helper functions that convert to and from a pipeline service PipelineHash and a DDPipelinesApiHash
DDPipelinesApiHash PipelineHashToDDApiHash(DevDriver::PipelineHash hash)
{
    DDPipelinesApiHash retHash = {};
    retHash.pipelineHashLo     = hash.qwords[0];
    retHash.pipelineHashHi     = hash.qwords[1];

    return retHash;
}

} // namespace

namespace DevDriver
{
Pipelines::Pipelines()
    : m_net(DD_API_INVALID_HANDLE),
      m_pConnectionApi(nullptr),
      m_pLogger(nullptr)
{
}

Pipelines::~Pipelines()
{
    ClearAfterRouterDisconnect();
    m_pConnectionApi = nullptr;
    m_pLogger        = nullptr;
}

DD_RESULT Pipelines::Initialize(DDApiRegistry* pApiRegistry)
{
    DD_RESULT result = pApiRegistry->Get(
        pApiRegistry->pInstance,
        DD_LOGGER_API_NAME,
        DDVersion{ DD_LOGGER_API_VERSION_MAJOR, DD_LOGGER_API_VERSION_MINOR, DD_LOGGER_API_VERSION_PATCH },
        reinterpret_cast<void**>(&m_pLogger));

    DD_ASSERT(result == DD_RESULT_SUCCESS);
    if (result == DD_RESULT_SUCCESS)
    {
        result = pApiRegistry->Get(pApiRegistry->pInstance,
                                   DD_CONNECTION_API_NAME,
                                   DDVersion{ DD_CONNECTION_API_VERSION_MAJOR,
                                              DD_CONNECTION_API_VERSION_MINOR,
                                              DD_CONNECTION_API_VERSION_PATCH },
                                   reinterpret_cast<void**>(&m_pConnectionApi));
    }

    DD_ASSERT(result == DD_RESULT_SUCCESS);
    if (result == DD_RESULT_SUCCESS)
    {
        DDPipelinesApi pipelinesApi{ reinterpret_cast<DDPipelinesInstance*>(this),
                                     ConnectWrapper,
                                     DisconnectWrapper,
                                     DumpDriverPipelinesWrapper,
                                     InjectPipelinesWrapper };

        result = pApiRegistry->Add(
            pApiRegistry->pInstance,
            DD_PIPELINES_API_NAME,
            DDVersion{ DD_PIPELINES_API_VERSION_MAJOR, DD_PIPELINES_API_VERSION_MINOR, DD_PIPELINES_API_VERSION_PATCH },
            &pipelinesApi,
            sizeof(pipelinesApi));
    }

    return result;
}

void Pipelines::ClearAfterRouterDisconnect()
{
    m_net = DD_API_INVALID_HANDLE;

    for (auto& it : m_traceEnabledClients)
    {
        it.second->Disconnect();
        delete it.second;
    }

    m_traceEnabledClients.clear();
}

void Pipelines::SetRpcClientInfo(DDNetConnection ddNet)
{
    m_net = ddNet;
}

DD_RESULT Pipelines::Connect(DDConnectionId umdConnectionId)
{
    LockGuard lock(m_clientsMutex);
    DD_RESULT result = DD_RESULT_DD_GENERIC_NOT_READY;
    if ((m_net != DD_API_INVALID_HANDLE) && (m_traceEnabledClients.count(umdConnectionId) == 0))
    {
        Client* pClient = new(std::nothrow) Client((IMsgChannel*)m_net);
        if (pClient != nullptr)
        {
            result = DevDriverToDDResult(pClient->Connect(umdConnectionId));

            if (result == DD_RESULT_SUCCESS)
            {
                m_traceEnabledClients.emplace(umdConnectionId, pClient);
            }
        }
        else
        {
            result = DD_RESULT_COMMON_OUT_OF_HEAP_MEMORY;
        }
    }

    return result;
}

void Pipelines::Disconnect(DDConnectionId umdConnection)
{
    LockGuard lock(m_clientsMutex);

    auto foundItr = m_traceEnabledClients.find(umdConnection);
    if (foundItr != m_traceEnabledClients.end())
    {
        foundItr->second->Disconnect();

        delete foundItr->second;
        m_traceEnabledClients.erase(foundItr);
    }
}

DD_RESULT Pipelines::DumpDriverPipelines(
    DDConnectionId           umdConnectionId,
    DDPipelineRecordCallback pCallback,
    void*                    pUserdata)
{
    LockGuard lock(m_clientsMutex);

    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;
    if ((m_traceEnabledClients.count(umdConnectionId) != 0) && (pCallback != nullptr))
    {
        Client* client = m_traceEnabledClients[umdConnectionId];

        Vector<uint8_t> responseBuffer(DevDriver::Platform::GenericAllocCb);
        result = DevDriverToDDResult(client->TransactURIRequest(nullptr, 0, &responseBuffer, "pipeline://getAllPipelines"));

        if (result == DD_RESULT_SUCCESS)
        {
            PipelineRecordsIterator iterator(responseBuffer.Data(), responseBuffer.Size());
            for (PipelineRecord record; iterator.Get(nullptr); iterator.Next())
            {
                iterator.Get(&record);

                // Fill in the code object data struct
                DDPipelinesCodeObjectData codeObjectData = {};
                codeObjectData.pData                     = record.pBinary;
                codeObjectData.size                      = record.header.size;
                codeObjectData.hash                      = PipelineHashToDDApiHash(record.header.hash);

                pCallback(&codeObjectData, pUserdata);
            }
        }
    }

    return result;
}

DD_RESULT Pipelines::InjectPipelines(
    DDConnectionId                   umdConnectionId,
    const DDPipelinesCodeObjectData* pObjects,
    size_t                           numObjects)
{
    LockGuard lock(m_clientsMutex);
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;
    if ((m_traceEnabledClients.count(umdConnectionId) != 0) && (pObjects != nullptr))
    {
        // Make sure the data size is in a reasonable range, the driver will reject anything over 256 kB
        static constexpr size_t kPipelinesPostSizeLimit = (256 * 1024);
        size_t sizeRequired = 0;

        for (size_t object = 0; object < numObjects; ++object)
        {
            sizeRequired += sizeof(PipelineRecordHeader) + static_cast<size_t>((pObjects + object)->size);
        }

        if (sizeRequired <= kPipelinesPostSizeLimit)
        {
            DD_DRIVER_STATE state = DD_DRIVER_STATE_UNKNOWN;
            result                = m_pConnectionApi->GetDriverState(m_pConnectionApi->pInstance, umdConnectionId, &state);
            if ((result == DD_RESULT_SUCCESS) && (DD_DRIVER_STATE_POSTDEVICEINIT == state))
            {
                DynamicBuffer tempBuf;
                tempBuf.Reserve(sizeRequired);

                PipelineRecordHeader header;
                for (size_t objectIndex = 0; objectIndex < numObjects; ++objectIndex)
                {
                    const DDPipelinesCodeObjectData* pObject = pObjects + objectIndex;
                    header.hash = PipelineHashToDDApiHash(pObject->hash);
                    header.size  = pObject->size;

                    tempBuf.Copy(&header, sizeof(header));

                    DD_ASSERT(pObject->size <= SIZE_MAX);
                    tempBuf.Copy(pObject->pData, (size_t)pObject->size);
                }

                Client* client = m_traceEnabledClients[umdConnectionId];

                DD_ASSERT(tempBuf.Size() <= UINT32_MAX);
                result = DevDriverToDDResult(client->TransactURIRequest(tempBuf.Data(), (uint32_t)tempBuf.Size(), nullptr, "pipeline://reinject"));
            }
            else
            {
                m_pLogger->Log(m_pLogger->pInstance,
                               DD_LOG_LVL_ERROR,
                               "[Pipelines] InjectPipeline should be called during driver state 'PostDeviceInit', but the "
                               "current driver state is: %d",
                               state);

                DD_ASSERT(false);
                result = DD_RESULT_DD_GENERIC_NOT_READY;
            }
        }
    }

    return result;
}

} // namespace DevDriver
