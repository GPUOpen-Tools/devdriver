/* Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_common_api.h>
#include <dd_settings_api.h>
#include <stdint.h>
#include <dd_settings_blob_core.h>

namespace DevDriver
{

/// Each subclass of `SettingsBlobNode` holds a raw buffer of Settings data string
/// blob, and is intended to be linked in a global linked list. All `SettingsBlobNode`s
/// can be received together in one buffer.
class SettingsBlobNode
{
public:
    /// A pointer to the first `SettingsBlob` in the global list.
    static SettingsBlobNode* s_pFirst;
    static SettingsBlobNode* s_pLast;

private:
    SettingsBlobNode* m_pNext;

public:
    SettingsBlobNode();

    /// Return a pointer to the raw Settings data string blob. The byte-size of
    /// the Settings blob is written to `pOutSize`. Note, the byte-size does
    /// not include the null-terminator at the end of the string blob (if it
    /// has one).
    virtual const uint8_t* GetBlob(uint32_t* pOutSize) = 0;

    /// Return whether this blob is encoded.
    virtual bool IsEncoded() = 0;

    /// Return the starting offset of the magic buffer used for encoding.
    virtual uint32_t GetMagicOffset() = 0;

    /// Return the hash of the blob.
    virtual uint64_t GetBlobHash() = 0;

    /// Return a pointer to the next `SettingsBlob` in the global linked list.
    SettingsBlobNode* Next() const
    {
        return m_pNext;
    }

    /// Fill the `pBuffer` with Settings blobs from all linked `SettingsBlobNode`s. All
    /// Settings blobs are packed into one buffer. See `SettingsBlobsAll` to learn how they
    /// are packed.
    ///
    /// @param pBuffer A pointer to a buffer to receive all Settings blobs. It can be nullptr.
    /// @param bufferSize The size of \param pBuffer.
    ///
    /// @return The size required for a buffer to receive all Settings blobs, regardless of
    /// whether \param pBuffer is nullptr.
    static uint32_t GetAllSettingsBlobs(uint8_t* pBuffer, uint32_t bufferSize);

};

} // namespace DevDriver
