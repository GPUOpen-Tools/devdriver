/* Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <stdint.h>

#define DD_RDF_USERSTREAM_INTERFACE_VERSION_MAJOR 1
#define DD_RDF_USERSTREAM_INTERFACE_VERSION_MINOR 4
#define DD_RDF_USERSTREAM_INTERFACE_VERSION_PATCH 0

/// Note that DDRdfFileWriter mirrors the rdfUserStream, some caveats:
///     Writing a file requires us to seek back to the start and also tell the current position
///     GetSize and Read are not required right now, but that is not an API guarantee
///     The return types are int here, but should be RDF return values:
///         rdfResultOk = 0,
///         rdfResultError = 1,
///         rdfResultInvalidArgument = 2

/// Read count bytes into buffer
typedef int (*PFN_ddFileRead)(
    void*         pUserData,   /// [in]  Userdata pointer
    const int64_t count,       /// [in]  Num bytes
    void*         pBuffer,     /// [in]  Buffer data, buffer can be null only if count is 0
    int64_t*      pBytesRead); /// [out] Optional. Set if it is non-null, number of bytes

/// Write count bytes from buffer
typedef int (*PFN_ddFileWrite)(
    void*         pUserData,      /// [in]  Userdata pointer
    const int64_t count,          /// [in]  Num bytes
    const void*   pBuffer,        /// [in]  Buffer data, buffer can be null only if count is 0
    int64_t*      pBytesWritten); /// [out] Optional. Set if it is non-null, number of bytes

// Gets the current position
typedef int (*PFN_ddFileTell)(
    void*         pUserData,  /// [in]  Userdata pointer
    int64_t*      pPosition); /// [out] Current Position, must no be null

/// Set the current position
typedef int (*PFN_ddFileSeek)(
    void*   pUserData,  /// [in]  Userdata pointer
    int64_t position);  /// [in]  Position to set

/// Get the size
typedef int (*PFN_ddFileGetSize)(
    void*    pUserData,  /// [in]  Userdata pointer
    int64_t* pSize);     /// [out] Size, must not be null

struct DDRdfFileWriter
{
    void*                  pUserData;
    PFN_ddFileRead         pfnFileRead;
    PFN_ddFileWrite        pfnFileWrite;
    PFN_ddFileTell         pfnFileTell;
    PFN_ddFileSeek         pfnFileSeek;
    PFN_ddFileGetSize      pfnFileGetSize;
};

// Validate a DDRdfFileWriter object
// This can handle NULL and should be checked before using the writer.
// Note that GetSize is currently not required by RDF and isn't check here.
inline bool IsValidDDRdfFileWriter(const DDRdfFileWriter* pRdfFileWriter)
{
    return ((pRdfFileWriter != nullptr)              &&
        (pRdfFileWriter->pfnFileRead  != nullptr)    &&
        (pRdfFileWriter->pfnFileWrite != nullptr)    &&
        (pRdfFileWriter->pfnFileTell  != nullptr)    &&
        (pRdfFileWriter->pfnFileSeek  != nullptr));
}
