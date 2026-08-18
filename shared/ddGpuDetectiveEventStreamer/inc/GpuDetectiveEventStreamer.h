/* Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <amdrdf.h>
#include <stdio.h>

#include <ddCommon.h>
#include <ddEventClient.h>
#include <ddEventParser.h>

#include <dd_event/common.h>

class GPUDetectiveStreamer
{

public:
    GPUDetectiveStreamer(const LoggerUtil& logger);
    ~GPUDetectiveStreamer();

    DD_RESULT BeginStreaming(
        DDClientId      clientId,
        DDNetConnection hConn,
        uint32_t        providerId);

    DD_RESULT EndStreaming(bool isClientAlive);

    uint64_t GetTotalDataSize() { return static_cast<uint64_t>(DevDriver::Platform::AtomicGet(&m_totalDataSize)); }

    DD_RESULT TransferDataStream(
        const DDIOHeartbeat& ioHeartbeat,
        rdfChunkFileWriter*  pRdfChunkWriter,
        bool                 useCompression);

    bool HasCrashOccured();

    void ResetCrashBoolean();

private:

    void OnEventData(const void* pData, size_t dataSize);

    DD_RESULT EventWritePayloadChunk(const DDEventParserEventInfo* pEvent, const void* pData, size_t dataSize);

    static void EventPullingThreadFn(void* pUserdata);

    DD_RESULT Init(
        DDClientId      clientId,
        DDNetConnection hConnection,
        uint32_t        providerId);

    void LogInfo(const char* pFmt, ...);
    void LogError(const char* pFmt, ...);
    void LogErrorOnFailure(bool condition, const char* pFmt, ...);

    DDEventParser                 m_hEventParser;
    DDEventClient                 m_hEventClient;
    uint32_t                      m_providerId;
    bool                          m_isStreaming;
    bool                          m_exitRequested;
    bool                          m_errorOccurred;
    DevDriver::Platform::Thread   m_eventThread;
    LoggerUtil                    m_logger;
    FILE*                         m_pStreamFile;
    DevDriver::Platform::Atomic64 m_totalDataSize;
    DevDriver::Platform::Mutex    m_streamMutex;
    DDEventProviderHeader         m_rdfChunkHeader; // Need to collect timestamp info from first event
    bool                          m_foundFirstEvent;
    bool                          m_crashEventOccured;
};
