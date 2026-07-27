/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_amd_log_utils.h>
#include <ddCommon.h>

#define LOG_ERROR(fmt, ...) s_pLogger->Log(s_pLogger->pInstance, DD_LOG_LVL_ERROR, "[DDAmdLogUtils] " fmt, ## __VA_ARGS__)

namespace
{
DDLoggerApi* s_pLogger = nullptr;

// DDAmdLogUtilsApi wrapper functions.

DD_RESULT IsServiceAvailableWrapper(DDAmdLogUtilsInstance* pInstance)
{
    DevDriver::AmdLogUtilsManager* pAmdLogUtils = reinterpret_cast<DevDriver::AmdLogUtilsManager*>(pInstance);

    return pAmdLogUtils->IsServiceAvailable();
}

DD_RESULT GetServiceInfoWrapper(DDAmdLogUtilsInstance* pInstance,
                                DDApiVersion*          pVersion)
{
    DevDriver::AmdLogUtilsManager* pAmdLogUtils = reinterpret_cast<DevDriver::AmdLogUtilsManager*>(pInstance);

    return pAmdLogUtils->GetServiceInfo(pVersion);
}

DD_RESULT SendRgdOcaConfigWrapper(DDAmdLogUtilsInstance* pInstance,
                                  const void*            pConfigData,
                                  size_t                 configDataSize)
{
    DevDriver::AmdLogUtilsManager* pAmdLogUtils = reinterpret_cast<DevDriver::AmdLogUtilsManager*>(pInstance);

    return pAmdLogUtils->SendRgdOcaConfig(pConfigData, configDataSize);
}

DD_RESULT SetOcaHighOverheadConfigWrapper(DDAmdLogUtilsInstance* pInstance,
                                          const void*            pConfigData,
                                          size_t                 configDataSize)
{
    DevDriver::AmdLogUtilsManager* pAmdLogUtils = reinterpret_cast<DevDriver::AmdLogUtilsManager*>(pInstance);

    return pAmdLogUtils->SetOcaHighOverheadConfig(pConfigData, configDataSize);
}

} // anonymous namespace

namespace DevDriver
{

AmdLogUtilsManager::AmdLogUtilsManager()
    : m_net{DD_API_INVALID_HANDLE}
    , m_amdLogConnectionId{0}
{
}

AmdLogUtilsManager::~AmdLogUtilsManager()
{
    ClearAfterRouterDisconnect();
}

DD_RESULT AmdLogUtilsManager::Initialize(DDApiRegistry* pApiRegistry)
{
    DD_RESULT result = pApiRegistry->Get(
        pApiRegistry->pInstance,
        DD_LOGGER_API_NAME,
        DDVersion{ DD_LOGGER_API_VERSION_MAJOR, DD_LOGGER_API_VERSION_MINOR, DD_LOGGER_API_VERSION_PATCH },
        reinterpret_cast<void**>(&s_pLogger));

    if (result == DD_RESULT_SUCCESS)
    {
        DDAmdLogUtilsApi amdLogUtilsApi{ reinterpret_cast<DDAmdLogUtilsInstance*>(this),
                                         IsServiceAvailableWrapper,
                                         GetServiceInfoWrapper,
                                         SendRgdOcaConfigWrapper,
                                         SetOcaHighOverheadConfigWrapper };

        result = pApiRegistry->Add(pApiRegistry->pInstance,
                                   DD_AMD_LOG_UTILS_API_NAME,
                                   DDVersion{ DD_AMD_LOG_UTILS_API_VERSION_MAJOR,
                                              DD_AMD_LOG_UTILS_API_VERSION_MINOR,
                                              DD_AMD_LOG_UTILS_API_VERSION_PATCH },
                                   &amdLogUtilsApi,
                                   sizeof(amdLogUtilsApi));

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("Failed to register DDAmdLogUtilsApi. DD_RESULT: %u.", result);
        }
    }

    return result;
}

void AmdLogUtilsManager::ClearAfterRouterDisconnect()
{
    m_net                  = DD_API_INVALID_HANDLE;
    m_amdLogConnectionId   = 0;
}

void AmdLogUtilsManager::SetRpcClientInfo(DDNetConnection ddNet, DDConnectionId amdLogConnectionId)
{
    m_net                  = ddNet;
    m_amdLogConnectionId   = amdLogConnectionId;
}

DD_RESULT AmdLogUtilsManager::IsServiceAvailable()
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_UNAVAILABLE;

    if (m_net != DD_API_INVALID_HANDLE)
    {
        AmdLogUtils::AmdLogUtilsClient rpcClient;

        DDRpcClientCreateInfo clientInfo = {};
        clientInfo.hConnection           = m_net;
        clientInfo.clientId              = static_cast<DDClientId>(m_amdLogConnectionId);

        result = rpcClient.Connect(clientInfo);
        if (result == DD_RESULT_SUCCESS)
        {
            result = rpcClient.IsServiceAvailable();
            if (result != DD_RESULT_SUCCESS)
            {
                LOG_ERROR("RPC call 'IsServiceAvailable' failed. DD_RESULT: %u.", result);
            }
        }
        else
        {
            LOG_ERROR("Failed to connect RPC client. DD_RESULT: %u.", result);
        }
    }

    return result;
}

DD_RESULT AmdLogUtilsManager::GetServiceInfo(DDApiVersion* pVersion)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if (pVersion != nullptr)
    {
        result = DD_RESULT_DD_GENERIC_UNAVAILABLE;

        if (m_net != DD_API_INVALID_HANDLE)
        {
            AmdLogUtils::AmdLogUtilsClient rpcClient;

            DDRpcClientCreateInfo clientInfo = {};
            clientInfo.hConnection           = m_net;
            clientInfo.clientId              = static_cast<DDClientId>(m_amdLogConnectionId);

            result = rpcClient.Connect(clientInfo);
            if (result == DD_RESULT_SUCCESS)
            {
                result = rpcClient.GetServiceInfo(pVersion);
                if (result != DD_RESULT_SUCCESS)
                {
                    LOG_ERROR("RPC call 'GetServiceInfo' failed. DD_RESULT: %u.", result);
                }
            }
            else
            {
                LOG_ERROR("Failed to connect RPC client. DD_RESULT: %u.", result);
            }
        }
    }

    return result;
}

DD_RESULT AmdLogUtilsManager::SendRgdOcaConfig(const void* pConfigData, size_t configDataSize)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if ((pConfigData != nullptr) && (configDataSize > 0))
    {
        result = DD_RESULT_DD_GENERIC_UNAVAILABLE;

        if (m_net != DD_API_INVALID_HANDLE)
        {
            AmdLogUtils::AmdLogUtilsClient rpcClient;

            DDRpcClientCreateInfo clientInfo = {};
            clientInfo.hConnection           = m_net;
            clientInfo.clientId              = static_cast<DDClientId>(m_amdLogConnectionId);

            result = rpcClient.Connect(clientInfo);
            if (result == DD_RESULT_SUCCESS)
            {
                result = rpcClient.SendRgdOcaConfig(pConfigData, configDataSize);
                if (result != DD_RESULT_SUCCESS)
                {
                    LOG_ERROR("RPC call 'SendRgdOcaConfig' failed. DD_RESULT: %u.", result);
                }
            }
            else
            {
                LOG_ERROR("Failed to connect RPC client. DD_RESULT: %u.", result);
            }
        }
    }

    return result;
}

DD_RESULT AmdLogUtilsManager::SetOcaHighOverheadConfig(const void* pConfigData, size_t configDataSize)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if ((pConfigData != nullptr) && (configDataSize > 0))
    {
        result = DD_RESULT_DD_GENERIC_UNAVAILABLE;

        if (m_net != DD_API_INVALID_HANDLE)
        {
            AmdLogUtils::AmdLogUtilsClient rpcClient;

            DDRpcClientCreateInfo clientInfo = {};
            clientInfo.hConnection           = m_net;
            clientInfo.clientId              = static_cast<DDClientId>(m_amdLogConnectionId);

            result = rpcClient.Connect(clientInfo);
            if (result == DD_RESULT_SUCCESS)
            {
                result = rpcClient.SetOcaHighOverheadConfig(pConfigData, configDataSize);
                if (result != DD_RESULT_SUCCESS)
                {
                    LOG_ERROR("RPC call 'SetOcaHighOverheadConfig' failed. DD_RESULT: %u.", result);
                }
            }
            else
            {
                LOG_ERROR("Failed to connect RPC client. DD_RESULT: %u.", result);
            }
        }
    }

    return result;
}

} // namespace DevDriver
