/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "gpuopen.h"
#include "util/string.h"

namespace DevDriver
{

size_t DecodeFromHexString(const char* pStrBuff, size_t strLength, void* pBytesOut, size_t numBytes)
{
    uint8* pBytes = static_cast<uint8*>(pBytesOut);

    // Byte offset that we've written into pBytes
    size_t bytesProcessed = 0;

    // Note: Only even-length hex strings are supported
    if ((strLength % 2 == 0) && (pBytes != nullptr) && (numBytes != 0) && (pStrBuff != nullptr) && (strLength != 0))
    {
        size_t byteIdx = 0;

        // Process two characters (one byte) per iteration.
        // This loop is bounded on two sizes: the string buffer and the byte buffer
        for (size_t strIdx = 0;
            ((strIdx + 1) < strLength) && (byteIdx < numBytes);
            strIdx += 2, byteIdx += 1)
        {
            const uint8 hi = HexDigitToValue(pStrBuff[strIdx + 0]); // High nibble first
            const uint8 lo = HexDigitToValue(pStrBuff[strIdx + 1]); // Low nibble

            if ((lo != 0xff) && (hi != 0xff))
            {
                pBytes[byteIdx] = static_cast<uint8>((hi << 4) | lo);
                bytesProcessed += 1;
            }
            else
            {
                // Non-hex digit encountered, this is a parsing error.
                // This log statement is compiled out, but may be useful for debugging something funny.
                DD_PRINT(LogLevel::Never,
                    "[DecodeFromHexString] Expected hex digits ([0-9a-fA-F]), but found \"%c%c\"",
                    pStrBuff[strIdx + 0],
                    pStrBuff[strIdx + 1]);
                break;
            }
        }
    }

    return bytesProcessed;
}

} // namespace DevDriver
