/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <util/ddByteReader.h>

namespace DevDriver
{
    size_t ByteReader::Remaining() const
    {
        const uint8* pCur      = reinterpret_cast<const uint8*>(m_pCur);
        const uint8* pEnd      = reinterpret_cast<const uint8*>(m_pEnd);
        const auto   remaining = (pEnd - pCur);

        DD_ASSERT(remaining >= 0);
        return static_cast<size_t>(remaining);
    }

    bool ByteReader::HasBytes() const
    {
        return (Remaining() > 0);
    }

    Result ByteReader::GetBytes(const void** ppData, size_t dataSize)
    {
        Result result = Result::InvalidParameter;

        if ((ppData != nullptr) && (dataSize > 0))
        {
            if (dataSize <= Remaining())
            {
                *ppData = m_pCur;
                m_pCur  = VoidPtrInc(m_pCur, dataSize);

                result = Result::Success;
            }
            else
            {
                result = Result::Error;
            }
        }

        return result;
    }

    Result ByteReader::ReadBytes(void* pDst, size_t dstSize, size_t numBytes)
    {
        Result result = Result::InvalidParameter;

        if ((pDst != nullptr) && (dstSize >= numBytes) && (numBytes > 0))
        {
            const void* pBytes = nullptr;
            result = GetBytes(&pBytes, numBytes);
            if (result == Result::Success)
            {
                Platform::Memcpy_s(pDst, dstSize, pBytes, numBytes);
            }
        }

        return result;
    }
}
