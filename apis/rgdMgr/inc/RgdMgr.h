/* Copyright (C) 2025-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddRouter.h>
#include <ddCommon.h>
#include <dd_tool_api.h>
#include <dd_connection_api.h>
#include <dd_enhanced_crash_info_api.h>
#include <ddRdf.h>
#include <amdrdf.h>
#include <ddDevModeControlDevice.h>
#include <ddAmdLogInterface.h>
#include <dd_uber_trace_api.h>

#include <thread>
#include <mutex>
#include <atomic>

// Forwards
struct DDDriverUtilsApi;
struct DDSettingsApi;
struct DDGpuDetectiveApi;
struct DDRouterUtilsApi;
struct DDEnhancedCrashInfoApi;
struct DDAmdLogUtilsApi;

struct ProcessInfoChunk
{
    uint32_t    processId;   // PID of the process.
    std::string processPath; // Absolute path of the process.
};

/// Configuration for Enhanced Crash Info data collection.
/// If nullptr is passed to MonitorApp, all flags default to enabled.
struct RgdEnhancedCrashInfoConfig
{
    bool captureWaveData   = true;    ///< Enables ShaderWaves, MmrRegisters, and SeInfo.
    bool captureVGPRData   = true;    ///< Enables VgprRegisters.
    bool captureSGPRData   = true;    ///< Enables SgprRegisters.
    bool enableSingleMemOp = false;   ///< Forces the shader run in a mode that only allows 1 memory fetch or write to be outstanding at a time.
    bool enableSingleAluOp = false;   ///< Forces the shader run in one-instruction-at-a-time mode.
};

namespace DevDriver
{
class WinKmIoCtlDevice;

class RgdMgr
{
public:
    RgdMgr()
        : m_router(DD_API_INVALID_HANDLE)
        , m_pToolApi(nullptr)
        , m_pApiRegistry(nullptr)
        , m_pConnectionApi(nullptr)
        , m_connectionCbs()
        , m_pDriverUtilsApi(nullptr)
        , m_pSettingsApi(nullptr)
        , m_pGpuDetectiveApi(nullptr)
        , m_pRouterUtilsApi(nullptr)
        , m_pEnhancedCrashInfoApi(nullptr)
        , m_pUberTraceApi(nullptr)
        , m_appName()
        , m_outputFile()
        , m_pAmdLogUtilsApi(nullptr)
        , m_rdfFileWriter()
        , m_heartbeat()
        , m_monitorStarted(false)
        , m_rgdState(DevDriver::RgdStateMonitoringNotEnabled)
        , m_reachedPostDeviceInit(false)
        , m_devDriverInit(false)
        , m_pid(0)
        , m_umdConnectionId(0)
        , m_dumpedTraceData(false)
        , m_crashDetected(false)
        , m_cancelTraceCollection(false)
        , m_pIoCtlDevice(nullptr)
        , m_umdTraceBuffer(DevDriver::Platform::GenericAllocCb)
        , m_traceData(DevDriver::Platform::GenericAllocCb)
        , m_processInfoChunk()
        , m_hRgdDataReadyEvent(NULL)
        , m_outputInfo()
        , m_traceCollectionThread()
    {}

    ~RgdMgr()
    {
        EndMonitoring();
        if (m_hRgdDataReadyEvent != NULL)
        {
            CloseHandle(m_hRgdDataReadyEvent);
            m_hRgdDataReadyEvent = NULL;
        }
    }

    // Functions used by end users:
    DD_RESULT           MonitorApp(const std::string& appName, uint32_t options = 0);
    void                EndMonitoring(bool deleteFile = true);
    DevDriver::RgdState GetRgdState() { return m_rgdState; }
    std::string         GetOutputFile() { return m_outputFile; }
    void                DumpTraceData();

    // Functions used by callbacks:
    std::string GetAppName() { return m_appName; }
    void        OnDriverStateChangedImpl(DDConnectionId umdConnectionId, DD_DRIVER_STATE state);
    void        OnDriverConnectedImpl(const DDConnectionInfo* pConnInfo);
    void        OnDriverDisconnectedImpl(DDConnectionId umdConnectionId);

private:
    DD_RESULT InitDevDriver();
    void      ShutdownDevDriver();
    DD_RESULT CreateRouter(DDRouter* pOutRouter);
    DD_RESULT CreateToolApi(DDToolApi** ppOutToolApi);
    DD_RESULT LoadConnectionCallbacks();
    DD_RESULT InitApis();
    DD_RESULT SetCrashAnalysisFeatureFlag(bool enable);
    DD_RESULT EnableEnhancedCrashInfo();
    DD_RESULT LoadOcaCaptureConfig(uint32_t options);
    DD_RESULT ForceDisableDriverOverlay(uint16_t umdConnectionId);
    void      CloseTraceFile();

    void TraceCollectionThreadProc(DDConnectionId umdConnectionId, bool reachedPostDeviceInit);

    // Helper to safely cleanup the trace collection thread
    void CleanupTraceCollectionThread();

    // Functions used to write additional trace chunks
    void      WriteAdditionalChunks();

    DD_RESULT WriteUbertraceChunks(rdfChunkFileWriter*   pChunkFileWriter,
                                   rdfChunkFile*         pFile,
                                   rdfChunkFileIterator* pIterator);

    DD_RESULT WriteProcessInfoChunk(rdfChunkFileWriter* pChunkFileWriter);

    uint32_t                        m_pid;
    DDConnectionId                  m_umdConnectionId;
    bool                            m_dumpedTraceData;
    bool                            m_crashDetected;
    std::atomic<bool>               m_cancelTraceCollection;
    DDRouter                        m_router;
    DDToolApi*                      m_pToolApi;
    DDApiRegistry*                  m_pApiRegistry;
    DDConnectionApi*                m_pConnectionApi;
    DDConnectionCallbacks           m_connectionCbs;
    DDDriverUtilsApi*               m_pDriverUtilsApi;
    DDSettingsApi*                  m_pSettingsApi;
    DDGpuDetectiveApi*              m_pGpuDetectiveApi;
    DDRouterUtilsApi*               m_pRouterUtilsApi;
    DDEnhancedCrashInfoApi*         m_pEnhancedCrashInfoApi;
    DDAmdLogUtilsApi*               m_pAmdLogUtilsApi;
    DDUberTraceApi*                 m_pUberTraceApi;
    std::string                     m_appName;
    std::string                     m_outputFile;
    bool                            m_monitorStarted;
    DevDriver::RgdState             m_rgdState;
    bool                            m_reachedPostDeviceInit;
    bool                            m_devDriverInit;
    struct DDRdfFileWriter          m_rdfFileWriter;
    struct DDIOHeartbeat            m_heartbeat;
    DevDriver::WinKmIoCtlDevice*    m_pIoCtlDevice;
    DevDriver::Vector<uint8_t>      m_umdTraceBuffer; // Buffer to hold collected UberTrace data from UMD
    DevDriver::Vector<uint8_t>      m_traceData;
    ProcessInfoChunk                m_processInfoChunk;
    HANDLE                          m_hRgdDataReadyEvent;
    RgdOcaClientUpdate              m_outputInfo;
    std::thread                     m_traceCollectionThread;
    std::mutex                      m_traceCollectionMutex; // Protects trace collection thread cleanup

    // Hashmap storing reg key configs and their bitmask modes for OCA captures
    std::unordered_map<std::string, uint32_t> m_OcaBitmasks;
    OcaCaptureConfig                          m_ocaCaptureConfig;
};

} // DevDriver namespace
