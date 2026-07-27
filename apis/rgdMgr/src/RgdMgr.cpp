/* Copyright (C) 2025-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "RgdMgr.h"
#include <ddCommon.h>
#include <dd_settings_api.h>
#include <dd_driver_utils_api.h>
#include <dd_gpu_detective_api.h>
#include <dd_router_utils_api.h>
#include <g_RouterUtilsModuleStatic.h>
#include <g_SystemTraceModuleStatic.h>
#include <dd_result.h>
#include <win/ddWinKmIoCtlDevice.h>
#include <dd_amd_log_utils_api.h>
#include <ddRgdMonitoringTypes.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sddl.h>
#include <iostream>

#ifdef _WIN32
#include <newdev.h>
#include <SetupAPI.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <ntddvdeo.h>
#include <devpkey.h>
#include <set>
#endif

namespace DevDriver
{

constexpr const char kRgdToolId[] = "RgdMgr";
constexpr const char kOcaGlobalConfigRegKey[] = "KMD_RgdGlobalConfigMode";
constexpr const char kOcaEcaConfigRegKey[] = "KMD_RgdHcaMode";
constexpr const char kRgdDataReadyEventName[] = "Global\\RgdDataReadyEvent";

// OCA Global Configuration Bitmask Values
enum OcaGlobalConfigFlags : uint32_t
{
    PfResetAction = 0x01,
    StablePstate  = 0x02,
    StallOnFault  = 0x04
};

// OCA Enhanced Crash Analysis config Bitmask Values
enum OcaEcaConfigFlags : uint32_t
{
    CaptureWaveData   = 0x01,
    CaptureVGPRData   = 0x02,
    CaptureSGPRData   = 0x04,
    SingleMemOp       = 0x08,
    SingleAluOp       = 0x10
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void* StdRealloc(DDAllocatorInstance* pInstance, void* pMemory, size_t oldSize, size_t newSize)
{
    DD_UNUSED(pInstance);
    DD_UNUSED(oldSize);

    return std::realloc(pMemory, newSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void StdFree(DDAllocatorInstance* pInstance, void* pMemory, size_t size)
{
    DD_UNUSED(pInstance);
    DD_UNUSED(size);

    std::free(pMemory);
}

static const DDAllocator kDDAllocator = { nullptr, StdRealloc, StdFree };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void CreateSecurityAttributes(SECURITY_DESCRIPTOR* pSecurityDescriptor, SECURITY_ATTRIBUTES* pSecurityAttributes)
{
    // Initialize the security descriptor
    InitializeSecurityDescriptor(pSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);

    // Set a NULL DACL, which allows full access to everyone
    // This is necessary for cross-process/service communication
    SetSecurityDescriptorDacl(pSecurityDescriptor, TRUE, NULL, FALSE);

    // Initialize security attributes
    pSecurityAttributes->nLength = sizeof(SECURITY_ATTRIBUTES);
    pSecurityAttributes->lpSecurityDescriptor = pSecurityDescriptor;
    pSecurityAttributes->bInheritHandle = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int FileRead(void* pUserData, const int64_t count, void* pBuffer, int64_t* pBytesRead)
{
    int   ret = rdfResult::rdfResultInvalidArgument;
    FILE* pFile = static_cast<FILE*>(pUserData);
    if (pFile != nullptr)
    {
        size_t bytesRead = fread(pBuffer, 1, count, pFile);
        if (pBytesRead)
        {
            *pBytesRead = bytesRead;
        }

        ret = (bytesRead == count) ? rdfResult::rdfResultOk : rdfResult::rdfResultError;
    }

    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int FileWrite(void* pUserData, const int64_t count, const void* pBuffer, int64_t* pBytesWritten)
{
    int   ret = rdfResult::rdfResultInvalidArgument;
    FILE* pFile = static_cast<FILE*>(pUserData);
    if (pFile)
    {
        size_t bytesWritten = fwrite(pBuffer, 1, count, pFile);
        if (pBytesWritten)
        {
            *pBytesWritten = bytesWritten;
        }
        ret = (bytesWritten == count) ? rdfResult::rdfResultOk : rdfResult::rdfResultError;
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int FileTell(void* pUserData, int64_t* pPosition)
{
    int   ret = rdfResult::rdfResultInvalidArgument;
    FILE* pFile = static_cast<FILE*>(pUserData);
    if ((pFile != nullptr) && (pPosition))
    {
        *pPosition = ftell(pFile);
        ret = (*pPosition == -1) ? rdfResult::rdfResultError : rdfResult::rdfResultOk;
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int FileSeek(void* pUserData, int64_t position)
{
    int   ret = rdfResult::rdfResultInvalidArgument;
    FILE* pFile = static_cast<FILE*>(pUserData);
    if (pFile != nullptr)
    {
        fseek(pFile, position, SEEK_SET);
        ret = rdfResult::rdfResultOk;
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int FileGetSize(void* pUserData, int64_t* pSize)
{
    int   ret = rdfResult::rdfResultInvalidArgument;
    FILE* pFile = static_cast<FILE*>(pUserData);
    if ((pFile != nullptr) && (pSize != nullptr))
    {
        // 1. Save the current location
        // 2. Jump to the end to get the size.
        // 3. Return back to the original location
        int64_t currentPos = ftell(pFile);
        fseek(pFile, 0, SEEK_END);
        *pSize = ftell(pFile);
        fseek(pFile, currentPos, SEEK_SET);
        ret = rdfResult::rdfResultOk;
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static DD_RESULT WriteHeartbeat(void* pUserdata, DD_RESULT result, DD_IO_STATUS status, size_t bytes)
{
    if (DD_IO_STATUS_END == status)
    {
        // Log a message if we have a logger
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool ShouldClientBeIgnored(void* pUserdata, const DDConnectionInfo* pConnectionInfo)
{
    RgdMgr* pRgd = static_cast<RgdMgr*>(pUserdata);
    bool ret = true;

    if (pRgd)
    {
        if (pConnectionInfo->pProcessName == pRgd->GetAppName())
        {
            ret = false;
        }
    }

    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void OnDriverStateChangedCb(DDConnectionCallbacksImpl* pImpl, DDConnectionId umdConnectionId, DD_DRIVER_STATE state)
{
    RgdMgr* pRgd = reinterpret_cast<RgdMgr*>(pImpl);
    if (pRgd)
    {
        pRgd->OnDriverStateChangedImpl(umdConnectionId, state);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void OnDriverConnectedCb(DDConnectionCallbacksImpl* pImpl, const DDConnectionInfo* pConnInfo)
{
    RgdMgr* pRgd = reinterpret_cast<RgdMgr*>(pImpl);
    if (pRgd)
    {
        pRgd->OnDriverConnectedImpl(pConnInfo);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void OnDriverDisconnectedCb(DDConnectionCallbacksImpl* pImpl, DDConnectionId umdConnectionId)
{
    RgdMgr* pRgd = reinterpret_cast<RgdMgr*>(pImpl);
    if (pRgd)
    {
        pRgd->OnDriverDisconnectedImpl(umdConnectionId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RgdMgr::TraceCollectionThreadProc(DDConnectionId umdConnectionId, bool reachedPostDeviceInit)
{
    DynamicBufferByteWriter dbbWriter        = DynamicBufferByteWriter();
    constexpr uint32_t      TimeoutInMs      = 1000;
    constexpr uint32_t      PollIntervalInMs = 200;

    DD_RESULT result = DD_RESULT_UNKNOWN;

    // Poll for trace data until it's ready or cancel is requested.
    while ((result != DD_RESULT_SUCCESS) && (m_cancelTraceCollection == false))
    {
        Sleep(PollIntervalInMs);
        result = m_pUberTraceApi->CollectTrace(m_pUberTraceApi->pInstance,
                                               umdConnectionId,
                                               TimeoutInMs,
                                               dbbWriter.Writer());
    }

    if (result == DD_RESULT_SUCCESS)
    {
        // Trace collection succeeded - process the data
        m_umdTraceBuffer = dbbWriter.Take();

        // End tracing and transfer crash data if detected
        result = m_pGpuDetectiveApi->EndTracing(m_pGpuDetectiveApi->pInstance,
                                                umdConnectionId,
                                                reachedPostDeviceInit,
                                                &m_crashDetected);

        // In this thread, we only update m_outputInfo in case of trace-success. The failure cases are handled in
        // DumpTraceData() which also forwards m_outputInfo to AmdLog/KMD.
        if (m_crashDetected && (result == DD_RESULT_SUCCESS))
        {
            result = m_pGpuDetectiveApi->TransferTraceData(m_pGpuDetectiveApi->pInstance,
                                                           umdConnectionId,
                                                           &m_rdfFileWriter,
                                                           &m_heartbeat);

            // Write the additional data chunks from Ubertrace and any other related data sources.
            WriteAdditionalChunks();

            if (result == DD_RESULT_SUCCESS)
            {
                m_rgdState = RgdStateDisconnectedTraceCaptured;
                snprintf(m_outputInfo.RgdFilePath, sizeof(m_outputInfo.RgdFilePath), "%s", m_outputFile.c_str());
                m_outputInfo.state = m_rgdState;
                CloseTraceFile();
            }
        }
    }

    // Signal AmdLog that RGD data is ready
    if (m_hRgdDataReadyEvent != NULL)
    {
        SetEvent(m_hRgdDataReadyEvent);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RgdMgr::CleanupTraceCollectionThread()
{
    // Ensure either OnDriverDisconnected or EndMonitoring performs cleanup at a time
    std::lock_guard<std::mutex> lock(m_traceCollectionMutex);

    // Signal cancellation and wait for thread to finish
    m_cancelTraceCollection = true;

    if (m_traceCollectionThread.joinable())
    {
        m_traceCollectionThread.join();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Create a temp file to write the data to:
// Note:
//     This temp file needs to be cleaned up explicitly as it will not be automatically deleted at close.
//     We can't just use tempfile() because that will only give a handle to the file, we need the name.
static std::string BuildOutputFilePath()
{
    std::string outputFile = "";
    char systemDir[MAX_PATH];
    UINT size = GetSystemDirectory(systemDir, MAX_PATH);

    if (size > 0)
    {
        std::time_t       t   = std::time(nullptr);
        std::tm           now = {};
        errno_t           err = localtime_s(&now, &t); // Use thread-safe version

        if (err == 0)
        {
            std::stringstream ss;
            ss << std::put_time(&now, "%Y%m%d_%H%M%S");
            outputFile = std::string(systemDir) + "\\drivers\\DriverData\\AMD\\" + ss.str() + ".rgd";
        }
        else
        {
            outputFile = std::string(systemDir) + "\\drivers\\DriverData\\AMD\\trace.rgd";
        }
    }

    return outputFile;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Create a file with secure access permissions using Windows API
// to comply with FIO06-C: Create files with appropriate access permissions
//
// The file is created with restricted access:
// - Creator/Owner has full access
// - SYSTEM account has full access (required for kernel driver access)
// - No handle inheritance to child processes
// - No file sharing during creation
//
// Returns: FILE* on success, nullptr on failure
static FILE* CreateSecureFile(const char* pFilePath)
{
    if (pFilePath == nullptr)
    {
        return nullptr;
    }

    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;  // Prevent handle inheritance to child processes

    FILE* pFile = nullptr;

    // Create file with default security but restricted sharing and non-inheritable handle
    // This addresses FIO06-C by:
    // 1. Using explicit CreateFileA instead of fopen (controlled permissions)
    // 2. No file sharing during creation (dwShareMode = 0)
    // 3. Non-inheritable handle (bInheritHandle = FALSE)
    // 4. Default Windows ACL provides appropriate user/SYSTEM access
    //
    // Note: Attempts to use restrictive SDDL strings like "D:(A;;GA;;;CO)(A;;GA;;;SY)"
    // or "D:(A;;GA;;;OW)(A;;GA;;;SY)" fail with ACCESS_DENIED because CO/OW don't
    // properly resolve during file creation. Default security provides adequate protection.
    HANDLE hFile = CreateFileA(
        pFilePath,
        GENERIC_READ | GENERIC_WRITE,
        0,  // No sharing - exclusive access during creation
        &sa,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile != INVALID_HANDLE_VALUE)
    {
        // Redundantly ensure handle inheritance is disabled
        SetHandleInformation(hFile, HANDLE_FLAG_INHERIT, 0);

        // Convert Windows handle to C file descriptor, then to FILE*
        int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hFile), _O_BINARY | _O_RDWR);
        if (fd != -1)
        {
            pFile = _fdopen(fd, "wb+");
            if (pFile == nullptr)
            {
                // If _fdopen fails, close the file descriptor
                // This also closes the underlying Windows HANDLE
                _close(fd);
            }
        }
        else
        {
            // If _open_osfhandle fails, close the Windows handle
            CloseHandle(hFile);
        }
    }

    return pFile;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check the registry for the modes for high overhead and enhanced crash analysis. These modes are expected to be set
// by the user before running the app.
static void GetOcaBitmasks(std::unordered_map<std::string, uint32_t>& ocaBitmasks)
{
    const std::string registryRoot = "SYSTEM\\CurrentControlSet\\Control\\Class\\";

    HDEVINFO hDevInfo = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_DISPLAY_ADAPTER,
                                             NULL,
                                             nullptr,
                                             DIGCF_DEVICEINTERFACE);

    if (hDevInfo != INVALID_HANDLE_VALUE)
    {
        SP_DEVINFO_DATA devInfo;
        devInfo.cbSize = sizeof(devInfo);

        // We enumerate devices until we find the device with the registry keys.
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
                        for (auto& regKeyPair : ocaBitmasks)
                        {
                            DWORD requiredSize = sizeof(DWORD32);
                            DWORD regOverheadVal = 0;

                            status = RegGetValueA(regKey,
                                                NULL,
                                                regKeyPair.first.c_str(),
                                                RRF_RT_ANY,
                                                NULL,
                                                &regOverheadVal,
                                                &requiredSize);

                            if (status == ERROR_SUCCESS)
                            {
                                regKeyPair.second = regOverheadVal;
                            }
                        }
                    }

                    RegCloseKey(regKey);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RgdMgr::InitDevDriver()
{
    // We shouldn't already be init
    if (m_devDriverInit)
    {
        return DD_RESULT_DD_GENERIC_CONNECTION_EXITS;
    }

    // Create Router
    DD_RESULT result = CreateRouter(&m_router);

    // Create tool API
    if (result == DD_RESULT_SUCCESS)
    {
        result = CreateToolApi(&m_pToolApi);
    }

    if (result == DD_RESULT_SUCCESS)
    {
        result = InitApis();
    }

    if (result == DD_RESULT_SUCCESS)
    {
        m_pIoCtlDevice = DD_NEW(WinKmIoCtlDevice, Platform::GenericAllocCb);

        if (m_pIoCtlDevice != nullptr)
        {
            Result r = m_pIoCtlDevice->Initialize();
            if (r != Result::Success)
            {
                DD_DELETE(m_pIoCtlDevice, Platform::GenericAllocCb);
                m_pIoCtlDevice = nullptr;
            }
            result = DevDriverToDDResult(r);
        }
        else
        {
            result = DevDriverToDDResult(Result::InsufficientMemory);
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        m_devDriverInit = true;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RgdMgr::ShutdownDevDriver()
{
    // If we started monitoring we should end it before cleanup as this can cause crashes sometimes
    DD_ASSERT(m_monitorStarted == false);

    // Disable the crash analysis feature flag first since it requires the DriverUtils API
    SetCrashAnalysisFeatureFlag(false);

    if (m_pToolApi)
    {
        m_pToolApi->Disconnect(m_pToolApi->pInstance);
    }

    if (m_pIoCtlDevice)
    {
        m_pIoCtlDevice->Destroy();
        DD_DELETE(m_pIoCtlDevice, Platform::GenericAllocCb);
        m_pIoCtlDevice = nullptr;
    }

    if (m_router != DD_API_INVALID_HANDLE)
    {
        ddRouterDestroy(m_router);
    }

    if (m_pToolApi)
    {
        DDToolApiDestroy(&m_pToolApi);
        m_pToolApi = nullptr;
    }

    m_devDriverInit         = false;
    m_reachedPostDeviceInit = false;
    m_pid                   = 0;
    m_umdConnectionId       = 0;
    m_rgdState              = RgdStateMonitoringNotEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RgdMgr::LoadOcaCaptureConfig(uint32_t options)
{
    DD_RESULT result = DD_RESULT_SUCCESS;
    m_ocaCaptureConfig = {};

    // Explicitly set the default states for clarity. We want to ECA to be enabled by default for OCA captures.
    m_ocaCaptureConfig.enablePfResetAction = false;
    m_ocaCaptureConfig.enableStablePstate  = false;
    m_ocaCaptureConfig.enableStallOnFault  = false;
    m_ocaCaptureConfig.captureWaveData     = true;
    m_ocaCaptureConfig.captureVGPRData     = true;
    m_ocaCaptureConfig.captureSGPRData     = true;
    m_ocaCaptureConfig.enableSingleMemOp   = true;
    m_ocaCaptureConfig.enableSingleAluOp   = true;

    // If options are provided (non-zero), use them to configure the capture settings
    if (options != 0)
    {
        // Map RgdGpuDumpOptions to OcaCaptureConfig

        // These don't matter for this path:
        m_ocaCaptureConfig.enablePfResetAction = false;
        m_ocaCaptureConfig.enableStablePstate  = false;
        m_ocaCaptureConfig.enableStallOnFault  = false;

        if (options & RgdGpuDumpOptionHighOverhead)
        {
            m_ocaCaptureConfig.enableSingleMemOp = true;
            m_ocaCaptureConfig.enableSingleAluOp = true;
            m_ocaCaptureConfig.captureWaveData   = true;
            m_ocaCaptureConfig.captureVGPRData   = true;
            m_ocaCaptureConfig.captureSGPRData   = true;
            result = DD_RESULT_SUCCESS;
        }
        else
        {
            // For all other modes, just return invalid
            result = DD_RESULT_COMMON_INVALID_PARAMETER;
        }
    }
    else
    {
        // No options provided, fall back to registry settings
        // Get the global configuration and ECA config bitmasks from registry.
        m_OcaBitmasks[kOcaGlobalConfigRegKey] = UINT32_MAX;
        m_OcaBitmasks[kOcaEcaConfigRegKey]    = UINT32_MAX;
        GetOcaBitmasks(m_OcaBitmasks);

        if (m_OcaBitmasks[kOcaGlobalConfigRegKey] != UINT32_MAX)
        {
            // Update global configs
            m_ocaCaptureConfig.enablePfResetAction
                = (m_OcaBitmasks[kOcaGlobalConfigRegKey] & OcaGlobalConfigFlags::PfResetAction) != 0;
            m_ocaCaptureConfig.enableStablePstate
                = (m_OcaBitmasks[kOcaGlobalConfigRegKey] & OcaGlobalConfigFlags::StablePstate) != 0;
            m_ocaCaptureConfig.enableStallOnFault
                = (m_OcaBitmasks[kOcaGlobalConfigRegKey] & OcaGlobalConfigFlags::StallOnFault) != 0;
        }

        // Update enhanced crash analysis configs if explicitly set via reg key.
        // CaptureWaveData is basically enabling ECA mode. If any of the other bits are set, it should be automatically
        // enabled.
        if (m_OcaBitmasks[kOcaEcaConfigRegKey] != UINT32_MAX)
        {
            m_ocaCaptureConfig.captureWaveData   = (m_OcaBitmasks[kOcaEcaConfigRegKey] > 0);
            m_ocaCaptureConfig.captureVGPRData   = (m_OcaBitmasks[kOcaEcaConfigRegKey] & OcaEcaConfigFlags::CaptureVGPRData) != 0;
            m_ocaCaptureConfig.captureSGPRData   = (m_OcaBitmasks[kOcaEcaConfigRegKey] & OcaEcaConfigFlags::CaptureSGPRData) != 0;
            m_ocaCaptureConfig.enableSingleMemOp = (m_OcaBitmasks[kOcaEcaConfigRegKey] & OcaEcaConfigFlags::SingleMemOp) != 0;
            m_ocaCaptureConfig.enableSingleAluOp = (m_OcaBitmasks[kOcaEcaConfigRegKey] & OcaEcaConfigFlags::SingleAluOp) != 0;
        }

        result = DD_RESULT_SUCCESS;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RgdMgr::MonitorApp(const std::string& appName, uint32_t options)
{
    DD_RESULT result = InitDevDriver();

    if (result == DD_RESULT_SUCCESS)
    {
        result = LoadOcaCaptureConfig(options);
    }

    // Only allow one app to be monitor at a time
    if ((result == DD_RESULT_SUCCESS) && (m_monitorStarted == false))
    {
        // Create event object for driver disconnection signaling
        SECURITY_DESCRIPTOR stSecurityDescriptor = {};
        SECURITY_ATTRIBUTES stSecurityAttributes = {};

        CreateSecurityAttributes(&stSecurityDescriptor, &stSecurityAttributes);

        // Create or open a manual-reset, initially non-signaled, named global event
        // Manual-reset (TRUE) allows multiple waiters to be signaled
        if (m_hRgdDataReadyEvent == NULL)
        {
            m_hRgdDataReadyEvent = CreateEvent(&stSecurityAttributes, TRUE, FALSE, kRgdDataReadyEventName);
            if (m_hRgdDataReadyEvent == NULL)
            {
                result = DD_RESULT_COMMON_OUT_OF_HEAP_MEMORY;
            }
        }
        else
        {
            // Reset the event if it already exists
            ResetEvent(m_hRgdDataReadyEvent);
        }

        m_outputFile = BuildOutputFilePath();

        FILE* pFile = CreateSecureFile(m_outputFile.c_str());
        m_appName   = appName;

        if (pFile != nullptr)
        {
            m_rdfFileWriter.pUserData      = pFile;
            m_rdfFileWriter.pfnFileRead    = FileRead;
            m_rdfFileWriter.pfnFileWrite   = FileWrite;
            m_rdfFileWriter.pfnFileTell    = FileTell;
            m_rdfFileWriter.pfnFileSeek    = FileSeek;
            m_rdfFileWriter.pfnFileGetSize = FileGetSize;
            result                         = DD_RESULT_SUCCESS;
        }
        else
        {
            result = DD_RESULT_DD_GENERIC_FILE_ACCESS_ERROR;
        }

        m_heartbeat.pfnWriteHeartbeat = WriteHeartbeat;
        m_heartbeat.pUserdata         = nullptr;

        // Load connection callbacks now since the filter requires the app name to be set
        if (result == DD_RESULT_SUCCESS)
        {
            result = LoadConnectionCallbacks();
        }

        // Connect to the router
        if (result == DD_RESULT_SUCCESS)
        {
            result = m_pToolApi->Connect(m_pToolApi->pInstance, nullptr, 0);
        }

        if (result == DD_RESULT_SUCCESS)
        {
            result = SetCrashAnalysisFeatureFlag(true);
        }

        if (result == DD_RESULT_SUCCESS)
        {
            m_monitorStarted = true;
            m_rgdState       = RgdStateMonitoringEnabledNotLaunched;

            // Open and signal the event created by the driver to notify that RgdMgr monitoring is ready
            HANDLE hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, "Global\\DriverToolInitComplete");
            if (hEvent != nullptr)
            {
                SetEvent(hEvent);
                CloseHandle(hEvent);
            }
        }
    }
    else
    {
        result = DD_RESULT_DD_GENERIC_NOT_READY;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RgdMgr::CloseTraceFile()
{
    FILE* pFile = static_cast<FILE*>(m_rdfFileWriter.pUserData);
    if (pFile)
    {
        // fclose() automatically closes the underlying file descriptor and Windows HANDLE
        // that were created by CreateSecureFile(), so no special cleanup is needed
        fclose(pFile);
        m_rdfFileWriter.pUserData = nullptr;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RgdMgr::EndMonitoring(bool deleteFile)
{
    if (m_devDriverInit)
    {
        // The app filter will continue to be in place as there isn't an unset,
        // but setting the name to an empty string will make it so it will ignore all the apps
        m_appName = "";

        m_monitorStarted = false;
        m_rgdState       = RgdStateMonitoringNotEnabled;

        CloseTraceFile();

        // Clean up trace collection thread if still running
        CleanupTraceCollectionThread();

        // We only support monitoring a single app right now, so just shut down DevDriver when we are done.
        ShutdownDevDriver();
    }

    if (m_hRgdDataReadyEvent != NULL)
    {
        CloseHandle(m_hRgdDataReadyEvent);
        m_hRgdDataReadyEvent = NULL;
    }

    // We are now safe to delete the file since we have either sent it to the KMD or are shutting down.
    // For debugging purposes we may want to keep the file around so allow the caller to specify:
    if (deleteFile && (m_outputFile != ""))
    {
       std::remove(m_outputFile.c_str());
       m_outputFile = "";
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RgdMgr::OnDriverStateChangedImpl(DDConnectionId umdConnectionId, DD_DRIVER_STATE state)
{
    if (state == DD_DRIVER_STATE_PLATFORMINIT)
    {
        ForceDisableDriverOverlay(umdConnectionId);

        m_pGpuDetectiveApi->EnableTracing(m_pGpuDetectiveApi->pInstance, umdConnectionId, m_pid);
        m_rgdState = RgdStateEarlyConnection;

        // UberTrace setup
        DD_RESULT uresult = m_pUberTraceApi->Connect(m_pUberTraceApi->pInstance, umdConnectionId);
        if (uresult == DD_RESULT_SUCCESS)
        {
            m_pUberTraceApi->EnableTracing(m_pUberTraceApi->pInstance, umdConnectionId);
        }

        m_umdConnectionId = umdConnectionId;

        // Populate the ProcessInfoChunk
        char*  pRawProcessPath = nullptr;
        size_t processPathSize = 0;
        uresult = m_pRouterUtilsApi->QueryPathByProcessId(m_pRouterUtilsApi->pInstance, m_pid, kDDAllocator, &pRawProcessPath, &processPathSize);

        // Assign to m_processInfoChunk
        m_processInfoChunk.processId = m_pid;

        if (uresult == DD_RESULT_SUCCESS && pRawProcessPath != nullptr)
        {
            m_processInfoChunk.processPath = std::string(pRawProcessPath);
            kDDAllocator.Free(kDDAllocator.pInstance, pRawProcessPath, processPathSize);
        }
    }
    else if (state == DD_DRIVER_STATE_POSTDEVICEINIT)
    {
        m_reachedPostDeviceInit = true;
        m_rgdState              = RgdStateConnectionPostDeviceInit;
    }
    else if (state == DD_DRIVER_STATE_RUNNING)
    {
        const std::string ubertraceParams =
                "{\"controller\":{\"config\":{\"enabled\":true},\"name\":\"tdr\"},\"sources\":[{\"name\":\"codeobject\"}]}";

        DD_RESULT uresult = m_pUberTraceApi->ConfigureTraceParams(m_pUberTraceApi->pInstance,
                                                                  m_umdConnectionId,
                                                                  ubertraceParams.c_str(),
                                                                  ubertraceParams.length());
        if (uresult == DD_RESULT_SUCCESS)
        {
            uresult = m_pUberTraceApi->RequestTrace(m_pUberTraceApi->pInstance, m_umdConnectionId);

            if (uresult == DD_RESULT_SUCCESS)
            {
                // Capture connection ID/state to avoid race with disconnect
                m_cancelTraceCollection = false;
                m_traceCollectionThread = std::thread(&RgdMgr::TraceCollectionThreadProc,
                                                      this,
                                                      m_umdConnectionId,
                                                      m_reachedPostDeviceInit);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RgdMgr::OnDriverConnectedImpl(const DDConnectionInfo* pConnInfo)
{
    m_pid             = pConnInfo->processId;
    m_umdConnectionId = pConnInfo->umdConnectionId;
    m_dumpedTraceData = false;

    // Enable Enhanced Crash Info now that we have the PID
    EnableEnhancedCrashInfo();

    m_pAmdLogUtilsApi->SetOcaHighOverheadConfig(m_pAmdLogUtilsApi->pInstance,
                                                &m_ocaCaptureConfig,
                                                sizeof(m_ocaCaptureConfig));
}

// Extract and write the chunks obtained from UMD/PAL/Ubertrace flow into the RGD-RDF stream
DD_RESULT RgdMgr::WriteUbertraceChunks(
    rdfChunkFileWriter*   pChunkFileWriter,
    rdfChunkFile*         pFile,
    rdfChunkFileIterator* pIterator)
{
    int atEnd = 0;
    while (rdfChunkFileIteratorIsAtEnd(pIterator, &atEnd) == rdfResultOk && atEnd == 0)
    {
        rdfChunkCreateInfo createInfo = {};
        if (rdfChunkFileIteratorGetChunkIdentifier(pIterator, createInfo.identifier) != rdfResultOk)
        {
            return DD_RESULT_UNKNOWN;
        }

        int index;
        if (rdfChunkFileIteratorGetChunkIndex(pIterator, &index) != rdfResultOk)
        {
            return DD_RESULT_UNKNOWN;
        }

        int64_t dataSize;
        if (rdfChunkFileGetChunkDataSize(pFile, createInfo.identifier, index, &dataSize) != rdfResultOk)
        {
            return DD_RESULT_UNKNOWN;
        }

        std::vector<uint8_t> data(dataSize);
        if (rdfChunkFileReadChunkData(pFile, createInfo.identifier, index, data.data()) != rdfResultOk)
        {
            return DD_RESULT_UNKNOWN;
        }

        if (rdfChunkFileGetChunkHeaderSize(pFile, createInfo.identifier, index, &createInfo.headerSize) != rdfResultOk)
        {
            return DD_RESULT_UNKNOWN;
        }

        std::vector<uint8_t> headerData(dataSize);
        createInfo.pHeader = headerData.data();

        if (rdfChunkFileReadChunkHeader(pFile, createInfo.identifier, index, headerData.data()) != rdfResultOk)
        {
            return DD_RESULT_UNKNOWN;
        }

        if (rdfChunkFileGetChunkVersion(pFile, createInfo.identifier, index, &createInfo.version) != rdfResultOk)
        {
            return DD_RESULT_UNKNOWN;
        }

        int writtenIndex = 0;
        if (rdfChunkFileWriterWriteChunk(pChunkFileWriter, &createInfo, dataSize, data.data(), &writtenIndex) != rdfResultOk)
        {
            return DD_RESULT_UNKNOWN;
        }

        if (rdfChunkFileIteratorAdvance(pIterator) != rdfResultOk)
        {
            return DD_RESULT_UNKNOWN;
        }
    }

    return DD_RESULT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RgdMgr::WriteProcessInfoChunk(rdfChunkFileWriter* pChunkFileWriter)
{
    rdfChunkCreateInfo createInfo = {};
    createInfo.version    = 1;
    createInfo.headerSize = 0;
    createInfo.pHeader    = nullptr;

    static const std::string chunkId = "TraceProcessInfo";
    Platform::Memcpy_s(createInfo.identifier, RDF_IDENTIFIER_SIZE, chunkId.c_str(), chunkId.size());

    std::vector<uint8_t> byteVector;
    uint32_t             processPathSize = static_cast<uint32_t>(m_processInfoChunk.processPath.size() + 1);
    byteVector.resize(sizeof(uint32_t) * 2 + processPathSize);

    uint32_t* pData = reinterpret_cast<uint32_t*>(byteVector.data());
    *pData          = m_processInfoChunk.processId;
    *(pData + 1)    = processPathSize;

    Platform::Strncpy(reinterpret_cast<char*>(pData + 2), m_processInfoChunk.processPath.c_str(), processPathSize);

    int index = 0;
    if (rdfChunkFileWriterWriteChunk(pChunkFileWriter, &createInfo, byteVector.size(), byteVector.data(), &index) !=
        rdfResultOk)
    {
        return DD_RESULT_DD_GENERIC_UNKNOWN;
    }

    return DD_RESULT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Writes additional chunks from Ubertrace and other related data.
void RgdMgr::WriteAdditionalChunks()
{
    rdfUserStream rdfUserStreamEvents = {};
    rdfUserStreamEvents.context = m_rdfFileWriter.pUserData;
    rdfUserStreamEvents.Read    = m_rdfFileWriter.pfnFileRead;
    rdfUserStreamEvents.Write   = m_rdfFileWriter.pfnFileWrite;
    rdfUserStreamEvents.Tell    = m_rdfFileWriter.pfnFileTell;
    rdfUserStreamEvents.Seek    = m_rdfFileWriter.pfnFileSeek;
    rdfUserStreamEvents.GetSize = m_rdfFileWriter.pfnFileGetSize;

    rdfStream* pRdfTraceStream = nullptr;
    if (rdfStreamCreateFromUserStream(&rdfUserStreamEvents, &pRdfTraceStream) != rdfResultOk)
    {
        return;
    }

    rdfChunkFileWriterCreateInfo writerCreateInfo = {};
    writerCreateInfo.stream       = pRdfTraceStream;
    writerCreateInfo.appendToFile = true;

    rdfChunkFileWriter* pChunkFileWriter = nullptr;
    if (rdfChunkFileWriterCreate2(&writerCreateInfo, &pChunkFileWriter) != rdfResultOk)
    {
        rdfStreamClose(&pRdfTraceStream);
        return;
    }

    // Write the TraceProcessInfo chunk
    WriteProcessInfoChunk(pChunkFileWriter);

    // Begin Ubertrace chunk extraction and writing
    rdfStream* pUbertraceStream;
    if (rdfStreamFromReadOnlyMemory(m_umdTraceBuffer.Size(), m_umdTraceBuffer.Data(), &pUbertraceStream) != rdfResultOk)
    {
        return;
    }

    rdfChunkFile* pUbertraceChunkFile;
    if (rdfChunkFileOpenStream(pUbertraceStream, &pUbertraceChunkFile) != rdfResultOk)
    {
        rdfStreamClose(&pUbertraceStream);
        return;
    }

    rdfChunkFileIterator* pIterator;
    if (rdfChunkFileCreateChunkIterator(pUbertraceChunkFile, &pIterator) != rdfResultOk)
    {
        rdfChunkFileClose(&pUbertraceChunkFile);
        rdfStreamClose(&pUbertraceStream);
        return;
    }

    // Write the Ubertrace chunks into the RGD-RDF stream
    DD_RESULT writeResult = WriteUbertraceChunks(pChunkFileWriter, pUbertraceChunkFile, pIterator);

    // Clean up resources which are no longer needed.
    rdfChunkFileDestroyChunkIterator(&pIterator);
    rdfChunkFileClose(&pUbertraceChunkFile);
    rdfStreamClose(&pUbertraceStream);
    rdfChunkFileWriterDestroy(&pChunkFileWriter);

    if (writeResult != DD_RESULT_SUCCESS)
    {
        rdfStreamClose(&pRdfTraceStream);
        return;
    }

    rdfStreamSeek(pRdfTraceStream, 0);
    int64_t streamSize = 0;
    rdfStreamGetSize(pRdfTraceStream, &streamSize);

    m_traceData.Resize(streamSize);
    int64_t bytesRead = 0;
    if (rdfStreamRead(pRdfTraceStream, streamSize, m_traceData.Data(), &bytesRead) == rdfResultOk)
    {
        // Get the current file size before overwriting
        int64_t currentFileSize = 0;
        m_rdfFileWriter.pfnFileGetSize(m_rdfFileWriter.pUserData, &currentFileSize);

        // Only overwrite if the new data is larger (ie. contains additional chunks)
        if (m_traceData.Size() >= static_cast<size_t>(currentFileSize))
        {
            // Seek to beginning and overwrite the file with the combined trace data
            m_rdfFileWriter.pfnFileSeek(m_rdfFileWriter.pUserData, 0);

            int64_t bytesWritten = 0;
            m_rdfFileWriter.pfnFileWrite(m_rdfFileWriter.pUserData, m_traceData.Size(), m_traceData.Data(), &bytesWritten);
        }
    }

    rdfStreamClose(&pRdfTraceStream);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RgdMgr::DumpTraceData()
{
    if (m_dumpedTraceData)
    {
        return;
    }

    snprintf(m_outputInfo.AppName, sizeof(m_outputInfo.AppName), "%s", m_appName.c_str());

    DD_RESULT result = DD_RESULT_SUCCESS;

    if (m_crashDetected)
    {
        if (m_rgdState == RgdStateDisconnectedTraceCaptured)
        {
            // Send the RGD file path via AmdLogUtils API
            if (!m_outputFile.empty())
            {
                m_pAmdLogUtilsApi->SendRgdOcaConfig(m_pAmdLogUtilsApi->pInstance, &m_outputInfo, sizeof(m_outputInfo));
            }

            result = DevDriverToDDResult(m_pIoCtlDevice->IoCtl(DevDriver::DevDriverRgdOcaBuffered, sizeof(m_outputInfo), &m_outputInfo, sizeof(m_outputInfo), &m_outputInfo));
        }
        else
        {
            m_rgdState                  = RgdStateDisconnectedPostDeviceInitTraceError;
            m_outputInfo.state          = m_rgdState;
            m_outputInfo.RgdFilePath[0] = '\0'; // Force it to be empty since we failed to get the trace

            m_pAmdLogUtilsApi->SendRgdOcaConfig(m_pAmdLogUtilsApi->pInstance, &m_outputInfo, sizeof(m_outputInfo));
            result = DevDriverToDDResult(m_pIoCtlDevice->IoCtl(DevDriver::DevDriverRgdOcaBuffered, sizeof(m_outputInfo), &m_outputInfo, sizeof(m_outputInfo), &m_outputInfo));
        }
    }
    else
    {
        if (m_reachedPostDeviceInit == true)
        {
            m_rgdState                  = RgdStateDisconnectedPostDeviceInitNoCrash;
            m_outputInfo.state          = m_rgdState;
            m_outputInfo.RgdFilePath[0] = '\0'; // Force it to be empty since we failed to get the trace

            m_pAmdLogUtilsApi->SendRgdOcaConfig(m_pAmdLogUtilsApi->pInstance, &m_outputInfo, sizeof(m_outputInfo));

            result = DevDriverToDDResult(m_pIoCtlDevice->IoCtl(DevDriver::DevDriverRgdOcaBuffered, sizeof(m_outputInfo), &m_outputInfo, sizeof(m_outputInfo), &m_outputInfo));
        }
        else
        {
            m_rgdState = RgdStateDisconnectedEarlyNoCrash;
        }
    }

    m_dumpedTraceData = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RgdMgr::OnDriverDisconnectedImpl(DDConnectionId umdConnectionId)
{
    m_umdConnectionId = umdConnectionId;
    DumpTraceData();

    CleanupTraceCollectionThread();

    // End/disable tracing for this connection
    if (m_rgdState != RgdStateDisconnectedTraceCaptured)
    {
        bool crashDetectedDuringCleanup = false;
        m_pGpuDetectiveApi->EndTracing(m_pGpuDetectiveApi->pInstance,
                                       umdConnectionId,
                                       m_reachedPostDeviceInit,
                                       &crashDetectedDuringCleanup);
    }

    m_pGpuDetectiveApi->DisableTracing(m_pGpuDetectiveApi->pInstance, umdConnectionId);
    m_pUberTraceApi->Disconnect(m_pUberTraceApi->pInstance, umdConnectionId);

    // Only reset state if it still refers to this connection
    if (m_umdConnectionId == umdConnectionId)
    {
        m_reachedPostDeviceInit = false;
        m_pid = 0;
        m_umdConnectionId = 0;
        m_dumpedTraceData = false;
        m_crashDetected = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RgdMgr::CreateRouter(DDRouter* pOutRouter)
{
    DDRouterCreateInfo routerCreateInfo = {};
    routerCreateInfo.pDescription = kRgdToolId;
    routerCreateInfo.alloc = { ddApiDefaultAlloc, ddApiDefaultFree, nullptr };

    DDLoggerInfo quietLogger = {};
    quietLogger.pUserdata    = nullptr;
    quietLogger.pfnLog       = [](void*, const DDLogEvent*, const char*) {};
    quietLogger.pfnWillLog   = [](void*, const DDLogEvent*) { return 0; };
    quietLogger.pfnPush      = [](void*, const DDLogEvent*, const char*) {};
    quietLogger.pfnPop       = [](void*, const DDLogEvent*, const char*) {};

    routerCreateInfo.logger = quietLogger;

    DD_RESULT result = ddRouterCreate(&routerCreateInfo, pOutRouter);
    if (result == DD_RESULT_SUCCESS)
    {
        result = ddRouterLoadBuiltinModule(*pOutRouter, RouterUtilsQueryModule(), nullptr);
    }

    if (result == DD_RESULT_SUCCESS)
    {
        result = ddRouterLoadBuiltinModule(*pOutRouter, SystemTraceQueryModule(), nullptr);
    }

    // TODO: Add Siphon if we need to query the settings blobs

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RgdMgr::CreateToolApi(DDToolApi** ppOutToolApi)
{
    DDToolApiCreateInfo createInfo = {};
    createInfo.pDescription        = kRgdToolId;
    createInfo.descriptionSize     = sizeof(kRgdToolId);
    createInfo.pModulesDir         = nullptr;
    createInfo.moduleDirSize       = 0;
    createInfo.pLogFilePath        = nullptr;
    createInfo.logFilePathSize     = 0;

    return DDToolApiCreate(&createInfo, ppOutToolApi);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RgdMgr::LoadConnectionCallbacks()
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_UNAVAILABLE;

    if (m_pConnectionApi != nullptr)
    {
        DDConnectionFilter connectionFilter = {};
        connectionFilter.pUserData          = this;
        connectionFilter.filter             = &ShouldClientBeIgnored;
        m_pConnectionApi->SetConnectionFilter(m_pConnectionApi->pInstance, connectionFilter);

        m_connectionCbs                       = {};
        m_connectionCbs.pImpl                 = reinterpret_cast<DDConnectionCallbacksImpl*>(this);
        m_connectionCbs.OnDriverStateChanged  = &OnDriverStateChangedCb;
        m_connectionCbs.OnDriverConnected     = &OnDriverConnectedCb;
        m_connectionCbs.OnDriverDisconnected  = &OnDriverDisconnectedCb;
        result = m_pConnectionApi->AddConnectionCallbacks(m_pConnectionApi->pInstance, &m_connectionCbs);
    }

    return result;
}

#define INIT_API(APINAME, pApi)                                             \
    m_pApiRegistry->Get(m_pApiRegistry->pInstance,                          \
                        DD_##APINAME##_API_NAME,                            \
                        DDVersion{ DD_##APINAME##_API_VERSION_MAJOR,        \
                                   DD_##APINAME##_API_VERSION_MINOR,        \
                                   DD_##APINAME##_API_VERSION_PATCH },      \
                        reinterpret_cast<void**>(&pApi))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RgdMgr::InitApis()
{
    m_pApiRegistry = m_pToolApi->GetApiRegistry(m_pToolApi->pInstance);

    DD_RESULT result = DD_RESULT_DD_GENERIC_NOT_READY;

    if (m_pApiRegistry)
    {
        result = INIT_API(DRIVER_UTILS, m_pDriverUtilsApi);

        if (result == DD_RESULT_SUCCESS)
        {
            result = INIT_API(GPU_DETECTIVE, m_pGpuDetectiveApi);
        }

        if (result == DD_RESULT_SUCCESS)
        {
            result  = INIT_API(CONNECTION, m_pConnectionApi);
        }

        if (result == DD_RESULT_SUCCESS)
        {
            result = INIT_API(SETTINGS, m_pSettingsApi);
        }

        if (result == DD_RESULT_SUCCESS)
        {
            result = INIT_API(UBER_TRACE, m_pUberTraceApi);
        }

        if (result == DD_RESULT_SUCCESS)
        {
            result = INIT_API(ROUTER_UTILS, m_pRouterUtilsApi);
        }

        if (result == DD_RESULT_SUCCESS)
        {
            result = INIT_API(ENHANCED_CRASH_INFO, m_pEnhancedCrashInfoApi);
        }

        if (result == DD_RESULT_SUCCESS)
        {
            result = INIT_API(AMD_LOG_UTILS, m_pAmdLogUtilsApi);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RgdMgr::SetCrashAnalysisFeatureFlag(bool enable)
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_NOT_READY;

    if (m_pDriverUtilsApi != nullptr)
    {
        constexpr size_t setterNameLen = sizeof(kRgdToolId);
        result = m_pDriverUtilsApi->SetFeature(m_pDriverUtilsApi->pInstance,
                                               DD_DRIVER_UTILS_FEATURE_CRASH_ANALYSIS,
                                               enable ? DD_DRIVER_UTILS_FEATURE_FLAG_ENABLE : DD_DRIVER_UTILS_FEATURE_FLAG_DISABLE,
                                               kRgdToolId,
                                               setterNameLen);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RgdMgr::EnableEnhancedCrashInfo()
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_NOT_READY;

    if (m_pEnhancedCrashInfoApi != nullptr)
    {
        DDEnhancedCrashInfoConfig ecaInfoConfig = {};

        ecaInfoConfig.processId                = m_pid;
        ecaInfoConfig.flags.captureWaveData    = m_ocaCaptureConfig.captureWaveData;
        ecaInfoConfig.flags.captureVGPRData    = m_ocaCaptureConfig.captureVGPRData;
        ecaInfoConfig.flags.captureSGPRData    = m_ocaCaptureConfig.captureSGPRData;
        ecaInfoConfig.flags.enableSingleMemOp  = m_ocaCaptureConfig.enableSingleMemOp;
        ecaInfoConfig.flags.enableSingleAluOp  = m_ocaCaptureConfig.enableSingleAluOp;

        result = m_pEnhancedCrashInfoApi->SetEnhancedCrashInfoConfig(m_pEnhancedCrashInfoApi->pInstance, &ecaInfoConfig);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT RgdMgr::ForceDisableDriverOverlay(uint16_t umdConnectionId)
{
    DD_RESULT result = DD_RESULT_DD_GENERIC_NOT_READY;

    if (m_pSettingsApi != nullptr)
    {
        // This setting is hardcoded since it isn't likely to change
        uint32_t           settingValue   = 0x2;
        DDSettingsValueRef overlaySetting = {};
        overlaySetting.hash               = 3552029138;
        overlaySetting.type               = DD_SETTINGS_TYPE_UINT32;
        overlaySetting.size               = sizeof(uint32_t);
        overlaySetting.pValue             = reinterpret_cast<void*>(&settingValue);
        DDSettingsComponentValueRefs componentValues = {};
        componentValues.pValues                      = &overlaySetting;
        componentValues.numValues                    = 1;

        snprintf(componentValues.componentName, sizeof(componentValues.componentName), "PalPlatform");

        result = m_pSettingsApi->SendAllUserOverrides(m_pSettingsApi->pInstance, umdConnectionId, 1, &componentValues);
    }

    return result;
}

} // DevDriver namespace
