/* Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <ddCommon.h>
#include <dd_enhanced_crash_info.h>
#include <g_AmdLogUtilsClient.h>

namespace
{
// DDEnhancedCrashInfoApi wrapper functions.

DD_RESULT QueryEnhancedCrashInfoConfigWrapper(DDEnhancedCrashInfoInstance* pInstance,
                                              DDEnhancedCrashInfoConfig*   pEnhancedCrashInfoConfig)
{
    DevDriver::EnhancedCrashInfo* pEnhancedCrashInfo = reinterpret_cast<DevDriver::EnhancedCrashInfo*>(pInstance);

    return pEnhancedCrashInfo->QueryEnhancedCrashInfoConfig(pEnhancedCrashInfoConfig);
}

DD_RESULT SetEnhancedCrashInfoConfigWrapper(DDEnhancedCrashInfoInstance* pInstance,
                                            const DDEnhancedCrashInfoConfig*   pEnhancedCrashInfoConfig)
{
    DevDriver::EnhancedCrashInfo* pEnhancedCrashInfo = reinterpret_cast<DevDriver::EnhancedCrashInfo*>(pInstance);

    return pEnhancedCrashInfo->SetEnhancedCrashInfoConfig(pEnhancedCrashInfoConfig);
}
} // anonymous namespace

namespace DevDriver
{

EnhancedCrashInfo::EnhancedCrashInfo()
    : m_net(DD_API_INVALID_HANDLE)
    , m_amdLogConnectionId(0)
    , m_pLogger(nullptr)
{

}

EnhancedCrashInfo::~EnhancedCrashInfo()
{
    ClearAfterRouterDisconnect();
}

DD_RESULT EnhancedCrashInfo::Initialize(DDApiRegistry* pApiRegistry)
{
    DD_RESULT result = pApiRegistry->Get(
        pApiRegistry->pInstance,
        DD_LOGGER_API_NAME,
        DDVersion{ DD_LOGGER_API_VERSION_MAJOR, DD_LOGGER_API_VERSION_MINOR, DD_LOGGER_API_VERSION_PATCH },
        reinterpret_cast<void**>(&m_pLogger));

    if (result == DD_RESULT_SUCCESS)
    {
        DDEnhancedCrashInfoApi enhancedCrashInfoApi{ reinterpret_cast<DDEnhancedCrashInfoInstance*>(this),
                               QueryEnhancedCrashInfoConfigWrapper,
                               SetEnhancedCrashInfoConfigWrapper };

        result = pApiRegistry->Add(pApiRegistry->pInstance,
                                   DD_ENHANCED_CRASH_INFO_API_NAME,
                                   DDVersion{ DD_ENHANCED_CRASH_INFO_API_VERSION_MAJOR,
                                              DD_ENHANCED_CRASH_INFO_API_VERSION_MINOR,
                                              DD_ENHANCED_CRASH_INFO_API_VERSION_PATCH },
                                   &enhancedCrashInfoApi,
                                   sizeof(enhancedCrashInfoApi));

        if (result != DD_RESULT_SUCCESS)
        {
            m_pLogger->Log(m_pLogger->pInstance,
                           DD_LOG_LVL_ERROR,
                           "[DDEnhancedCrashInfo] Failed to register DDEnhancedCrashInfoApi. DD_RESULT: %u.",
                           result);
        }
    }

    return result;
}

void EnhancedCrashInfo::ClearAfterRouterDisconnect()
{
    m_net                = DD_API_INVALID_HANDLE;
    m_amdLogConnectionId = DD_API_INVALID_CLIENT_ID;
}

void EnhancedCrashInfo::SetRpcClientInfo(DDNetConnection ddNet, uint16_t amdLogConnectionId)
{
    m_net                = ddNet;
    m_amdLogConnectionId = amdLogConnectionId;
}

DD_RESULT EnhancedCrashInfo::QueryEnhancedCrashInfoConfig(DDEnhancedCrashInfoConfig* pEnhancedCrashInfoConfig)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if ((pEnhancedCrashInfoConfig != nullptr) && (m_net != DD_API_INVALID_HANDLE))
    {
        AmdLogUtils::AmdLogUtilsClient amdLogUtilsClient;
        DynamicBufferByteWriter writer;

        DDRpcClientCreateInfo info = {};
        info.hConnection           = m_net;
        info.clientId              = static_cast<DDClientId>(m_amdLogConnectionId);

        result = amdLogUtilsClient.Connect(info);

        if (result == DD_RESULT_SUCCESS)
        {
            result = amdLogUtilsClient.QueryEnhancedCrashInfoConfig(*writer.Writer());
        }

        if (result == DD_RESULT_SUCCESS)
        {
            Platform::Memcpy_s(pEnhancedCrashInfoConfig,
                               sizeof(DDEnhancedCrashInfoConfig),
                               writer.Buffer(),
                               sizeof(DDEnhancedCrashInfoConfig));
        }
    }

    return result;
}

DD_RESULT EnhancedCrashInfo::SetEnhancedCrashInfoConfig(const DDEnhancedCrashInfoConfig* pEnhancedCrashInfoConfig)
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_UNAVAILABLE;

    if (m_net != DD_API_INVALID_HANDLE)
    {
        AmdLogUtils::AmdLogUtilsClient amdLogUtilsClient;
        DDRpcClientCreateInfo info = {};
        info.hConnection           = m_net;
        info.clientId              = static_cast<DDClientId>(m_amdLogConnectionId);

        result = amdLogUtilsClient.Connect(info);

        if (result == DD_RESULT_SUCCESS)
        {
            result = amdLogUtilsClient.SetEnhancedCrashInfoConfig(pEnhancedCrashInfoConfig,
                                                                  sizeof(*pEnhancedCrashInfoConfig));
        }
    }

    return result;
}

} // namespace DevDriver
