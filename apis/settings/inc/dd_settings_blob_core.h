/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

namespace DevDriver
{

struct SettingsBlob
{
    /// The total size of this object.
    /// Computed as the blob size plus the size of this struct and any alignment required.
    /// When multiple blobs are stored in one buffer, use `size` to get the relative
    /// offset to the next blob.
    uint32_t size;
    /// The size in bytes of the blob payload.
    uint32_t blobSize;
    /// Whether the blob is encoded.
    bool encoded;
    /// The starting offset of the magic buffer used for encoding.
    uint32_t magicOffset;
    /// hash of the blob
    uint64_t blobHash;
    /// A variable-sized byte array, representing a Settings blob.
    uint8_t blob[1];
};

/// All Settings blobs are packed in one buffer. This struct always sit at the
/// very beginning of the buffer. Each blob is prefixed with a `SettingsBlob`.
struct SettingsBlobsAll
{
    /// The version of the schema based on which Settings blobs are packed.
    /// Bump this number when either `SettingsBlobsAll` or `SettingsBlob`
    /// changes. `version` must always be the FIRST field in this struct.
    uint32_t version;
    /// The number of blobs in a buffer.
    uint32_t nblobs;
};

}
