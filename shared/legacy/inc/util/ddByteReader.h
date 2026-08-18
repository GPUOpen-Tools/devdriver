/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
#pragma once

#include <ddPlatform.h>

namespace DevDriver
{
    //---------------------------------------------------------------------
    // An object that reads from a byte range into value types.
    // ByteReader performs bounds checking and automatically sizes the reads for the appropriate data type.
    class ByteReader
    {
    public:
        ByteReader(const void* pData, size_t dataSize)
            : m_pCur(pData),
              m_pEnd(VoidPtrInc(pData, dataSize))
        {}

        ByteReader(const ByteReader&) = default;
        ByteReader(ByteReader&&) = default;
        ByteReader& operator=(const ByteReader&) = delete;
        ByteReader& operator=(ByteReader&&) = delete;

        // Get the number of remaining bytes in the range.
        size_t Remaining() const;

        // Returns true if there are more bytes to read
        bool HasBytes() const;

        // Returns a pointer to a sized subset of the byte array based on the current position
        // Fails if there are not enough bytes remaining, or if ppValue is NULL.
        Result GetBytes(const void** ppData, size_t dataSize);

        // Returns a type pointer at the current byte array position
        // Fails if there are not enough bytes remaining, or if ppValue is NULL.
        template <typename T>
        Result Get(const T** ppValue)
        {
            static_assert(!Platform::IsPointer<T>::Value, "Don't read pointers from byte arrays.");

            return GetBytes(reinterpret_cast<const void**>(ppValue), sizeof(T));
        }

        // Copies data from bytes to the buffer pointer provided.
        // Fails if there are not enough bytes remaining, if pValue is NULL, or if dstSize is not large enough.
        Result ReadBytes(void* pDst, size_t dstSize, size_t numBytes);

        // Copies data from bytes to the value type pointer provided.
        // Fails if there are not enough bytes remaining, or if pValue is NULL.
        template <typename T>
        Result Read(T* pValue, size_t destSize)
        {
            static_assert(!Platform::IsPointer<T>::Value, "Don't read pointers from byte arrays.");

            Result result = Result::InvalidParameter;

            if (pValue != nullptr)
            {
                result = ReadBytes(reinterpret_cast<void*>(pValue), destSize, sizeof(T));
            }

            return result;
        }

        // Move the reading cursor forward numBytes, as if a struct of size numBytes was read with Read().
        // Fails if there are not enough bytes remaining. The current position remains unchanged on failure.
        Result Skip(size_t numBytes)
        {
            Result result = Result::Error;
            if (numBytes <= Remaining())
            {
                m_pCur = VoidPtrInc(m_pCur, numBytes);
                result = Result::Success;
            }
            return result;
        }

    private:
        const void* m_pCur;       // Current byte position
                                  // This is incremented as data is read/get
        const void* const m_pEnd; // End byte position
                                  // This is set during the constructor and never changed. It's used to calculate the
                                  // number of bytes remaining.
    };

    // Get infers a size from the type and void* has no size.
    // Use GetBytes() with an explicit size instead.
    template <>
    Result ByteReader::Get(const void** ppValue) = delete;

    // Read infers a size from the type and void* has no size.
    // Use ReadBytes() with an explicit size instead.
    template <>
    Result ByteReader::Read(void* pValue, size_t destSize) = delete;
}
