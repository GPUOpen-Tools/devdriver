/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <ddCommon.h>
#include <dd_uber_trace.h>

namespace
{
// DDUberTraceApi wrapper functions.

DD_RESULT ConnectWrapper(DDUberTraceInstance* pInstance, DDConnectionId umdConnectionId)
{
    DevDriver::UberTrace* pUberTrace = reinterpret_cast<DevDriver::UberTrace*>(pInstance);
    return pUberTrace->Connect(umdConnectionId);
}

void DisconnectWrapper(DDUberTraceInstance* pInstance, DDConnectionId umdConnectionId)
{
    DevDriver::UberTrace* pUberTrace = reinterpret_cast<DevDriver::UberTrace*>(pInstance);
    pUberTrace->Disconnect(umdConnectionId);
}

DD_RESULT EnableTracingWrapper(DDUberTraceInstance* pInstance, DDConnectionId umdConnectionId)
{
    DevDriver::UberTrace* pUberTrace = reinterpret_cast<DevDriver::UberTrace*>(pInstance);
    return pUberTrace->EnableTracing(umdConnectionId);
}

DD_RESULT ConfigureTraceParamsWrapper(DDUberTraceInstance* pInstance,
                                      DDConnectionId       umdConnectionId,
                                      const char*          pData,
                                      size_t               dataSize)
{
    DevDriver::UberTrace* pUberTrace = reinterpret_cast<DevDriver::UberTrace*>(pInstance);
    return pUberTrace->ConfigureTraceParams(umdConnectionId, pData, dataSize);
}

DD_RESULT RequestTraceWrapper(DDUberTraceInstance* pInstance, DDConnectionId umdConnectionId)
{
    DevDriver::UberTrace* pUberTrace = reinterpret_cast<DevDriver::UberTrace*>(pInstance);
    return pUberTrace->RequestTrace(umdConnectionId);
}

DD_RESULT CancelTraceWrapper(DDUberTraceInstance* pInstance, DDConnectionId umdConnectionId)
{
    DevDriver::UberTrace* pUberTrace = reinterpret_cast<DevDriver::UberTrace*>(pInstance);
    return pUberTrace->CancelTrace(umdConnectionId);
}

DD_RESULT CollectTraceWrapper(DDUberTraceInstance* pInstance,
                              DDConnectionId       umdConnectionId,
                              uint32_t             timeoutInMs,
                              const DDByteWriter*  pWriter)
{
    DevDriver::UberTrace* pUberTrace = reinterpret_cast<DevDriver::UberTrace*>(pInstance);
    return pUberTrace->CollectTrace(umdConnectionId, timeoutInMs, pWriter);
}

} // anonymous namespace

namespace DevDriver
{

UberTrace::UberTrace()
    : m_net(DD_API_INVALID_HANDLE),
      m_pLogger(nullptr)
{
}

UberTrace::~UberTrace()
{
    ClearAfterRouterDisconnect();
    m_pLogger = nullptr;
}

DD_RESULT UberTrace::Initialize(DDApiRegistry* pApiRegistry)
{
    DD_RESULT result = pApiRegistry->Get(
        pApiRegistry->pInstance,
        DD_LOGGER_API_NAME,
        DDVersion{
            DD_LOGGER_API_VERSION_MAJOR,
            DD_LOGGER_API_VERSION_MINOR,
            DD_LOGGER_API_VERSION_PATCH },
        reinterpret_cast<void**>(&m_pLogger));

    DD_ASSERT(result == DD_RESULT_SUCCESS);
    if (result == DD_RESULT_SUCCESS)
    {
        DDUberTraceApi uberTraceApi{
            reinterpret_cast<DDUberTraceInstance*>(this),
            ConnectWrapper,
            DisconnectWrapper,
            EnableTracingWrapper,
            ConfigureTraceParamsWrapper,
            RequestTraceWrapper,
            CancelTraceWrapper,
            CollectTraceWrapper};

        result = pApiRegistry->Add(
            pApiRegistry->pInstance,
            DD_UBER_TRACE_API_NAME,
            DDVersion{
                DD_UBER_TRACE_API_VERSION_MAJOR,
                DD_UBER_TRACE_API_VERSION_MINOR,
                DD_UBER_TRACE_API_VERSION_PATCH},
            &uberTraceApi,
            sizeof(uberTraceApi));

        if (result != DD_RESULT_SUCCESS)
        {
            m_pLogger->Log(
                m_pLogger->pInstance,
                DD_LOG_LVL_ERROR,
                "[UberTrace] Failed to register DDUberTraceApi. DD_RESULT: %u.",
                result);
        }
    }

    return result;
}

void UberTrace::ClearAfterRouterDisconnect()
{
    m_net = DD_API_INVALID_HANDLE;
    m_traceEnabledClients.clear();
}

void UberTrace::SetRpcClientInfo(DDNetConnection ddNet)
{
    m_net = ddNet;
}

DD_RESULT UberTrace::Connect(DDConnectionId umdConnectionId)
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_NOT_READY;

    LockGuard lock(m_clientsMutex);

    if ((m_net != DD_API_INVALID_HANDLE) && (m_traceEnabledClients.count(umdConnectionId) == 0))
    {
        DDRpcClientCreateInfo info = {};
        info.hConnection           = m_net;
        info.clientId              = static_cast<DDClientId>(umdConnectionId);

        m_traceEnabledClients.insert({ umdConnectionId, {} });

        ::UberTrace::UberTraceClient& uberTraceClient = m_traceEnabledClients[umdConnectionId].m_uberTraceClient;
        DD_RESULT uberTraceResult = uberTraceClient.Connect(info);

        if (uberTraceResult != DD_RESULT_SUCCESS)
        {
            m_pLogger->Log(m_pLogger->pInstance,
                           DD_LOG_LVL_ERROR,
                           "[DDUberTrace] Failed to connect UberTrace client: %s (%u).",
                           ddApiResultToString(uberTraceResult),
                           uberTraceResult);

            m_traceEnabledClients.erase(umdConnectionId);
            result = uberTraceResult;
        }
        else
        {
            m_pLogger->Log(m_pLogger->pInstance,
                           DD_LOG_LVL_INFO,
                           "[DDUberTrace] Successfully started tracing on a new client: %u.",
                           umdConnectionId);

            result = DD_RESULT_SUCCESS;
        }
    }

    return result;
}

void UberTrace::Disconnect(DDConnectionId umdConnectionId)
{
    LockGuard lock(m_clientsMutex);

    if (m_traceEnabledClients.count(umdConnectionId) > 0)
    {
        m_traceEnabledClients.erase(umdConnectionId);
    }
}

DD_RESULT UberTrace::ConfigureTraceParams(DDConnectionId umdConnectionId, const char* pData, size_t dataSize)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    LockGuard lock(m_clientsMutex);

    if (ValidateOptionalBuffer(pData, dataSize))
    {
        if (m_traceEnabledClients.count(umdConnectionId) != 0)
        {
            result = m_traceEnabledClients[umdConnectionId].m_uberTraceClient.ConfigureTraceParams(pData, dataSize);
        }
        else
        {
            result = DD_RESULT_DD_GENERIC_NOT_READY;
        }
    }

    return result;
}

DD_RESULT UberTrace::EnableTracing(DDConnectionId umdConnectionId)
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_NOT_READY;

    LockGuard lock(m_clientsMutex);

    if (m_traceEnabledClients.count(umdConnectionId) > 0)
    {
        result = m_traceEnabledClients[umdConnectionId]
                    .m_uberTraceClient
                    .EnableTracing();
    }

    return result;
}

DD_RESULT UberTrace::RequestTrace(DDConnectionId umdConnectionId)
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_NOT_READY;

    LockGuard lock(m_clientsMutex);

    if (m_traceEnabledClients.count(umdConnectionId) != 0)
    {
        result = m_traceEnabledClients[umdConnectionId].m_uberTraceClient.RequestTrace();
    }

    return result;
}

DD_RESULT UberTrace::CancelTrace(DDConnectionId umdConnectionId)
{
    LockGuard lock(m_clientsMutex);

    DD_RESULT result = DD_RESULT_DD_GENERIC_NOT_READY;
    if (m_traceEnabledClients.count(umdConnectionId) != 0)
    {
        result = m_traceEnabledClients[umdConnectionId].m_uberTraceClient.CancelTrace();
    }

    return result;
}

DD_RESULT UberTrace::CollectTrace(DDConnectionId umdConnectionId, uint32_t timeoutInMs, const DDByteWriter* pWriter)
{
    LockGuard lock(m_clientsMutex);

    // TODO: We need to actually pass this into the RPC layer somehow
    DD_API_UNUSED(timeoutInMs);

    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;
    if (pWriter != nullptr)
    {
        if (m_traceEnabledClients.count(umdConnectionId) != 0)
        {
            result = m_traceEnabledClients[umdConnectionId].m_uberTraceClient.CollectTrace(*pWriter);
        }
        else
        {
            result = DD_RESULT_DD_GENERIC_NOT_READY;
        }
    }

    return result;
}

} // namespace DevDriver
