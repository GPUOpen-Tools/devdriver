/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_clocks.h>
#include <dd_connection_api.h>
#include <util/vector.h>
#include <system_info_reader.h>
#include <g_RouterUtilsRpcClient.h>
#include <ddCommon.h>

#include <cstring>

#define LOG_ERROR(fmt, ...) s_pLogger->Log(s_pLogger->pInstance, DD_LOG_LVL_ERROR, "[DDClocks] " fmt, ## __VA_ARGS__)

namespace
{
DDLoggerApi* s_pLogger = nullptr;

// An exact copy of the same struct in RouterUtilsServiceLinux.cpp
struct ClockModeInfo
{
    DD_DEVICE_CLOCK_MODE mode;
    uint64_t             engineClock;
    uint64_t             memoryClock;
};

template<typename RpcClientT>
DD_RESULT RpcQueryCurrentClockMode(
    DDNetConnection          ddnet,
    DDClientId               clientId,
    DDGpuId                  gpuId,
    DynamicBufferByteWriter& writer)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    RpcClientT rpcClient;

    DDRpcClientCreateInfo clientInfo = {};
    clientInfo.hConnection           = ddnet;
    clientInfo.clientId              = clientId;

    result = rpcClient.Connect(clientInfo);
    if (result == DD_RESULT_SUCCESS)
    {
        result = rpcClient.QueryCurrentClockMode(&gpuId, sizeof(gpuId), *writer.Writer());
        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("RPC call 'QueryCurrentClockMode' failed. DD_RESULT: %u.", result);
        }
    }
    else
    {
        LOG_ERROR("Failed to connect RPC client. DD_RESULT: %u.", result);
    }

    return result;
}

template<typename RpcClientT>
DD_RESULT RpcSetClockMode(DDNetConnection ddnet, DDClientId clientId, DDClockModeInfo clockInfo)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    RpcClientT rpcClient;

    DDRpcClientCreateInfo clientInfo = {};
    clientInfo.hConnection           = ddnet;
    clientInfo.clientId              = clientId;

    result = rpcClient.Connect(clientInfo);
    if (result == DD_RESULT_SUCCESS)
    {
        result = rpcClient.SetClockMode(&clockInfo, sizeof(clockInfo));
        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("RPC call 'SetClockMode' failed. DD_RESULT: %u.", result);
        }
    }
    else
    {
        LOG_ERROR("Failed to connect RPC client. DD_RESULT: %u.", result);
    }

    return result;
}

// DDClocksApi wrapper functions.

DD_RESULT QueryClockModesWrapper(DDClocksInstance*                   pInstance,
                                 uint32_t*                           pNumClockModes,
                                 DDDeviceClocksClockModeInfo*        pClockModes,
                                 DDGpuId                             gpuId)
{
    DevDriver::Clocks* pClocks = reinterpret_cast<DevDriver::Clocks*>(pInstance);

    return pClocks->QueryClockModes(pNumClockModes, pClockModes, gpuId);
}

DD_RESULT QueryCurrentClockModeWrapper(DDClocksInstance*        pInstance,
                                       DD_DEVICE_CLOCK_MODE*    pClockModeId,
                                       DDGpuId                  gpuId)
{
    DevDriver::Clocks* pClocks = reinterpret_cast<DevDriver::Clocks*>(pInstance);

    return pClocks->QueryCurrentClockMode(pClockModeId, gpuId);
}

DD_RESULT SetClockModeWrapper(DDClocksInstance*       pInstance,
                              DD_DEVICE_CLOCK_MODE    clockModeId,
                              DDGpuId                 gpuId)
{
    DevDriver::Clocks* pClocks = reinterpret_cast<DevDriver::Clocks*>(pInstance);

    return pClocks->SetClockMode(clockModeId, gpuId);
}

// DDConnectionCallbacks wrapper functions.

void OnRouterConnectedWrapper(DDConnectionCallbacksImpl* pInstance, DDConnectionId connectionId)
{
    auto pClocks = reinterpret_cast<DevDriver::Clocks*>(pInstance);
    pClocks->OnRouterConnected(connectionId);
}
} // anonymous namespace

namespace DevDriver
{

/// Static array of clock mode descriptions that will be returned to applications
constexpr DDDeviceClocksClockModeDescription kClockModeDescriptions[DD_DEVICE_CLOCK_MODE_COUNT] = {
    {
        "Unknown",                                       // Name
        "An unknown or invalid clocking mode.",          // Description
        DD_DEVICE_CLOCK_MODE_UNKNOWN,                    // Index into this array
    },

    {
        "Normal",
        "The device clocks are variable and downclock when the device is idle. This mode is the normal clocking mode.",
        DD_DEVICE_CLOCK_MODE_NORMAL,
    },

    {
        "Stable",
        // TODO: Get more elaborate descriptions from the Tools team and elaborate here.
        "Attempts to keep all clocks as stable as possible. These clocks are thermally stable.",
        DD_DEVICE_CLOCK_MODE_STABLE,
    },

    {
        "Peak",
        // TODO: Get more elaborate descriptions from the Tools team and elaborate here.
        "Attempts to keep all clocks as high as possible. This is not thermally stable.",
        DD_DEVICE_CLOCK_MODE_PEAK,
    }
};

/// Looks up the clock mode description associated with the provided id
const DDDeviceClocksClockModeDescription& GetClockModeDesc(uint32_t id)
{
    if (id >= Platform::ArraySize(kClockModeDescriptions))
    {
        id = 0;
    }

    return kClockModeDescriptions[id];
}

Clocks::Clocks()
    : m_net {DD_API_INVALID_HANDLE}
    , m_amdLogConnectionId {0}
    , m_routerConnectionId {0}
    , m_isTargetLinux {false}
    , m_targetPlatformChecked {false}
    , m_pRouterUtilsApi {nullptr}
{

}

Clocks::~Clocks()
{
    ClearAfterRouterDisconnect();
}

DD_RESULT Clocks::PopulateClockModes(DDGpuId                         gpuId,
                                     DDDeviceClocksClockModeInfo*    pClockModes,
                                     AmdLogUtils::AmdLogUtilsClient* pAmdLogUtilsClient)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    // Init the clock mode descriptions:
    for (uint32_t modeIndex = 0; modeIndex < DD_DEVICE_CLOCK_MODE_COUNT; ++modeIndex)
    {
        pClockModes[modeIndex].pDescription = &kClockModeDescriptions[modeIndex];
    }

    // We start at index 1 because the 0th item in the list is the unknown clock mode which shouldn't be queried.
    const uint32_t kFirstValidModeIndex = 1;
    for (uint32_t modeIndex = kFirstValidModeIndex; modeIndex < Platform::ArraySize(kClockModeDescriptions); ++modeIndex)
    {
        DynamicBufferByteWriter writer;
        DDClockModeInfo info = {};
        info.gpuId           = gpuId;
        info.mode            = GetClockModeDesc(modeIndex).id;

        // Attempt to query the device clock from the remote client
        result = pAmdLogUtilsClient->QueryDeviceClocks(
            &info, sizeof(info), *writer.Writer());

        if (result == DD_RESULT_SUCCESS)
        {

            DDClockFreqs clk = {};
            DD_ASSERT(writer.Size() == sizeof(clk));
            Platform::Memcpy_s(&clk, sizeof(clk), writer.Buffer(), sizeof(DDClockFreqs));

            // Convert the values to hertz
            const uint64_t gpuClockInHz = static_cast<uint64_t>(clk.gpuClock * 1e6);
            const uint64_t memClockInHz = static_cast<uint64_t>(clk.memoryClock * 1e6);

            // Write them into the clock mode info array
            pClockModes[modeIndex].clks.gpuClock    = gpuClockInHz;
            pClockModes[modeIndex].clks.memoryClock = memClockInHz;
        }
        else
        {
            // Break out of the loop if we encounter an issue
            LOG_ERROR("Failed to get clock info for GPUID %d: DD_RESULT: %u.", gpuId, result);
            break;
        }
    }

    return result;
}

DD_RESULT Clocks::Initialize(DDApiRegistry* pApiRegistry)
{
    DD_RESULT result = pApiRegistry->Get(
        pApiRegistry->pInstance,
        DD_LOGGER_API_NAME,
        DDVersion{ DD_LOGGER_API_VERSION_MAJOR, DD_LOGGER_API_VERSION_MINOR, DD_LOGGER_API_VERSION_PATCH },
        reinterpret_cast<void**>(&s_pLogger));

    if (result == DD_RESULT_SUCCESS)
    {
        DDClocksApi clocksApi{ reinterpret_cast<DDClocksInstance*>(this),
                               QueryClockModesWrapper,
                               QueryCurrentClockModeWrapper,
                               SetClockModeWrapper };

        result = pApiRegistry->Add(pApiRegistry->pInstance,
                                   DD_CLOCKS_API_NAME,
                                   DDVersion{ DD_CLOCKS_API_VERSION_MAJOR,
                                              DD_CLOCKS_API_VERSION_MINOR,
                                              DD_CLOCKS_API_VERSION_PATCH },
                                   &clocksApi,
                                   sizeof(clocksApi));

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("Failed to register DDClocksApi. DD_RESULT: %u.", result);
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

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("Failed to get DDRouterUtilsApi. DD_RESULT: %u.", result);
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        DDConnectionApi* pConnectionApi {};

        result = pApiRegistry->Get(
            pApiRegistry->pInstance,
            DD_CONNECTION_API_NAME,
            DDVersion{
                DD_CONNECTION_API_VERSION_MAJOR,
                DD_CONNECTION_API_VERSION_MINOR,
                DD_CONNECTION_API_VERSION_PATCH},
            reinterpret_cast<void**>(&pConnectionApi));

        if (result == DD_RESULT_SUCCESS)
        {
            // Register connection callbacks
            DDConnectionCallbacks connectionCbs = {};
            connectionCbs.pImpl                 = reinterpret_cast<DDConnectionCallbacksImpl*>(this);
            connectionCbs.OnRouterConnected     = &OnRouterConnectedWrapper;

            pConnectionApi->AddConnectionCallbacks(pConnectionApi->pInstance, &connectionCbs);
        }
        else
        {
            LOG_ERROR("Failed to get DDConnectionApi. DD_RESULT: %u.", result);
        }
    }

    return result;
}

void Clocks::ClearAfterRouterDisconnect()
{
    m_net                = DD_API_INVALID_HANDLE;
    m_amdLogConnectionId = DD_API_INVALID_CLIENT_ID;
}

void Clocks::SetRpcClientInfo(DDNetConnection ddNet, uint16_t amdLogConnectionId)
{
    m_net                = ddNet;
    m_amdLogConnectionId = amdLogConnectionId;
}

DD_RESULT Clocks::QueryClockModes(
    uint32_t*                     pNumClockModes,
    DDDeviceClocksClockModeInfo*  pClockModes,
    DDGpuId                       gpuId)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    if (pNumClockModes == nullptr)
    {
        LOG_ERROR("Failed to query clock modes: the parameter `pNumClockModes` is NULL.");
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    if ((result == DD_RESULT_SUCCESS) && (m_net == DD_API_INVALID_HANDLE))
    {
        LOG_ERROR("Failed to query clock modes: m_net is NULL.");
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    if ((result == DD_RESULT_SUCCESS) && (m_targetPlatformChecked == false))
    {
        LOG_ERROR("Failed to query clock modes: target platform unknown.");
        result = DD_RESULT_COMMON_UNKNOWN;
    }

    if (result == DD_RESULT_SUCCESS)
    {
        *pNumClockModes = DD_DEVICE_CLOCK_MODE_COUNT;

        if (pClockModes != nullptr)
        {
            if (m_isTargetLinux)
            {
                for (uint32 modeIndex = 0; modeIndex < DD_DEVICE_CLOCK_MODE_COUNT; ++modeIndex)
                {
                    pClockModes[modeIndex].pDescription = &kClockModeDescriptions[modeIndex];
                }

                RouterUtilsRpc::RouterUtilsRpcClient routerUtilsRpcClient;

                DDRpcClientCreateInfo clientInfo {};
                clientInfo.hConnection = m_net;
                clientInfo.clientId    = m_routerConnectionId;
                result = routerUtilsRpcClient.Connect(clientInfo);

                if (result == DD_RESULT_SUCCESS)
                {
                    DynamicBufferByteWriter writer;
                    result = routerUtilsRpcClient.QueryDeviceClocks(&gpuId, sizeof(gpuId), *writer.Writer());
                    if (result == DD_RESULT_SUCCESS)
                    {
                        size_t dataOffset = 0;
                        const uint8_t* pData = static_cast<const uint8_t*>(writer.Buffer());
                        uint32_t clockInfoCount = 0;
                        Platform::Memcpy_s(&clockInfoCount, sizeof(clockInfoCount), pData + dataOffset, sizeof(clockInfoCount));
                        dataOffset += sizeof(clockInfoCount);

                        for (uint32_t i = 0; i < clockInfoCount; ++i)
                        {
                            ClockModeInfo clockInfo {};
                            Platform::Memcpy_s(&clockInfo, sizeof(clockInfo), pData + dataOffset, sizeof(clockInfo));
                            dataOffset += sizeof(clockInfo);

                            pClockModes[clockInfo.mode].clks.gpuClock = clockInfo.engineClock;
                            pClockModes[clockInfo.mode].clks.memoryClock = clockInfo.memoryClock;
                        }
                    }
                    else
                    {
                        LOG_ERROR("Failed to query device clocks. DD_RESULT: %u.", result);
                    }
                }
                else
                {
                    LOG_ERROR("Failed to connect RouterUtilsRpcClient. DD_RESULT: %u.", result);
                }
            }
            else
            {
                DDRpcClientCreateInfo info = {};
                info.hConnection           = m_net;
                info.clientId              = static_cast<DDClientId>(m_amdLogConnectionId);
                AmdLogUtils::AmdLogUtilsClient amdLogUtilsCLient;
                result = amdLogUtilsCLient.Connect(info);

                if (result == DD_RESULT_SUCCESS)
                {
                    result = PopulateClockModes(gpuId, pClockModes, &amdLogUtilsCLient);
                }
            }
        }
    }

    return result;
}

DD_RESULT Clocks::QueryCurrentClockMode(DD_DEVICE_CLOCK_MODE* pClockModeId, DDGpuId gpuId)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    if (pClockModeId == nullptr)
    {
        LOG_ERROR("Failed to query current clock mode: the parameter `pClockModeId` is NULL.");
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    if ((result == DD_RESULT_SUCCESS) && (m_net == DD_API_INVALID_HANDLE))
    {
        LOG_ERROR("Failed to query current clock mode: m_net is NULL.");
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    if ((result == DD_RESULT_SUCCESS) && (m_targetPlatformChecked == false))
    {
        LOG_ERROR("Failed to query current clock mode: target platform unknown.");
        result = DD_RESULT_COMMON_UNKNOWN;
    }

    if (result == DD_RESULT_SUCCESS)
    {
        DynamicBufferByteWriter writer;
        if (m_isTargetLinux)
        {
            result = RpcQueryCurrentClockMode<RouterUtilsRpc::RouterUtilsRpcClient>(
                m_net, m_routerConnectionId, gpuId, writer);
        }
        else
        {
            result = RpcQueryCurrentClockMode<AmdLogUtils::AmdLogUtilsClient >(
                m_net, m_amdLogConnectionId, gpuId, writer);
        }
        if (result == DD_RESULT_SUCCESS)
        {
            Platform::Memcpy_s(pClockModeId, sizeof(*pClockModeId), writer.Buffer(), sizeof(*pClockModeId));
        }
    }

    return result;
}

DD_RESULT Clocks::SetClockMode(DD_DEVICE_CLOCK_MODE clockModeId, DDGpuId gpuId)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    if (m_net == DD_API_INVALID_HANDLE)
    {
        LOG_ERROR("Failed to query clock modes: m_net is NULL.");
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    if ((result == DD_RESULT_SUCCESS) && (m_targetPlatformChecked == false))
    {
        LOG_ERROR("Failed to query clock modes: target platform unknown.");
        result = DD_RESULT_COMMON_UNKNOWN;
    }

    if (result == DD_RESULT_SUCCESS)
    {
        DDClockModeInfo clockInfo = {};
        clockInfo.gpuId           = gpuId;
        clockInfo.mode            = clockModeId;

        if (m_isTargetLinux)
        {
            result = RpcSetClockMode<RouterUtilsRpc::RouterUtilsRpcClient>(m_net, m_routerConnectionId, clockInfo);
        }
        else
        {
            result = RpcSetClockMode<AmdLogUtils::AmdLogUtilsClient>(m_net, m_amdLogConnectionId, clockInfo);
        }
    }

    return result;
}

void Clocks::OnRouterConnected(DDConnectionId connectionId)
{
    m_routerConnectionId = connectionId;
    CheckTargetPlatform();
}

void Clocks::CheckTargetPlatform()
{
    size_t systemInfoSize = 0;
    Vector<char> systemInfoBuf(Platform::GenericAllocCb);
    DD_RESULT result = m_pRouterUtilsApi->GetSysInfo(m_pRouterUtilsApi->pInstance, nullptr, &systemInfoSize);
    if (result == DD_RESULT_SUCCESS)
    {
        systemInfoBuf.Resize(systemInfoSize);
        result = m_pRouterUtilsApi->GetSysInfo(
            m_pRouterUtilsApi->pInstance,
            reinterpret_cast<char*>(systemInfoBuf.Data()),
            &systemInfoSize);

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("Failed to get system info data. DD_RESULT: %u.", result);
        }
    }
    else
    {
        LOG_ERROR("Failed to get system info size. DD_RESULT: %u.", result);
    }

    if (result == DD_RESULT_SUCCESS)
    {
        system_info_utils::SystemInfo systemInfo {};
        std::string sysInfoJson = std::string(systemInfoBuf.Data(), systemInfoBuf.Size());
        bool parsed = system_info_utils::SystemInfoReader::Parse(sysInfoJson, systemInfo);
        if (parsed)
        {
            const std::string_view name(systemInfo.os.name);
            const std::string_view desc(systemInfo.os.desc);
            if ((name == Platform::OsInfo::kOsTypeLinux) ||
                (desc.find(Platform::OsInfo::kOsTypeLinux) != std::string::npos))
            {
                m_isTargetLinux = true;
            }
            else
            {
                m_isTargetLinux = false;
            }
            m_targetPlatformChecked = true;
        }
        else
        {
            LOG_ERROR("Failed to parse system info json");
        }
    }
}

} // namespace DevDriver
