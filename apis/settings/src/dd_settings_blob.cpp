/* Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_settings_blob.h>
#include <string.h>
#include <ddPlatform.h>

namespace DevDriver
{

uint32_t CalcSettingsBlobSizeAligned(uint32_t blobSize)
{
    // Align to 8 bytes on 32-bit and 64-bit machines.
    const uint32_t WORD_SIZE = sizeof(uint64_t);

    // `unalignedSize` equals the offset of `blob[blobSize]` relative to the
    // beginning of `SettingsBlob`. Can't use `offsetof` because `blobSize` is
    // not a constant.
    size_t unalignedSize = (size_t)&(((SettingsBlob*)0)->blob[blobSize]);

    uint32_t alignedSize = (unalignedSize + WORD_SIZE - 1) & ~(WORD_SIZE - 1);

    return alignedSize;
}

SettingsBlobNode* SettingsBlobNode::s_pFirst = nullptr;
SettingsBlobNode* SettingsBlobNode::s_pLast = nullptr;

SettingsBlobNode::SettingsBlobNode()
{
    if (s_pLast == nullptr)
    {
        s_pFirst = this;
        s_pLast = this;
    }
    else
    {
        DD_ASSERT(s_pFirst != nullptr);
        s_pLast->m_pNext = this;
        s_pLast = this;
    }

}

uint32_t SettingsBlobNode::GetAllSettingsBlobs(uint8_t* pBuffer, uint32_t bufferSize)
{
    uint32_t totalSizeRequired = 0;

    const uint32_t blobsAllHeaderSize = sizeof(SettingsBlobsAll);
    totalSizeRequired += blobsAllHeaderSize;

    uint8_t* pBlobBufCurrPos = nullptr;
    uint8_t* pBlobBufEnd = nullptr;
    if (pBuffer != nullptr)
    {
        pBlobBufEnd = pBuffer + bufferSize;
        pBlobBufCurrPos = pBuffer + blobsAllHeaderSize; // account for `SettingsBlobsAll`
    }

    uint32_t blobCount = 0;

    SettingsBlobNode* pCurr = s_pFirst;
    while (pCurr != nullptr)
    {
        uint32_t blobSize = 0;
        const uint8_t* pBlobData = pCurr->GetBlob(&blobSize);
        if (blobSize > 0)
        {
            uint32_t blobSizeAligned = CalcSettingsBlobSizeAligned(blobSize);

            totalSizeRequired += blobSizeAligned;
            DD_ASSERT(totalSizeRequired >= blobSizeAligned); // overflow check

            if (pBlobBufCurrPos)
            {
                uint8_t* pBlobBufNextPos = pBlobBufCurrPos + blobSizeAligned;

                if (pBlobBufNextPos <= pBlobBufEnd)
                {
                    SettingsBlob* pBlob = reinterpret_cast<SettingsBlob*>(pBlobBufCurrPos);
                    pBlob->size         = blobSizeAligned;
                    pBlob->blobSize     = blobSize;
                    pBlob->magicOffset  = pCurr->GetMagicOffset();
                    pBlob->encoded      = pCurr->IsEncoded();
                    pBlob->blobHash     = pCurr->GetBlobHash();

                    DevDriver::Platform::Memcpy_s(pBlob->blob, blobSize, pBlobData, blobSize);

                    blobCount += 1;
                }

                // Advance to next buffer position regardless whether `pBlobBufCurrPos`
                // has already passed `pBlobBufEnd`. If it's already past end, we won't
                // copy blobs into the buffer.
                pBlobBufCurrPos = pBlobBufNextPos;
            }
            else
            {
                // no-op, just calculate and return required size
            }
        }

        pCurr = pCurr->Next();
    }

    if ((pBuffer != nullptr) && (bufferSize >= blobsAllHeaderSize))
    {
        SettingsBlobsAll* pBlobsAll = reinterpret_cast<SettingsBlobsAll*>(pBuffer);
        pBlobsAll->version = 1;
        pBlobsAll->nblobs = blobCount;
    }

    return totalSizeRequired;
}

} // namespace DevDriver
