/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_settings_api.h>

#pragma pack(push, 1)
struct DDSettingsAllComponentsHeader
{
    uint16_t version;
    uint16_t numComponents;
};

struct DDSettingsComponentHeader
{
    // The name of the component, null terminated.
    char name[DD_SETTINGS_MAX_COMPONENT_NAME_SIZE];
    // The hash value of the JSON blob of this component.
    uint64_t blobHash;
    // The number of values in the component.
    uint16_t numValues;
    // The size of this header plus the size of all values immediately following this header.
    uint32_t size;
};

struct DDSettingsValueHeader
{
    DD_SETTINGS_NAME_HASH hash;
    uint8_t type;  // DD_SETTINGS_TYPE
    uint16_t valueSize; // The size of value data immediately following this header.
};

struct DDSettingsSiphonQuerySettingsBlobsAllParams
{
    /// Client driver type.
    DD_SETTINGS_DRIVER_TYPE driverType;

    /// Whether to reload settings blobs or use the cached data.
    bool reload;

    /// The size of the absolute path of driver to override, including null-terminator. If 0, the
    /// default path is used.
    uint16_t driverPathOverrideSize;
};

/// @brief Wire format for WriteKernelSettingOverride RPC params.
///
/// Layout: [DDSettingsSiphonWriteKernelSettingOverrideParams][settingName\0][stringValue\0 (if type==kString)]
/// settingNameSize includes the null terminator.
/// stringValueSize is non-zero only when type == DD_SETTINGS_TYPE_STRING; includes null terminator.
struct DDSettingsSiphonWriteKernelSettingOverrideParams
{
    DDGpuId  gpuId;           ///< GPU identified by bus<<16 | device<<8 | function.
    uint8_t  type;            ///< DD_SETTINGS_TYPE of the value.
    uint64_t numericValue;    ///< Raw numeric bits (unused for string type).
    uint16_t settingNameSize; ///< Byte length of the null-terminated setting name that follows.
    uint16_t stringValueSize; ///< Byte length of the null-terminated string value (0 if not kString).
};

#pragma pack(pop)

static_assert(sizeof(DDSettingsAllComponentsHeader) == 4, "Unexpected size for DDSettingsAllComponentsHeader.");

static_assert(
    sizeof(DDSettingsComponentHeader) == DD_SETTINGS_MAX_COMPONENT_NAME_SIZE + 14,
    "Unexpected size for DDSettingsComponentHeader.");

static_assert(sizeof(DDSettingsValueHeader) == 7, "Unexpected size for DDSettingsValueHeader.");

static_assert(sizeof(DDSettingsSiphonWriteKernelSettingOverrideParams) == 17, "Unexpected size for DDSettingsSiphonWriteKernelSettingOverrideParams.");
