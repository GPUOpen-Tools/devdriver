/* Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddApi.h>
#include <ddCommon.h>
#include <ddPlatform.h>
#include <ddEventParserApi.h>
#include <ddEventClientApi.h>

namespace DevDriver
{

class RmtEventTracer;

class RmtEventStreamer
{
private:
    enum class TraceState : uint32_t
    {
        NotStarted = 0,
        Running,
        Ended
    };

    /// Structure used to manage an individual data stream. The data associated
    /// with each stream is buffered on disk until it's written into the main
    /// trace output file.
    struct TraceDataStream
    {
        FILE*                      pFileHandle;
        DevDriver::ProcessId       processId;
        uint32_t                   threadId;
        size_t                     totalDataSize;
        uint16_t                   rmtMajorVersion;
        uint16_t                   rmtMinorVersion;
        DevDriver::Platform::Mutex streamMutex;
    };

public:
    RmtEventStreamer(RmtEventTracer* pTracer, const LoggerUtil& logger);
    ~RmtEventStreamer();

    DD_RESULT BeginStreaming(
        DDClientId      clientId,
        DDNetConnection hConnection,
        uint32_t        dataStreamId,
        uint32_t        providerId);

    DD_RESULT EndStreaming(bool isClientAlive);

    bool IsStreaming() const { return m_isStreaming; }
    bool HasEncounteredErrors() const { return m_encounteredErrors; }

private:
    DD_RESULT Init(DDClientId clientId, DDNetConnection hConnection, uint32_t providerId);

    void OnEventData(const void* pData, size_t dataSize);

    DD_RESULT EventBegin(const DDEventParserEventInfo* pEvent, uint64_t totalPayloadSize);
    DD_RESULT EventWritePayloadChunk(const DDEventParserEventInfo* pEvent, const void* pData, size_t dataSize);
    DD_RESULT EventEnd(const DDEventParserEventInfo* pEvent, DD_RESULT finalResult);

    void LogInfo(const char* pFmt, ...);
    void LogError(const char* pFmt, ...);

    static void EventReceiveThreadFunc(void* pUserdata);

    // This struct defines the RmtVersion event format expected from the UMD
    // provider
    struct RmtVersionEvent
    {
        uint16_t majorVersion;
        uint16_t minorVersion;
    };

    DDEventClient               m_hEventClient;
    DDEventParser               m_hEventParser;
    uint32_t                    m_dataStreamId;
    uint32_t                    m_providerId;
    DevDriver::Platform::Thread m_eventThread;
    uint32_t                    m_expectedEventDataIndex;
    bool                        m_expectedEventDataIndexValid;
    bool                        m_exitRequested;
    bool                        m_isStreaming;
    bool                        m_encounteredErrors;
    uint8_t                     m_rmtVersionScratch[sizeof(RmtVersionEvent)];
    size_t                      m_rmtVersionScratchOffset;
    RmtEventTracer*             m_pRmtTracer;
    LoggerUtil                  m_logger;
};

} // namespace DevDriver
