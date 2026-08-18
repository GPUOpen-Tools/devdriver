/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "dd_legacy_utils.h"
#include <dd_gpu_detective.h>
#include <dd_result.h>

#include <GpuDetectiveEventStreamer.h>
#include <dd_event/gpu_detective/umd_crash_analysis.h>
#include <dd_event/gpu_detective/kernel_crash_analysis.h>

#include <cstring>

#define LOG_INFO(fmt, ...) m_pLogger->Log(              \
                               m_pLogger->pInstance,    \
                               DD_LOG_LVL_INFO,         \
                               "[DDGpuDetective] " fmt, \
                               ## __VA_ARGS__)

#define LOG_ERROR(fmt, ...) m_pLogger->Log(               \
                                m_pLogger->pInstance,     \
                                DD_LOG_LVL_ERROR,         \
                                "[DDGpuDetective] " fmt,  \
                                ## __VA_ARGS__)

namespace
{

// DDGpuDetectiveApi wrapper functions.

DD_RESULT EnableTracingWrapper(DDGpuDetectiveInstance* pInstance, DDConnectionId umdConnectionId, DDProcessId processId)
{
    DevDriver::GpuDetective* pGpuDetective = reinterpret_cast<DevDriver::GpuDetective*>(pInstance);
    return pGpuDetective->EnableTracing(umdConnectionId, processId);
}

void DisableTracingWrapper(DDGpuDetectiveInstance* pInstance, DDConnectionId umdConnectionId)
{
    DevDriver::GpuDetective* pGpuDetective = reinterpret_cast<DevDriver::GpuDetective*>(pInstance);
    pGpuDetective->DisableTracing(umdConnectionId);
}

DD_RESULT EndTracingWrapper(DDGpuDetectiveInstance* pInstance,
                            DDConnectionId          umdConnectionId,
                            bool                    isClientInitialized,
                            bool*                   didDetectCrash)
{
    DevDriver::GpuDetective* pGpuDetective = reinterpret_cast<DevDriver::GpuDetective*>(pInstance);
    return pGpuDetective->EndTracing(umdConnectionId, isClientInitialized, didDetectCrash);
}

DD_RESULT TransferTraceDataWrapper(DDGpuDetectiveInstance* pInstance,
                                   DDConnectionId          umdConnectionId,
                                   const DDRdfFileWriter*  pRdfFileWriter,
                                   const DDIOHeartbeat*    pHeartbeat)
{
    DevDriver::GpuDetective* pGpuDetective = reinterpret_cast<DevDriver::GpuDetective*>(pInstance);
    return pGpuDetective->TransferTraceData(umdConnectionId, pRdfFileWriter, pHeartbeat);
}

void UpdateSysInfoBufferWrapper(DDConnectionCallbacksImpl* pInstance, DDConnectionId connectionId)
{
    DD_UNUSED(connectionId);
    DevDriver::GpuDetective* pGpuDetective = reinterpret_cast<DevDriver::GpuDetective*>(pInstance);
    pGpuDetective->UpdateSysInfoBuffer();
}

void OnDriverConnectedWrapper(DDConnectionCallbacksImpl* pInstance, const DDConnectionInfo* pConnInfo)
{
    DevDriver::GpuDetective* pGpuDetective = reinterpret_cast<DevDriver::GpuDetective*>(pInstance);
    pGpuDetective->OnDriverConnected(pConnInfo);
}

} // anonymous namespace

namespace DevDriver
{

GpuDetective::GpuDetective()
    : m_net(DD_API_INVALID_HANDLE)
    , m_pSystemClients(nullptr)
    , m_sysInfoBuffer(Platform::GenericAllocCb)
    , m_pRouterUtilsApi(nullptr)
    , m_pConnectionApi(nullptr)
    , m_pLogger(nullptr)
{
}

GpuDetective::~GpuDetective()
{
    ClearAfterRouterDisconnect();
}

DD_RESULT GpuDetective::Initialize(DDApiRegistry* pApiRegistry)
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
            DD_ROUTER_UTILS_API_NAME,
            DDVersion{
                DD_ROUTER_UTILS_API_VERSION_MAJOR,
                DD_ROUTER_UTILS_API_VERSION_MINOR,
                DD_ROUTER_UTILS_API_VERSION_PATCH},
            reinterpret_cast<void**>(&m_pRouterUtilsApi));
        DD_ASSERT(result == DD_RESULT_SUCCESS);
    }

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
            connectionCbs.OnRouterConnected     = &UpdateSysInfoBufferWrapper;
            connectionCbs.OnDriverConnected     = &OnDriverConnectedWrapper;

            m_pConnectionApi->AddConnectionCallbacks(m_pConnectionApi->pInstance, &connectionCbs);
        }
    }

    DD_ASSERT(result == DD_RESULT_SUCCESS);
    if (result == DD_RESULT_SUCCESS)
    {
        DDGpuDetectiveApi gpuDetectiveApi{
            reinterpret_cast<DDGpuDetectiveInstance*>(this),
            EnableTracingWrapper,
            DisableTracingWrapper,
            EndTracingWrapper,
            TransferTraceDataWrapper};

        result = pApiRegistry->Add(
            pApiRegistry->pInstance,
            DD_GPU_DETECTIVE_API_NAME,
            DDVersion{
                DD_GPU_DETECTIVE_API_VERSION_MAJOR,
                DD_GPU_DETECTIVE_API_VERSION_MINOR,
                DD_GPU_DETECTIVE_API_VERSION_PATCH},
            &gpuDetectiveApi,
            sizeof(gpuDetectiveApi));

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("Failed to register DDGpuDetectiveApi. DD_RESULT: %s.", StringResult(result));
        }
    }

    return result;
}

void GpuDetective::ClearAfterRouterDisconnect()
{
    m_net            = DD_API_INVALID_HANDLE;
    m_pSystemClients = nullptr;
}

void GpuDetective::OnDriverConnected(const DDConnectionInfo* pConnInfo)
{
    // RGD tool only care about API type, not major/minor version, leave them zero.
    TraceChunkApiInfo apiInfo {};

    // Reference: Pal::Platform::GetClientApiStr()
    if (std::strstr(pConnInfo->pDescription, "Vulkan") != nullptr)
    {
        apiInfo.apiType = ApiType::VULKAN;
    }
    else if (std::strstr(pConnInfo->pDescription, "DirectX12") != nullptr)
    {
        apiInfo.apiType = ApiType::DIRECTX_12;
    }
    else
    {
        apiInfo.apiType = ApiType::GENERIC;
    }

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

    RmtEventTracer*       pTracer      = new RmtEventTracer(loggerUtil, allocCb);
    GPUDetectiveStreamer* pKmdStreamer = new GPUDetectiveStreamer(loggerUtil);
    GPUDetectiveStreamer* umdStreamer  = new GPUDetectiveStreamer(loggerUtil);

    std::pair<decltype(m_traceEnabledClients)::iterator, bool> emplaceItr;
    {
        RWLockGuard<RWLock::LockType::Write> lock(m_clientsMutex);

        emplaceItr = m_traceEnabledClients.emplace(
            pConnInfo->umdConnectionId,
            TraceEnabledClient
            {
                apiInfo,
                pTracer,
                pKmdStreamer,
                umdStreamer
            }
        );
    }

    if (emplaceItr.second == false)
    {
        delete pTracer;
        delete pKmdStreamer;
        delete umdStreamer;

        LOG_ERROR(
            "OnDriverConnected() was already called with the same umd connection id: %u",
            pConnInfo->umdConnectionId);
    }
}

DD_RESULT GpuDetective::UpdateSysInfoBuffer()
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_NOT_READY;
    size_t    size   = 0;

    if (m_pRouterUtilsApi != nullptr)
    {
        result = m_pRouterUtilsApi->GetSysInfo(m_pRouterUtilsApi->pInstance, nullptr, &size);
        if (result == DD_RESULT_SUCCESS)
        {
            m_sysInfoBuffer.ResizeAndZero(size);
            result = m_pRouterUtilsApi->GetSysInfo(
                m_pRouterUtilsApi->pInstance,
                reinterpret_cast<char*>(m_sysInfoBuffer.Data()),
                &size);
        }
    }

    return result;
}

void GpuDetective::SetRpcClientInfo(DDNetConnection ddNet)
{
    m_net = ddNet;
}

void GpuDetective::SetSystemClients(const DDModuleSystemClientInfo* pSysClients)
{
    m_pSystemClients = pSysClients;
}

DD_RESULT GpuDetective::EnableTracing(DDConnectionId umdConnectionId, DDProcessId processId)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    if ((m_net == DD_API_INVALID_HANDLE) || (m_pSystemClients == nullptr))
    {
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
        LOG_ERROR("Failed to enalbe tracing: m_net or m_pSystemClients is invalid.");
    }

    if (result == DD_RESULT_SUCCESS)
    {
        RWLockGuard<RWLock::LockType::Read> lock(m_clientsMutex);

        auto foundItr = m_traceEnabledClients.find(umdConnectionId);
        if (foundItr == m_traceEnabledClients.end())
        {
            result = DD_RESULT_COMMON_DOES_NOT_EXIST;
            LOG_ERROR("TraceClient data has not been initialized.");
        }

        if (result == DD_RESULT_SUCCESS)
        {
            DD_DRIVER_STATE state = DD_DRIVER_STATE_UNKNOWN;
            result = m_pConnectionApi->GetDriverState(m_pConnectionApi->pInstance, umdConnectionId, &state);
            if (state != DD_DRIVER_STATE_PLATFORMINIT)
            {
                result = DD_RESULT_DD_GENERIC_NOT_READY;
                LOG_ERROR(
                    "EnableTracing should be called during driver state 'PlatformInit', "
                    "but the current driver state is: %d",
                    state);

                DD_ASSERT_ALWAYS();
            }
        }

        RmtEventTracer*       pTracer      = nullptr;
        GPUDetectiveStreamer* pKmdStreamer = nullptr;
        GPUDetectiveStreamer* umdStreamer  = nullptr;

        if (result == DD_RESULT_SUCCESS)
        {
            pTracer      = foundItr->second.m_rmtTracer;
            pKmdStreamer = foundItr->second.m_krnlCrashAnalysisStreamer;
            umdStreamer  = foundItr->second.m_umdCrashAnalysisStreamer;

            if (pTracer == nullptr)
            {
                LOG_ERROR("RmtEventTracer is not initialized.");
                result = DD_RESULT_COMMON_DOES_NOT_EXIST;
            }

            if (pKmdStreamer == nullptr)
            {
                LOG_ERROR("pKmdStreamer is not initialized.");
                result = DD_RESULT_COMMON_DOES_NOT_EXIST;
            }

            if (umdStreamer == nullptr)
            {
                LOG_ERROR("umdStreamer is not initialized.");
                result = DD_RESULT_COMMON_DOES_NOT_EXIST;
            }
        }

        const DDClientId gfxKernelId = m_pSystemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_GRAPHICS_DRIVER].id;
        const DDClientId amdlogId    = m_pSystemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_UTILITY_DRIVER].id;
        const DDClientId routerId    = m_pSystemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_ROUTER].id;
        const DDClientId umdId       = static_cast<DDClientId>(umdConnectionId);

        if (result == DD_RESULT_SUCCESS)
        {
            result = pTracer->BeginTrace(
                processId,
                m_net,
                gfxKernelId,
                amdlogId,
                umdId,
                routerId,
                m_sysInfoBuffer,
                true);

            if (result != DD_RESULT_SUCCESS)
            {
                LOG_ERROR("Failed to begin RMT trace. Result: %s", StringResult(result));
            }
        }

        if (result == DD_RESULT_SUCCESS)
        {
            result = umdStreamer->BeginStreaming(umdId, m_net, UmdCrashAnalysisEvents::ProviderId);
            if (result == DD_RESULT_SUCCESS)
            {
                LOG_INFO("Successfully started UMD execution marker trace");
            }
            else
            {
                LOG_ERROR("Failed to begin UMD execution marker trace. Result: %s", StringResult(result));
            }
        }

        if (result == DD_RESULT_SUCCESS)
        {
            result = pKmdStreamer->BeginStreaming(amdlogId, m_net, KernelCrashAnalysisEvents::ProviderId);
            if (result == DD_RESULT_SUCCESS)
            {
                LOG_INFO("Successfully started kernel event streaming.");
            }
            else
            {
                LOG_ERROR("Failed to begin kernel event streaming. Result: %s", StringResult(result));
            }
        }
    }

    if (result != DD_RESULT_SUCCESS)
    {
        RWLockGuard<RWLock::LockType::Write> lock(m_clientsMutex);

        auto foundItr = m_traceEnabledClients.find(umdConnectionId);
        if (foundItr != m_traceEnabledClients.end())
        {
            EndTraceInternal(foundItr->second, RmtEventTracer::EndTraceReason::Abort, false);
            CleanupClient(foundItr->second);
            m_traceEnabledClients.erase(foundItr);
        }
    }

    return result;
}

void GpuDetective::DisableTracing(DDConnectionId umdConnectionId)
{
    RWLockGuard<RWLock::LockType::Write> clientLock(m_clientsMutex);
    auto foundItr = m_traceEnabledClients.find(umdConnectionId);
    if (foundItr != m_traceEnabledClients.end())
    {
        CleanupClient(foundItr->second);
        m_traceEnabledClients.erase(foundItr);
    }
}

DD_RESULT GpuDetective::EndTracing(DDConnectionId umdConnectionId,
                                   bool           isClientInitialized,
                                   bool*          didDetectCrash)
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_NOT_READY;

    if (didDetectCrash != nullptr)
    {
        *didDetectCrash = false;
    }

    RWLockGuard<RWLock::LockType::Read> clientLock(m_clientsMutex);
    auto foundItr = m_traceEnabledClients.find(umdConnectionId);
    if (foundItr != m_traceEnabledClients.end())
    {
        TraceEnabledClient& client = foundItr->second;
        result = EndTraceInternal(client, RmtEventTracer::EndTraceReason::AppExited, isClientInitialized);

        if (result == DD_RESULT_SUCCESS)
        {
            bool hasCrashOccurred = client.m_krnlCrashAnalysisStreamer->HasCrashOccured() ||
                                    client.m_umdCrashAnalysisStreamer->HasCrashOccured();
            if (didDetectCrash != nullptr)
            {
                *didDetectCrash = hasCrashOccurred;
            }
        }
        else
        {
            LOG_ERROR("Failed to end RGD trace.");
        }
    }

    return result;
}

DD_RESULT GpuDetective::EndTraceInternal(GpuDetective::TraceEnabledClient& client,
                                         RmtEventTracer::EndTraceReason    endReason,
                                         bool                              isClientInitialized)
{
    DD_RESULT result = client.m_rmtTracer->EndTrace(endReason, isClientInitialized);
    if (result != DD_RESULT_SUCCESS)
    {
        LOG_ERROR("Failed to end rmt event streaming. DDResult: %s", StringResult(result));
    }

    result = client.m_umdCrashAnalysisStreamer->EndStreaming(false);
    if (result != DD_RESULT_SUCCESS)
    {
        LOG_ERROR("Failed to end UMD Execution Marker event streaming. DDResult: %s", StringResult(result));
    }

    result = client.m_krnlCrashAnalysisStreamer->EndStreaming(false);
    if (result != DD_RESULT_SUCCESS)
    {
        LOG_ERROR("Failed to end kernel crash analysis event streaming. DDResult: %s", StringResult(result));
    }

    return result;
}

DD_RESULT GpuDetective::TransferTraceData(DDConnectionId         umdConnectionId,
                                          const DDRdfFileWriter* pRdfFileWriter,
                                          const DDIOHeartbeat*   pHeartbeat)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    uint32_t dd_rdf_version = RDF_MAKE_VERSION(DD_RDF_USERSTREAM_INTERFACE_VERSION_MAJOR,
                                               DD_RDF_USERSTREAM_INTERFACE_VERSION_MINOR,
                                               DD_RDF_USERSTREAM_INTERFACE_VERSION_PATCH);

    if (dd_rdf_version != RDF_INTERFACE_VERSION)
    {
        // Because DDRdfFileCallbacks is an exact copy of rdfUserStream, we want
        // to be warned even minor version changed.
        result = DD_RESULT_COMMON_VERSION_MISMATCH;
        LOG_ERROR("Failed to TransferTraceData(), DDRdf interface version mismatch.");
    }

    RWLockGuard<RWLock::LockType::Read> lock(m_clientsMutex);

    auto foundItr = m_traceEnabledClients.end();
    if (result == DD_RESULT_SUCCESS)
    {
        foundItr = m_traceEnabledClients.find(umdConnectionId);
        if (foundItr == m_traceEnabledClients.end())
        {
            result = DD_RESULT_COMMON_DOES_NOT_EXIST;
            LOG_ERROR("TransferTraceData() failed, no trace data is associated with umd connection id: %u", umdConnectionId);
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        TraceEnabledClient& client = foundItr->second;

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
                                               static_cast<size_t>(client.m_rmtTracer->GetTotalDataSize()));

        if (result == DD_RESULT_SUCCESS)
        {
            // Create the RDF stream to write all RDF chunks.
            rdfStream* pRdfStream = nullptr;
            int        rdfResult  = rdfStreamCreateFromUserStream(&userstream, &pRdfStream);

            if (rdfResult == rdfResult::rdfResultOk)
            {
                rdfChunkFileWriter* pRdfChunkWriter = nullptr;
                // Create the chunkFileWriter to setup the chunk data
                // structures and buffers to collect the incoming chunks
                rdfResult = rdfChunkFileWriterCreate(pRdfStream, &pRdfChunkWriter);

                bool useCompression = true;
                if (rdfResult == rdfResult::rdfResultOk)
                {
                    result = client.m_rmtTracer->TransferTraceData(
                        pHeartbeat,
                        pRdfChunkWriter,
                        useCompression,
                        &client.m_apiInfo);
                }

                if (result == DD_RESULT_SUCCESS)
                {
                    result = client.m_umdCrashAnalysisStreamer->TransferDataStream(*pHeartbeat,
                                                                                   pRdfChunkWriter,
                                                                                   useCompression);
                    if (result == DD_RESULT_SUCCESS)
                    {
                        LOG_INFO(
                            "Transferred umdCrashAnalysis data: %u bytes.",
                            client.m_umdCrashAnalysisStreamer->GetTotalDataSize());
                    }
                }

                if (result == DD_RESULT_SUCCESS)
                {
                    DD_RESULT kernelResult = client.m_krnlCrashAnalysisStreamer->TransferDataStream(*pHeartbeat,
                                                                                                    pRdfChunkWriter,
                                                                                                    useCompression);
                    if (kernelResult == DD_RESULT_SUCCESS)
                    {
                        LOG_INFO(
                            "Transferred kernelCrashAnalysis data: %u bytes.",
                            client.m_krnlCrashAnalysisStreamer->GetTotalDataSize());
                    }
                    else
                    {
                        LOG_ERROR("Kernel write data failed");
                    }
                }

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
                    LOG_ERROR("Failed to write RMT data to RDF file.");
                }

                rdfStreamClose(&pRdfStream);
            }

            result = RdfResultToDDResult(rdfResult);
        }
        else
        {
            LOG_ERROR("Failed to begin Heartbeat.");
        }

        pHeartbeat->pfnWriteHeartbeat(pHeartbeat->pUserdata, result, DD_IO_STATUS_END, 0);
    }

    return result;
}

void GpuDetective::CleanupClient(GpuDetective::TraceEnabledClient& client)
{
    delete client.m_rmtTracer;
    delete client.m_krnlCrashAnalysisStreamer;
    delete client.m_umdCrashAnalysisStreamer;
}

} // namespace DevDriver
