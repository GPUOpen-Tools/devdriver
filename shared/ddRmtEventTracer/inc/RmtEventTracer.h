/* Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddApi.h>
#include <ddDefs.h>
#include <ddCommon.h>
#include <util/vector.h>
#include <util/rmtWriter.h>
#include <system_info_reader.h>
#include <amdrdf.h>

namespace DevDriver
{

DD_RESULT RdfResultToDDResult(int rResult);

class RmtEventStreamer;

// Both `ApiType` and `TraceChunkApiInfo` are derived from the "ApiInfo" RDF Spec.
enum class ApiType : uint32
{
    GENERIC    = 0,
    DIRECTX_9  = 1,
    DIRECTX_11 = 2,
    DIRECTX_12 = 3,
    VULKAN     = 4,
    OPENGL     = 5,
    OPENCL     = 6,
    MANTLE     = 7,
    HIP        = 8,
    METAL      = 9
};

struct TraceChunkApiInfo
{
    ApiType  apiType;
    uint16_t apiVersionMajor; // Major client API version
    uint16_t apiVersionMinor; // Minor client API version
};

class RmtEventTracer
{
    friend class RmtEventStreamer;

private:
    DD_STATIC_CONST uint32 kKmdProviderId      = 0x60183;
    DD_STATIC_CONST uint32 kAmdLogProviderId   = 0x71294;
    DD_STATIC_CONST uint32 kRouterProviderId   = 0x21777465;
    DD_STATIC_CONST uint32 kUmdProviderId      = 0x50616C45;
    DD_STATIC_CONST uint32 kGpuNameMaxLen      = 128; // GPU name max length including null terminator
    DD_STATIC_CONST uint32 kMaxSnapshotNameLen = 128; // Snapshot name length including null terminator

public:
    enum class TraceState : uint32_t
    {
        NotStarted = 0,
        Running,
        Ended
    };

    enum class EndTraceReason : uint32_t
    {
        Unknown = 0,
        UserRequested,
        AppRequested,
        AppExited,
        UserRequestedContinue,
        Abort
    };

    struct TraceAdapterInfo
    {
        /// Name of the gpu
        char name[kGpuNameMaxLen];

        /// PCI Family
        uint32_t familyId;
        /// PCI Revision
        uint32_t revisionId;
        /// PCI Device
        uint32_t deviceId;
        /// Minumum engine clock in Mhz
        uint32_t minEngineClock;
        /// Maximum engine clock in Mhz
        uint32_t maxEngineClock;
        /// Type of memory
        uint32_t memoryType;
        /// Number of memory operations per clock
        uint32_t memoryOpsPerClock;
        /// Bus width of memory interface in bits
        uint32_t memoryBusWidth;
        /// Bandwidth of memory in MB/s
        uint32_t memoryBandwidth;
        /// Minumum memory clock in Mhz
        uint32_t minMemoryClock;
        /// Minumum memory clock in Mhz
        uint32_t maxMemoryClock;
    };

    /// Data for the snapshots
    struct TraceSnapShot
    {
        char     name[kMaxSnapshotNameLen];
        /// 64bit timestamp of the snapshot.
        uint64_t snapshotPoint;
        /// Size in bytes of the snapshot name.
        uint32_t nameLength;
        uint32_t version;
    };

    /// Header written for each stream
    struct TraceStreamHeader
    {
        DevDriver::ProcessId processId;
        uint32_t             threadId;
        size_t               totalDataSize;
        uint32_t             streamIndex;
        uint16_t             rmtMajorVersion;
        uint16_t             rmtMinorVersion;
    };

    /// Enums representing the various heap types
    enum DDHeapType
    {
        DD_HEAP_TYPE_LOCAL     = 0,
        DD_HEAP_TYPE_INVISIBLE = 1,
        DD_HEAP_TYPE_SYSTEM    = 2,
        DD_HEAP_TYPE_COUNT     = 3,
    };

    /// Info for a heap
    struct TraceHeapInfo
    {
        DDHeapType type;
        uint64_t   physicalBaseAddress;
        uint64_t   size;
    };

public:
    RmtEventTracer(
        const LoggerUtil&       logger,
        const DDAllocCallbacks& pApiAlloc);

    ~RmtEventTracer();

    DD_RESULT BeginTrace(
        ProcessId              processId,
        DDNetConnection        hConnection,
        DDClientId             gfxKernelId,
        DDClientId             amdLogId,
        DDClientId             umdId,
        DDClientId             routerId,
        const Vector<uint8_t>& systemInfoBuffer,
        bool                   useKmd);

    DD_RESULT EndTrace(EndTraceReason endReason, bool isClientInitialized);

    /// Inserts a snapshot into the trace
    DD_RESULT InsertSnapshot(
        /// Name of the snapshot
        const char* pSnapshotName,
        /// Timestamp from the target machine that indicates when the snapshot
        /// was taken.
        uint64_t snapshotTimestamp);

    DD_RESULT TransferTraceData(
        const DDIOHeartbeat*     pIoCb,
        rdfChunkFileWriter*      pRdfChunkWriter,
        bool                     useCompression,
        const TraceChunkApiInfo* pApiInfoChunk = nullptr);

    /// Clears the internal contents of the data context and resets it back to
    /// its initial state
    void Clear();

    TraceState GetTraceState() const { return m_traceState; }

    EndTraceReason GetEndTraceReason() const { return m_endReason; }

    uint64_t GetTotalDataSize() const { return static_cast<uint64_t>(Platform::AtomicGet(&m_totalDataSize)); }

    DD_RESULT GetTraceResult() const { return m_traceResult; }

private:
    /// Begins a memory trace
    void BeginTraceInternal(const system_info_utils::SystemInfo& systemInfo);

    /// Acquires a data stream
    /// The caller can write RMT tokens into these streams via WriteDataStream and the streams will later be written out
    /// into RMT chunks in the final trace output file.
    /// Once EndTrace is called, all existing stream ids are invalidated
    DD_RESULT AcquireDataStream(uint32_t* pDataStreamId, ProcessId processId, uint32_t threadId);

    /// Updates the RMT version in a data stream
    DD_RESULT WriteRmtVersion(uint32_t dataStreamId, uint16_t rmtMajorVersion, uint16_t rmtMinorVersion);

    /// Writes data into an existing stream
    DD_RESULT WriteDataStream(uint32_t dataStreamId, const void* pData, size_t dataSize);

    /// Ends a memory trace
    DD_RESULT EndTraceInternal(EndTraceReason reason, bool isDataValid);

    DD_RESULT WriteSavedChunks(rdfChunkFileWriter* pRdfChunkWriter);

    /// Returns true if there's currently running
    bool IsTraceRunning() const { return (m_traceState == TraceState::Running); }

private:
    /// Structure used to manage an individual data stream
    /// The data associated with each stream is buffered on disk until it's written into the main trace output file.
    struct TraceDataStream
    {
        FILE*           pFileHandle;
        ProcessId       processId;
        uint32_t        threadId;
        size_t          totalDataSize;
        uint16_t        rmtMajorVersion;
        uint16_t        rmtMinorVersion;
        Platform::Mutex streamMutex;
    };

    void ProcessSystemInfo(const system_info_utils::SystemInfo& systemInfo);

    DD_RESULT TransferDataStream(
        rdfChunkFileWriter*    pRdfStream,
        TraceDataStream*       pStream,
        uint8_t*               pScratchBuffer,
        size_t                 scratchBufferSize,
        const DDIOHeartbeat*   pIoCb,
        rdfChunkCreateInfo*    pChunkInfo);

    DD_RESULT TransferFileData(
        void*                pBuffer,
        size_t               bufferSize,
        FILE*                pSourceFile,
        const DDIOHeartbeat* pIoCb,
        rdfChunkCreateInfo*  pChunkInfo,
        rdfChunkFileWriter*  pRdfChunkWriter);

    void DiscardDataStreams();
    void UpdateTraceResult(DD_RESULT result);

    void LogInfo(const char* pFmt, ...);
    void LogError(const char* pFmt, ...);

    DDAllocCallbacks                 m_apiAlloc;                  /// Api allocation callbacks
    AllocCb                          m_ddAlloc;                   /// DevDriver allocation callbacks
    TraceState                       m_traceState;                /// Current state of the memory trace
    EndTraceReason                   m_endReason;                 /// Reason for the end of the trace
    Vector<TraceDataStream>          m_dataStreams;               /// Array of data streams that are part of the trace
    Platform::Atomic64               m_totalDataSize;             /// Total data size of the memory trace in bytes
    DD_RESULT                        m_traceResult;               /// The final result value for the trace operation
    int                              m_currentChunkIndex;         /// The chunk idx used in calls to Create/Begin chunks
    TraceHeapInfo                    m_heaps[DD_HEAP_TYPE_COUNT]; /// The heap info
    TraceAdapterInfo                 m_adapterInfo;               /// The adapter info
    DevDriver::Vector<TraceSnapShot> m_snapshots;                 /// A vector containing each snapshot taken
    std::string                      m_sysInfoJson;               /// SysInfo as Json

    RmtEventStreamer* m_pKmdStreamer;
    RmtEventStreamer* m_pUmdStreamer;
    RmtEventStreamer* m_pRouterStreamer;

    LoggerUtil m_logger;
};

} // namespace DevDriver
