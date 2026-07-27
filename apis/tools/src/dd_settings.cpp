/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_settings.h>
#include <dd_dynamic_buffer.h>
#include <dd_settings_rpc_types.h>
#include <dd_assert.h>
#include <dd_logger_api.h>
#include <dd_integer.h>
#include <ddPlatform.h>

#include <g_SettingsRpcClient.h>
#include <g_DriverSiphonClient.h>
#include <g_AmdLogUtilsClient.h>

#include <cstdlib>
#include <cstring>
#include <limits>
#include <dd_settings_blob.h>
#if defined(DD_PLATFORM_WINDOWS_UM)
#include <set>
#include <string>
#include <dd_registry_utils.h>
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wpragmas"
    #pragma clang diagnostic ignored "-Wunknown-warning-option"
    #pragma clang diagnostic ignored "-Wunused-local-typedef"
#elif defined(__GNUC__)
    #pragma GCC   diagnostic push
    #pragma GCC   diagnostic ignored "-Wpragmas"
#endif

#include "rapidjson/document.h"

#if defined(__clang__)
    #pragma clang diagnostic pop
#elif defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif
#endif

using namespace DriverSiphon;

#define LOG_ERROR(fmt, ...) s_pLogger->Log(            \
                                s_pLogger->pInstance,  \
                                DD_LOG_LVL_ERROR,      \
                                "[DDSettings] " fmt,   \
                                ## __VA_ARGS__)

namespace
{

DDLoggerApi* s_pLogger;

DD_RESULT ByteWriterBegin(void* pUserData, const size_t* pTotalDataSize)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    if (pUserData)
    {
        auto pDynBuf = static_cast<DevDriver::DynamicBuffer*>(pUserData);
        if (pTotalDataSize)
        {
            result = pDynBuf->Reserve(*pTotalDataSize);
        }
    }
    else
    {
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    return result;
}

DD_RESULT ByteWriterWriteByte(void* pUserData, const void* pData, size_t dataSize)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    if (pUserData)
    {
        auto pDynBuf = static_cast<DevDriver::DynamicBuffer*>(pUserData);
        if (pData && (dataSize > 0))
        {
            pDynBuf->Copy(pData, dataSize);
            result = pDynBuf->Error();
        }
    }
    else
    {
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    return result;
}

void ByteWriterEnd(void*, DD_RESULT)
{
    // no-op
}

DD_RESULT QuerySettingsBlobsAll_Wrapper(
    DDSettingsInstance*     pInstance,
    DD_SETTINGS_DRIVER_TYPE driverType,
    const char*             pDriverPathOverride,
    size_t                  driverPathOverrideSize,
    bool                    reload,
    char**                  ppSettingsBlobs,
    size_t*                 pSettingsBlobsSize,
    DDAllocator             alloc)
{
    auto pSettings = reinterpret_cast<DevDriver::Settings*>(pInstance);

    DevDriver::DynamicBuffer recvBuf(alloc);

    DD_RESULT result = pSettings->QuerySettingsBlobsAll(
        driverType,
        pDriverPathOverride,
        driverPathOverrideSize,
        reload,
        &recvBuf);

    if (result == DD_RESULT_SUCCESS)
    {
        if (recvBuf.Size() != recvBuf.Capacity())
        {
            *ppSettingsBlobs = static_cast<char*>(alloc.Realloc(alloc.pInstance, nullptr, 0, recvBuf.Size()));
            if (*ppSettingsBlobs != nullptr)
            {
                DevDriver::Platform::Memcpy_s(*ppSettingsBlobs, recvBuf.Size(), recvBuf.Data(), recvBuf.Size());
                *pSettingsBlobsSize = recvBuf.Size();
            }
        }
        else
        {
            *ppSettingsBlobs = static_cast<char*>(recvBuf.Transfer(pSettingsBlobsSize));
        }
    }

    return result;
}

DD_RESULT SendAllUserOverrides_Wrapper(
    DDSettingsInstance*                 pInstance,
    uint16_t                            umdConnectionId,
    size_t                              numComponents,
    const DDSettingsComponentValueRefs* pComponentsOverrides)
{
    auto pSettings = reinterpret_cast<DevDriver::Settings*>(pInstance);
    return pSettings->SendAllUserOverrides(numComponents, pComponentsOverrides, umdConnectionId);
}

DD_RESULT QueryAllCurrentValues_Wrapper(
    DDSettingsInstance* pInstance,
    uint16_t            umdConnectionId,
    uint8_t**           ppBuffer,
    size_t*             pBufSize,
    DDAllocator         alloc)
{
    auto pSettings = reinterpret_cast<DevDriver::Settings*>(pInstance);

    DevDriver::DynamicBuffer recvBuf(alloc);

    DD_RESULT result = pSettings->QueryAllCurrentValues(&recvBuf, umdConnectionId);
    if (result == DD_RESULT_SUCCESS)
    {
        if (recvBuf.Size() != recvBuf.Capacity())
        {
            *ppBuffer = static_cast<uint8_t*>(alloc.Realloc(alloc.pInstance, nullptr, 0, recvBuf.Size()));
            if (*ppBuffer != nullptr)
            {
                DevDriver::Platform::Memcpy_s(*ppBuffer, recvBuf.Size(), recvBuf.Data(), recvBuf.Size());
                *pBufSize = recvBuf.Size();
            }
        }
        else
        {
            *ppBuffer = static_cast<uint8_t*>(recvBuf.Transfer(pBufSize));
        }
    }

    return result;
}

DD_RESULT GetUnsupportedExperiments_Wrapper(DDSettingsInstance* pInstance,
                                            uint16_t            umdConnectionId,
                                            uint8_t**           ppBuffer,
                                            size_t*             pBufSize,
                                            DDAllocator         alloc)
{
    auto pSettings = reinterpret_cast<DevDriver::Settings*>(pInstance);

    DevDriver::DynamicBuffer recvBuf(alloc);

    DD_RESULT result = pSettings->GetUnsupportedExperiments(&recvBuf, umdConnectionId);
    if (result == DD_RESULT_SUCCESS)
    {
        if (recvBuf.Size() != recvBuf.Capacity())
        {
            *ppBuffer = static_cast<uint8_t*>(alloc.Realloc(alloc.pInstance, nullptr, 0, recvBuf.Size()));
            if (*ppBuffer != nullptr)
            {
                DevDriver::Platform::Memcpy_s(*ppBuffer, recvBuf.Size(), recvBuf.Data(), recvBuf.Size());
                *pBufSize = recvBuf.Size();
            }
        }
        else
        {
            *ppBuffer = static_cast<uint8_t*>(recvBuf.Transfer(pBufSize));
        }
    }

    return result;
}

DD_RESULT QueryRegistryOverrides_Wrapper(DDSettingsInstance*     pInstance,
                                         DD_SETTINGS_DRIVER_TYPE driverType,
                                         const char*             pBlobs,
                                         uint8_t**               ppBuffer,
                                         size_t*                 pSize,
                                         DDAllocator             alloc)
{
    auto pSettings = reinterpret_cast<DevDriver::Settings*>(pInstance);

    std::vector <DDSettingsRegistryInfo> registryInfo;

    DD_RESULT result = pSettings->QueryRegistryOverrides(driverType, pBlobs, registryInfo);

    if (result == DD_RESULT_SUCCESS)
    {
        size_t bufferSize = registryInfo.size() * sizeof(DDSettingsRegistryInfo);
        *ppBuffer = static_cast<uint8_t*>(
            alloc.Realloc(alloc.pInstance, nullptr, 0, bufferSize));

        if (*ppBuffer != nullptr)
        {
            DevDriver::Platform::Memcpy_s(*ppBuffer, bufferSize, registryInfo.data(), bufferSize);
            *pSize = registryInfo.size();
        }
    }

    return result;
}

DD_RESULT ClearRegistryOverride_Wrapper(DDSettingsInstance*           pInstance,
                                        DD_SETTINGS_DRIVER_TYPE       driverType,
                                        const DDSettingsRegistryInfo* pRegistrySetting)
{
    auto pSettings = reinterpret_cast<DevDriver::Settings*>(pInstance);

    return pSettings->ClearRegistryOverride(driverType, pRegistrySetting);
}

DD_RESULT QueryKernelSettingsBlobsAll_Wrapper(DDSettingsInstance* pInstance,
                                              DDGpuId             gpuId,
                                              uint8_t**           ppSettingsBlobs,
                                              size_t*             pSettingsBlobsSize,
                                              DDAllocator         alloc)
{
    auto pSettings = reinterpret_cast<DevDriver::Settings*>(pInstance);
    DevDriver::DynamicBuffer recvBuf(alloc);

    DD_RESULT result = pSettings->QueryKernelSettingsBlobsAll(gpuId, &recvBuf);

    if (result == DD_RESULT_SUCCESS)
    {
        if (recvBuf.Size() != recvBuf.Capacity())
        {
            *ppSettingsBlobs = static_cast<uint8_t*>(alloc.Realloc(alloc.pInstance, nullptr, 0, recvBuf.Size()));
            if (*ppSettingsBlobs != nullptr)
            {
                DevDriver::Platform::Memcpy_s(*ppSettingsBlobs, recvBuf.Size(), recvBuf.Data(), recvBuf.Size());
                *pSettingsBlobsSize = recvBuf.Size();
            }
        }
        else
        {
            *ppSettingsBlobs = static_cast<uint8_t*>(recvBuf.Transfer(pSettingsBlobsSize));
        }
    }

    return result;
}

DD_RESULT QueryAllCurrentKernelValues_Wrapper(DDSettingsInstance* pInstance,
                                              DDGpuId             gpuId,
                                              uint8_t**           ppBuffer,
                                              size_t*             pSize,
                                              DDAllocator         alloc)
{
    auto pSettings = reinterpret_cast<DevDriver::Settings*>(pInstance);

    DevDriver::DynamicBuffer recvBuf(alloc);

    DD_RESULT result = pSettings->QueryAllCurrentKernelValues(gpuId, &recvBuf);
    if (result == DD_RESULT_SUCCESS)
    {
        if (recvBuf.Size() != recvBuf.Capacity())
        {
            *ppBuffer = static_cast<uint8_t*>(alloc.Realloc(alloc.pInstance, nullptr, 0, recvBuf.Size()));
            if (*ppBuffer != nullptr)
            {
                DevDriver::Platform::Memcpy_s(*ppBuffer, recvBuf.Size(), recvBuf.Data(), recvBuf.Size());
                *pSize = recvBuf.Size();
            }
        }
        else
        {
            *ppBuffer = static_cast<uint8_t*>(recvBuf.Transfer(pSize));
        }
    }

    return result;
}

DD_RESULT WriteKernelSettingOverride_Wrapper(DDSettingsInstance* pInstance,
                                             DDGpuId             gpuId,
                                             uint8_t             type,
                                             uint64_t            numericValue,
                                             const char*         pSettingName,
                                             const char*         pStringValue)
{
    auto pSettings = reinterpret_cast<DevDriver::Settings*>(pInstance);
    return pSettings->WriteKernelSettingOverride(gpuId, type, numericValue, pSettingName, pStringValue);
}

DD_RESULT TriggerKernelPnpReload_Wrapper(DDSettingsInstance* pInstance,
                                          DDGpuId             gpuId)
{
    auto pSettings = reinterpret_cast<DevDriver::Settings*>(pInstance);
    return pSettings->TriggerKernelPnpReload(gpuId);
}

} // anonymous namespace

namespace DevDriver
{

Settings::Settings()
    : m_net{DD_API_INVALID_HANDLE}
    , m_netProtocolId{0}
    , m_routerConnectionId{0}
{
}

DD_RESULT Settings::Initialize(DDApiRegistry* pApiRegistry)
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
        DDSettingsApi settingsApi {
            reinterpret_cast<DDSettingsInstance*>(this),
            QuerySettingsBlobsAll_Wrapper,
            SendAllUserOverrides_Wrapper,
            QueryAllCurrentValues_Wrapper,
            GetUnsupportedExperiments_Wrapper,
            QueryRegistryOverrides_Wrapper,
            ClearRegistryOverride_Wrapper,
            QueryKernelSettingsBlobsAll_Wrapper,
            QueryAllCurrentKernelValues_Wrapper,
            WriteKernelSettingOverride_Wrapper,
            TriggerKernelPnpReload_Wrapper
        };

        result = pApiRegistry->Add(
            pApiRegistry->pInstance,
            DD_SETTINGS_API_NAME,
            DDVersion {
                DD_SETTINGS_API_VERSION_MAJOR,
                DD_SETTINGS_API_VERSION_MINOR,
                DD_SETTINGS_API_VERSION_PATCH},
            &settingsApi,
            sizeof(settingsApi));

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("Failed to register DDSettingsApi. DD_RESULT: %u.", result);
        }
    }

    return result;
}

void Settings::SetRpcClientInfo(DDNetConnection ddNet, uint16_t routerConnectionId, uint16_t amdLogConnectionId)
{
    m_net = ddNet;
    m_routerConnectionId = routerConnectionId;
    m_amdLogConnectionId = amdLogConnectionId;
}

void Settings::ClearAfterRouterDisconnect()
{
    m_net                = DD_API_INVALID_HANDLE;
    m_routerConnectionId = DD_API_INVALID_CLIENT_ID;
    m_amdLogConnectionId = DD_API_INVALID_CLIENT_ID;
}

DD_RESULT Settings::QuerySettingsBlobsAll(
    DD_SETTINGS_DRIVER_TYPE driverType,
    const char*             pDriverPathOverride,
    size_t                  driverPathOverrideSize,
    bool                    reload,
    DynamicBuffer*          pOutBuf)
{
    const size_t MAX_DRIVER_PATH_SIZE = 4 * 1024;

    DD_RESULT result = DD_RESULT_SUCCESS;

    if (driverPathOverrideSize > MAX_DRIVER_PATH_SIZE)
    {
        LOG_ERROR(
            "Failed to QuerySettingsBlobsAll: the maximum driver override path size is %u. The actual size is: %u.",
            MAX_DRIVER_PATH_SIZE,
            driverPathOverrideSize);

        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    if ((driverPathOverrideSize > 0) && pDriverPathOverride == nullptr)
    {
        LOG_ERROR(
            "Failed to QuerySettingsBlobsAll: "
            "`pDriverPathOverride` is NULL, but `driverPathOverrideSize` is not 0.");

        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    DriverSiphonClient rpcClient;
    if (result == DD_RESULT_SUCCESS)
    {
        DDRpcClientCreateInfo createInfo = {};
        createInfo.hConnection = m_net;
        createInfo.protocolId  = m_netProtocolId;
        createInfo.clientId    = m_routerConnectionId;

        result = rpcClient.Connect(createInfo);
    }

    if (result == DD_RESULT_SUCCESS)
    {
        DynamicBuffer paramBuf;
        paramBuf.Reserve(1024);

        uint16_t pathSizeWithNullTerminator = SafeCastToU16(
            (driverPathOverrideSize == 0) ? 0 : (driverPathOverrideSize + 1));

        DDSettingsSiphonQuerySettingsBlobsAllParams params {driverType, reload, pathSizeWithNullTerminator};
        paramBuf.Copy(&params, sizeof(params));

        if (driverPathOverrideSize > 0)
        {
            paramBuf.Copy(pDriverPathOverride, driverPathOverrideSize);
            paramBuf.Copy("\0", 0);
        }

        DDByteWriter writer {
            ByteWriterBegin,
            ByteWriterWriteByte,
            ByteWriterEnd,
            pOutBuf};

        result = rpcClient.QuerySettingsBlobsAll(paramBuf.Data(), paramBuf.Size(), writer);
    }

    return result;
}

DD_RESULT Settings::SendAllUserOverrides(
    size_t                              numComponents,
    const DDSettingsComponentValueRefs* pAllOverrides,
    uint16_t                            umdConnectionId)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    // Data are laid out in the following format:
    //
    // DDSettingsAllComponentsHeader
    // DDSettingsComponentHeader
    //   DDSettingsValueHeader | variable-sized value data
    //   .. repeat for all the settings in the component
    // .. repeat for all components

    DynamicBuffer compsBuf;
    compsBuf.Reserve(1024);

    DDSettingsAllComponentsHeader allCompsHeader {};
    allCompsHeader.version = 1;
    allCompsHeader.numComponents = SafeCastToU16(numComponents);

    compsBuf.Copy(&allCompsHeader, sizeof(allCompsHeader));

    const DDSettingsComponentValueRefs* pCompOverrides = pAllOverrides;

    for (size_t compIndex = 0; compIndex < numComponents; ++compIndex)
    {
        size_t componentOffset = compsBuf.Size();

        DDSettingsComponentHeader compHeader {};

        Platform::Memcpy_s(compHeader.name, sizeof(compHeader.name), pCompOverrides->componentName, sizeof(compHeader.name));
        compHeader.name[DD_SETTINGS_MAX_COMPONENT_NAME_SIZE - 1] = '\0';

        compHeader.size = 0;
        compHeader.numValues = SafeCastToU16(pCompOverrides->numValues);

        compsBuf.Copy(&compHeader, sizeof(compHeader));

        DDSettingsValueRef* pValue = pCompOverrides->pValues;
        for (size_t valueIndex = 0; valueIndex < pCompOverrides->numValues; ++valueIndex)
        {
            DDSettingsValueHeader valueHeader {};
            valueHeader.hash      = pValue->hash;
            valueHeader.type      = pValue->type;
            valueHeader.valueSize = pValue->size;

            compsBuf.Copy(&valueHeader, sizeof(valueHeader));
            compsBuf.Copy(pValue->pValue, pValue->size);
            if (compsBuf.Error() != DD_RESULT_SUCCESS)
            {
                break;
            }

            pValue += 1;
        }

        if (compsBuf.Error() == DD_RESULT_SUCCESS)
        {
            size_t compSize = compsBuf.Size() - componentOffset;

            auto pWrittenCompHeader = (DDSettingsComponentHeader*)((uint8_t*)compsBuf.Data() + componentOffset);
            pWrittenCompHeader->size = SafeCastToU32(compSize);
        }
        else
        {
            break;
        }

        pCompOverrides += 1;
    }

    result = compsBuf.Error();
    if (result == DD_RESULT_SUCCESS)
    {
        DDRpcClientCreateInfo createInfo = {};
        createInfo.hConnection = m_net;
        createInfo.protocolId  = m_netProtocolId;
        createInfo.clientId    = umdConnectionId;

        SettingsRpc::SettingsRpcClient settingsRpcClient;
        result = settingsRpcClient.Connect(createInfo);

        if (result == DD_RESULT_SUCCESS)
        {
            result = settingsRpcClient.SendAllUserOverrides(compsBuf.Data(), compsBuf.Size());
        }
    }

    return result;
}

DD_RESULT Settings::QueryAllCurrentValues(DynamicBuffer* pOutBuf, uint16_t umdConnectionId)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    DDRpcClientCreateInfo createInfo = {};
    createInfo.hConnection = m_net;
    createInfo.protocolId  = m_netProtocolId;
    createInfo.clientId    = umdConnectionId;

    SettingsRpc::SettingsRpcClient settingsRpcClient;
    result = settingsRpcClient.Connect(createInfo);

    if (result == DD_RESULT_SUCCESS)
    {
        DDByteWriter writer {
            ByteWriterBegin,
            ByteWriterWriteByte,
            ByteWriterEnd,
            pOutBuf};

        result = settingsRpcClient.QueryAllCurrentValues(writer);
    }

    return result;
}

DD_RESULT Settings::GetUnsupportedExperiments(DynamicBuffer* pOutBuf, uint16_t umdConnectionId)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    DDRpcClientCreateInfo createInfo = {};
    createInfo.hConnection           = m_net;
    createInfo.protocolId            = m_netProtocolId;
    createInfo.clientId              = umdConnectionId;

    SettingsRpc::SettingsRpcClient settingsRpcClient;
    result = settingsRpcClient.Connect(createInfo);

    if (result == DD_RESULT_SUCCESS)
    {
        DDByteWriter writer{ ByteWriterBegin, ByteWriterWriteByte, ByteWriterEnd, pOutBuf };

        result = settingsRpcClient.GetUnsupportedExperiments(writer);
    }

    return result;
}

#if defined(DD_PLATFORM_WINDOWS_UM)

DD_RESULT FinalizeSettings(const char* pBlobs, std::vector<DDSettingsRegistryInfo>& output)
{
    DD_RESULT result = DD_RESULT_SUCCESS;
    const SettingsBlobsAll* pSettingsBlobAllHeader = reinterpret_cast<const SettingsBlobsAll*>(pBlobs);

    if (pSettingsBlobAllHeader->nblobs > 0)
    {
        const SettingsBlob* pBlob = reinterpret_cast<const SettingsBlob*>(pSettingsBlobAllHeader + 1);
        for (uint32_t blob = 0; blob < pSettingsBlobAllHeader->nblobs; ++blob)
        {
            if (pBlob->blobSize == 0)
            {
                continue;
            }
            rapidjson::Document document;
            document.Parse((const char*)pBlob->blob, pBlob->blobSize);

            const rapidjson::Value& settings = document["Settings"];

            rapidjson::Value::ConstValueIterator itr = settings.Begin();

            // Iterate the settings json and update the missing fields
            for (; itr != settings.End(); ++itr)
            {
                // There should only be a few settings set in the registry, so this isn't a huge problem
                for (auto& registrySetting : output)
                {
                    const auto nameHashField  = itr->FindMember("NameHash");
                    const auto nameField      = itr->FindMember("Name");
                    const auto whitelistField = itr->FindMember("whitelist");

                    if (whitelistField != itr->MemberEnd() && whitelistField->value.IsBool())
                    {
                        registrySetting.whitelisted = whitelistField->value.GetBool();
                    }
                    else
                    {
                        registrySetting.whitelisted = false;
                    }

                    if ((registrySetting.storedAsHash) && (nameHashField->value.GetUint() == registrySetting.nameHash))
                    {
                        Platform::Strncpy(registrySetting.settingNameStr, nameField->value.GetString());
                    }
                    else if (strcmp(nameField->value.GetString(), registrySetting.settingNameStr) == 0)
                    {
                        registrySetting.nameHash = nameHashField->value.GetUint();
                    }
                }
            }
        }

        // TODO: Should we validate that all the settings were updated properly?
    }
    else
    {
        result = DD_RESULT_SETTINGS_SERVICE_INVALID_SETTING_DATA;
    }

    return result;
}

#endif

DD_RESULT Settings::QueryRegistryOverrides(DD_SETTINGS_DRIVER_TYPE              driverType,
                                           const char*                          pBlobs,
                                           std::vector<DDSettingsRegistryInfo>& output)
{
#if defined(DD_PLATFORM_WINDOWS_UM)
    DD_RESULT result = DD_RESULT_SUCCESS;

    DriverSiphonClient rpcClient;

    DDRpcClientCreateInfo createInfo = {};
    createInfo.hConnection           = m_net;
    createInfo.protocolId            = m_netProtocolId;
    createInfo.clientId              = m_routerConnectionId;

    result = rpcClient.Connect(createInfo);

    if (result == DD_RESULT_SUCCESS)
    {
        DevDriver::DynamicBuffer outBuf;

        DDByteWriter writer {
            ByteWriterBegin,
            ByteWriterWriteByte,
            ByteWriterEnd,
            &outBuf};

        result = rpcClient.QuerySettingsRegistryOverrides(&driverType, sizeof(driverType), writer);

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("Failed to query settings registry overrides. DD_RESULT: %u.", result);
            return result;
        }

        size_t outBufSize = outBuf.Size();
        output.resize(outBufSize / sizeof(DDSettingsRegistryInfo));

        const size_t copySize = output.size() * sizeof(DDSettingsRegistryInfo);
        Platform::Memcpy_s(output.data(), copySize, outBuf.Data(), copySize);

        // Finish populating the settings info:
        result = FinalizeSettings(pBlobs, output);
    }

    return result;
#else
    DD_API_UNUSED(driverType);
    DD_API_UNUSED(pBlobs);
    DD_API_UNUSED(output);
    return DD_RESULT_COMMON_UNSUPPORTED;
#endif
}

DD_RESULT Settings::ClearRegistryOverride(DD_SETTINGS_DRIVER_TYPE       driverType,
                                          const DDSettingsRegistryInfo* pRegistrySetting)
{
#if defined(DD_PLATFORM_WINDOWS_UM)
    DD_RESULT          result = DD_RESULT_SUCCESS;
    DriverSiphonClient rpcClient;

    DDRpcClientCreateInfo createInfo = {};
    createInfo.hConnection           = m_net;
    createInfo.protocolId            = m_netProtocolId;
    createInfo.clientId              = m_routerConnectionId;

    result = rpcClient.Connect(createInfo);

    if (result == DD_RESULT_SUCCESS)
    {
        constexpr size_t driverTypeSize = sizeof(driverType);
        constexpr size_t registryInfoSize = sizeof(DDSettingsRegistryInfo);
        constexpr size_t bufferSize = driverTypeSize + registryInfoSize;
        uint8_t buffer[bufferSize];

        Platform::Memcpy_s(buffer, driverTypeSize, &driverType, driverTypeSize);
        Platform::Memcpy_s(buffer + driverTypeSize, registryInfoSize, pRegistrySetting, registryInfoSize);

        result = rpcClient.ClearSettingsRegistryOverride(&buffer, bufferSize);
    }

    return result;
#else
    DD_API_UNUSED(driverType);
    DD_API_UNUSED(pRegistrySetting);
    return DD_RESULT_COMMON_UNSUPPORTED;
#endif
}

DD_RESULT Settings::QueryKernelSettingsBlobsAll(DDGpuId        gpuId,
                                                DynamicBuffer* pOutBuf)
{
    DDRpcClientCreateInfo info = {};
    info.hConnection           = m_net;
    info.clientId              = static_cast<DDClientId>(m_amdLogConnectionId);
    AmdLogUtils::AmdLogUtilsClient amdLogUtilsCLient;
    DD_RESULT result = amdLogUtilsCLient.Connect(info);

    if (result == DD_RESULT_SUCCESS)
    {
        DDByteWriter writer {
            ByteWriterBegin,
            ByteWriterWriteByte,
            ByteWriterEnd,
            pOutBuf};

        result = amdLogUtilsCLient.QueryKernelSettingsBlobsAll(&gpuId, sizeof(gpuId), writer);

    }

    return result;
}

DD_RESULT Settings::QueryAllCurrentKernelValues(DDGpuId        gpuId,
                                                DynamicBuffer* pOutBuf)
{
    DDRpcClientCreateInfo info = {};
    info.hConnection           = m_net;
    info.clientId              = static_cast<DDClientId>(m_amdLogConnectionId);
    AmdLogUtils::AmdLogUtilsClient amdLogUtilsCLient;
    DD_RESULT result = amdLogUtilsCLient.Connect(info);

    if (result == DD_RESULT_SUCCESS)
    {
        DDByteWriter writer {
            ByteWriterBegin,
            ByteWriterWriteByte,
            ByteWriterEnd,
            pOutBuf};

        result = amdLogUtilsCLient.QueryAllCurrentKernelValues(&gpuId, sizeof(gpuId), writer);
    }

    return result;
}

DD_RESULT Settings::WriteKernelSettingOverride(DDGpuId        gpuId,
                                               uint8_t        type,
                                               uint64_t       numericValue,
                                               const char*    pSettingName,
                                               const char*    pStringValue)
{
#if defined(DD_PLATFORM_WINDOWS_UM)
    DriverSiphonClient rpcClient;

    DDRpcClientCreateInfo createInfo = {};
    createInfo.hConnection           = m_net;
    createInfo.protocolId            = m_netProtocolId;
    createInfo.clientId              = m_routerConnectionId;

    DD_RESULT result = rpcClient.Connect(createInfo);
    if (result != DD_RESULT_SUCCESS)
    {
        return result;
    }

    const size_t settingNameSize = (pSettingName != nullptr) ? strlen(pSettingName) + 1 : 0;
    const size_t stringValueSize = (pStringValue != nullptr) ? strlen(pStringValue) + 1 : 0;

    if (settingNameSize == 0)
    {
        return DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
    }

    // Wire format uses uint16_t for sizes — reject strings that would truncate.
    if ((settingNameSize > UINT16_MAX) || (stringValueSize > UINT16_MAX))
    {
        return DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
    }

    const size_t bufferSize = sizeof(DDSettingsSiphonWriteKernelSettingOverrideParams)
                            + settingNameSize
                            + stringValueSize;

    std::vector<uint8_t> buffer(bufferSize);
    auto* pParams            = reinterpret_cast<DDSettingsSiphonWriteKernelSettingOverrideParams*>(buffer.data());
    pParams->gpuId           = gpuId;
    pParams->type            = type;
    pParams->numericValue    = numericValue;
    pParams->settingNameSize = static_cast<uint16_t>(settingNameSize);
    pParams->stringValueSize = static_cast<uint16_t>(stringValueSize);

    char* pDst = reinterpret_cast<char*>(pParams + 1);
    Platform::Memcpy_s(pDst, settingNameSize, pSettingName, settingNameSize);
    if (stringValueSize > 0)
    {
        Platform::Memcpy_s(pDst + settingNameSize, stringValueSize, pStringValue, stringValueSize);
    }

    return rpcClient.WriteKernelSettingOverride(buffer.data(), bufferSize);
#else
    DD_API_UNUSED(gpuId);
    DD_API_UNUSED(type);
    DD_API_UNUSED(numericValue);
    DD_API_UNUSED(pSettingName);
    DD_API_UNUSED(pStringValue);
    return DD_RESULT_COMMON_UNSUPPORTED;
#endif
}

DD_RESULT Settings::TriggerKernelPnpReload(DDGpuId gpuId)
{
#if defined(DD_PLATFORM_WINDOWS_UM)
    DriverSiphonClient rpcClient;

    DDRpcClientCreateInfo createInfo = {};
    createInfo.hConnection           = m_net;
    createInfo.protocolId            = m_netProtocolId;
    createInfo.clientId              = m_routerConnectionId;

    DD_RESULT result = rpcClient.Connect(createInfo);
    if (result != DD_RESULT_SUCCESS)
    {
        return result;
    }

    return rpcClient.TriggerKernelPnpReload(&gpuId, sizeof(gpuId));
#else
    DD_API_UNUSED(gpuId);
    return DD_RESULT_COMMON_UNSUPPORTED;
#endif
}

} // namespace DevDriver
