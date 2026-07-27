/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include "gpuopen.h"
#include "ddPlatform.h"
#include "ddTransferProtocol.h"

/*
***********************************************************************************************************************
* URI Protocol
***********************************************************************************************************************
*/

#define URI_PROTOCOL_VERSION 3

#define URI_PROTOCOL_MINIMUM_VERSION 1

/*
***********************************************************************************************************************
*| Version | Change Description                                                                                       |
*| ------- | ---------------------------------------------------------------------------------------------------------|
*|  3.0    | Added support for POST data.                                                                             |
*|  2.0    | Added support for response data formats.                                                                 |
*|  1.0    | Initial version                                                                                          |
***********************************************************************************************************************
*/

#define URI_POST_PROTOCOL_VERSION 3
#define URI_RESPONSE_FORMATS_VERSION 2
#define URI_INITIAL_VERSION 1

namespace DevDriver
{
    namespace URIProtocol
    {
        using BlockId = TransferProtocol::BlockId;
        ///////////////////////
        // GPU Open URI Protocol
        enum struct URIMessage : MessageCode
        {
            Unknown = 0,
            URIRequest,
            URIResponse,
            URIPostRequest,
            URIPostResponse,
            Count,
        };

        enum struct RequestType : uint32
        {
            Get = 0,
            Post,
            Put,
            Count
        };

        ///////////////////////
        // URI Types
        enum struct TransferDataFormat : uint32
        {
            Unknown = 0,
            Text,
            Binary,
            Count
        };

        using ResponseDataFormat = TransferDataFormat;

        ///////////////////////
        // URI Constants
        DD_STATIC_CONST uint32 kURIStringSize = 256;
        DD_STATIC_CONST uint32 kLegacyMaxSize = 260; // Legacy packets are always kURIStringSize + 4 byte header

        ///////////////////////
        // URI Payloads

        DD_NETWORK_STRUCT(URIHeader, 4)
        {
            URIMessage  command;
            // pad out to 4 bytes for alignment requirements
            char        padding[3];

            constexpr URIHeader(URIMessage command)
                : command(command)
                , padding()
            {}
        };

        DD_CHECK_SIZE(URIHeader, 4);

        DD_NETWORK_STRUCT(URIRequestPayload, 4)
        {
            URIHeader          header;
            char               uriString[kURIStringSize];
            BlockId            blockId;         // valid only in v3 sessions or higher
            TransferDataFormat dataFormat;      // valid only in v3 sessions or higher
            uint32             dataSize;        // valid only in v3 sessions or higher

            URIRequestPayload(const char*        pRequest,
                              BlockId            block = TransferProtocol::kInvalidBlockId,
                              TransferDataFormat dataFormat = TransferDataFormat::Unknown,
                              uint32             size = 0);
        };

        DD_CHECK_SIZE(URIRequestPayload, 272);

        DD_NETWORK_STRUCT(URIResponsePayload, 4)
        {
            URIHeader          header;
            Result             result;
            BlockId            blockId;
            TransferDataFormat format;      // valid only in v2 sessions or higher
            uint32             dataSize;    // valid only in v3 sessions or higher

            URIResponsePayload(Result             status,
                               BlockId            block = TransferProtocol::kInvalidBlockId,
                               TransferDataFormat format = TransferDataFormat::Unknown,
                               uint32             size = 0);
        };

        DD_CHECK_SIZE(URIResponsePayload, 20);

        DD_NETWORK_STRUCT(URIPostRequestPayload, 4)
        {
            URIHeader header;
            char      uriString[kURIStringSize];
            uint32    dataSize;

            URIPostRequestPayload(const char* pRequest, uint32 size);
        };

        DD_CHECK_SIZE(URIPostRequestPayload, 264);

        DD_NETWORK_STRUCT(URIPostResponsePayload, 4)
        {
            URIHeader header;
            Result    result;
            BlockId   blockId;

            URIPostResponsePayload(Result  status,
                                   BlockId block);
        };

        DD_CHECK_SIZE(URIPostResponsePayload, 12);

        // Helper Functions
        DD_STATIC_CONST size_t kMaxInlineDataSize = sizeof(SizedPayloadContainer::payload) - sizeof(URIRequestPayload);

        void* GetInlineDataPtr(SizedPayloadContainer* pPayload);
    }

}
