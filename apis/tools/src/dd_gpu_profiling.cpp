/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_gpu_profiling.h>
#include <dd_gpu_profiling_api.h>

#include <thread>
#include <chrono>
#include <string.h>

#include <ddCommon.h>

#define LOG_ERROR(fmt, ...) m_pLogger->Log(m_pLogger->pInstance, DD_LOG_LVL_ERROR, "[DDGpuProfiling] " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) m_pLogger->Log(m_pLogger->pInstance, DD_LOG_LVL_WARN, "[DDGpuProfiling] " fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) m_pLogger->Log(m_pLogger->pInstance, DD_LOG_LVL_INFO, "[DDGpuProfiling] " fmt, ##__VA_ARGS__)

struct TraceTransferContext
{
    const DDGpuProfilingTraceArgs* pConfig; /// Pointer to the config structure for this trace
    bool abortRequested;                    /// Indicates if the application has requested that we abort the trace
};

namespace
{
DD_RESULT EnableTracingWrapper(DDGpuProfilingInstance* pInstance, DDConnectionId umdConnectionId, const DDGpuProfilingConfig* pConfig)
{
    auto pProfiling = reinterpret_cast<DevDriver::GpuProfiling*>(pInstance);
    return pProfiling->EnableTracing(umdConnectionId, pConfig);
}

void DisableTracingWrapper(DDGpuProfilingInstance* pInstance, DDConnectionId umdConnectionId)
{
    auto pProfiling = reinterpret_cast<DevDriver::GpuProfiling*>(pInstance);
    return pProfiling->DisableTracing(umdConnectionId);
}

void AbortTraceWrapper(DDGpuProfilingInstance* pInstance,
                            DDConnectionId          umdConnectionId)
{
    auto pProfiling = reinterpret_cast<DevDriver::GpuProfiling*>(pInstance);
    pProfiling->AbortTrace(umdConnectionId);
}

DD_RESULT ExecuteTraceWrapper(DDGpuProfilingInstance*        pInstance,
                              DDConnectionId                 umdConnectionId,
                              const DDGpuProfilingTraceArgs* pArgs)
{
    auto pProfiling = reinterpret_cast<DevDriver::GpuProfiling*>(pInstance);
    return pProfiling->ExecuteTrace(umdConnectionId, pArgs);
}

DD_RESULT QueryClientProtocolVersionWrapper(DDGpuProfilingInstance* pInstance,
                                            DDConnectionId          umdConnectionId,
                                            uint16_t*               pVersion)
{
    auto pProfiling = reinterpret_cast<DevDriver::GpuProfiling*>(pInstance);
    return pProfiling->QueryClientProtocolVersion(umdConnectionId, pVersion);
}

DD_RESULT SetSpmCountersWrapper(DDGpuProfilingInstance*           pInstance,
                                DDConnectionId                    umdConnectionId,
                                const DDGpuProfilingSpmCounterId* pCounters,
                                uint32_t                          numCounters)
{
    auto pProfiling = reinterpret_cast<DevDriver::GpuProfiling*>(pInstance);
    return pProfiling->SetSpmCounters(umdConnectionId, pCounters, numCounters);
}

// RGPClient trace chunk callback
// Called whenever the RGP client receives a new chunk of RGP data from the network
void RGPChunkFunc(const DevDriver::RGPProtocol::TraceDataChunk* pChunk, void* pUserdata)
{
    DD_ASSERT(pUserdata != nullptr);

    auto* pTransferContext = reinterpret_cast<TraceTransferContext*>(pUserdata);
    DD_ASSERT(pTransferContext != nullptr);

    const DDGpuProfilingTraceArgs* pConfig = pTransferContext->pConfig;
    DD_ASSERT(pConfig != nullptr);
    DD_ASSERT(IsValidDDByteWriter(&pConfig->writer));

    // Abort trace if application returns non success from write callback
    const bool abortRequested =
        (pConfig->writer.pfnWriteBytes(pConfig->writer.pUserdata, pChunk->data, static_cast<size_t>(pChunk->dataSize)) !=
         DD_RESULT_SUCCESS);

    pTransferContext->abortRequested = abortRequested;
}

DevDriver::RGPProtocol::CaptureTriggerMode ConvertTriggerMode(DDGpuProfilingTriggerMode value)
{
    DevDriver::RGPProtocol::CaptureTriggerMode triggerMode = DevDriver::RGPProtocol::CaptureTriggerMode::Count;

    switch (value)
    {
        case DD_GPU_PROFILING_TRIGGER_MODE_PRESENT:
        {
            triggerMode = DevDriver::RGPProtocol::CaptureTriggerMode::Present;
            break;
        }
        case DD_GPU_PROFILING_TRIGGER_MODE_MARKER:
        case DD_GPU_PROFILING_TRIGGER_MODE_TAG:
        {
            triggerMode = DevDriver::RGPProtocol::CaptureTriggerMode::Markers;
            break;
        }
        case DD_GPU_PROFILING_TRIGGER_MODE_FRAME_INDEX:
        case DD_GPU_PROFILING_TRIGGER_MODE_DISPATCH_INDEX:
        {
            triggerMode = DevDriver::RGPProtocol::CaptureTriggerMode::Index;
            break;
        }
        case DD_GPU_PROFILING_TRIGGER_MODE_UNKNOWN:
        {
            // Do nothing
            break;
        }
        default:
            break;
    }

    return triggerMode;
}

} // namespace

namespace DevDriver
{
GpuProfiling::GpuProfiling()
    : m_net(DD_API_INVALID_HANDLE),
      m_pLogger(nullptr)
{
}

GpuProfiling::~GpuProfiling()
{
    ClearAfterRouterDisconnect();
    m_pLogger = nullptr;
}

DD_RESULT GpuProfiling::Initialize(DDApiRegistry* pApiRegistry)
{
    DD_RESULT result = pApiRegistry->Get(
        pApiRegistry->pInstance,
        DD_LOGGER_API_NAME,
        DDVersion{ DD_LOGGER_API_VERSION_MAJOR, DD_LOGGER_API_VERSION_MINOR, DD_LOGGER_API_VERSION_PATCH },
        reinterpret_cast<void**>(&m_pLogger));
    DD_ASSERT(result == DD_RESULT_SUCCESS);
    if (result == DD_RESULT_SUCCESS)
    {
        DDGpuProfilingApi profilingApi{ reinterpret_cast<DDGpuProfilingInstance*>(this),
                                        EnableTracingWrapper,
                                        DisableTracingWrapper,
                                        ExecuteTraceWrapper,
                                        AbortTraceWrapper,
                                        SetSpmCountersWrapper,
                                        QueryClientProtocolVersionWrapper };

        result = pApiRegistry->Add(pApiRegistry->pInstance,
                                   DD_GPU_PROFILING_API_NAME,
                                   DDVersion{ DD_GPU_PROFILING_API_VERSION_MAJOR,
                                              DD_GPU_PROFILING_API_VERSION_MINOR,
                                              DD_GPU_PROFILING_API_VERSION_PATCH },
                                   &profilingApi,
                                   sizeof(profilingApi));

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("Failed to register DDGpuProfilingApi. DD_RESULT: %u.", result);
        }
    }

    return result;
}

void GpuProfiling::SetRpcClientInfo(DDNetConnection ddNet)
{
    m_net = ddNet;
}

void GpuProfiling::ClearAfterRouterDisconnect()
{
    m_net = DD_API_INVALID_HANDLE;
    m_clients.clear();
}

DD_RESULT GpuProfiling::EnableTracing(DDConnectionId umdConnectionId, const DDGpuProfilingConfig* pConfig)
{
    LockGuard lock(m_clientsMutex);

    Result result = Result::Success;
    if (pConfig == nullptr)
    {
        result = Result::InvalidParameter;
    }

    if (result == Result::Success)
    {
        if ((m_net != DD_API_INVALID_HANDLE) && (umdConnectionId != kBroadcastClientId))
        {
            auto rgpClient = RGPProtocol::RGPClient(reinterpret_cast<IMsgChannel*>(m_net));
            auto driverControlClient = DriverControlProtocol::DriverControlClient(reinterpret_cast<IMsgChannel*>(m_net));
            Client* pClient = new Client{ rgpClient, driverControlClient, DriverUtils::DriverUtilsClient{}, {}, false, false };

            auto insertedItr = m_clients.insert({umdConnectionId, pClient});
            if (insertedItr.second)
            {
                result = pClient->rgpClient.Connect(umdConnectionId);

                if (result == Result::Success)
                {
                    result = pClient->driverControlClient.Connect(umdConnectionId);
                }

                if (result == Result::Success)
                {
                    DDRpcClientCreateInfo info = {};
                    info.hConnection           = m_net;
                    info.clientId              = umdConnectionId;

                    if (pClient->driverUtilsClient.Connect(info) != DD_RESULT_SUCCESS)
                    {
                        LOG_ERROR("DriverUtils RPC failed to connect. DD_RESULT: %u.", result);
                    }
                }

                if (result == Result::Success)
                {
                    result = pClient->rgpClient.EnableProfiling();
                }

                if (result != Result::Success)
                {
                    LOG_ERROR("Failed to enable RGP profiling on remote client with id %u",
                              static_cast<uint32_t>(pClient->rgpClient.GetRemoteClientId()));
                }

                if (result == Result::Success)
                {
                    result = UpdateTraceParameters(*pClient, pConfig);
                }

                if (result != Result::Success)
                {
                    LOG_ERROR("Failed to update pre-launch trace parameters on remote client with id %u",
                              static_cast<uint32_t>(pClient->rgpClient.GetRemoteClientId()));
                }

                if (result != Result::Success)
                {
                    m_clients.erase(umdConnectionId);
                    delete pClient;
                }
            }
            else // A client with the same umdConnectionId already exists
            {
                delete pClient;
            }
        }
        else
        {
            result = Result::NotReady;
        }
    }

    return DevDriverToDDResult(result);
}

void GpuProfiling::DisableTracing(DDConnectionId umdConnectionId)
{
    // We need to wait (the while loop) to ensure that the client is not still running a trace before we destroy it since
    // ExecuteTrace doesn't hold the lock for the entire duration of the function.
    // We also don't want to hold a lock on m_clientsMutex for too long to stop other clients from connecting.
    Client* pClient = nullptr;
    m_clientsMutex.Lock();

    auto foundItr = m_clients.find(umdConnectionId);
    if (foundItr != m_clients.end())
    {
        pClient = foundItr->second;
    }

    m_clientsMutex.Unlock();

    while (pClient != nullptr)
    {
        static constexpr uint32_t kFinishedTracePollTime = 50;
        while (pClient->executing_trace)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(kFinishedTracePollTime));
        }

        // Need to do an additional check JUST IN CASE something executed a trace in between the wait and now.
        // In reality this is incredibly unlikely to happen, and the outer loop will really only run once.
        LockGuard lock(m_clientsMutex);
        if (!pClient->executing_trace)
        {
            m_clients.erase(umdConnectionId);
            delete pClient;

            pClient = nullptr;
        }
    }

}

DD_RESULT GpuProfiling::ExecuteTrace(DDConnectionId umdConnectionId, const DDGpuProfilingTraceArgs* pArgs)
{
    Result  result = Result::Success;
    Client* pClient = nullptr;

    // Because client->rgpClient.EndTrace might take quite a long time, we don't want to hold the lock to the clients
    // for that time, otherwise no other profiling operations will be able to be performed. We set executing_trace
    // to true which prevents writes to the client while it is executing the trace.
    {
        LockGuard lock(m_clientsMutex);
        auto foundItr = m_clients.find(umdConnectionId);
        if (foundItr != m_clients.end())
        {
            pClient = foundItr->second;
        }

        // We require both a valid transfer callback, and a valid data context in order to execute a trace
        if ((pArgs == nullptr) || !IsValidDDByteWriter(&pArgs->writer) || (pClient == nullptr))
        {
            result = Result::InvalidParameter;
        }
        else
        {
            if (!pClient->executing_trace)
            {
                pClient->executing_trace = true;
            }
            else
            {
                result = Result::NotReady;
            }
        }
    }

    if (result == Result::Success)
    {
        TraceTransferContext transferContext = {};

        transferContext.pConfig        = pArgs;
        transferContext.abortRequested = false;

        RGPProtocol::BeginTraceInfo traceInfo = {};
        traceInfo.callbackInfo.chunkCallback  = &RGPChunkFunc;
        traceInfo.callbackInfo.chunkCallback  = &RGPChunkFunc;
        traceInfo.callbackInfo.pUserdata      = &transferContext;

        uint32 numChunks        = 0;
        uint64 traceSizeInBytes = 0;

        // Send the latest copy of the trace parameters.
        result = UpdateTraceParameters(*pClient, &pArgs->config);

        bool                                   changedClockMode  = false;
        DriverControlProtocol::DeviceClockMode previousClockMode = DriverControlProtocol::DeviceClockMode::Default;
        if (result == Result::Success)
        {
            pClient->driverControlClient.QueryDeviceClockMode(0, &previousClockMode);

            // Change the gpu clock mode to peak to ensure that we don't have clock fluctuations while tracing
            // Don't propagate the result as we still want to allow tracing without peak clock mode set
            // gpuIndex = UINT32_MAX forces a clock update for every device in the system
            uint32 gpuIndex =
                (pClient->driverControlClient.GetSessionVersion() >= DRIVERCONTROL_SET_CLOCKS_ALL_ADAPTERS_VERSION) ?
                    UINT32_MAX :
                    0;
            pClient->driverControlClient.SetDeviceClockMode(gpuIndex, DriverControlProtocol::DeviceClockMode::Peak);

            changedClockMode = true;
        }

        if (result == Result::Success)
        {
            result = pClient->rgpClient.BeginTrace(traceInfo);

            if (pArgs->pPostBeginTraceCallback != nullptr)
            {
                pArgs->pPostBeginTraceCallback(pArgs->pPostBeginTraceUserdata);
            }
        }

        if (result == Result::Success)
        {
            pClient->abort_trace = false;
            result = Result::NotReady;

            uint32_t timeRemaining = pArgs->timeoutInMs;
            static constexpr uint32_t kMaxWaitTimeMs = 200;

            while ((result == Result::NotReady) && (timeRemaining > 0) && (!pClient->abort_trace))
            {
                uint32_t attemptTimeout = timeRemaining >= kMaxWaitTimeMs ? kMaxWaitTimeMs : timeRemaining;
                result = pClient->rgpClient.EndTrace(&numChunks, &traceSizeInBytes, attemptTimeout);

                timeRemaining -= attemptTimeout;
            }
        }

        if (changedClockMode)
        {
            // Attempt to restore the gpu clock mode to the default state
            // Avoid propagating the result here so we don't prevent the user from getting their trace data if this fails since
            // it should still be perfectly valid in this situation.
            // gpuIndex = UINT32_MAX forces a clock update for every device in the system
            // gpuIndex = UINT32_MAX forces a clock update for every device in the system (if supported)
            uint32 gpuIndex =
                (pClient->driverControlClient.GetSessionVersion() >= DRIVERCONTROL_SET_CLOCKS_ALL_ADAPTERS_VERSION) ?
                    UINT32_MAX :
                    0;
            DD_UNHANDLED_RESULT(pClient->driverControlClient.SetDeviceClockMode(gpuIndex, previousClockMode));
        }

        if (result == Result::Success)
        {
            const auto traceSizeInBytesSizeT = static_cast<size_t>(traceSizeInBytes);
            const bool abortRequested =
                (pArgs->writer.pfnBegin(pArgs->writer.pUserdata, &traceSizeInBytesSizeT) != DD_RESULT_SUCCESS) ||
                pClient->abort_trace;

            // If the application returns false from their callback, we should abort the transfer.
            if (abortRequested)
            {
                result = pClient->rgpClient.AbortTrace();

                if (result == Result::Success)
                {
                    result = Result::Aborted;
                }
            }
        }

        if (result == Result::Success)
        {
            // Read chunks until we hit the end of the stream.
            do
            {
                result = pClient->rgpClient.ReadTraceDataChunk();

                if (result == Result::Success)
                {
                    // The application may request that we abort the trace inside their data callback which will be
                    // returned to us through the abortRequested field in TraceTransferContext.
                    if (transferContext.abortRequested || pClient->abort_trace)
                    {
                        result = pClient->rgpClient.AbortTrace();

                        if (result == Result::Success)
                        {
                            result = Result::Aborted;
                        }
                    }
                }

            } while (result == Result::Success);

            if (result == Result::EndOfStream)
            {
                result = Result::Success;
            }
        }

        // Call the end transfer callback
        pArgs->writer.pfnEnd(pArgs->writer.pUserdata, DevDriverToDDResult(result));
    }

    if (pClient != nullptr)
    {
        // We don't need to reaquire the lock here since executing_trace should prevent the client from being deleted.
        pClient->executing_trace = false;
    }

    return DevDriverToDDResult(result);
}

void GpuProfiling::AbortTrace(DDConnectionId umdConnectionId)
{
    LockGuard lock(m_clientsMutex);
    auto foundItr = m_clients.find(umdConnectionId);
    if (foundItr != m_clients.end())
    {
        foundItr->second->abort_trace = true;
    }
}

DD_RESULT GpuProfiling::QueryClientProtocolVersion(DDConnectionId umdConnectionId, uint16_t* pVersion)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    LockGuard lock(m_clientsMutex);
    auto foundItr = m_clients.find(umdConnectionId);
    if ((foundItr != m_clients.end()) && (pVersion != nullptr))
    {
        Client* pClient = foundItr->second;
        if (pClient->rgpClient.IsConnected())
        {
            *pVersion = pClient->rgpClient.GetSessionVersion();
            result    = DD_RESULT_SUCCESS;
        }
        else
        {
            result = DD_RESULT_NET_NOT_CONNECTED;
        }
    }

    return result;
}

DD_RESULT GpuProfiling::SetSpmCounters(DDConnectionId                    umdConnectionId,
                                       const DDGpuProfilingSpmCounterId* pCounters,
                                       uint32_t                          numCounters)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    LockGuard lock(m_clientsMutex);

    auto foundItr = m_clients.find(umdConnectionId);
    if (foundItr != m_clients.end())
    {
        Client* pClient = foundItr->second;
        if (!pClient->executing_trace)
        {
            if ((numCounters > 0) && (pCounters != nullptr))
            {
                pClient->m_spmCounters.resize(numCounters);
                for (uint32 counterIndex = 0; counterIndex < numCounters; ++counterIndex)
                {
                    pClient->m_spmCounters[counterIndex] = pCounters[counterIndex];
                }

                result = DD_RESULT_SUCCESS;
            }
            else
            {
                pClient->m_spmCounters.clear();
            }
        }
        else
        {
            result = DD_RESULT_DD_GENERIC_NOT_READY;
        }
    }

    return result;
}

Result GpuProfiling::UpdateTraceParameters(Client& client, const DDGpuProfilingConfig* pConfig)
{
    RGPProtocol::ClientTraceParametersInfo traceParameters = {};
    PopulateTraceConfig(pConfig, traceParameters);

    Result result = client.rgpClient.UpdateTraceParameters(traceParameters);
    if (result != Result::Success)
    {
        LOG_ERROR("Failed to update RGP trace parameters: %s", ResultToString(result));
    }

    if (result == Result::Success)
    {
        if (traceParameters.flags.enableSpm)
        {
            RGPProtocol::ClientSpmConfig                 spmConfig = {};
            std::vector<RGPProtocol::ClientSpmCounterId> spmCounters;

            result = PopulateSpmConfig(client, pConfig, &spmConfig, &spmCounters);

            if (result == Result::Success)
            {
                result = client.rgpClient.UpdateCounterConfig(spmConfig);
                if (result != Result::Success)
                {
                    LOG_WARN("Failed to update RGP SPM counter config: %s", ResultToString(result));
                }
            }
        }
    }

    return result;
}

void GpuProfiling::PopulateTraceConfig(const DDGpuProfilingConfig*             pConfig,
                                       RGPProtocol::ClientTraceParametersInfo& rgpConfig)
{
    rgpConfig.gpuMemoryLimitInMb   = pConfig->gpuMemoryLimitInMb;
    rgpConfig.numPreparationFrames = pConfig->numPreparationFrames;
    rgpConfig.captureMode          = ConvertTriggerMode(pConfig->captureMode);

    rgpConfig.flags.enableInstructionTokens  = pConfig->flags.enableInstructionTokens;
    rgpConfig.flags.allowComputePresents     = pConfig->flags.allowComputePresents;
    rgpConfig.flags.captureDriverCodeObjects = pConfig->flags.captureDriverCodeObjects;
    rgpConfig.flags.enableSpm                = pConfig->flags.enableSpm;

    rgpConfig.captureStartIndex = pConfig->captureStartIndex;
    rgpConfig.captureStopIndex  = pConfig->captureStopIndex;

    rgpConfig.beginTag = pConfig->captureStartTag;
    rgpConfig.endTag   = pConfig->captureStopTag;

    Platform::Strncpy(rgpConfig.beginMarker, pConfig->captureStartMarker, kDDGpuProfilingConfigMarkerStringLen);
    Platform::Strncpy(rgpConfig.endMarker, pConfig->captureStopMarker, kDDGpuProfilingConfigMarkerStringLen);

    rgpConfig.pipelineHash = pConfig->instructionTraceApiPsoHash;
    rgpConfig.seMask       = pConfig->shaderEngineInstructionTraceMask;
}

Result GpuProfiling::PopulateSpmConfig(Client&                                       client,
                                       const DDGpuProfilingConfig*                   pConfig,
                                       RGPProtocol::ClientSpmConfig*                 pSpmConfig,
                                       std::vector<RGPProtocol::ClientSpmCounterId>* pCounters)
{
    Result result = Result::InvalidParameter;

    const auto& spmCounters = client.m_spmCounters;
    if ((pConfig != nullptr) && (pCounters != nullptr))
    {
        pSpmConfig->sampleFrequency = pConfig->spmSampleFrequency;
        pSpmConfig->memoryLimitInMb = pConfig->spmMemoryLimit;

        pCounters->resize(spmCounters.size());
        for (size_t counterIndex = 0; counterIndex < spmCounters.size(); ++counterIndex)
        {
            RGPProtocol::ClientSpmCounterId&  rgpCounter       = (*pCounters)[counterIndex];
            const DDGpuProfilingSpmCounterId& profilingCounter = spmCounters[counterIndex];

            rgpCounter.blockId = profilingCounter.blockId;

            // Perform a conversion between the protocol level "all instances" identifier and the module
            // level identifier.
            rgpCounter.instanceId = (profilingCounter.instanceId == DD_GPU_PROFILING_SPM_ALL_INSTANCES) ?
                                        RGPProtocol::kSpmAllInstancesId :
                                        profilingCounter.instanceId;
            rgpCounter.eventId    = profilingCounter.eventId;
        }

        pSpmConfig->numCounters = static_cast<uint32>(pCounters->size());
        pSpmConfig->pCounters   = pCounters->empty() ? nullptr : pCounters->data();

        result = Result::Success;
    }

    return result;
}

} // namespace DevDriver
