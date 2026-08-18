/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_driver_utils_api.h>
#include <dd_driver_utils.h>
#include <dd_logger_api.h>
#include <dd_assert.h>
#include <dd_connection_api.h>
#include <dd_tool.h>
#include <dd_result.h>

#include <ddRpcClient.h>
#include <g_DriverUtilsClient.h>
#include <ddPlatform.h>

#include <algorithm>

#define LOG_ERROR(fmt, ...) s_pLogger->Log(s_pLogger->pInstance, DD_LOG_LVL_ERROR, "[DDDriverUtils] " fmt, ## __VA_ARGS__)
#define LOG_WARN(fmt, ...) s_pLogger->Log(s_pLogger->pInstance, DD_LOG_LVL_WARN, "[DDDriverUtils] " fmt, ## __VA_ARGS__)
#define LOG_INFO(fmt, ...) s_pLogger->Log(s_pLogger->pInstance, DD_LOG_LVL_INFO, "[DDDriverUtils] " fmt, ## __VA_ARGS__)

namespace
{

DDLoggerApi* s_pLogger;

// Structure to control states for driver features.
struct DriverUtilsFeatures
{
    bool tracing;                 // Enables tracing
    bool crashAnalysis;           // Enables Crash Analysis Mode
    bool shaderInstrumentationRt; // Enables Raytracing Shader Data Tokens
    bool staticVmid;              // Enables debug VMID
};

struct DriverDbgLogOriginationOp
{
    uint32_t origination;
    bool     enable;
};

// DDDriverUtilsApi wrapper functions

DD_RESULT SetFeatureWrapper(
    DDDriverUtilsInstance*       pInstance,
    DD_DRIVER_UTILS_FEATURE      feature,
    DD_DRIVER_UTILS_FEATURE_FLAG flag,
    const char*                  pSetterName,
    uint32_t                     setterNameSize)
{
    DevDriver::DriverUtils* pDriverUtils = reinterpret_cast<DevDriver::DriverUtils*>(pInstance);
    return pDriverUtils->SetFeature(feature, flag, pSetterName, setterNameSize);
}

void OnDriverConnected(DDConnectionCallbacksImpl* pImpl, const DDConnectionInfo* pConnInfo)
{
    DevDriver::DriverUtils* pDriverUtils = reinterpret_cast<DevDriver::DriverUtils*>(pImpl);
    pDriverUtils->SendDriverFeatureFlags(pConnInfo->umdConnectionId);
}

DD_RESULT QueryPalDriverInfoWapper(DDDriverUtilsInstance *pInstance, DDConnectionId umdConnection, const DDByteWriter& writer)
{
    DevDriver::DriverUtils* pDriverUtils = reinterpret_cast<DevDriver::DriverUtils*>(pInstance);
    return pDriverUtils->QueryPalDriverInfo(umdConnection, writer);
}

DD_RESULT SetDriverOverlayStringWrapper(DDDriverUtilsInstance* pInstance,
                                        DDConnectionId         umdConnectionId,
                                        const char*            pOverlayString,
                                        uint32_t               strIdx)
{
    DevDriver::DriverUtils* pDriverUtils = reinterpret_cast<DevDriver::DriverUtils*>(pInstance);
    return pDriverUtils->SetDriverOverlayString(umdConnectionId, pOverlayString, strIdx);
}

DD_RESULT SetDbgLogSeverityLevelWrapper(DDDriverUtilsInstance*  pInstance,
                                        DDConnectionId          umdConnectionId,
                                        uint32_t                severityLevel)
{
    DevDriver::DriverUtils* pDriverUtils = reinterpret_cast<DevDriver::DriverUtils*>(pInstance);
    return pDriverUtils->SetDbgLogSeverityLevel(umdConnectionId, severityLevel);
}

DD_RESULT SetDbgLogOriginationMaskWrapper(DDDriverUtilsInstance*   pInstance,
                                          DDConnectionId           umdConnectionId,
                                          uint32_t                 mask)
{
    DevDriver::DriverUtils* pDriverUtils = reinterpret_cast<DevDriver::DriverUtils*>(pInstance);
    return pDriverUtils->SetDbgLogOriginationMask(umdConnectionId, mask);
}

DD_RESULT ModifyDbgLogOriginationMaskWrapper(DDDriverUtilsInstance* pInstance,
                                             DDConnectionId         umdConnectionId,
                                             uint32_t               origination,
                                             bool                   enable)
{
    DevDriver::DriverUtils* pDriverUtils = reinterpret_cast<DevDriver::DriverUtils*>(pInstance);
    return pDriverUtils->ModifyDbgLogOriginationMask(umdConnectionId, origination, enable);
}

} // anonymous namespace

namespace DevDriver
{
DriverUtils::DriverUtils(Tool* pTool)
    : m_pTool{pTool}
    , m_net{DD_API_INVALID_HANDLE}
{
    m_features[DD_DRIVER_UTILS_FEATURE_TRACING].pName                = "Tracing";
    m_features[DD_DRIVER_UTILS_FEATURE_CRASH_ANALYSIS].pName         = "Crash Analysis";
    m_features[DD_DRIVER_UTILS_FEATURE_SHADER_INSTRUMENTATION].pName = "Shader Instrumentation";
    m_features[DD_DRIVER_UTILS_FEATURE_STATIC_VMID].pName            = "Static VMID";

    for (int i = 0; i < DD_DRIVER_UTILS_FEATURE_COUNT; ++i)
    {
        m_features[i].setterNames.reserve(8);
        m_features[i].flag = DD_DRIVER_UTILS_FEATURE_FLAG_IGNORE;
    }
}

DD_RESULT DriverUtils::Initialize(DDApiRegistry* pApiRegistry)
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
        DDDriverUtilsApi driverUtilsApi {
            reinterpret_cast<DDDriverUtilsInstance*>(this),
            SetFeatureWrapper,
            QueryPalDriverInfoWapper,
            SetDriverOverlayStringWrapper,
            SetDbgLogSeverityLevelWrapper,
            SetDbgLogOriginationMaskWrapper,
            ModifyDbgLogOriginationMaskWrapper
        };

        result = pApiRegistry->Add(
            pApiRegistry->pInstance,
            DD_DRIVER_UTILS_API_NAME,
            DDVersion {
                DD_DRIVER_UTILS_API_VERSION_MAJOR,
                DD_DRIVER_UTILS_API_VERSION_MINOR,
                DD_DRIVER_UTILS_API_VERSION_PATCH},
            &driverUtilsApi,
            sizeof(driverUtilsApi));

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("Failed to register DDDriverUtilsApi. DD_RESULT: %s", StringResult(result));
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        DDConnectionApi* pConnApi = nullptr;
        result = pApiRegistry->Get(
            pApiRegistry->pInstance,
            DD_CONNECTION_API_NAME,
            DDVersion {
                DD_CONNECTION_API_VERSION_MAJOR,
                DD_CONNECTION_API_VERSION_MINOR,
                DD_CONNECTION_API_VERSION_PATCH},
            reinterpret_cast<void**>(&pConnApi));

        if (result == DD_RESULT_SUCCESS)
        {
            // Register OnDriverConnected callback.

            DDConnectionCallbacks connCallbacks {};
            connCallbacks.pImpl             = reinterpret_cast<DDConnectionCallbacksImpl*>(this);
            connCallbacks.OnDriverConnected = OnDriverConnected;
            result = pConnApi->AddConnectionCallbacks(pConnApi->pInstance, &connCallbacks);
            if (result != DD_RESULT_SUCCESS)
            {
                LOG_ERROR("Failed to add DDConnectionCallbacks. DD_RESULT: %s", StringResult(result));
            }
        }
        else
        {
            LOG_ERROR("Failed to query DDConnectionApi. DD_RESULT: %s", StringResult(result));
        }
    }

    return result;
}

void DriverUtils::SetRpcClientInfo(DDNetConnection ddNet)
{
    m_net = ddNet;
}

DD_RESULT DriverUtils::SetFeature(
    DD_DRIVER_UTILS_FEATURE      featureIndex,
    DD_DRIVER_UTILS_FEATURE_FLAG flagToSet,
    const char*                  pSetterName,
    uint32_t                     setterNameSize)
{

    DD_RESULT result = DD_RESULT_SUCCESS;

    if (m_pTool->GetConnectionCount() == 0)
    {
        Feature& feature = m_features[featureIndex];
        LockGuard lock(feature.mutex);

        std::string_view setterName(pSetterName, setterNameSize);

        const char* modifier = (flagToSet == DD_DRIVER_UTILS_FEATURE_FLAG_ENABLE) ? "enabled" : "disabled";
        // Handle first time enable/disable calls
        if (feature.flag == DD_DRIVER_UTILS_FEATURE_FLAG_IGNORE)
        {
            if (flagToSet != DD_DRIVER_UTILS_FEATURE_FLAG_IGNORE)
            {
                if (std::find(feature.setterNames.begin(), feature.setterNames.end(), setterName) ==
                    feature.setterNames.end())
                {
                    feature.flag = flagToSet;
                    feature.setterNames.emplace_back(pSetterName, setterNameSize);
                    LOG_INFO("'%.*s' %s the feature '%s'", setterNameSize, pSetterName, modifier, feature.pName);
                }
                else
                {
                    LOG_WARN("'%.*s' tried to %s the feature '%s' multiple times.",
                             setterNameSize,
                             pSetterName,
                             modifier,
                             feature.pName);
                }
            }
        }
        // Handle ignore calls
        else if (flagToSet == DD_DRIVER_UTILS_FEATURE_FLAG_IGNORE)
        {
            // Erase all entries for this setter.
            const auto oldSize = feature.setterNames.size();
            feature.setterNames.erase(std::remove(feature.setterNames.begin(), feature.setterNames.end(), setterName),
                                      feature.setterNames.end());
            // Comparing old and new size to see if this setter had enabled/disabled previously.
            if (oldSize == feature.setterNames.size())
            {
                LOG_WARN("'%.*s' ignored a feature that itself didn't enable or disable previously.",
                         setterNameSize,
                         pSetterName);
            }
            else
            {
                if (feature.setterNames.empty())
                {
                    feature.flag = DD_DRIVER_UTILS_FEATURE_FLAG_IGNORE;
                }
                LOG_INFO("'%.*s' ignored the feature '%s'", setterNameSize, pSetterName, feature.pName);
            }
        }
        // Handle redundant enable/disable calls - allow tracking multiple setters for the same state
        else if (flagToSet == feature.flag)
        {
            if (std::find(feature.setterNames.begin(), feature.setterNames.end(), setterName) ==
                feature.setterNames.end())
            {
                feature.setterNames.emplace_back(pSetterName, setterNameSize);
                LOG_INFO("'%.*s' %s the feature '%s'",
                         setterNameSize,
                         pSetterName,
                         modifier,
                         feature.pName);
            }
            else
            {
                LOG_WARN("'%.*s' tried to %s the feature '%s' multiple times.",
                         setterNameSize,
                         pSetterName,
                         modifier,
                         feature.pName);
            }
        }
        // Handle conflicting enable/disable calls
        else
        {
            LOG_ERROR("'%.*s' failed to %s the feature '%s', "
                      "because it's already %s by others.",
                      setterNameSize,
                      pSetterName,
                      modifier,
                      feature.pName,
                      (feature.flag == DD_DRIVER_UTILS_FEATURE_FLAG_ENABLE) ? "enabled" : "disabled");
            result = DD_RESULT_DD_GENERIC_NOT_READY;
        }
    }
    else
    {
        result = DD_RESULT_DD_GENERIC_CONNECTION_EXITS;
    }

    return result;
}

DD_RESULT DriverUtils::QueryPalDriverInfo(DDConnectionId umdConnection, const DDByteWriter& writer)
{
    DDRpcClientCreateInfo createInfo = {};
    createInfo.hConnection           = m_net;
    createInfo.clientId              = umdConnection;

    ::DriverUtils::DriverUtilsClient driverUtilsRpc = {};
    DD_RESULT result = driverUtilsRpc.Connect(createInfo);
    if (result == DD_RESULT_SUCCESS)
    {
        result = driverUtilsRpc.QueryPalDriverInfo(writer);
        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR(
                "RPC call to query PAL driver state info failed at UMD connection %u. DD_RESULT: %s.",
                umdConnection,
                StringResult(result));
        }
    }
    return result;
}

void DriverUtils::SendDriverFeatureFlags(DDConnectionId umdConnectionId)
{
    bool shouldSendFlags = false;
    for (int i = 0; i < DD_DRIVER_UTILS_FEATURE_COUNT; ++i)
    {
        shouldSendFlags |= (m_features[i].flag == DD_DRIVER_UTILS_FEATURE_FLAG_ENABLE);
    }

    if (shouldSendFlags)
    {
        DDRpcClientCreateInfo createInfo = {};
        createInfo.hConnection = m_net;
        createInfo.clientId = umdConnectionId;

        ::DriverUtils::DriverUtilsClient driverUtilsRpc;
        DD_RESULT result = driverUtilsRpc.Connect(createInfo);
        if (result == DD_RESULT_SUCCESS)
        {
            DriverUtilsFeatures driverFeatures{};

            driverFeatures.tracing                 = (m_features[DD_DRIVER_UTILS_FEATURE_TRACING].flag == DD_DRIVER_UTILS_FEATURE_FLAG_ENABLE);
            driverFeatures.crashAnalysis           = (m_features[DD_DRIVER_UTILS_FEATURE_CRASH_ANALYSIS].flag == DD_DRIVER_UTILS_FEATURE_FLAG_ENABLE);
            driverFeatures.shaderInstrumentationRt = (m_features[DD_DRIVER_UTILS_FEATURE_SHADER_INSTRUMENTATION].flag == DD_DRIVER_UTILS_FEATURE_FLAG_ENABLE);
            driverFeatures.staticVmid              = (m_features[DD_DRIVER_UTILS_FEATURE_STATIC_VMID].flag == DD_DRIVER_UTILS_FEATURE_FLAG_ENABLE);

            result = driverUtilsRpc.EnableDriverFeatures(&driverFeatures, sizeof(driverFeatures));
            if (result != DD_RESULT_SUCCESS)
            {
                LOG_ERROR(
                    "RPC call to send driver feature flags failed at UMD connection %u. DD_RESULT: %s.",
                    umdConnectionId,
                    StringResult(result));
            }
        }
        else
        {
            LOG_ERROR(
                "Failed to create RPC client for umd connection %u. DD_RESULT: %s.",
                umdConnectionId,
                StringResult(result));
        }
    }
    else
    {
        LOG_INFO("No feature is enabled, skipping sending feature flags to driver.");
    }
}

DD_RESULT DriverUtils::SetDriverOverlayString(DDConnectionId umdConnectionId,
                                              const char*    pOverlayString,
                                              uint32_t       strIdx)
{
    DDRpcClientCreateInfo createInfo = {};
    createInfo.hConnection           = m_net;
    createInfo.clientId              = umdConnectionId;

    ::DriverUtils::DriverUtilsClient driverUtilsRpc = {};
    DD_RESULT                        result         = driverUtilsRpc.Connect(createInfo);
    if (result == DD_RESULT_SUCCESS)
    {
        if ((pOverlayString != nullptr) &&
            (DevDriver::Platform::Strlen_s(pOverlayString, kMaxOverlayStringLength) < kMaxOverlayStringLength) &&
            (strIdx < kNumOverlayStrings))
        {
            DDOverlayInfo info = {};
            info.strIdx        = strIdx;
            Platform::Strncpy(info.str, pOverlayString, kMaxOverlayStringLength);
            result = driverUtilsRpc.SetOverlayString(&info, sizeof(DDOverlayInfo));
            if (result != DD_RESULT_SUCCESS)
            {
                LOG_ERROR("RPC call to set overlay string failed at UMD connection %u. DD_RESULT: %s.",
                          umdConnectionId,
                          StringResult(result));
            }
        }
        else
        {
            result = DD_RESULT_COMMON_INVALID_PARAMETER;
        }
    }
    return result;
}

DD_RESULT DriverUtils::SetDbgLogSeverityLevel(DDConnectionId umdConnectionId,
                                              uint32_t       severity)
{
    DDRpcClientCreateInfo createInfo = {};
    createInfo.hConnection           = m_net;
    createInfo.clientId              = umdConnectionId;

    ::DriverUtils::DriverUtilsClient driverUtilsRpc = {};
    DD_RESULT                        result         = driverUtilsRpc.Connect(createInfo);
    if (result == DD_RESULT_SUCCESS)
    {
        result = driverUtilsRpc.SetDbgLogSeverityLevel(&severity, sizeof(severity));
        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("RPC call to set DbgLog severity level failed at UMD connection %u. DD_RESULT: %s.",
                      umdConnectionId,
                      StringResult(result));
        }
    }
    return result;
}

DD_RESULT DriverUtils::SetDbgLogOriginationMask(DDConnectionId umdConnectionId,
                                                uint32_t       mask)
{
    DDRpcClientCreateInfo createInfo = {};
    createInfo.hConnection           = m_net;
    createInfo.clientId              = umdConnectionId;

    ::DriverUtils::DriverUtilsClient driverUtilsRpc = {};
    DD_RESULT                        result         = driverUtilsRpc.Connect(createInfo);

    if (result == DD_RESULT_SUCCESS)
    {
        result = driverUtilsRpc.SetDbgLogOriginationMask(&mask, sizeof(mask));

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("RPC call to set DbgLog origination mask failed at UMD connection %u. DD_RESULT: %s.",
                      umdConnectionId,
                      StringResult(result));
        }
    }
    return result;
}

DD_RESULT DriverUtils::ModifyDbgLogOriginationMask(DDConnectionId umdConnectionId,
                                                   uint32_t       origination,
                                                   bool           enable)
{
    DDRpcClientCreateInfo createInfo = {};
    createInfo.hConnection           = m_net;
    createInfo.clientId              = umdConnectionId;

    ::DriverUtils::DriverUtilsClient driverUtilsRpc = {};
    DD_RESULT                        result         = driverUtilsRpc.Connect(createInfo);

    if (result == DD_RESULT_SUCCESS)
    {
        DriverDbgLogOriginationOp op;
        op.origination = origination;
        op.enable      = enable;

        result = driverUtilsRpc.ModifyDbgLogOriginationMask(&op, sizeof(op));

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("RPC call to modify DbgLog origination mask failed at UMD connection %u. DD_RESULT: %s.",
                      umdConnectionId,
                      StringResult(result));
        }
    }
    return result;
}

} // namespace DevDriver
