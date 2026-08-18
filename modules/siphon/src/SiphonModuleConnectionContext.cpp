/* Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "SiphonModuleConnectionContext.h"
#include <ddPlatform.h>
#include <ddAdapterInfo.h>
#include <ddApi.h>
#include <ddRpcServer.h>
#include <ddYaml.h>
#include <dd_settings_rpc_types.h>
#include <dd_settings_blob.h>
#include <util/vector.h>

#include <cwalk.h>
#include <yaml.h>

#include <cstdlib>
#include <cstdio>
#include <string_view>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <newdev.h>
#include <cwalk.h>
#include <SetupAPI.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <ntddvdeo.h>
#include <devpkey.h>
#include <set>
#include <dd_registry_utils.h>
#endif

using namespace DevDriver;

#ifdef _WIN32
// Registry keys that have the driver install directory paths.
std::unordered_map<std::string, std::string> driverRegKeys =
{
    {"amdvlk64.dll",   "VulkanDriverName"},
    {"amdvlk32.dll",   "VulkanDriverNameWow"},
    {"amdocl64.dll",   "OpenCLDriverName"},
    {"amdocl32.dll",   "OpenCLDriverNameWow"},
    {"amdhip64.dll",   "OpenCLDriverName"},
    {"amdhip64_6.dll", "OpenCLDriverName"},
    {"atio6axx.dll",   "OpenGLVendorName"},
    {"atioglxx.dll",   "OpenGLVendorNameWow"},
};

const std::vector<std::string> g_RegistryPathDrivers =
{
   "amdocl64.dll",
   "amdocl32.dll",
   "amdhip64.dll",
   "amdhip64_6.dll",
   "amdvlk64.dll",
   "amdvlk32.dll",
   "atio6axx.dll",
   "atioglxx.dll"
};
#endif

namespace
{

typedef int (*PFN_GetSettingsBlobsAll)(uint8_t* pBuffer, size_t bufferSize);

} // anonymous namespace

namespace SiphonModule
{

// =======================================================================================
ModuleConnectionContext::ModuleConnectionContext(
    const DDModuleConnectionContextCreateInfo& createInfo)
    : BaseModuleConnectionContext(createInfo)
    , m_windowsDriverInstallDir {}
    , m_settingsBlobs {}
#ifdef _WIN32
    , m_kmdRegistryKeyByGpuId {}
#endif
{
}

// =======================================================================================
ModuleConnectionContext::~ModuleConnectionContext()
{
}

// =======================================================================================
DD_RESULT ModuleConnectionContext::Initialize()
{
#ifdef _WIN32
    InitDriverInstallDir();
    InitKmdRegistryKeyMap();
#endif

    LoadSettingsBlobsFromAllDrivers();

    // Always call this last to make sure everything has been initialized
    // before receiving RPC requests.
    DD_RESULT result = RegisterRpcService();

    return result;
}

// =======================================================================================
DD_RESULT ModuleConnectionContext::QuerySettingsBlobsAll(
    const void*         pParamBuffer,
    size_t              paramBufferSize,
    const DDByteWriter& writer)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    if (paramBufferSize < sizeof(DDSettingsSiphonQuerySettingsBlobsAllParams))
    {
        result = DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
        m_logger.Error("SiphonModule | ParamBuffer size too small: %u.", paramBufferSize);
    }

    if (result == DD_RESULT_SUCCESS)
    {
        auto pParams = reinterpret_cast<const DDSettingsSiphonQuerySettingsBlobsAllParams*>(pParamBuffer);

        const char* pDriverPathOverride = nullptr;
        if (pParams->driverPathOverrideSize > 0)
        {
            pDriverPathOverride = (const char*)pParamBuffer + sizeof(*pParams);
        }

        if (pParams->driverType < DD_SETTINGS_DRIVER_TYPE_COUNT)
        {
            result = WriteQuerySettingsBlobsAllResultWithReload(
                writer,
                pDriverPathOverride,
                pParams->driverPathOverrideSize,
                pParams->driverType,
                pParams->reload);
        }
        else
        {
            result = DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
            m_logger.Error("SiphonModule | Invalid driver kind: %d.", pParams->driverType);

        }
    }

    return result;
}

#ifdef _WIN32
static std::string FindPathWithSubstring(const std::set<std::string>& registryPaths, const std::string& substring)
{
    for (const auto& path : registryPaths)
    {
        if (path.find(substring) != std::string::npos)
        {
            return path;
        }
    }
    return ""; // Return an empty string if no match is found
}

static const char* GetDriverComponentNameFromType(DD_SETTINGS_DRIVER_TYPE driverType)
{
    switch (driverType)
    {
        case DD_SETTINGS_DRIVER_TYPE_DX12: return "DXC";
        case DD_SETTINGS_DRIVER_TYPE_DX10: return "DXXP";
        case DD_SETTINGS_DRIVER_TYPE_DX9:  return "DX9P";
        case DD_SETTINGS_DRIVER_TYPE_VULKAN: return "VULKAN";
        case DD_SETTINGS_DRIVER_TYPE_OPENGL: return "OGLP";
        default: return "UNKNOWN";
    }
}
#endif

// =======================================================================================
DD_RESULT ModuleConnectionContext::QuerySettingsRegistryOverrides(
        const void*         pParamBuffer,
        size_t              paramBufferSize,
        const DDByteWriter& writer)
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_UNAVAILABLE;
#ifdef _WIN32
    if (paramBufferSize < sizeof(DD_SETTINGS_DRIVER_TYPE))
    {
        result = DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
        m_logger.Error("SiphonModule | ParamBuffer size too small: %u.", paramBufferSize);
    }
    else
    {
        DD_SETTINGS_DRIVER_TYPE driverType =
            *reinterpret_cast<const DD_SETTINGS_DRIVER_TYPE*>(pParamBuffer);
        std::set<std::string> registryPaths;
        GetRegistryPaths(&registryPaths);

        std::string path = FindPathWithSubstring(registryPaths, GetDriverComponentNameFromType(driverType));

        if (path != "")
        {
            std::vector<DDSettingsRegistryInfo> output;
            result = EnumerateDriverRegistry(path, output);

            if ((result == DD_RESULT_SUCCESS) && (!output.empty()))
            {
                size_t totalSize = output.size() * sizeof(DDSettingsRegistryInfo);
                result = writer.pfnBegin(writer.pUserdata, &totalSize);
                if (result == DD_RESULT_SUCCESS)
                {
                    result = writer.pfnWriteBytes(
                        writer.pUserdata,
                        output.data(),
                        output.size() * sizeof(DDSettingsRegistryInfo));

                    writer.pfnEnd(writer.pUserdata, result);
                }
            }
        }
        else
        {
            result = DD_RESULT_DD_GENERIC_FILE_NOT_FOUND;
            m_logger.Error("SiphonModule | Failed to find registry path for driver type: %d.", driverType);
        }
    }
#else
    DD_UNUSED(pParamBuffer);
    DD_UNUSED(paramBufferSize);
    DD_UNUSED(writer);
#endif
    return result;
}

// =======================================================================================
DD_RESULT ModuleConnectionContext::ClearSettingsRegistryOverride(
        const void* pParamBuffer,
        size_t      paramBufferSize)
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_UNAVAILABLE;
#ifdef _WIN32

    if (paramBufferSize < (sizeof(DDSettingsRegistryInfo) + sizeof(DD_SETTINGS_DRIVER_TYPE)))
    {
        result = DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
        m_logger.Error("SiphonModule | ParamBuffer size too small: %u.", paramBufferSize);
        return result;
    }

    const DD_SETTINGS_DRIVER_TYPE driverType =
        *reinterpret_cast<const DD_SETTINGS_DRIVER_TYPE*>(pParamBuffer);

    const DDSettingsRegistryInfo* pRegistrySetting =
        reinterpret_cast<const DDSettingsRegistryInfo*>(
            static_cast<const char*>(pParamBuffer) + sizeof(DD_SETTINGS_DRIVER_TYPE));

    if (pRegistrySetting == nullptr)
    {
        result = DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
        m_logger.Error("SiphonModule | pRegistrySetting is NULL.");
        return result;
    }

    std::set<std::string> registryPaths;

    GetRegistryPaths(&registryPaths);

    std::string path = FindPathWithSubstring(registryPaths, GetDriverComponentNameFromType(driverType));

    if (path != "")
    {
        result = DeleteRegistrySetting(path, pRegistrySetting);
    }
    else
    {
        result = DD_RESULT_DD_GENERIC_FILE_NOT_FOUND;
    }
#else
    DD_UNUSED(pParamBuffer);
    DD_UNUSED(paramBufferSize);
#endif

    return result;
}

#ifdef _WIN32
// Converts a UTF-8 string value and name to wide strings and writes a REG_SZ registry value.
// Returns ERROR_SUCCESS on success, or a Win32 error code on failure.
// The caller is responsible for closing hkey regardless of return value.
static LONG WriteStringRegistryValue(HKEY hkey, const char* pName, const char* pValue, ModuleLogger& logger)
{
    const int wideValueLen = MultiByteToWideChar(CP_UTF8, 0, pValue, -1, nullptr, 0);
    if (wideValueLen <= 0)
    {
        logger.Error("SiphonModule | WriteStringRegistryValue: MultiByteToWideChar value size query failed (%d).", GetLastError());
        return ERROR_INVALID_DATA;
    }
    std::wstring wideStr(wideValueLen, L'\0');
    if (MultiByteToWideChar(CP_UTF8, 0, pValue, -1, wideStr.data(), wideValueLen) <= 0)
    {
        logger.Error("SiphonModule | WriteStringRegistryValue: MultiByteToWideChar value conversion failed (%d).", GetLastError());
        return ERROR_INVALID_DATA;
    }
    const int wideNameLen = MultiByteToWideChar(CP_UTF8, 0, pName, -1, nullptr, 0);
    if (wideNameLen <= 0)
    {
        logger.Error("SiphonModule | WriteStringRegistryValue: MultiByteToWideChar name size query failed (%d).", GetLastError());
        return ERROR_INVALID_DATA;
    }
    std::wstring wideName(wideNameLen, L'\0');
    if (MultiByteToWideChar(CP_UTF8, 0, pName, -1, wideName.data(), wideNameLen) <= 0)
    {
        logger.Error("SiphonModule | WriteStringRegistryValue: MultiByteToWideChar name conversion failed (%d).", GetLastError());
        return ERROR_INVALID_DATA;
    }
    return RegSetValueExW(hkey, wideName.c_str(), 0, REG_SZ,
                          reinterpret_cast<const BYTE*>(wideStr.c_str()),
                          static_cast<DWORD>(wideValueLen * sizeof(wchar_t)));
}
#endif

// =======================================================================================
DD_RESULT ModuleConnectionContext::WriteKernelSettingOverride(
        const void* pParamBuffer,
        size_t      paramBufferSize)
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_UNAVAILABLE;
#ifdef _WIN32
    constexpr size_t kMinParamSize = sizeof(DDSettingsSiphonWriteKernelSettingOverrideParams);
    if (paramBufferSize < kMinParamSize)
    {
        m_logger.Error("SiphonModule | WriteKernelSettingOverride: param buffer too small: %zu.", paramBufferSize);
        return DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
    }

    const auto* pParams = reinterpret_cast<const DDSettingsSiphonWriteKernelSettingOverrideParams*>(pParamBuffer);

    const size_t expectedSize = kMinParamSize +
                                pParams->settingNameSize +
                                pParams->stringValueSize;
    if (paramBufferSize < expectedSize || pParams->settingNameSize == 0)
    {
        m_logger.Error("SiphonModule | WriteKernelSettingOverride: param buffer size mismatch.");
        return DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
    }

    const char* pSettingName = reinterpret_cast<const char*>(pParams + 1);
    const char* pStringValue = pSettingName + pParams->settingNameSize;

    // Validate that the setting name buffer is null-terminated within its declared size.
    if (pSettingName[pParams->settingNameSize - 1] != '\0')
    {
        m_logger.Error("SiphonModule | WriteKernelSettingOverride: settingName not null-terminated.");
        return DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
    }

    // Validate that the string value buffer (if present) is null-terminated within its declared size.
    if ((pParams->stringValueSize > 0) && (pStringValue[pParams->stringValueSize - 1] != '\0'))
    {
        m_logger.Error("SiphonModule | WriteKernelSettingOverride: stringValue not null-terminated.");
        return DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
    }

    // Locate the HKLM registry key for this GPU via the pre-built map
    const auto it = m_kmdRegistryKeyByGpuId.find(pParams->gpuId);
    if (it == m_kmdRegistryKeyByGpuId.end())
    {
        m_logger.Error("SiphonModule | WriteKernelSettingOverride: no PCI device matched GPU 0x%08X.", pParams->gpuId);
        return DD_RESULT_DD_GENERIC_FILE_NOT_FOUND;
    }
    const std::string& registryKeyPath = it->second;

    HKEY  hkey       = nullptr;
    LONG  openResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, registryKeyPath.c_str(), 0, KEY_SET_VALUE, &hkey);
    if (openResult != ERROR_SUCCESS)
    {
        if (openResult == ERROR_ACCESS_DENIED)
        {
            m_logger.Error("SiphonModule | WriteKernelSettingOverride: access denied — RDS must run as administrator.");
            return DD_RESULT_COMMON_ACCESS_DENIED;
        }
        m_logger.Error("SiphonModule | WriteKernelSettingOverride: RegOpenKeyExA failed (%ld).", openResult);
        return DD_RESULT_DD_GENERIC_UNAVAILABLE;
    }

    LONG regResult = ERROR_SUCCESS;

    // DD_SETTINGS_TYPE numeric values match dd_settings_api.h ordering.
    // We use raw uint64 bits for all numeric types and reinterpret as needed.
    switch (pParams->type)
    {
    case DD_SETTINGS_TYPE_BOOL:
    {
        if ((pParams->numericValue != 0) && (pParams->numericValue != 1))
        {
            RegCloseKey(hkey);
            m_logger.Error("SiphonModule | WriteKernelSettingOverride: bool value must be 0 or 1, got %llu.", pParams->numericValue);
            return DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
        }
        const DWORD dw = static_cast<DWORD>(pParams->numericValue);
        regResult = RegSetValueExA(hkey, pSettingName, 0, REG_DWORD,
                                   reinterpret_cast<const BYTE*>(&dw), sizeof(DWORD));
        break;
    }
    case DD_SETTINGS_TYPE_UINT32:
    {
        const DWORD dw = static_cast<DWORD>(pParams->numericValue & 0xFFFFFFFF);
        regResult = RegSetValueExA(hkey, pSettingName, 0, REG_DWORD,
                                   reinterpret_cast<const BYTE*>(&dw), sizeof(DWORD));
        break;
    }
    case DD_SETTINGS_TYPE_UINT64:
    {
        regResult = RegSetValueExA(hkey, pSettingName, 0, REG_QWORD,
                                   reinterpret_cast<const BYTE*>(&pParams->numericValue), sizeof(ULONGLONG));
        break;
    }
    case DD_SETTINGS_TYPE_STRING:
    {
        if (pParams->stringValueSize == 0)
        {
            RegCloseKey(hkey);
            return DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
        }
        regResult = WriteStringRegistryValue(hkey, pSettingName, pStringValue, m_logger);
        break;
    }
    default:
        RegCloseKey(hkey);
        m_logger.Error("SiphonModule | WriteKernelSettingOverride: unsupported type %u.", pParams->type);
        return DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
    }

    RegCloseKey(hkey);
    result = (regResult == ERROR_SUCCESS) ? DD_RESULT_SUCCESS : DD_RESULT_DD_GENERIC_UNAVAILABLE;
    if (result != DD_RESULT_SUCCESS)
    {
        m_logger.Error("SiphonModule | WriteKernelSettingOverride: RegSetValueEx failed (%ld) for '%s'.",
                       regResult, pSettingName);
    }
#else
    DD_UNUSED(pParamBuffer);
    DD_UNUSED(paramBufferSize);
#endif
    return result;
}

// =======================================================================================
DD_RESULT ModuleConnectionContext::TriggerKernelPnpReload(
        const void* pParamBuffer,
        size_t      paramBufferSize)
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_UNAVAILABLE;
#ifdef _WIN32
    if (paramBufferSize < sizeof(DDGpuId))
    {
        m_logger.Error("SiphonModule | TriggerKernelPnpReload: param buffer too small: %zu.", paramBufferSize);
        return DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
    }

    // Unpack the GPU identifier into PCI bus/device/function fields for device matching.
    const DDGpuId gpuId = *reinterpret_cast<const DDGpuId*>(pParamBuffer);

    // Locate the device node via the cached PCI→registry-key map.  We need the DEVINST, so we
    // re-enumerate only for the matched GPU rather than the full device list.
    const auto it = m_kmdRegistryKeyByGpuId.find(gpuId);
    if (it == m_kmdRegistryKeyByGpuId.end())
    {
        m_logger.Error("SiphonModule | TriggerKernelPnpReload: no PCI device matched GPU 0x%08X.", gpuId);
        return DD_RESULT_DD_GENERIC_FILE_NOT_FOUND;
    }

    PciLocation targetPci = {};
    targetPci.u32All = gpuId;

    // Enumerate PCI devices to obtain the DEVINST for the matched GPU.
    const HDEVINFO deviceInfoSet = SetupDiGetClassDevsExA(
        nullptr, "PCI", nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES, nullptr, nullptr, nullptr);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        m_logger.Error("SiphonModule | TriggerKernelPnpReload: SetupDiGetClassDevsExA failed (%lu).", GetLastError());
        return DD_RESULT_DD_GENERIC_UNAVAILABLE;
    }

    bool            found = false;
    SP_DEVINFO_DATA deviceInfoData = {};
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); ++i)
    {
        uint32_t bus     = 0;
        uint32_t address = 0;

        if (!SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData, SPDRP_BUSNUMBER, nullptr,
                                               reinterpret_cast<PBYTE>(&bus), sizeof(bus), nullptr))
        {
            continue;
        }

        if (!SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData, SPDRP_ADDRESS, nullptr,
                                               reinterpret_cast<PBYTE>(&address), sizeof(address), nullptr))
        {
            continue;
        }

        // SPDRP_ADDRESS encodes (device << 16) | function for PCI devices.
        const uint32_t pciDevice   = (address >> 16) & 0xFF;
        const uint32_t pciFunction = address & 0xFF;

        if ((bus != targetPci.bits.bus) || (pciDevice != targetPci.bits.device) || (pciFunction != targetPci.bits.function))
        {
            continue;
        }

        // Found the proper device, perform the PnP disable/re-enable cycle to force the KMD to reinitialise
        // and pick up any registry overrides written since the last driver load.
        found = true;
        const DEVINST inst = deviceInfoData.DevInst;

        CONFIGRET cmResult = CM_Disable_DevNode(inst, 0);
        if (cmResult != CR_SUCCESS)
        {
            switch (cmResult)
            {
            case CR_ACCESS_DENIED:
                m_logger.Error("SiphonModule | TriggerKernelPnpReload: CM_Disable_DevNode access denied (CONFIGRET %u) — RDS must run as administrator.", cmResult);
                result = DD_RESULT_COMMON_ACCESS_DENIED;
                break;
            case CR_REMOVE_VETOED:
                // A driver or filter vetoed the disable. The override is already written to the
                // registry and may take effect if the driver resets via another path.
                m_logger.Error("SiphonModule | TriggerKernelPnpReload: CM_Disable_DevNode vetoed (CONFIGRET %u) — override written, reboot may be needed.", cmResult);
                result = DD_RESULT_COMMON_SUCCESS_WITH_ERRORS;
                break;
            case CR_NOT_DISABLEABLE:
                m_logger.Error("SiphonModule | TriggerKernelPnpReload: CM_Disable_DevNode not disableable (CONFIGRET %u) — reboot required for override to take effect.", cmResult);
                result = DD_RESULT_COMMON_UNSUPPORTED;
                break;
            case CR_NEED_RESTART:
                m_logger.Error("SiphonModule | TriggerKernelPnpReload: CM_Disable_DevNode requires restart (CONFIGRET %u).", cmResult);
                result = DD_RESULT_DD_GENERIC_NOT_READY;
                break;
            default:
                m_logger.Error("SiphonModule | TriggerKernelPnpReload: CM_Disable_DevNode failed (CONFIGRET %u).", cmResult);
                result = DD_RESULT_DD_GENERIC_UNAVAILABLE;
                break;
            }
            break;
        }

        cmResult = CM_Enable_DevNode(inst, 0);
        if (cmResult != CR_SUCCESS)
        {
            switch (cmResult)
            {
            case CR_ACCESS_DENIED:
                m_logger.Error("SiphonModule | TriggerKernelPnpReload: CM_Enable_DevNode access denied (CONFIGRET %u) — RDS must run as administrator.", cmResult);
                result = DD_RESULT_COMMON_ACCESS_DENIED;
                break;
            case CR_NEED_RESTART:
                m_logger.Error("SiphonModule | TriggerKernelPnpReload: CM_Enable_DevNode requires restart (CONFIGRET %u).", cmResult);
                result = DD_RESULT_DD_GENERIC_NOT_READY;
                break;
            default:
                m_logger.Error("SiphonModule | TriggerKernelPnpReload: CM_Enable_DevNode failed (CONFIGRET %u).", cmResult);
                result = DD_RESULT_DD_GENERIC_UNAVAILABLE;
                break;
            }
            break;
        }

        result = DD_RESULT_SUCCESS;
        break;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    if (!found)
    {
        // Device was in the cached map but not found during re-enumeration (e.g. unplugged since init).
        m_logger.Error("SiphonModule | TriggerKernelPnpReload: GPU 0x%08X not present during re-enumeration.", gpuId);
        result = DD_RESULT_DD_GENERIC_FILE_NOT_FOUND;
    }
#else
    DD_UNUSED(pParamBuffer);
    DD_UNUSED(paramBufferSize);
#endif
    return result;
}

// =======================================================================================
std::string ModuleConnectionContext::GetDefaultDriverPath(DD_SETTINGS_DRIVER_TYPE driverType)
{
    std::string outDxcpPath;

#ifdef _WIN32
    const char* pDriverName = nullptr;
    switch (driverType)
    {
        case DD_SETTINGS_DRIVER_TYPE_DX12:   pDriverName = "amdxc64.dll"; break;
        case DD_SETTINGS_DRIVER_TYPE_DX10:   pDriverName = "amdxx64.dll"; break;
        case DD_SETTINGS_DRIVER_TYPE_DX9:    pDriverName = "amdxn64.dll"; break;
        case DD_SETTINGS_DRIVER_TYPE_VULKAN: pDriverName = "amdvlk64.dll"; break;
        case DD_SETTINGS_DRIVER_TYPE_OPENGL: pDriverName = "atio6axx.dll"; break;
        default: DD_ASSERT_ALWAYS(); break;
    }

    std::string driverInstallPath = m_windowsDriverInstallDir;

    // Update the driverstore path from registry if applicable.
    if (pDriverName != nullptr)
    {
        const bool driverPathInRegistry =
            (std::find(g_RegistryPathDrivers.begin(), g_RegistryPathDrivers.end(), std::string_view(pDriverName)))
            != g_RegistryPathDrivers.end();

        if (driverPathInRegistry)
        {
            driverInstallPath = GetDriverInstallPathFromRegistry(pDriverName);
        }
    }

    if (!driverInstallPath.empty() && (pDriverName != nullptr))
    {
        size_t requiredSize = cwk_path_join(
            driverInstallPath.c_str(),
            pDriverName,
            nullptr,
            0);

        outDxcpPath.resize(requiredSize + 1);

        cwk_path_join(
            driverInstallPath.c_str(),
            pDriverName,
            outDxcpPath.data(),
            outDxcpPath.size());
    }
    else
    {
        m_logger.Error(
            "SiphonModule | Failed to get default driver path on Windows. DriverInstallDir: %s. DriverType: %d.",
            driverInstallPath.c_str(),
            driverType);
    }
#elif __ANDROID__
    (void)driverType;
#else
    if (driverType == DD_SETTINGS_DRIVER_TYPE_OPENGL)
    {
    }
#endif

    return outDxcpPath;
}

// =======================================================================================
// Validates a Vulkan manifest file path from untrusted input (environment variables)
// to prevent path traversal and other security issues (CWE-22: Improper Limitation of a Pathname).
//
// This function is specifically for validating VK_DRIVER_FILES / VK_ICD_FILENAMES environment
// variables used to locate Vulkan ICD manifest files.
//
// Note: The Vulkan specification intentionally allows flexible manifest paths for development
// and testing. This validation provides basic security checks while maintaining compatibility
// with the Vulkan ecosystem. See: https://github.com/KhronosGroup/Vulkan-Loader
//
// Returns true if the path passes basic security checks, false otherwise.
static bool ValidateVulkanManifestFilePath(const std::string& filePath)
{
    if (filePath.empty())
    {
        return false;
    }

    // Check file extension - must be .json (Vulkan manifest requirement)
    if (filePath.length() < 5 || filePath.substr(filePath.length() - 5) != ".json")
    {
        return false;
    }

    // Note: We intentionally do NOT restrict paths beyond basic sanity checks because:
    // 1. The official Vulkan Loader accepts any path from VK_DRIVER_FILES/VK_ICD_FILENAMES
    // 2. Vulkan developers need to test custom drivers from arbitrary locations (including UNC paths)
    // 3. The primary security mechanism is rejecting these environment variables when elevated
    // 4. The file will be parsed as JSON, limiting the attack surface
    // 5. Invalid paths will fail naturally at file open time

    return true;
}

// =======================================================================================
std::string ModuleConnectionContext::GetXglPathFromEnvVar()
{
    std::string outXglPath;

    const char kEnvVkDriverFiles[] = "VK_DRIVER_FILES";
    const char kEnvVkDriverFilesDeprecated[] = "VK_ICD_FILENAMES";

    // Use secure environment variable access to prevent malicious injection
    // when running with elevated privileges
    const char* manifestFilePathList = Platform::SecureGetEnv(kEnvVkDriverFiles);
    if (manifestFilePathList == nullptr)
    {
        manifestFilePathList = Platform::SecureGetEnv(kEnvVkDriverFilesDeprecated);
        if (manifestFilePathList == nullptr)
        {
            m_logger.Info(
                "SiphonModule | environment variable %s and %s not found.",
                kEnvVkDriverFiles,
                kEnvVkDriverFilesDeprecated);
        }
    }

    FILE* pManifestFile = nullptr;

    if (manifestFilePathList != nullptr)
    {
#ifdef _WIN32
        const char kDelimiter = ';';
        // MAX_PATH on Windows (without extended path support)
        constexpr size_t kMaxPathLength = 260;
#else
        const char kDelimiter = ':';
        // PATH_MAX on Linux/Unix
        constexpr size_t kMaxPathLength = 4096;
#endif

        // Find the first path in the list.
        // We only need to scan up to kMaxPathLength since we're extracting only the first path
        const size_t ManifestFilePathListLength = Platform::Strlen_s(manifestFilePathList, kMaxPathLength);
        size_t firstManifestFilePathLen = ManifestFilePathListLength;
        for (size_t i = 0; i < ManifestFilePathListLength; ++i)
        {
            char ch = manifestFilePathList[i];
            if ((ch == kDelimiter) || (ch == '\0'))
            {
                firstManifestFilePathLen = i;
                break;
            }
        }

        std::string manifestFilePath;

        if (firstManifestFilePathLen == 0)
        {
            m_logger.Error("SiphonModule | Failed to parse Vulkan manifest file list: %s", manifestFilePathList);
        }
        else
        {
            manifestFilePath = std::string(manifestFilePathList, firstManifestFilePathLen);
        }

        // Validate the Vulkan manifest file path to prevent path traversal and other security issues (CWE-22)
        if (ValidateVulkanManifestFilePath(manifestFilePath))
        {
            pManifestFile = Platform::Fopen_s(manifestFilePath.c_str(), "r");
            if (pManifestFile == nullptr)
            {
                m_logger.Error("SiphonModule | Vulkan manifest file not found at: %s.", manifestFilePath.c_str());
            }
        }
    }
#if defined(__linux__)
    else
    {
        // Try the default icd file for amd-pro driver.
        const char defaultIcdFilePath[] = "/etc/vulkan/icd.d/amd_icd64.json";
        pManifestFile = fopen(defaultIcdFilePath, "r");
    }
#endif

    if (pManifestFile != nullptr)
    {
        DD_RESULT result = DD_RESULT_SUCCESS;

        // Vulkan manifest file is of JSON format, but we can use YAML
        // parser to parse it.

        yaml_parser_t parser;
        yaml_document_t document;
        yaml_node_t* pRoot = nullptr;
        yaml_node_t* pLibPathNode = nullptr;

        int err = yaml_parser_initialize(&parser);
        if (err != 1)
        {
            result = DD_RESULT_DD_UNKNOWN;
            m_logger.Error("SiphonModule | Failed to init YAML parser for parsing Vulkan manifest file.");
        }
        else
        {
            yaml_parser_set_input_file(&parser, pManifestFile);
            err = yaml_parser_load(&parser, &document);
            if (err != 1)
            {
                result = DD_RESULT_DD_UNKNOWN;
                m_logger.Error(
                    "SiphonModule | YAML parser failed to load for parsing Vulkan manifest file.");
            }
        }

        if (result == DD_RESULT_SUCCESS)
        {
            pRoot = yaml_document_get_root_node(&document);
            if (pRoot == nullptr)
            {
                result = DD_RESULT_PARSING_INVALID_JSON;
                m_logger.Error(
                    "SiphonModule | Failed to get root node from Vulkan manifest file.");
            }
        }

        if (result == DD_RESULT_SUCCESS)
        {
            yaml_node_t* pIcdNode = YamlDocumentFindNodeByKey(&document, pRoot, "ICD");
            if (pIcdNode == nullptr)
            {
                result = DD_RESULT_PARSING_INVALID_JSON;
                m_logger.Error(
                    "SiphonModule | Failed to find \"ICD\" field in Vulkan manifest file.");
            }
            else
            {
                pLibPathNode = YamlDocumentFindNodeByKey(&document, pIcdNode, "library_path");
                if (pLibPathNode == nullptr)
                {
                    result = DD_RESULT_PARSING_INVALID_JSON;
                    m_logger.Error(
                        "SiphonModule | Failed to find \"library_path\" field in Vulkan manifest file.");
                }
                else if (pLibPathNode->type != YAML_SCALAR_NODE)
                {
                    result = DD_RESULT_PARSING_INVALID_JSON;
                    m_logger.Error(
                        "SiphonModule | Field \"library_path\" has invalid value in Vulkan manifest file.");
                }
            }
        }

        if (result == DD_RESULT_SUCCESS)
        {
            if (pLibPathNode->data.scalar.value != nullptr)
            {
                outXglPath = std::string(
                    (const char*)pLibPathNode->data.scalar.value,
                    pLibPathNode->data.scalar.length);
            }
            else
            {
                m_logger.Error(
                    "SiphonModule | Field \"library_path\" has null value in Vulkan manifest file.");
            }
        }

        fclose(pManifestFile);
    }

    return outXglPath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
std::string ModuleConnectionContext::GetDriverInstallPathFromRegistry(const char* pDriverName)
{
    // Initialize to the default driver path. This path will be used if registry key is unavailable.
    std::string driverPath = std::string("C:\\Windows\\System32\\");

    const std::string registryRoot = "SYSTEM\\CurrentControlSet\\Control\\Class\\";
    const std::string regKeyName   = driverRegKeys[std::string(pDriverName)];

    HDEVINFO hDevInfo = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_DISPLAY_ADAPTER,
                                             NULL,
                                             nullptr,
                                             DIGCF_DEVICEINTERFACE);

    if (hDevInfo != INVALID_HANDLE_VALUE)
    {
        SP_DEVINFO_DATA devInfo;
        devInfo.cbSize = sizeof(devInfo);

        // We enumerate devices until we find the device with the correct driverstore paths in the registry keys.
        for (uint32_t devIndex = 0; SetupDiEnumDeviceInfo(hDevInfo, devIndex, &devInfo); devIndex++)
        {
            BYTE devicesPresent = 0;
            DEVPROPTYPE type;
            BOOL success = SetupDiGetDevicePropertyW(hDevInfo,
                                                     &devInfo,
                                                     &DEVPKEY_Device_IsPresent,
                                                     &type,
                                                     &devicesPresent,
                                                     sizeof(devicesPresent),
                                                     NULL,
                                                     0);
            if (success)
            {
                // Ensure a device is present.
                if (devicesPresent > 0)
                {
                    ULONG IDSize;
                    CM_Get_Device_ID_Size(&IDSize, devInfo.DevInst, 0);

                    char  relativePath[2048] = {};
                    ULONG len                = sizeof(relativePath);
                    CM_Get_DevNode_Registry_PropertyA(devInfo.DevInst, CM_DRP_DRIVER, NULL, relativePath, &len, 0);

                    std::string registryPath = registryRoot + std::string(relativePath, len);

                    HKEY regKey;
                    LSTATUS status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, registryPath.c_str(), 0, KEY_QUERY_VALUE, &regKey);

                    if (status == ERROR_SUCCESS)
                    {
                        DWORD requiredSize = 2048U;
                        std::string registryValues(requiredSize, '\0'); // Stores the output registry value ie. the driverstore path.

                        status = RegGetValueA(regKey,
                                              NULL,
                                              regKeyName.c_str(),
                                              RRF_RT_ANY,
                                              NULL,
                                              registryValues.data(),
                                              &requiredSize);

                        if (status == ERROR_SUCCESS)
                        {
                            std::string_view currPath(registryValues.data());

                            const size_t lastSlash = currPath.find_last_of('\\');
                            if (lastSlash != std::string::npos)
                            {
                                // Stop iterating when the device with the correct driverstore paths is found.
                                driverPath = currPath.substr(0, lastSlash);
                                break;
                            }
                        }
                        else
                        {
                            m_logger.Warn("Unable to retrieve registry value (%s). RegGetValueA() failed. Status error: %ld.",
                                    regKeyName.c_str(), status);
                        }
                    }
                    else
                    {
                        m_logger.Warn("Unable to open registry key (%s). RegOpenKeyExA() failed. Status error: %ld.",
                                registryPath.c_str(), status);
                    }
                }
                else
                {
                    m_logger.Warn("SetupDiGetDevicePropertyW() returned no device.");
                }
            }
            else
            {
                m_logger.Warn("SetupDiGetDevicePropertyW() failed. Windows error: %ld.", GetLastError());
            }

        }
    }
    else
    {
        m_logger.Warn("Invalid device info returned. SetupDiGetClassDevsA() failed. Windows error: %ld.", GetLastError());
    }

    return driverPath;
}

// =======================================================================================
// Build a map from DDGpuId (packed PCI bus/device/function) to the HKLM CurrentControlSet
// registry subkey for that GPU's KMD class entry.  Called once during Initialize() so that
// WriteKernelSettingOverride and TriggerKernelPnpReload can look up the key in O(1).
void ModuleConnectionContext::InitKmdRegistryKeyMap()
{
    const HDEVINFO deviceInfoSet = SetupDiGetClassDevsExA(
        nullptr, "PCI", nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES, nullptr, nullptr, nullptr);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        m_logger.Error("SiphonModule | InitKmdRegistryKeyMap: SetupDiGetClassDevsExA failed (%lu).", GetLastError());
        return;
    }

    SP_DEVINFO_DATA deviceInfoData = {};
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); ++i)
    {
        uint32_t bus     = 0;
        uint32_t address = 0;

        if (!SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData, SPDRP_BUSNUMBER, nullptr,
                                               reinterpret_cast<PBYTE>(&bus), sizeof(bus), nullptr))
        {
            continue;
        }

        if (!SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData, SPDRP_ADDRESS, nullptr,
                                               reinterpret_cast<PBYTE>(&address), sizeof(address), nullptr))
        {
            continue;
        }

        // SPDRP_ADDRESS encodes (device << 16) | function for PCI devices.
        const uint32_t pciDevice   = (address >> 16) & 0xFF;
        const uint32_t pciFunction = address & 0xFF;

        char driverKey[MAX_PATH] = {};
        if (!SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData, SPDRP_DRIVER, nullptr,
                                               reinterpret_cast<PBYTE>(driverKey), MAX_PATH, nullptr))
        {
            continue;
        }

        PciLocation pci   = {};
        pci.bits.bus      = bus;
        pci.bits.device   = pciDevice;
        pci.bits.function = pciFunction;

        m_kmdRegistryKeyByGpuId[pci.u32All] =
            std::string("SYSTEM\\CurrentControlSet\\Control\\Class\\") + driverKey;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
}

// =======================================================================================
// Initialize a default driver install directory, queried from KMD.
void ModuleConnectionContext::InitDriverInstallDir()
{
    // Try QueryAdapterInfo first
    Vector<ddAmdAdapterInfo> adapters(Platform::GenericAllocCb);
    DD_RESULT result = DevDriverToDDResult(QueryAdapterInfo(adapters));

    if ((result == DD_RESULT_SUCCESS) && (adapters.Size() > 0))
    {
        // Default to use the path to the driver of the first found GPU.
        m_windowsDriverInstallDir = adapters[0].driverInstallDir;
    }
}
#endif

// =======================================================================================
DD_RESULT ModuleConnectionContext::LoadSettingsBlobFromDefaultPath(
    DD_SETTINGS_DRIVER_TYPE driverType)
{
    DD_RESULT result = DD_RESULT_COMMON_DOES_NOT_EXIST;
    DD_ASSERT(driverType < DD_SETTINGS_DRIVER_TYPE_COUNT);

    SettingsBlob& driverSettingsBlob = m_settingsBlobs[driverType];

    if (driverType == DD_SETTINGS_DRIVER_TYPE_VULKAN)
    {
        driverSettingsBlob.defaultDllPath = GetXglPathFromEnvVar();
    }

    if (driverSettingsBlob.defaultDllPath.empty())
    {
#if defined(_WIN32)
        driverSettingsBlob.defaultDllPath = GetDefaultDriverPath(driverType);
#elif defined(__ANDROID__)
        // TODO: get path on android
        result = DD_RESULT_COMMON_UNSUPPORTED;
#else
        // TODO: get path on linux
#endif
    }

    if (!driverSettingsBlob.defaultDllPath.empty())
    {
        result = LoadSettingsBlobs(driverSettingsBlob.defaultDllPath.c_str(),
                                   driverSettingsBlob.blob);
        if (result == DD_RESULT_SUCCESS)
        {
            driverSettingsBlob.currentDllPath = driverSettingsBlob.defaultDllPath;
        }
    }

    return result;
}

// =======================================================================================
DD_RESULT ModuleConnectionContext::LoadSettingsBlobsFromAllDrivers()
{
    DD_RESULT result = DD_RESULT_SUCCESS;
    size_t    loadedCount = 0;

#ifdef _WIN32
    if (LoadSettingsBlobFromDefaultPath(DD_SETTINGS_DRIVER_TYPE_DX12) == DD_RESULT_SUCCESS)
    {
        loadedCount += 1;
    }
    else
    {
        m_logger.Error("SiphonModule | Failed to load DX12 settings blobs.");
    }

    if (LoadSettingsBlobFromDefaultPath(DD_SETTINGS_DRIVER_TYPE_DX10) == DD_RESULT_SUCCESS)
    {
        loadedCount += 1;
    }
    else
    {
        m_logger.Error("SiphonModule | Failed to load DX10 settings blobs.");
    }

    if (LoadSettingsBlobFromDefaultPath(DD_SETTINGS_DRIVER_TYPE_DX9) == DD_RESULT_SUCCESS)
    {
        loadedCount += 1;
    }
    else
    {
        m_logger.Error("SiphonModule | Failed to load DX9 settings blobs.");
    }
#endif

    // Xgl is cross-platform. Don't need to guard it behind __linux__.
    if (LoadSettingsBlobFromDefaultPath(DD_SETTINGS_DRIVER_TYPE_VULKAN) == DD_RESULT_SUCCESS)
    {
        loadedCount += 1;
    }
    else
    {
        m_logger.Error("SiphonModule | Failed to load Vulkan settings blobs.");
    }

    // OpenGL is also cross-platform
    if (LoadSettingsBlobFromDefaultPath(DD_SETTINGS_DRIVER_TYPE_OPENGL) == DD_RESULT_SUCCESS)
    {
        loadedCount += 1;
    }
    else
    {
        m_logger.Error("SiphonModule | Failed to load OpenGL settings blobs.");
    }

    // Return success as long as at least one loading succeeded.
    if (0 == loadedCount)
    {
        result = DD_RESULT_DD_GENERIC_FILE_NOT_FOUND;
    }

    return result;
}

// =======================================================================================
DD_RESULT ModuleConnectionContext::RegisterRpcService()
{
    DD_RESULT result = DriverSiphon::RegisterService(m_createInfo.hRpcServer, this);
    if (result != DD_RESULT_SUCCESS)
    {
        m_logger.Error("SiphonModule | Initialize | Register RPC service failed.");
    }

    return result;
}

// =======================================================================================
DD_RESULT ModuleConnectionContext::LoadSettingsBlobs(
    const char*           driverPath,
    std::vector<uint8_t>& settingsBlobs)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    PFN_GetSettingsBlobsAll pfnGetSettingsBlobsAll = nullptr;

    Platform::Library driverLib;
    result = DevDriverToDDResult(driverLib.Load(driverPath,
        Platform::LibrarySearchPaths::DllLoadDir | Platform::LibrarySearchPaths::System));
    if (result == DD_RESULT_SUCCESS)
    {
        if (driverLib.GetFunction("GetSettingsBlobsAll", &pfnGetSettingsBlobsAll) == false)
        {
            result = DD_RESULT_COMMON_INTERFACE_NOT_FOUND;
            m_logger.Error(
                "[SiphonModule] Failed to retrieve function pointer to GetSettingsBlobsAll from driver: %s.",
                driverPath);
        }
    }
    else
    {
        m_logger.Error("SiphonModule | Failed to load driver lib.");
    }

    if (result == DD_RESULT_SUCCESS)
    {
        size_t requiredSize = pfnGetSettingsBlobsAll(nullptr, 0);
        if (requiredSize > 0)
        {
            m_logger.Info(
                "[SiphonModule] Successfully fetched settings blobs from driver at path: %s.",
                driverPath);

            settingsBlobs.resize(requiredSize);
            requiredSize = pfnGetSettingsBlobsAll(settingsBlobs.data(), settingsBlobs.size());
            DD_ASSERT(requiredSize <= settingsBlobs.size());

            result = DecodePrivateSettingsBlobs(settingsBlobs.data(), settingsBlobs.size());
        }
        else
        {
            m_logger.Error("[SiphonModule] Settings blobs size is 0 from driver: %s.", driverPath);
        }
    }

    return result;
}

// =======================================================================================
DD_RESULT ModuleConnectionContext::WriteQuerySettingsBlobsAllResult(
    const DDByteWriter&    writer,
    const std::string&     driverPath,
    std::vector<uint8_t>&  settingsBlobs)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    DD_ASSERT((driverPath.size() + 1) <= UINT16_MAX);
    uint16_t driverPathSize = static_cast<uint16_t>(driverPath.size() + 1);

    size_t totalWriteSize = sizeof(driverPathSize) + driverPathSize;
    if (settingsBlobs.size() > 0)
    {
        totalWriteSize += settingsBlobs.size();
    }
    else
    {
        totalWriteSize += sizeof(SettingsBlobsAll);
    }

    result = writer.pfnBegin(writer.pUserdata, &totalWriteSize);
    if (result == DD_RESULT_SUCCESS)
    {
        result = writer.pfnWriteBytes(writer.pUserdata, &driverPathSize, sizeof(driverPathSize));
    }

    if (result == DD_RESULT_SUCCESS)
    {
        result = writer.pfnWriteBytes(writer.pUserdata, driverPath.c_str(), driverPathSize);
    }

    if (result == DD_RESULT_SUCCESS)
    {
        if (settingsBlobs.size() > 0)
        {
            result = writer.pfnWriteBytes(writer.pUserdata, settingsBlobs.data(), settingsBlobs.size());
        }
        else
        {
            SettingsBlobsAll blobsHeader {};
            blobsHeader.version = 1;
            blobsHeader.nblobs = 0;

            result = writer.pfnWriteBytes(writer.pUserdata, &blobsHeader, sizeof(blobsHeader));
        }
    }

    writer.pfnEnd(writer.pUserdata, result);

    return result;
}

// =======================================================================================
DD_RESULT ModuleConnectionContext::WriteQuerySettingsBlobsAllResultWithReload(
    const DDByteWriter&     writer,
    const char*             pDriverPathOverride,
    size_t                  driverPathOverrideSize,
    DD_SETTINGS_DRIVER_TYPE driverType,
    bool                    reload)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    SettingsBlob& settingsBlob = m_settingsBlobs[driverType];

    std::string pathToLoadFrom = settingsBlob.defaultDllPath;
    if (driverPathOverrideSize > 0)
    {
        std::string_view driverPathOverride = std::string_view(pDriverPathOverride, driverPathOverrideSize - 1);
        pathToLoadFrom = driverPathOverride;
    }

    reload |= settingsBlob.currentDllPath != pathToLoadFrom;

    if (reload)
    {
        result = LoadSettingsBlobs(pathToLoadFrom.c_str(), settingsBlob.blob);
        if (result == DD_RESULT_SUCCESS)
        {
            if (pathToLoadFrom != settingsBlob.currentDllPath)
            {
                settingsBlob.currentDllPath = pathToLoadFrom;
            }
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        result = WriteQuerySettingsBlobsAllResult(
            writer,
            settingsBlob.currentDllPath,
            settingsBlob.blob);
    }

    return result;
}

DD_RESULT ModuleConnectionContext::DecodePrivateSettingsBlobs(uint8_t* pBlobsAll, size_t blobsAllSize)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    // Settings blobs in open source distributions are not supposed to be encoded, so do nothing.
    DD_UNUSED(pBlobsAll);
    DD_UNUSED(blobsAllSize);

    return result;
}

} // namespace SiphonModule
