/* Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "RouterUtilsService.h"
#include <dd_common_api.h>
#include <dd_result.h>

// legacy
#include <ddCommon.h>

// third party
#include <stb_sprintf.h>

// system
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <amdgpu_drm.h>
#include <xf86drm.h>
#include <charconv>

namespace
{
DDLoggerInfo s_logger;

void LogHelper(DDLogEvent* pLogEvent, const char* pFormat, va_list args)
{
    const uint32_t MaxLogMsgSize = 1024;
    char logMsg[MaxLogMsgSize] {};

    stbsp_vsnprintf(logMsg, MaxLogMsgSize, pFormat, args);

    s_logger.pfnLog(s_logger.pUserdata, pLogEvent, logMsg);
}

void LogError(const char* pFormat, ...)
{
    DDLogEvent s_logEvent = { "DDRouterUtils", nullptr, __func__, __LINE__, DD_LOG_LEVEL_ERROR };

    va_list args;
    va_start(args, pFormat);
    LogHelper(&s_logEvent, pFormat, args);
    va_end(args);
}

void LogWarn(const char* pFormat, ...)
{
    DDLogEvent s_logEvent = { "DDRouterUtils", nullptr, __func__, __LINE__, DD_LOG_LEVEL_WARN };

    va_list args;
    va_start(args, pFormat);
    LogHelper(&s_logEvent, pFormat, args);
    va_end(args);
}

const char PerformanceLevelNormal[] = "auto";
const char PerformanceLevelStable[] = "profile_standard";
const char PerformanceLevelPeak[] = "profile_peak";

// This struct represents one line in the sysfile pp_dpm_sclk.
// 0: 500Mhz
// 1: 1000Mhz *
// 2: 2025Mhz
struct ClockInfo
{
    uint32_t level;
    uint32_t frequency; // in Mhz
    bool     isCurrent; // The line with '*' at the end indicates the current clock.
};

struct ClockModeInfo
{
    DD_DEVICE_CLOCK_MODE mode;
    uint64_t             engineClock;
    uint64_t             memoryClock;
};

/// Safe replacement for sscanf when parsing "<level>: <frequency>Mhz" pattern
/// Returns true if the string matches the clock info format
/// @param str The string to parse (e.g., "0: 500Mhz" or "1: 1000Mhz *")
/// @param maxSize Maximum size of the buffer to prevent reading past the end
/// @param outLevel Output parameter for the parsed level
/// @param outFrequency Output parameter for the parsed frequency in MHz
/// @return true if successfully parsed the clock format, false otherwise
static bool ParseClockInfo(const char* str, size_t maxSize, uint32_t& outLevel, uint32_t& outFrequency)
{
    if (str == nullptr || maxSize == 0)
    {
        return false;
    }

    // Find the end of the string, but don't go past maxSize
    const char* strEnd = str;
    const char* maxEnd = str + maxSize;
    while (strEnd < maxEnd && *strEnd != '\0')
    {
        ++strEnd;
    }

    const char* ptr = str;

    // Parse the level number
    uint32_t level = 0;
    auto [ptr1, ec1] = std::from_chars(ptr, strEnd, level);
    if (ec1 != std::errc{})
    {
        return false;
    }
    ptr = ptr1;

    // Skip whitespace after level
    while (ptr < strEnd && (*ptr == ' ' || *ptr == '\t'))
    {
        ++ptr;
    }

    // Expect ':'
    if (ptr >= strEnd || *ptr != ':')
    {
        return false;
    }
    ++ptr;

    // Skip whitespace after ':'
    while (ptr < strEnd && (*ptr == ' ' || *ptr == '\t'))
    {
        ++ptr;
    }

    // Parse the frequency number
    uint32_t frequency = 0;
    auto [ptr2, ec2] = std::from_chars(ptr, strEnd, frequency);
    if (ec2 != std::errc{})
    {
        return false;
    }
    ptr = ptr2;

    // Expect "Mhz" (case sensitive)
    if ((strEnd - ptr) < 3 || ptr[0] != 'M' || ptr[1] != 'h' || ptr[2] != 'z')
    {
        return false;
    }
    ptr += 3;

    // The rest of the string can be anything (e.g., " *" or empty)

    outLevel = level;
    outFrequency = frequency;
    return true;
}

/// Safe replacement for sscanf when parsing "card<number>" pattern
/// Returns true if the string matches exactly "card<number>" with no extra characters
/// @param str The string to parse (e.g., "card0")
/// @param maxSize Maximum size of the buffer to prevent reading past the end
/// @param outIndex Output parameter for the parsed index
/// @return true if successfully parsed "card<number>" with no extra characters, false otherwise
static bool ParseCardIndexOnly(const char* str, size_t maxSize, int& outIndex)
{
    if (str == nullptr || maxSize == 0)
    {
        return false;
    }

    // Check if string starts with "card"
    constexpr const char* prefix = "card";
    constexpr size_t prefixLen = 4; // length of "card"

    // Make sure we have enough space to check the prefix
    if (maxSize < prefixLen)
    {
        return false;
    }

    for (size_t i = 0; i < prefixLen; ++i)
    {
        if (str[i] != prefix[i])
        {
            return false;
        }
    }

    // Move past the "card" prefix
    const char* numStart = str + prefixLen;

    // Empty string after "card" is invalid
    if (*numStart == '\0')
    {
        return false;
    }

    // Find the end of the string, but don't go past maxSize
    const char* numEnd = numStart;
    const char* maxEnd = str + maxSize;
    while (numEnd < maxEnd && *numEnd != '\0')
    {
        ++numEnd;
    }

    // Parse the number using std::from_chars (noexcept)
    int value = 0;
    auto [ptr, ec] = std::from_chars(numStart, numEnd, value);

    // Check if parsing succeeded and consumed the entire string
    if (ec != std::errc{} || ptr != numEnd)
    {
        return false;
    }

    outIndex = value;
    return true;
}

drmDevicePtr FindTargetDrmDevice(DDGpuId targetGpuId)
{
    drmDevicePtr pTargetDevice = NULL;

    uint32_t bus  = (targetGpuId & 0x00ff'0000) >> 16;
    uint32_t dev  = (targetGpuId & 0x0000'ff00) >> 8;
    uint32_t func = (targetGpuId & 0x0000'00ff);

    int deviceCount = 0;
    const int32_t MaxDeviceCount = 64;
    drmDevicePtr devices[MaxDeviceCount] = {};

    const int totalDeviceCount = drmGetDevices(NULL, 0);
    if (totalDeviceCount > MaxDeviceCount)
    {
        LogWarn(
            "The system has %d total number of drm devices. But the max number we have hold is: %d.",
            totalDeviceCount, MaxDeviceCount);
    }
    deviceCount = drmGetDevices(devices, MaxDeviceCount);

    for (int32_t i = 0; i < deviceCount; ++i)
    {
        if (devices[i]->bustype == DRM_BUS_PCI)
        {
            if ((bus == devices[i]->businfo.pci->bus) &&
                (dev == devices[i]->businfo.pci->dev) &&
                (func == devices[i]->businfo.pci->func))
            {
                pTargetDevice = devices[i];
                break;
            }
        }
        else
        {
            const char* DeviceBusTypes[4] = { "DRM_BUS_PCI", "DRM_BUS_USB", "DRM_BUS_PLATFORM", "DRM_BUS_HOST1X" };
            LogWarn("Found non-PCI device. The device type is %s.", DeviceBusTypes[devices[i]->bustype]);
        }
    }

    return pTargetDevice;
}

int32_t FindCardIndexDrmDevice(const drmDevicePtr pDevice)
{
    // - Iterate through "/sys/class/drm/card<index>/" directories and check the file
    // "device/device" to find the matching device id, then return <index>.

    const unsigned long targetDeviceId = pDevice->deviceinfo.pci->device_id;

    int32_t gpuCardIndex = -1;

    const char drmCardParentDir[] = "/sys/class/drm/";
    DIR* pDir = opendir(drmCardParentDir);
    if (pDir)
    {
        dirent* pDirEntry = nullptr;
        for (;;)
        {
            pDirEntry = readdir(pDir);
            if (pDirEntry == nullptr)
            {
                break;
            }

            if (std::strcmp(pDirEntry->d_name, ".") == 0 ||
                std::strcmp(pDirEntry->d_name, "..") == 0)
            {
                continue;
            }

            int indexSuffix = 0;
            // Directory names are limited by NAME_MAX (typically 255 on Linux)
            constexpr size_t maxDirNameLen = 256;
            if (!ParseCardIndexOnly(pDirEntry->d_name, maxDirNameLen, indexSuffix))
            {
                // We're only interested in "card<number>" with no extra characters. "card<number><other-chars>" should be ignored.
                continue;
            }

            const int32_t DeviceFilenameBufSize = 256;
            char deviceFilenameBuf[DeviceFilenameBufSize] {};
            int32_t writtenSize = stbsp_snprintf(
                deviceFilenameBuf, DeviceFilenameBufSize,
                "%s%s/device/device", drmCardParentDir, pDirEntry->d_name);

            if (writtenSize >= DeviceFilenameBufSize)
            {
                LogWarn("deviceFilenameBuf too small for the dir_entry: %s", pDirEntry->d_name);
            }
            else
            {
                FILE* pDeviceFile = fopen(deviceFilenameBuf, "r");
                if (pDeviceFile)
                {
                    unsigned long parsedDeviceId = 0;
                    int parsed = fscanf(pDeviceFile, "0x%04lx", &parsedDeviceId);
                    if (parsed == 1)
                    {
                        if (parsedDeviceId == targetDeviceId)
                        {
                            gpuCardIndex = indexSuffix;
                        }
                    }
                    else
                    {
                        LogError("Failed to parse device id from the file: %s.", deviceFilenameBuf);
                    }
                    fclose(pDeviceFile);
                }
                else
                {
                    LogError(
                        "Failed to open device file: %s. Error: %s.", deviceFilenameBuf, std::strerror(errno));
                }
            }

            if (gpuCardIndex != -1)
            {
                // Found gpu index.
                break;
            }
        }

        closedir(pDir);
    }
    else
    {
        LogError("Failed to open directory: %s. Error: %s.", drmCardParentDir, std::strerror(errno));
    }

    return gpuCardIndex;
}

uint32_t ParseClockInfos(const char* pClockInfoFilename, ClockInfo* pClockInfos, uint32_t MaxClockInfoCount)
{
    uint32_t totalClockInfos = 0;

    FILE* pClockInfoFile = std::fopen(pClockInfoFilename, "r");
    if (pClockInfoFile)
    {
        const uint32_t ClockInfoBufSize = 512;
        char clockInfoBuf[ClockInfoBufSize] {};

        size_t byteRead = std::fread(clockInfoBuf, sizeof(clockInfoBuf[0]), ClockInfoBufSize, pClockInfoFile);
        if (std::feof(pClockInfoFile) != 0)
        {
            const uint32_t TempBufSize = 256;
            char tempBuf[TempBufSize] {};

            const char* pClockInfoEnd = clockInfoBuf + byteRead;
            const char* pLine = clockInfoBuf;
            const char* pLineEnd = nullptr;

            uint32_t clkInfoIndex = 0;

            for (;;)
            {
                if (pLine >= pClockInfoEnd)
                {
                    break;
                }

                pLineEnd = std::strchr(pLine, '\n');
                if (pLineEnd == nullptr)
                {
                    pLineEnd = pClockInfoEnd;
                }

                uint32_t lineSize = pLineEnd - pLine;
                if (lineSize > 0)
                {
                    if (lineSize < TempBufSize)
                    {
                        if (clkInfoIndex >= MaxClockInfoCount)
                        {
                            LogError("Failed to parse %s. More lines than the max expected.", pClockInfoFilename);
                            break;
                        }

                        DevDriver::Platform::Memcpy_s(tempBuf, TempBufSize, pLine, lineSize);
                        tempBuf[lineSize] = '\0';

                        pClockInfos[clkInfoIndex] = {};

                        uint32_t level = 0;
                        uint32_t frequency = 0;

                        if (ParseClockInfo(tempBuf, lineSize, level, frequency))
                        {
                            pClockInfos[clkInfoIndex].level = level;
                            pClockInfos[clkInfoIndex].frequency = frequency;

                            if (std::strchr(tempBuf, '*') != nullptr)
                            {
                                pClockInfos[clkInfoIndex].isCurrent = true;
                            }

                            clkInfoIndex += 1;
                        }
                        else
                        {
                            LogError("Failed to parse the file: %s. Error: unrecognized format.", pClockInfoFilename);
                        }
                    }
                    else
                    {
                        LogError("Failed to parse the file: %s. Error: line too long.", pClockInfoFilename);
                    }
                }

                pLine = pLineEnd + 1;
            }

            totalClockInfos = clkInfoIndex;
        }
        else
        {
            LogError(
                "Failed to device clocks info from the file: %s. Error: %s",
                pClockInfoFilename, std::strerror(errno));
        }

        fclose(pClockInfoFile);
    }
    else
    {
        LogError("Failed to open file: %s. Error: %s.", pClockInfoFilename, std::strerror(errno));
    }

    return totalClockInfos;
}

uint32_t ParseSClockInfos(int32_t gpuIndex, ClockInfo* pClockInfos, uint32_t MaxClockInfoCount)
{
    uint32_t totalClockInfoCount = 0;

    const int32_t SClockFilenameBufSize = 128;
    char sclockFilename[SClockFilenameBufSize] {};
    int bytesWritten = stbsp_snprintf(
        sclockFilename, SClockFilenameBufSize, "/sys/class/drm/card%d/device/pp_dpm_sclk", gpuIndex);

    if (bytesWritten >= SClockFilenameBufSize)
    {
        LogError("Failed to parse sclk info for device at index %d. Error: sclkFilenameBuf too small", gpuIndex);
    }
    else
    {
        totalClockInfoCount = ParseClockInfos(sclockFilename, pClockInfos, MaxClockInfoCount);
    }

    return totalClockInfoCount;
}

uint32_t ParseMClockInfos(int32_t gpuIndex, ClockInfo* pClockInfos, uint32_t MaxClockInfoCount)
{
    uint32_t totalClockInfoCount = 0;

    const int32_t SClockFilenameBufSize = 512;
    char sclockFilename[SClockFilenameBufSize] {};
    int bytesWritten = stbsp_snprintf(
        sclockFilename, SClockFilenameBufSize, "/sys/class/drm/card%d/device/pp_dpm_mclk", gpuIndex);

    if (bytesWritten >= SClockFilenameBufSize)
    {
        LogError("Failed to parse mclk info for device at index %d. Error: sclkFilenameBuf too small", gpuIndex);
    }
    else
    {
        totalClockInfoCount = ParseClockInfos(sclockFilename, pClockInfos, MaxClockInfoCount);
    }

    return totalClockInfoCount;
}

uint32_t GetPeakClock(ClockInfo* clockInfoList, uint32_t clockInfoCount)
{
    uint32_t peakClock = 0;
    for (uint32_t i = 0; i < clockInfoCount; ++i)
    {
        if (clockInfoList[i].frequency > peakClock)
        {
            peakClock = clockInfoList[i].frequency;
        }
    }
    return peakClock;
}

DD_RESULT SetClockModeBySysFs(int32_t gpuCardIndex, DD_DEVICE_CLOCK_MODE clockModeToSet)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    const int32_t PerformanceLevelFilenameBufSize = 128;
    char performanceLevelFilenameBuf[PerformanceLevelFilenameBufSize] {};
    int32_t writtenSize = stbsp_snprintf(
        performanceLevelFilenameBuf, PerformanceLevelFilenameBufSize,
        "/sys/class/drm/card%d/device/power_dpm_force_performance_level", gpuCardIndex);
    if (writtenSize >= PerformanceLevelFilenameBufSize)
    {
        result = DD_RESULT_COMMON_BUFFER_TOO_SMALL;
        LogError(
            "SetClockMode failed. "
            "Error: performanceLevelFilenameBuf too small for device index %d", gpuCardIndex);
    }
    else
    {
        FILE* pPerfLevelFile = fopen(performanceLevelFilenameBuf, "w");
        if (pPerfLevelFile)
        {
            const char* pPerfLevelStr = nullptr;
            size_t perfLevelStrLen = 0;

            if (clockModeToSet == DD_DEVICE_CLOCK_MODE_NORMAL)
            {
                pPerfLevelStr = PerformanceLevelNormal;
                perfLevelStrLen = sizeof(PerformanceLevelNormal) - 1;
            }
            if (clockModeToSet == DD_DEVICE_CLOCK_MODE_STABLE)
            {
                pPerfLevelStr = PerformanceLevelStable;
                perfLevelStrLen = sizeof(PerformanceLevelStable) - 1;
            }
            if (clockModeToSet == DD_DEVICE_CLOCK_MODE_PEAK)
            {
                pPerfLevelStr = PerformanceLevelPeak;
                perfLevelStrLen = sizeof(PerformanceLevelPeak) - 1;
            }

            if (pPerfLevelStr)
            {
                size_t bytesWritten = std::fwrite(
                    pPerfLevelStr, sizeof(pPerfLevelStr[0]), perfLevelStrLen, pPerfLevelFile);
                if (bytesWritten < perfLevelStrLen)
                {
                    result = DevDriver::ResultFromErrno(errno);
                    LogError(
                        "SetClockMode failed. Failed to write level('%s') to the file. Posix error: %s.",
                        pPerfLevelStr, std::strerror(errno));
                }
            }

            fclose(pPerfLevelFile);
        }
        else
        {
            result = DevDriver::ResultFromErrno(errno);
            LogError(
                "SetClockMode failed. Failed to open the file: %s. Posix error: %s",
                performanceLevelFilenameBuf, std::strerror(errno));
        }
    }

    return result;
}
} // anonymous namespace

namespace RouterUtilsModule
{
RouterUtilsService::RouterUtilsService(DDLoggerInfo logger)
    : RouterUtilsRpc::IRouterUtilsRpcService {},
      m_sysInfo(DevDriver::Platform::GenericAllocCb)
{
    s_logger = logger;
    QueryAndCacheSystemInfoAsync();
}

RouterUtilsService::~RouterUtilsService()
{
    for (auto deviceIter : m_gpuDevices)
    {
        if (deviceIter.second.hGpuContext != nullptr)
        {
            amdgpu_cs_ctx_free(deviceIter.second.hGpuContext);
        }
        if (deviceIter.second.hGpuDevice != nullptr)
        {
            amdgpu_device_deinitialize(deviceIter.second.hGpuDevice);
        }
    }

    m_sysInfoThread.Join();
}

DD_RESULT RouterUtilsService::QueryPathByProcessId(
    const void* pParamBuffer,
    size_t paramBufferSize,
    const DDByteWriter& writer)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    uint32_t processId = 0;

    if (paramBufferSize >= sizeof(uint32_t))
    {
        DevDriver::Platform::Memcpy_s(&processId, sizeof(processId), pParamBuffer, sizeof(processId));
    }
    else
    {
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    if (result == DD_RESULT_SUCCESS)
    {
        const int ProcPathMaxSize           = 128;
        char      procPath[ProcPathMaxSize] = {};
        int       requiredSize              = stbsp_snprintf(procPath, ProcPathMaxSize, "/proc/%u/exe", processId);
        if (requiredSize > 0)
        {
            if (requiredSize < ProcPathMaxSize)
            {
                procPath[requiredSize] = '\0';

                // lstat returns size 0 for symlinks under /proc. No need to call it to get
                // the file size. Just set it to a reasonably large value.
                const ssize_t appPathBufSize = 4000;
                char*         pathBuf        = (char*)malloc(appPathBufSize);
                if (pathBuf != NULL)
                {
                    ssize_t bytesRead = readlink(procPath, pathBuf, appPathBufSize);
                    if (bytesRead <= appPathBufSize)
                    {
                        ByteWriterWrapper wrapper(writer);
                        result = wrapper.Begin(bytesRead);
                        if (result == DD_RESULT_SUCCESS)
                        {
                            result = wrapper.Write(pathBuf, bytesRead);
                        }
                        wrapper.End(result);
                    }
                    else
                    {
                        result = DD_RESULT_COMMON_UNKNOWN;
                    }

                    free(pathBuf);
                }
                else
                {
                    result = DD_RESULT_COMMON_OUT_OF_HEAP_MEMORY;
                }
            }
            else
            {
                result = DD_RESULT_COMMON_BUFFER_TOO_SMALL;
            }
        }
        else
        {
            result = DD_RESULT_COMMON_UNKNOWN;
        }
    }

    return result;
}

DD_RESULT RouterUtilsService::QueryTimestampAndFrequency(const DDByteWriter& writer)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    uint64_t counter   = 0;
    uint64_t frequency = 0;

    // On Posix we use nanosecond as time unit, so frequency is always 1000'000'000.
    frequency = 1000 * 1000 * 1000;

    timespec timeSpec = {};
    if (clock_gettime(CLOCK_MONOTONIC, &timeSpec) == 0)
    {
        counter = ((timeSpec.tv_sec * frequency) + timeSpec.tv_nsec);
    }
    else
    {
        result = DD_RESULT_COMMON_UNKNOWN;
    }

    ByteWriterWrapper wrapper(writer);
    result = wrapper.Begin(sizeof(frequency) + sizeof(counter));
    if (result == DD_RESULT_SUCCESS)
    {
        result = wrapper.Write(&counter, sizeof(counter));
        result = wrapper.Write(&frequency, sizeof(frequency));
    }
    wrapper.End(result);

    return result;
}

DD_RESULT RouterUtilsService::QueryDeviceClocks(
    const void* pParamBuffer, size_t paramBufferSize, const DDByteWriter& writer)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    drmDevicePtr pTargetDevice = NULL;
    int32_t targetGpuCardIndex = -1;

    uint32_t peakSClockMhz = 0;
    uint32_t peakMClockMhz = 0;

    uint32_t stableSClockMhz = 0;
    uint32_t stableMClockMhz = 0;

    if (paramBufferSize < sizeof(DDGpuId))
    {
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
        LogError("QueryDeviceClocks failed. RCP parameter buffer too small (size: %u).", paramBufferSize);
    }

    if (result == DD_RESULT_SUCCESS)
    {
        // Find target drm device.

        DDGpuId gpuId = 0;
        DevDriver::Platform::Memcpy_s(&gpuId, sizeof(gpuId), pParamBuffer, sizeof(gpuId));

        pTargetDevice = FindTargetDrmDevice(gpuId);
        if (pTargetDevice == NULL)
        {
            result = DD_RESULT_COMMON_INVALID_PARAMETER;
            LogError("QueryDeviceClocks failed. Failed to find the target drm device based the gpuId: %u.", gpuId);
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        // Find GPU device index.

        targetGpuCardIndex = FindCardIndexDrmDevice(pTargetDevice);
        if (targetGpuCardIndex < 0)
        {
            result = DD_RESULT_COMMON_INVALID_PARAMETER;
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        // Find peak clock frequencies.

        const uint32_t MaxClockInfoCount = 16;
        ClockInfo sclockInfoList[MaxClockInfoCount] {};
        ClockInfo mclockInfoList[MaxClockInfoCount] {};

        const uint32_t sclockInfoCount = ParseSClockInfos(targetGpuCardIndex, sclockInfoList, MaxClockInfoCount);
        const uint32_t mclockInfoCount = ParseMClockInfos(targetGpuCardIndex, mclockInfoList, MaxClockInfoCount);

        if ((sclockInfoCount != 0) && (mclockInfoCount != 0))
        {
            peakSClockMhz = GetPeakClock(sclockInfoList, sclockInfoCount);
            peakMClockMhz = GetPeakClock(mclockInfoList, mclockInfoCount);
        }
        else
        {
            result = DD_RESULT_DD_UNKNOWN;
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        // Find stable clock frequencies.

        amdgpu_device_handle deviceHandle {};
        uint32_t majorVersion = 0;
        uint32_t minorVersion = 0;
        int renderNodeFd  = open(pTargetDevice->nodes[DRM_NODE_RENDER], O_RDWR, 0);

        int err = amdgpu_device_initialize(renderNodeFd, &majorVersion, &minorVersion, &deviceHandle);
        if (err != 0)
        {
            result = DevDriver::ResultFromErrno(err);
            LogError(
                "QueryDeviceClocks failed. amdgpu_device_initialize() failed. Posix error: %d", std::strerror(-err));
        }

        if (result == DD_RESULT_SUCCESS)
        {
            err = amdgpu_query_sensor_info(
                deviceHandle,
                AMDGPU_INFO_SENSOR_STABLE_PSTATE_GFX_SCLK,
                sizeof(stableSClockMhz), &stableSClockMhz);
            if (err != 0)
            {
                result = DevDriver::ResultFromErrno(err);
                LogError(
                    "QueryDeviceClocks failed to query stable sclk. "
                    "amdgpu_query_sensor_info() failed. Posix error: %d", std::strerror(-err));
            }

            if (result == DD_RESULT_SUCCESS)
            {
                err = amdgpu_query_sensor_info(
                    deviceHandle,
                    AMDGPU_INFO_SENSOR_STABLE_PSTATE_GFX_MCLK,
                    sizeof(stableMClockMhz), &stableMClockMhz);
                if (err != 0)
                {
                    result = DevDriver::ResultFromErrno(err);
                    LogError(
                        "QueryDeviceClocks failed to query stable mclk. "
                        "amdgpu_query_sensor_info() failed. Posix error: %d", std::strerror(-err));
                }
            }

            amdgpu_device_deinitialize(deviceHandle);
        }

        if (renderNodeFd >= 0)
        {
            close(renderNodeFd);
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        // Write the data back.

        const uint32_t MaxClockModeInfoCount = 3;
        ClockModeInfo clockModeInfoList[MaxClockModeInfoCount] {};
        uint32_t clockModeInfoCount = 0;

        clockModeInfoList[clockModeInfoCount++] = { DD_DEVICE_CLOCK_MODE_NORMAL, 0, 0 };

        clockModeInfoList[clockModeInfoCount++] = {
            DD_DEVICE_CLOCK_MODE_STABLE, stableSClockMhz * 1000'000ull, stableMClockMhz * 1000'000ull };

        clockModeInfoList[clockModeInfoCount++] = {
            DD_DEVICE_CLOCK_MODE_PEAK, peakSClockMhz * 1000'000ull, peakMClockMhz * 1000'000ull };

        result = writer.pfnBegin(writer.pUserdata, nullptr);
        if (result == DD_RESULT_SUCCESS)
        {
            result = writer.pfnWriteBytes(writer.pUserdata, &clockModeInfoCount, sizeof(clockModeInfoCount));
        }
        if (result == DD_RESULT_SUCCESS)
        {
            result = writer.pfnWriteBytes(
                writer.pUserdata, clockModeInfoList, clockModeInfoCount * sizeof(clockModeInfoList[0]));
        }
        writer.pfnEnd(writer.pUserdata, result);
    }

    return result;
}

DD_RESULT RouterUtilsService::QueryCurrentClockMode(
    const void* pParamBuffer, size_t paramBufferSize, const DDByteWriter& writer)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    drmDevicePtr pTargetDevice = NULL;
    int32_t targetGpuCardIndex = -1;

    DD_DEVICE_CLOCK_MODE clockMode = DD_DEVICE_CLOCK_MODE_UNKNOWN;

    if (paramBufferSize < sizeof(DDGpuId))
    {
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
        LogError("QueryDeviceClocks failed. RCP parameter buffer too small (size: %u).", paramBufferSize);
    }

    if (result == DD_RESULT_SUCCESS)
    {
        // Find target drm device.

        DDGpuId gpuId = 0;
        DevDriver::Platform::Memcpy_s(&gpuId, sizeof(gpuId), pParamBuffer, sizeof(gpuId));

        pTargetDevice = FindTargetDrmDevice(gpuId);
        if (pTargetDevice == NULL)
        {
            result = DD_RESULT_COMMON_INVALID_PARAMETER;
            LogError(
                "QueryDeviceClocks failed. Failed to find the target drm device based on the gpuId: %u.",
                gpuId);
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        // Find GPU device index.

        targetGpuCardIndex = FindCardIndexDrmDevice(pTargetDevice);
        if (targetGpuCardIndex < 0)
        {
            result = DD_RESULT_COMMON_INVALID_PARAMETER;
        }
    }

    const uint32_t PerfLevelStrBufSize = 64;
    char perfLevelStrBuf[PerfLevelStrBufSize] {};

    if (result == DD_RESULT_SUCCESS)
    {
        const int32_t PerformanceLevelFilenameBufSize = 128;
        char performanceLevelFilenameBuf[PerformanceLevelFilenameBufSize] {};
        int32_t writtenSize = stbsp_snprintf(
            performanceLevelFilenameBuf, PerformanceLevelFilenameBufSize,
            "/sys/class/drm/card%d/device/power_dpm_force_performance_level", targetGpuCardIndex);
        if (writtenSize >= PerformanceLevelFilenameBufSize)
        {
            result = DD_RESULT_COMMON_BUFFER_TOO_SMALL;
            LogError(
                "QueryCurrentClockMode failed. "
                "Error: performanceLevelFilenameBuf too small for device index %d", targetGpuCardIndex);
        }
        else
        {
            FILE* pPerfLevelFile = fopen(performanceLevelFilenameBuf, "r");
            if (pPerfLevelFile)
            {
                int parsed = fscanf(pPerfLevelFile, "%63s", perfLevelStrBuf);
                if (parsed != 1)
                {
                    result = DD_RESULT_PARSING_UNKNOWN;
                    LogError(
                        "QueryCurrentClockMode failed. Failed to parse the file %s.", performanceLevelFilenameBuf);
                }
                fclose(pPerfLevelFile);
            }
            else
            {
                result = DD_RESULT_FS_UNKNOWN;
                LogError(
                    "QueryCurrentClockMode failed. Failed to open the file: %s. Posix error: %s",
                    performanceLevelFilenameBuf, std::strerror(errno));
            }
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        if (std::strcmp(perfLevelStrBuf, PerformanceLevelNormal) == 0)
        {
            clockMode = DD_DEVICE_CLOCK_MODE_NORMAL;
        }
        else if (std::strcmp(perfLevelStrBuf, PerformanceLevelStable) == 0)
        {
            clockMode = DD_DEVICE_CLOCK_MODE_STABLE;
        }
        else if (std::strcmp(perfLevelStrBuf, PerformanceLevelPeak) == 0)
        {
            clockMode = DD_DEVICE_CLOCK_MODE_PEAK;
        }
        else
        {
            clockMode = DD_DEVICE_CLOCK_MODE_UNKNOWN;
        }

        result = writer.pfnBegin(writer.pUserdata, nullptr);
        if (result == DD_RESULT_SUCCESS)
        {
            result = writer.pfnWriteBytes(writer.pUserdata, &clockMode, sizeof(clockMode));
        }
        writer.pfnEnd(writer.pUserdata, result);
    }

    return result;
}

DD_RESULT RouterUtilsService::SetClockMode(const void* pParamBuffer, size_t paramBufferSize)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    drmDevicePtr pTargetDevice = NULL;
    int32_t targetGpuCardIndex = -1;

    DDClockModeInfo setClockInput {};

    if (paramBufferSize < sizeof(setClockInput))
    {
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
        LogError("SetClockMode failed. RCP parameter buffer too small (size: %u).", paramBufferSize);
    }

    if (result == DD_RESULT_SUCCESS)
    {
        DevDriver::Platform::Memcpy_s(&setClockInput, sizeof(setClockInput), pParamBuffer, sizeof(setClockInput));

        if ((setClockInput.mode != DD_DEVICE_CLOCK_MODE_NORMAL) &&
            (setClockInput.mode != DD_DEVICE_CLOCK_MODE_STABLE) &&
            (setClockInput.mode != DD_DEVICE_CLOCK_MODE_PEAK))
        {
            result = DD_RESULT_COMMON_INVALID_PARAMETER;
            LogError("SetClockMode failed. Unrecognized input clock mode: %d.", setClockInput.mode);
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        // Find target drm device.

        pTargetDevice = FindTargetDrmDevice(setClockInput.gpuId);
        if (pTargetDevice == NULL)
        {
            result = DD_RESULT_COMMON_INVALID_PARAMETER;
            LogError(
                "SetClockMode failed. Failed to find the target drm device based on the gpuId: %u.",
                setClockInput.gpuId);
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        const uint32_t PStateIoctlSupportVersionMajor = 3;
        const uint32_t PStateIoctlSupportVersionMinor = 49;

        bool isPowerStateIoctlSupported = false;

        auto deviceItr = m_gpuDevices.find(setClockInput.gpuId);
        if (deviceItr == m_gpuDevices.end())
        {
            int renderNodeFd = open(pTargetDevice->nodes[DRM_NODE_RENDER], O_RDWR, 0);
            if (renderNodeFd == -1)
            {
                result = DevDriver::ResultFromErrno(errno);
                LogError("Failed to open render node file. Error: %s.", std::strerror(errno));
            }
            else
            {
                amdgpu_device_handle  hGpuDevice {};
                uint32_t              majorVersion {};
                uint32_t              minorVersion {};

                int err = amdgpu_device_initialize(renderNodeFd, &majorVersion, &minorVersion, &hGpuDevice);
                if (err == 0)
                {
                    // Context IOCTL stable pstate interface was introduced from drm 3.45,
                    // but kernel bugs was not fixed until 3.49
                    if ((majorVersion > PStateIoctlSupportVersionMajor) ||
                        (majorVersion == PStateIoctlSupportVersionMajor && minorVersion >= PStateIoctlSupportVersionMinor))
                    {
                        isPowerStateIoctlSupported = true;

                        AmdGpuDevice device { nullptr, hGpuDevice, majorVersion, minorVersion };
                        deviceItr = m_gpuDevices.emplace(setClockInput.gpuId, device).first;
                    }
                    else
                    {
                        amdgpu_device_deinitialize(hGpuDevice);
                        LogWarn(
                            "libdrm >= 3.49 is required for pstable state interface. "
                            "The existing libdrm version: %u.%u.", majorVersion, minorVersion);
                    }
                }
                else
                {
                    result = DevDriver::ResultFromErrno(err);
                    LogError(
                        "SetClockMode failed. Failed to initialize drm device. Posix error: %s.", std::strerror(-err));
                    if (hGpuDevice != nullptr)
                    {
                        amdgpu_device_deinitialize(hGpuDevice);
                    }
                }
            }

            if (renderNodeFd >= 0)
            {
                close(renderNodeFd);
            }
        }

        if (deviceItr != m_gpuDevices.end())
        {
            // double check
            isPowerStateIoctlSupported = (
                (deviceItr->second.majorVersion > PStateIoctlSupportVersionMajor) ||
                (deviceItr->second.majorVersion == PStateIoctlSupportVersionMajor &&
                 deviceItr->second.minorVersion >= PStateIoctlSupportVersionMinor));
        }

        if (isPowerStateIoctlSupported)
        {
            if (deviceItr->second.hGpuContext == nullptr)
            {
                int err = amdgpu_cs_ctx_create(deviceItr->second.hGpuDevice, &(deviceItr->second.hGpuContext));
                if (err != 0)
                {
                    result = DevDriver::ResultFromErrno(err);
                    LogError(
                        "SetClockMode failed. Failed to create execution context. Error: %s.\n", std::strerror(-err));
                    if (deviceItr->second.hGpuContext != nullptr)
                    {
                        amdgpu_cs_ctx_free(deviceItr->second.hGpuContext);
                        deviceItr->second.hGpuContext = nullptr;
                    }
                }
            }

            if (result == DD_RESULT_SUCCESS)
            {
                uint32_t powerStateToSet = AMDGPU_CTX_STABLE_PSTATE_NONE;
                switch (setClockInput.mode)
                {
                case DD_DEVICE_CLOCK_MODE_NORMAL: powerStateToSet = AMDGPU_CTX_STABLE_PSTATE_NONE; break;
                case DD_DEVICE_CLOCK_MODE_STABLE: powerStateToSet = AMDGPU_CTX_STABLE_PSTATE_STANDARD; break;
                case DD_DEVICE_CLOCK_MODE_PEAK: powerStateToSet = AMDGPU_CTX_STABLE_PSTATE_PEAK; break;
                default: powerStateToSet = AMDGPU_CTX_STABLE_PSTATE_NONE;
                }

                uint32_t pOutFlags = 0;
                int err = amdgpu_cs_ctx_stable_pstate(
                    deviceItr->second.hGpuContext, AMDGPU_CTX_OP_SET_STABLE_PSTATE, powerStateToSet, &pOutFlags);
                if (err != 0)
                {
                    result = DevDriver::ResultFromErrno(err);
                    LogError("SetClockMode failed. Failed to set power state. Error: %s.\n", std::strerror(-err));
                }
            }
        }
        else
        {
            // libdrm doesn't support power state interface. Use sysfs.

            targetGpuCardIndex = FindCardIndexDrmDevice(pTargetDevice);
            if (targetGpuCardIndex < 0)
            {
                result = DD_RESULT_COMMON_INVALID_PARAMETER;
            }

            if (result == DD_RESULT_SUCCESS)
            {
                result = SetClockModeBySysFs(targetGpuCardIndex, setClockInput.mode);
            }
        }
    }

    return result;
}
} // namespace RouterUtilsModule

