/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef DD_EVENT_PARSER_HEADER
#define DD_EVENT_PARSER_HEADER

#include <ddEventParserApi.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Attempts to create a new parser object with the provided creation information
DD_RESULT ddEventParserCreate(
    /// Create info
    const DDEventParserCreateInfo* pInfo,
    /// Handle to the new parser object
    DDEventParser*                 phParser);

/// Destroys an existing parser object
void ddEventParserDestroy(
    /// Handle to the existing parser object
    DDEventParser hParser);

/// Parses the provided buffer of formatted event data
///
/// Returns parsed data through the DDEventWriter that was provided during the parser creation
DD_RESULT ddEventParserParse(
    /// Handle to the existing parser object
    DDEventParser hParser,
    /// Pointer to a buffer that contains formatted event data
    const void*   pData,
    /// Size of the buffer pointed to by pData
    size_t        dataSize);

/// Create a new parser.
DD_RESULT ddEventParserCreateEx(DDEventParser* phParser);

/// Set the buffer to be parsed.
void ddEventParserSetBuffer(DDEventParser hParser, const void* pBuffer, size_t size);

/// Parse the buffer. To parse a buffer, users should call this function
/// repeatedly and take actions based on the value it returns.
DD_EVENT_PARSER_STATE ddEventParserParseNext(DDEventParser hParser);

/// Get the info about the event received.
DDEventParserEventInfo ddEventParserGetEventInfo(DDEventParser hParser);

/// Get the info of the parsed data payload. Callers can use the returned info
/// to copy the payload data away. Note, the returned payload info might not be
/// complete. Callers can call `ddEventParserParseNext()` repeatedly to get
/// remaining payload.
DDEventParserDataPayload ddEventParserGetDataPayload(DDEventParser hParser);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

