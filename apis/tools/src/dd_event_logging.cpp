/* Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <new>
#include "dd_legacy_utils.h"
#include <dd_event_logging.h>

#include <ddCommon.h>
#include <RdfEventStreamer.h>

namespace
{

// DDEventLoggingApi wrapper functions.

DD_RESULT EnableTracingWrapper(
    DDEventLoggingInstance* pInstance,
    DDConnectionId          connectionId,
    DDProcessId             processId,
    uint32_t                providerId,
    bool                    noUmd)
{
    DevDriver::EventLogging* pEventLogging = reinterpret_cast<DevDriver::EventLogging*>(pInstance);
    return pEventLogging->EnableTracing(connectionId, processId, providerId, noUmd);
}

DD_RESULT RegisterEventReceiveCbWrapper(DDEventLoggingInstance*            pInstance,
                                        const DDEventReceiveEventCallback* pReceiveCallback)
{
    DevDriver::EventLogging* pEventLogging = reinterpret_cast<DevDriver::EventLogging*>(pInstance);
    return pEventLogging->RegisterEventReceiveCb(pReceiveCallback);
}

void DisableTracingWrapper(DDEventLoggingInstance* pInstance, DDConnectionId connectionId)
{
    DevDriver::EventLogging* pEventLogging = reinterpret_cast<DevDriver::EventLogging*>(pInstance);
    pEventLogging->DisableTracing(connectionId);
}

DD_RESULT EndTracingWrapper(DDEventLoggingInstance* pInstance,
                            DDConnectionId          connectionId,
                            bool                    isClientInitialized)
{
    DevDriver::EventLogging* pEventLogging = reinterpret_cast<DevDriver::EventLogging*>(pInstance);
    return pEventLogging->EndTracing(connectionId, isClientInitialized);
}

DD_RESULT TransferTraceDataWrapper(DDEventLoggingInstance* pInstance,
                                   DDConnectionId          connectionId,
                                   const DDRdfFileWriter*  pRdfFileWriter,
                                   const DDIOHeartbeat*    pHeartbeat)
{
    DevDriver::EventLogging* pEventLogging = reinterpret_cast<DevDriver::EventLogging*>(pInstance);
    return pEventLogging->TransferTraceData(connectionId, pRdfFileWriter, pHeartbeat);
}

} // anonymous namespace

namespace DevDriver
{

EventLogging::EventLogging()
    : m_net(DD_API_INVALID_HANDLE)
    , m_pConnectionApi(nullptr)
    , m_pLogger(nullptr)
    , m_receiveCallback()
    , m_eventPayloadBuffer(1024)
{
}

EventLogging::~EventLogging()
{
    ClearAfterRouterDisconnect();
}

DD_RESULT EventLogging::Initialize(DDApiRegistry* pApiRegistry)
{
    DD_RESULT result = pApiRegistry->Get(
        pApiRegistry->pInstance,
        DD_LOGGER_API_NAME,
        DDVersion{
            DD_LOGGER_API_VERSION_MAJOR,
            DD_LOGGER_API_VERSION_MINOR,
            DD_LOGGER_API_VERSION_PATCH},
        reinterpret_cast<void**>(&m_pLogger));

    DD_ASSERT(result == DD_RESULT_SUCCESS);
    if (result == DD_RESULT_SUCCESS)
    {
        result = pApiRegistry->Get(
            pApiRegistry->pInstance,
            DD_CONNECTION_API_NAME,
            DDVersion{
                DD_CONNECTION_API_VERSION_MAJOR,
                DD_CONNECTION_API_VERSION_MINOR,
                DD_CONNECTION_API_VERSION_PATCH},
            reinterpret_cast<void**>(&m_pConnectionApi));

        if (result == DD_RESULT_SUCCESS)
        {
            // Register connection callbacks
            DDConnectionCallbacks connectionCbs = {};
            connectionCbs.pImpl                 = reinterpret_cast<DDConnectionCallbacksImpl*>(this);

            m_pConnectionApi->AddConnectionCallbacks(m_pConnectionApi->pInstance, &connectionCbs);
        }
    }

    DD_ASSERT(result == DD_RESULT_SUCCESS);
    if (result == DD_RESULT_SUCCESS)
    {
        DDEventLoggingApi EventLoggingApi{
            reinterpret_cast<DDEventLoggingInstance*>(this),
            EnableTracingWrapper,
            RegisterEventReceiveCbWrapper,
            DisableTracingWrapper,
            EndTracingWrapper,
            TransferTraceDataWrapper};

        result = pApiRegistry->Add(
            pApiRegistry->pInstance,
            DD_EVENT_LOGGING_API_NAME,
            DDVersion{
                DD_EVENT_LOGGING_API_VERSION_MAJOR,
                DD_EVENT_LOGGING_API_VERSION_MINOR,
                DD_EVENT_LOGGING_API_VERSION_PATCH},
            &EventLoggingApi,
            sizeof(EventLoggingApi));

        if (result != DD_RESULT_SUCCESS)
        {
            m_pLogger->Log(
                m_pLogger->pInstance,
                DD_LOG_LVL_ERROR,
                "[EventLogging] Failed to register DDEventLoggingApi. DD_RESULT: %u.",
                result);
        }
    }

    return result;
}

void EventLogging::ClearAfterRouterDisconnect()
{
    m_net            = DD_API_INVALID_HANDLE;
}

void EventLogging::SetRpcClientInfo(DDNetConnection ddNet)
{
    m_net = ddNet;
}

static void ReceiveStreamerEventFunc(
    void* pUserdata,
    DDEventParserEventInfo eventInfo,
    const void* pEventDataPayload,
    size_t                 eventDataPayloadSize)
{
    EventLogging* pEventLogging = static_cast<EventLogging*>(pUserdata);
    pEventLogging->ReceiveStreamerEvent(eventInfo, pEventDataPayload, eventDataPayloadSize);
}

void EventLogging::ReceiveStreamerEvent(
    DDEventParserEventInfo eventInfo,
    const void*            pEventDataPayload,
    size_t                 eventDataPayloadSize)
{
    if (m_receiveCallback.ReceiveEvent != nullptr)
    {
        DDEventLoggingEventInfo eventLoggingInfo = {};
        eventLoggingInfo.eventId = eventInfo.eventId;
        eventLoggingInfo.eventIndex = eventInfo.eventIndex;
        eventLoggingInfo.providerId = eventInfo.providerId;
        eventLoggingInfo.timestamp = eventInfo.timestamp;
        eventLoggingInfo.timestampFrequency = eventInfo.timestampFrequency;
        eventLoggingInfo.totalPayloadSize = eventInfo.totalPayloadSize;

        if (eventDataPayloadSize > m_eventPayloadBuffer.size())
        {
            m_eventPayloadBuffer.resize(eventDataPayloadSize);
        }
        Platform::Memcpy_s(m_eventPayloadBuffer.data(), eventDataPayloadSize, pEventDataPayload, eventDataPayloadSize);

        m_receiveCallback.ReceiveEvent(m_receiveCallback.pImpl, eventLoggingInfo, m_eventPayloadBuffer.data());
    }
}

DD_RESULT EventLogging::EnableTracing(DDConnectionId connectionId, DDProcessId processId, uint32_t providerId, bool noUmdConnection)
{
    DD_API_UNUSED(processId);

    DDLoggerInfo loggerInfo = {};
    loggerInfo.pUserdata    = m_pLogger;
    loggerInfo.pfnLog       = LoggerLogLegacy;
    loggerInfo.pfnWillLog   = WillLogLegacy;
    loggerInfo.pfnPop       = LoggerLogLegacy;
    loggerInfo.pfnPush      = LoggerLogLegacy;
    LoggerUtil loggerUtil(loggerInfo);

    LockGuard lock(m_clientsMutex);
    DD_RESULT result = DD_RESULT_DD_GENERIC_NOT_READY;

    if (m_net != DD_API_INVALID_HANDLE)
    {
        if (m_traceEnabledClients.count(connectionId) == 0)
        {
            DD_DRIVER_STATE state = DD_DRIVER_STATE_UNKNOWN;
            m_pConnectionApi->GetDriverState(m_pConnectionApi->pInstance, connectionId, &state);
            if ((DD_DRIVER_STATE_PLATFORMINIT == state) || (noUmdConnection == true))
            {
                RdfEventStreamer* pStreamer  = new(std::nothrow) RdfEventStreamer(loggerUtil);

                if (pStreamer != nullptr)
                {
                    m_traceEnabledClients.emplace(connectionId, pStreamer);

                    const DDClientId umdId = static_cast<DDClientId>(connectionId);

                    pStreamer->RegisterReceiveEventFunc(this, &ReceiveStreamerEventFunc);

                    result = pStreamer->BeginStreaming(umdId, m_net, providerId);
                    if (result == DD_RESULT_SUCCESS)
                    {
                        m_pLogger->Log(m_pLogger->pInstance,
                                       DD_LOG_LVL_INFO,
                                       "[EventLogging] Successfully started Event Logging.");
                    }
                    else
                    {
                        m_pLogger->Log(m_pLogger->pInstance,
                                       DD_LOG_LVL_ERROR,
                                       "[EventLogging] Failed to begin Event Logging. Result: %s",
                                       ddApiResultToString(result));
                    }

                    if (result != DD_RESULT_SUCCESS)
                    {
                        EndTraceInternal(pStreamer, false);
                        delete pStreamer;

                        m_traceEnabledClients.erase(connectionId);
                    }
                }
                else
                {
                    result = DD_RESULT_COMMON_OUT_OF_HEAP_MEMORY;
                }
            }
            else
            {
                m_pLogger->Log(
                    m_pLogger->pInstance,
                    DD_LOG_LVL_ERROR,
                    "[EventLogging] EnableTracing should be called during driver state 'PlatformInit', but the "
                    "current driver state is: %d", state);

                DD_ASSERT(false);
                result = DD_RESULT_DD_GENERIC_NOT_READY;
            }
        }
        else
        {
            result = DD_RESULT_SUCCESS;
        }
    }

    return result;
}

DD_RESULT EventLogging::RegisterEventReceiveCb(const DDEventReceiveEventCallback* pReceiveCallback)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;
    // We either need the entire struct to be null, or if it's not null for the callback function
    // to be non-null.
    if ((pReceiveCallback != nullptr) && (pReceiveCallback->ReceiveEvent != nullptr))
    {
        m_receiveCallback.pImpl = pReceiveCallback->pImpl;
        m_receiveCallback.ReceiveEvent = pReceiveCallback->ReceiveEvent;
        result = DD_RESULT_SUCCESS;
    }

    return result;
}

void EventLogging::DisableTracing(DDConnectionId connectionId)
{
    LockGuard clientLock(m_clientsMutex);
    if (m_traceEnabledClients.count(connectionId) != 0)
    {
        RdfEventStreamer* pStreamer = m_traceEnabledClients[connectionId];
        delete pStreamer;
        m_traceEnabledClients.erase(connectionId);
    }
}

DD_RESULT EventLogging::EndTracing(DDConnectionId connectionId,
                                   bool           isClientInitialized)
{
    LockGuard clientLock(m_clientsMutex);
    DD_RESULT result = DD_RESULT_DD_GENERIC_NOT_READY;

    if (m_traceEnabledClients.count(connectionId) != 0)
    {
        RdfEventStreamer* pStreamer = m_traceEnabledClients[connectionId];
        DD_ASSERT(pStreamer != nullptr);
        result = EndTraceInternal(pStreamer, isClientInitialized);

        if (result != DD_RESULT_SUCCESS)
        {
            m_pLogger->Log(m_pLogger->pInstance, DD_LOG_LVL_ERROR, "[EventLogging] Failed to end RGD trace.");
        }
    }

    return result;
}

DD_RESULT EventLogging::EndTraceInternal(RdfEventStreamer* pStreamer,
                                         bool              isClientInitialized)
{
    DD_API_UNUSED(isClientInitialized);

    DD_RESULT result = pStreamer->EndStreaming(false);
    if (result != DD_RESULT_SUCCESS)
    {
        m_pLogger->Log(
            m_pLogger->pInstance,
            DD_LOG_LVL_ERROR,
            "[EventLogging] Failed to end event streaming, result: %s",
            ddApiResultToString(result));
    }

    return result;
}

DD_RESULT EventLogging::TransferTraceData(DDConnectionId         connectionId,
                                          const DDRdfFileWriter* pRdfFileWriter,
                                          const DDIOHeartbeat*   pHeartbeat)
{
    LockGuard lock(m_clientsMutex);
    RdfEventStreamer* pStreamer = m_traceEnabledClients[connectionId];

    // Because DDRdfFileCallbacks is an exact copy of rdfUserStream, we want
    // to be warned even minor version changed.
    uint32_t dd_rdf_version = RDF_MAKE_VERSION(DD_RDF_USERSTREAM_INTERFACE_VERSION_MAJOR,
                                               DD_RDF_USERSTREAM_INTERFACE_VERSION_MINOR,
                                               DD_RDF_USERSTREAM_INTERFACE_VERSION_PATCH);

    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;
    if (dd_rdf_version == RDF_INTERFACE_VERSION)
    {
        rdfUserStream userstream{};
        userstream.context = pRdfFileWriter->pUserData;
        userstream.Read    = pRdfFileWriter->pfnFileRead;
        userstream.Write   = pRdfFileWriter->pfnFileWrite;
        userstream.Tell    = pRdfFileWriter->pfnFileTell;
        userstream.Seek    = pRdfFileWriter->pfnFileSeek;
        userstream.GetSize = pRdfFileWriter->pfnFileGetSize;

        result = pHeartbeat->pfnWriteHeartbeat(pHeartbeat->pUserdata,
                                               DD_RESULT_SUCCESS,
                                               DD_IO_STATUS_BEGIN,
                                               static_cast<size_t>(pStreamer->GetTotalDataSize()));

        if (result == DD_RESULT_SUCCESS)
        {
            // Create the RDF stream to write the RDF chunk.
            rdfStream* pRdfStream = nullptr;
            int        rdfResult  = rdfStreamCreateFromUserStream(&userstream, &pRdfStream);

            if (rdfResult == rdfResult::rdfResultOk)
            {
                rdfChunkFileWriter* pRdfChunkWriter = nullptr;
                // Create the chunkFileWriter to setup the chunk data
                // structures and buffers to collect the incoming chunks
                rdfResult = rdfChunkFileWriterCreate(pRdfStream, &pRdfChunkWriter);

                const bool useCompression = true;
                result = pStreamer->TransferDataStream((*pHeartbeat), pRdfChunkWriter, useCompression);

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
                    m_pLogger->Log(m_pLogger->pInstance, DD_LOG_LVL_WARN, "[EventLogging] Event data write failed");
                }

                rdfStreamClose(&pRdfStream);
            }

            result = ConvertRdfResult(rdfResult);
        }
        else
        {
            m_pLogger->Log(m_pLogger->pInstance, DD_LOG_LVL_ERROR, "[EventLogging] Failed to begin Heartbeat.");
        }

        pHeartbeat->pfnWriteHeartbeat(pHeartbeat->pUserdata, result, DD_IO_STATUS_END, 0);
    }
    else
    {
        m_pLogger->Log(m_pLogger->pInstance, DD_LOG_LVL_ERROR, "[EventLogging] Version mismatch between DDRdf and amdrdf.");
    }

    return result;
}

} // namespace DevDriver
