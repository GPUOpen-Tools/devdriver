/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "protocols/ddTransferProtocol.h"

namespace DevDriver
{
    namespace TransferProtocol
    {
        // ============================================================================================================
        TransferHeader::TransferHeader(TransferMessage message)
            : command(message)
        {
        }

        // ============================================================================================================
        TransferRequest::TransferRequest(BlockId blockId, TransferType type, uint32 size)
            : command(TransferMessage::TransferRequest)
            , blockId(blockId)
            , type(type)
            , sizeInBytes(size)
        {
        }

        // ============================================================================================================
        TransferDataHeader::TransferDataHeader(Result result, uint32 size)
            : command(TransferMessage::TransferDataHeader)
            , result(result)
            , sizeInBytes(size)
        {
        }

        // ============================================================================================================
        TransferDataHeaderV2::TransferDataHeaderV2(uint32 size)
            : command(TransferMessage::TransferDataHeader)
            , sizeInBytes(size)
        {
        }

        // ============================================================================================================
        void TransferDataChunk::WritePayload(
            const void* pData,
            size_t bytesToSend,
            SizedPayloadContainer* pContainer)
        {
            pContainer->payloadSize = static_cast<uint32>(bytesToSend + offsetof(TransferDataChunk, data));
            DD_ASSERT(pContainer->payloadSize <= kMaxPayloadSizeInBytes);
            TransferDataChunk& payload = pContainer->GetPayload<TransferDataChunk>();
            payload.command = TransferMessage::TransferDataChunk;
            Platform::Memcpy_s(&payload.data[0], kMaxTransferDataChunkSize, pData, bytesToSend);
        }

        // ============================================================================================================
        TransferDataSentinel::TransferDataSentinel(Result result, uint32 crc32)
            : command(TransferMessage::TransferDataSentinel)
            , result(result)
            , crc32(crc32)
        {
        }

        // ============================================================================================================
        TransferStatus::TransferStatus(Result result)
            : command(TransferMessage::TransferStatus)
            , result(result)
        {
        }
    }
}
