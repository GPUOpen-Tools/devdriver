/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "dd_legacy_utils.h"
#include <dd_memory_trace.h>

#include <ddRdf.h>
#include <util/ddStructuredReader.h>

#define LOG_ERROR(fmt, ...) m_pLogger->Log(              \
                                m_pLogger->pInstance,    \
                                DD_LOG_LVL_ERROR,        \
                                "[DDMemoryTrace] " fmt,  \
                                ## __VA_ARGS__)
namespace
{
//
// DDMemoryTraceApi wrapper functions.
//
DD_RESULT EnableTracingWrapper(DDMemoryTraceInstance* pInstance,
                               DDConnectionId         umdConnectionId,
                               DDProcessId            processId,
                               bool                   useKmd)
{
    DevDriver::MemoryTrace* pMemoryTrace = reinterpret_cast<DevDriver::MemoryTrace*>(pInstance);
    return pMemoryTrace->EnableTracing(umdConnectionId, processId, useKmd);
}

DD_RESULT DisableTracingWrapper(DDMemoryTraceInstance* pInstance, DDConnectionId umdConnectionId)
{
    DevDriver::MemoryTrace* pMemoryTrace = reinterpret_cast<DevDriver::MemoryTrace*>(pInstance);
    return pMemoryTrace->DisableTracing(umdConnectionId);
}

DD_RESULT EndTracingWrapper(DDMemoryTraceInstance* pInstance, DDConnectionId umdConnectionId, bool isClientInitialized)
{
    DevDriver::MemoryTrace* pMemoryTrace = reinterpret_cast<DevDriver::MemoryTrace*>(pInstance);
    return pMemoryTrace->EndTracing(umdConnectionId, isClientInitialized);
}

DD_RESULT DumpTraceWrapper(DDMemoryTraceInstance* pInstance, DDConnectionId umdConnectionId, bool isClientInitialized)
{
    DevDriver::MemoryTrace* pMemoryTrace = reinterpret_cast<DevDriver::MemoryTrace*>(pInstance);
    return pMemoryTrace->DumpTrace(umdConnectionId, isClientInitialized);
}

DD_RESULT AbortTraceWrapper(DDMemoryTraceInstance* pInstance, DDConnectionId umdConnectionId, bool isClientInitialized)
{
    DevDriver::MemoryTrace* pMemoryTrace = reinterpret_cast<DevDriver::MemoryTrace*>(pInstance);
    return pMemoryTrace->AbortTrace(umdConnectionId, isClientInitialized);
}

DD_RESULT InsertSnapshotWrapper(DDMemoryTraceInstance* pInstance, DDConnectionId umdConnectionId, const char* pSnapshotName)
{
    DevDriver::MemoryTrace* pMemoryTrace = reinterpret_cast<DevDriver::MemoryTrace*>(pInstance);
    return pMemoryTrace->InsertSnapshot(umdConnectionId, pSnapshotName);
}

DD_RESULT ClearTraceWrapper(DDMemoryTraceInstance* pInstance, DDConnectionId umdConnectionId)
{
    DevDriver::MemoryTrace* pMemoryTrace = reinterpret_cast<DevDriver::MemoryTrace*>(pInstance);
    return pMemoryTrace->ClearTrace(umdConnectionId);
}

DD_RESULT QueryStatusWrapper(DDMemoryTraceInstance* pInstance, DDConnectionId umdConnectionId, DDMemoryTraceStatus* pStatus)
{
    DevDriver::MemoryTrace* pMemoryTrace = reinterpret_cast<DevDriver::MemoryTrace*>(pInstance);
    return pMemoryTrace->QueryStatus(umdConnectionId, pStatus);
}

DD_RESULT TransferTraceDataWrapper(DDMemoryTraceInstance* pInstance,
                                   DDConnectionId         umdConnectionId,
                                   const DDRdfFileWriter* pFileWriter,
                                   const DDIOHeartbeat*   pIoCb,
                                   bool                   useCompression)
{
    DevDriver::MemoryTrace* pMemoryTrace = reinterpret_cast<DevDriver::MemoryTrace*>(pInstance);
    return pMemoryTrace->TransferTraceData(umdConnectionId, pFileWriter, pIoCb, useCompression);
}

void UpdateSysInfoBufferWrapper(DDConnectionCallbacksImpl* pInstance, DDConnectionId routerConnectionId)
{
    DD_UNUSED(routerConnectionId);
    DevDriver::MemoryTrace* pMemoryTrace = reinterpret_cast<DevDriver::MemoryTrace*>(pInstance);
    pMemoryTrace->UpdateSysInfoBuffer();
}

void OnDriverConnectedWrapper(DDConnectionCallbacksImpl* pImpl, const DDConnectionInfo* pConnInfo)
{
    DevDriver::MemoryTrace* pMemTrace = reinterpret_cast<DevDriver::MemoryTrace*>(pImpl);
    pMemTrace->OnDriverConnected(pConnInfo);
}

} // anonymous namespace

namespace DevDriver
{

MemoryTrace::MemoryTrace()
    : m_net(DD_API_INVALID_HANDLE)
    , m_pRouterUtilsApi(nullptr)
    , m_sysInfoBuffer(Platform::GenericAllocCb)
    , m_pSystemClients(nullptr)
    , m_pLogger(nullptr)
{
}

MemoryTrace::~MemoryTrace()
{
    ClearAfterRouterDisconnect();
    m_pLogger         = nullptr;
    m_pSystemClients  = nullptr;
    m_pRouterUtilsApi = nullptr;
}

DD_RESULT MemoryTrace::Initialize(DDApiRegistry* pApiRegistry)
{
    DD_RESULT result = pApiRegistry->Get(
        pApiRegistry->pInstance,
        DD_LOGGER_API_NAME,
        DDVersion{ DD_LOGGER_API_VERSION_MAJOR, DD_LOGGER_API_VERSION_MINOR, DD_LOGGER_API_VERSION_PATCH },
        reinterpret_cast<void**>(&m_pLogger));

    DD_ASSERT(result == DD_RESULT_SUCCESS);
    if (result == DD_RESULT_SUCCESS)
    {
        DDMemoryTraceApi memoryTraceApi{ reinterpret_cast<DDMemoryTraceInstance*>(this),
                                         EnableTracingWrapper,
                                         DisableTracingWrapper,
                                         EndTracingWrapper,
                                         DumpTraceWrapper,
                                         AbortTraceWrapper,
                                         InsertSnapshotWrapper,
                                         ClearTraceWrapper,
                                         QueryStatusWrapper,
                                         TransferTraceDataWrapper };

        result = pApiRegistry->Add(pApiRegistry->pInstance,
                                   DD_MEMORY_TRACE_API_NAME,
                                   DDVersion{ DD_MEMORY_TRACE_API_VERSION_MAJOR,
                                              DD_MEMORY_TRACE_API_VERSION_MINOR,
                                              DD_MEMORY_TRACE_API_VERSION_PATCH },
                                   &memoryTraceApi,
                                   sizeof(memoryTraceApi));

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("Failed to register DDMemoryTraceApi. DDResult: %s", ddApiResultToString(result));
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        result = pApiRegistry->Get(
            pApiRegistry->pInstance,
            DD_ROUTER_UTILS_API_NAME,
            DDVersion{
                DD_ROUTER_UTILS_API_VERSION_MAJOR,
                DD_ROUTER_UTILS_API_VERSION_MINOR,
                DD_ROUTER_UTILS_API_VERSION_PATCH},
            reinterpret_cast<void**>(&m_pRouterUtilsApi));
        DD_ASSERT(result == DD_RESULT_SUCCESS);
    }

    // Register OnRouterConnected callback to get system-info.
    if (result == DD_RESULT_SUCCESS)
    {
        result = pApiRegistry->Get(pApiRegistry->pInstance,
                                   DD_CONNECTION_API_NAME,
                                   DDVersion{ DD_CONNECTION_API_VERSION_MAJOR,
                                              DD_CONNECTION_API_VERSION_MINOR,
                                              DD_CONNECTION_API_VERSION_PATCH },
                                   reinterpret_cast<void**>(&m_pConnectionApi));

        if (result == DD_RESULT_SUCCESS)
        {
            // Register connection callbacks
            DDConnectionCallbacks connectionCbs = {};
            connectionCbs.pImpl                 = reinterpret_cast<DDConnectionCallbacksImpl*>(this);
            connectionCbs.OnRouterConnected     = &UpdateSysInfoBufferWrapper;
            connectionCbs.OnDriverConnected     = &OnDriverConnectedWrapper;

            m_pConnectionApi->AddConnectionCallbacks(m_pConnectionApi->pInstance, &connectionCbs);
        }
    }

    return result;
}

void MemoryTrace::ClearAfterRouterDisconnect()
{
    m_net = DD_API_INVALID_HANDLE;
    m_traceEnabledClients.clear();
}

void MemoryTrace::SetRpcClientInfo(DDNetConnection ddNet)
{
    m_net = ddNet;
}

void MemoryTrace::SetSystemClients(const DDModuleSystemClientInfo* pSysClients)
{
    m_pSystemClients = pSysClients;
}

DD_RESULT MemoryTrace::EnableTracing(DDConnectionId umdConnectionId, ProcessId processId, bool useKmd)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    RWLockGuard<RWLock::LockType::Read> lock(m_clientsMutex);

    auto foundItr = m_traceEnabledClients.find(umdConnectionId);
    if (foundItr == m_traceEnabledClients.end())
    {
        result = DD_RESULT_COMMON_DOES_NOT_EXIST;
        LOG_ERROR("EnableTracing() failed. Couldn't find trace-enabled client.");
    }

    if (result == DD_RESULT_SUCCESS)
    {
        result = foundItr->second->BeginTrace(
            processId,
            m_net,
            GetGfxKernelId(),
            GetAmdLogId(),
            (DDClientId)umdConnectionId,
            GetRouterId(),
            m_sysInfoBuffer,
            useKmd);

        if (result != DD_RESULT_SUCCESS)
        {
            DD_DRIVER_STATE state = DD_DRIVER_STATE_UNKNOWN;
            DD_RESULT driverStateResult = m_pConnectionApi->GetDriverState(
                m_pConnectionApi->pInstance,
                umdConnectionId,
                &state);
            if (driverStateResult == DD_RESULT_SUCCESS)
            {
                LOG_ERROR("BeginTrace failed. Current driver state: %d. DDResult: %s",
                          state,
                          ddApiResultToString(result));

            }
            else
            {
                LOG_ERROR("BeginTrace failed, DDResult: %s. "
                          "Also failed to query current driver state, DDResult: %s",
                          ddApiResultToString(result),
                          ddApiResultToString(driverStateResult));
            }
        }
    }

    return result;
}

DD_RESULT MemoryTrace::DisableTracing(DDConnectionId umdConnectionId)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    RWLockGuard<RWLock::LockType::Write> lock(m_clientsMutex);

    auto foundItr = m_traceEnabledClients.find(umdConnectionId);
    if (foundItr == m_traceEnabledClients.end())
    {
        result = DD_RESULT_COMMON_DOES_NOT_EXIST;
    }
    else
    {
        delete foundItr->second;
        m_traceEnabledClients.erase(foundItr);
    }

    return result;
}

DDClientId MemoryTrace::GetGfxKernelId() const
{
    DDClientId id = 0;
    if (m_pSystemClients)
    {
        id = m_pSystemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_GRAPHICS_DRIVER].id;
    }
    return id;
}

DDClientId MemoryTrace::GetAmdLogId() const
{
    DDClientId id = 0;
    if (m_pSystemClients)
    {
        id = m_pSystemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_UTILITY_DRIVER].id;
    }
    return id;
}

DDClientId MemoryTrace::GetRouterId() const
{
    DDClientId id = 0;
    if (m_pSystemClients)
    {
        id = m_pSystemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_ROUTER].id;
    }
    return id;
}

DD_RESULT MemoryTrace::UpdateSysInfoBuffer()
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_NOT_READY;
    size_t    size   = 0;

    if (m_pRouterUtilsApi != nullptr)
    {
        result = m_pRouterUtilsApi->GetSysInfo(m_pRouterUtilsApi->pInstance, nullptr, &size);

        if (result == DD_RESULT_SUCCESS)
        {
            m_sysInfoBuffer.ResizeAndZero(size);
            result =
                m_pRouterUtilsApi->GetSysInfo(
                    m_pRouterUtilsApi->pInstance,
                    reinterpret_cast<char*>(m_sysInfoBuffer.Data()),
                    &size);
        }
    }

    return result;
}

DD_RESULT MemoryTrace::EndTracing(DDConnectionId umdConnectionId, bool isClientInitialized)
{
    DD_RESULT result = DD_RESULT_COMMON_DOES_NOT_EXIST;
    RWLockGuard<RWLock::LockType::Read> lock(m_clientsMutex);

    auto foundItr = m_traceEnabledClients.find(umdConnectionId);
    if (foundItr != m_traceEnabledClients.end())
    {
        result = foundItr->second->EndTrace(
            RmtEventTracer::EndTraceReason::AppExited,
            isClientInitialized);
    }
    else
    {
        LOG_ERROR("Failed to end tracing: no trace client with umdConnectionId (%d) exists.", umdConnectionId);
    }

    return result;
}

DD_RESULT MemoryTrace::DumpTrace(DDConnectionId umdConnectionId, bool isClientInitialized)
{
    DD_RESULT result = DD_RESULT_COMMON_DOES_NOT_EXIST;

    RWLockGuard<RWLock::LockType::Read> lock(m_clientsMutex);

    auto foundItr = m_traceEnabledClients.find(umdConnectionId);
    if (foundItr != m_traceEnabledClients.end())
    {
        result = foundItr->second->EndTrace(
            RmtEventTracer::EndTraceReason::UserRequestedContinue,
            isClientInitialized);
    }
    else
    {
        LOG_ERROR("Failed to end tracing: no trace client with umdConnectionId (%d) exists.", umdConnectionId);
    }

    return result;
}

DD_RESULT MemoryTrace::AbortTrace(DDConnectionId umdConnectionId, bool isClientInitialized)
{
    RWLockGuard<RWLock::LockType::Read> lock(m_clientsMutex);

    DD_RESULT result = DD_RESULT_COMMON_DOES_NOT_EXIST;

    auto foundItr = m_traceEnabledClients.find(umdConnectionId);
    if (foundItr != m_traceEnabledClients.end())
    {
        result = foundItr->second->EndTrace(
            RmtEventTracer::EndTraceReason::Abort,
            isClientInitialized);
    }
    else
    {
        LOG_ERROR("Failed to end tracing: no trace client with umdConnectionId (%d) exists.", umdConnectionId);
    }

    return result;
}

DD_RESULT MemoryTrace::InsertSnapshot(DDConnectionId umdConnectionId, const char* pSnapshotName)
{
    RWLockGuard<RWLock::LockType::Read> lock(m_clientsMutex);

    DD_RESULT result = DD_RESULT_SUCCESS;

    auto foundItr = m_traceEnabledClients.find(umdConnectionId);
    if (foundItr == m_traceEnabledClients.end())
    {
        result = DD_RESULT_COMMON_DOES_NOT_EXIST;
    }

    if (result == DD_RESULT_SUCCESS)
    {
        const uint64_t startTimestamp = Platform::QueryTimestamp();
        uint64 targetTicksPerSecond   = 0;
        uint64 targetTicks            = 0;

        result = m_pRouterUtilsApi->GetTimestampAndFrequency(m_pRouterUtilsApi->pInstance,
                                                             &targetTicks,
                                                             &targetTicksPerSecond);

        const uint64 stopTimestamp = Platform::QueryTimestamp();

        if (result == DD_RESULT_SUCCESS)
        {
            // Calculate how much time we spent gathering the timestamp data from the target machine
            // We use the roundtrip time divided by two here since we're trying to predict exactly when the target
            // machine sampled its timestamp value. This should be the instant the request arrives, minus the time it
            // takes to send the response back to us. We're assuming the send and receive delays are symmetrical here,
            // but it's more accurate than not using an offset at all.
            const uint64 elapsedTimeInTicks = ((stopTimestamp - startTimestamp) / 2);
            const uint64 elapsedTimeInUs    = (elapsedTimeInTicks * 1000000) / Platform::QueryTimestampFrequency();

            // We need to convert the time offset into the correct time domain using the timestamp frequency that we
            // queried from the target machine earlier.
            const uint64 targetTicksPerUs        = (targetTicksPerSecond / 1000000);
            const uint64 targetTimeOffsetInTicks = (targetTicksPerUs * elapsedTimeInUs);

            // We don't deal with the possibility of wrapping here since that should be impossible.
            DD_ASSERT(targetTicks >= targetTimeOffsetInTicks);

            // Offset the timestamp backwards based on how long it took to retrieve the timestamp data from the target
            // machine over the network. This will make our snapshot timestamp values as close to the initial function
            // call time as they can be.
            const uint64 adjustedTimestamp = (targetTicks - targetTimeOffsetInTicks);

            result = foundItr->second->InsertSnapshot(pSnapshotName,
                                                      adjustedTimestamp);
        }
    }

    return result;
}

DD_RESULT MemoryTrace::ClearTrace(DDConnectionId umdConnectionId)
{
    RWLockGuard<RWLock::LockType::Read> lock(m_clientsMutex);

    DD_RESULT result = DD_RESULT_COMMON_DOES_NOT_EXIST;

    auto foundItr = m_traceEnabledClients.find(umdConnectionId);
    if (foundItr != m_traceEnabledClients.end())
    {
        RmtEventTracer::TraceState traceState = foundItr->second->GetTraceState();
        if (traceState == RmtEventTracer::TraceState::Ended)
        {
            foundItr->second->Clear();
            result = DD_RESULT_SUCCESS;
        }
        else
        {
            result = DD_RESULT_COMMON_INVALID_PARAMETER;
            LOG_ERROR("Failed to clear trace: wrong trace state. Current trace state: %d", traceState);
        }
    }
    else
    {
        LOG_ERROR("Failed to end tracing: no trace client with umdConnectionId (%d) exists.", umdConnectionId);
    }

    return result;
}

DD_RESULT MemoryTrace::QueryStatus(DDConnectionId umdConnectionId, DDMemoryTraceStatus* pStatus)
{
    RWLockGuard<RWLock::LockType::Read> lock(m_clientsMutex);

    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    auto foundItr = m_traceEnabledClients.find(umdConnectionId);

    if ((foundItr != m_traceEnabledClients.end()) && (pStatus != nullptr))
    {
        RmtEventTracer* pTracer     = foundItr->second;
        DD_MEMORY_TRACE_STATE state = DD_MEMORY_TRACE_STATE_UNKNOWN;

        switch (pTracer->GetTraceState())
        {
            case RmtEventTracer::TraceState::NotStarted:
            {
                state = DD_MEMORY_TRACE_STATE_NOT_STARTED;
                break;
            }
            case RmtEventTracer::TraceState::Running:
            {
                state = DD_MEMORY_TRACE_STATE_RUNNING;
                break;
            }
            case RmtEventTracer::TraceState::Ended:
            {
                switch (pTracer->GetEndTraceReason())
                {
                    case RmtEventTracer::EndTraceReason::Unknown:
                    {
                        state = DD_MEMORY_TRACE_STATE_ENDED_UNKNOWN;
                        break;
                    }
                    case RmtEventTracer::EndTraceReason::UserRequested:
                    {
                        state = DD_MEMORY_TRACE_STATE_ENDED_USER_REQUESTED;
                        break;
                    }
                    case RmtEventTracer::EndTraceReason::AppRequested:
                    {
                        state = DD_MEMORY_TRACE_STATE_ENDED_APP_REQUESTED;
                        break;
                    }
                    case RmtEventTracer::EndTraceReason::AppExited:
                    {
                        state = DD_MEMORY_TRACE_STATE_ENDED_APP_EXITED;
                        break;
                    }
                    case RmtEventTracer::EndTraceReason::UserRequestedContinue:
                    {
                        // We should not be able to hit this case, if the user requested we continue the trace state should still be Running.
                        // fallthrough
                    }
                    default:
                    {
                        DD_ASSERT_ALWAYS();
                    }
                }

                break;
            }
        }

        pStatus->state  = state;
        pStatus->size   = pTracer->GetTotalDataSize();
        pStatus->result = pTracer->GetTraceResult();

        result = DD_RESULT_SUCCESS;
    }

    return result;
}

DD_RESULT MemoryTrace::TransferTraceData(DDConnectionId         umdConnectionId,
                                         const DDRdfFileWriter* pFileWriter,
                                         const DDIOHeartbeat*   pIoCb,
                                         bool                   useCompression)
{
    RWLockGuard<RWLock::LockType::Read> lock(m_clientsMutex);

    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    auto foundItr = m_traceEnabledClients.find(umdConnectionId);

    if ((foundItr != m_traceEnabledClients.end()) &&
        foundItr->second != nullptr               &&
        IsValidDDIOHeartbeat(pIoCb)               &&
        IsValidDDRdfFileWriter(pFileWriter))
    {
        RmtEventTracer* pTracer = foundItr->second;

        // Note that totalDataSize doesn't account for the asic info, heaps, or snapshots.
        result = pIoCb->pfnWriteHeartbeat(pIoCb->pUserdata,
                                          DD_RESULT_SUCCESS,
                                          DD_IO_STATUS_BEGIN,
                                          static_cast<size_t>(pTracer->GetTotalDataSize()));

        if (result == DD_RESULT_SUCCESS)
        {
            uint32_t dd_rdf_version = RDF_MAKE_VERSION(DD_RDF_USERSTREAM_INTERFACE_VERSION_MAJOR,
                                                       DD_RDF_USERSTREAM_INTERFACE_VERSION_MINOR,
                                                       DD_RDF_USERSTREAM_INTERFACE_VERSION_PATCH);

            if (dd_rdf_version == RDF_INTERFACE_VERSION)
            {
                rdfUserStream userStream = {};
                userStream.Read          = pFileWriter->pfnFileRead;
                userStream.Write         = pFileWriter->pfnFileWrite;
                userStream.Tell          = pFileWriter->pfnFileTell;
                userStream.Seek          = pFileWriter->pfnFileSeek;
                userStream.GetSize       = pFileWriter->pfnFileGetSize;
                userStream.context       = pFileWriter->pUserData;

                // Create the RDF stream to write all RDF chunks.
                rdfStream*          pRdfStream      = nullptr;
                rdfChunkFileWriter* pRdfChunkWriter = nullptr;
                int                 rdfResult       = rdfStreamCreateFromUserStream(&userStream, &pRdfStream);

                if (rdfResult == rdfResult::rdfResultOk)
                {
                    // Create the chunkFileWriter to setup the chunk data
                    // structures and buffers to collect the incoming chunks
                    rdfResult = rdfChunkFileWriterCreate(pRdfStream, &pRdfChunkWriter);

                    if (rdfResult == rdfResult::rdfResultOk)
                    {
                        result = pTracer->TransferTraceData(pIoCb, pRdfChunkWriter, useCompression);

                        if (result == DD_RESULT_SUCCESS)
                        {
                            // Destroying (closing) the ChunkWriter ensures that all
                            // data, both compressed and uncompressed, is written to
                            // the data stream. This step also completes the RDF file
                            // by adding the final parts (index entries) of the file.
                            // Trace data and data-sizes will be correctly output only
                            // after this step.
                            rdfResult = rdfChunkFileWriterDestroy(&pRdfChunkWriter);
                        }
                        else
                        {
                            LOG_ERROR("Failed to TransferTraceData. DDResult: %d", result);
                        }
                    }

                    rdfStreamClose(&pRdfStream);
                }

                result = RdfResultToDDResult(rdfResult);
            }
            else
            {
                result = DD_RESULT_COMMON_VERSION_MISMATCH;
            }
        }

        // Signal to the user that the transfer is ending
        result = pIoCb->pfnWriteHeartbeat(pIoCb->pUserdata, result, DD_IO_STATUS_END, 0);
    }
    else
    {
        result = DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
    }

    return result;
}

void MemoryTrace::OnDriverConnected(const DDConnectionInfo* pConnInfo)
{
    DDLoggerInfo loggerInfo = {};
    loggerInfo.pUserdata    = m_pLogger;
    loggerInfo.pfnLog       = LoggerLogLegacy;
    loggerInfo.pfnWillLog   = WillLogLegacy;
    loggerInfo.pfnPop       = LoggerLogLegacy;
    loggerInfo.pfnPush      = LoggerLogLegacy;
    LoggerUtil loggerUtil(loggerInfo);

    DDAllocCallbacks allocCb = {};
    allocCb.pfnAlloc         = AllocCbFuncLegacy;
    allocCb.pfnFree          = FreeCbFuncLegacy;
    allocCb.pUserdata        = nullptr;

    RmtEventTracer* pTracer = new RmtEventTracer(loggerUtil, allocCb);

    std::pair<decltype(m_traceEnabledClients)::iterator, bool> emplaceItr;
    {
        // Set up trace client per each umd connection in OnDriverConnected() to minimize
        // the time the write lock needs to be held.
        RWLockGuard<RWLock::LockType::Write> lock(m_clientsMutex);
        emplaceItr = m_traceEnabledClients.emplace(pConnInfo->umdConnectionId, pTracer);
    }

    if (emplaceItr.second == false)
    {
        delete pTracer;

        LOG_ERROR(
            "OnDriverConnected() was already called with the same umd connection id: %u",
            pConnInfo->umdConnectionId);
    }
}

} // namespace DevDriver
