/* Copyright (C) 2025-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_settings_api.h>
#include <string>
#include <vector>

namespace DevDriver
{
namespace SettingsUtils
{

struct SettingValue
{
    bool operator==(const SettingValue& other) const
    {
        return (numVal.all == other.numVal.all) && (strVal == other.strVal) && (isOptional == other.isOptional);
    }

    union
    {
        bool  b;
        float f;

        int8_t  i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;

        uint8_t  u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;

        uint64_t all;

    } numVal; // The numerical value of the setting.

    std::string strVal; // The string value of the setting.
    bool        isOptional;
};

struct SettingValidValue
{
    std::string  name;    // The name of the value.
    std::string  desc;    // The description of the value.
    std::string  logicOp; // The logical operation for the value.
    SettingValue value;   // The value of the value.
};

struct SettingValidValues
{
    std::string name; // The name of the enum.
    std::string desc; // The description of the enum.

    bool isEnum;
    bool is64Bit;
    bool hasValidValues;

    std::vector<SettingValidValue> values; ///< The values of the enum.
};

// @ToDo: Populate valid values, tags, etc as needed
struct SettingsData
{
    std::string           name;
    std::string           description;
    std::string           structName; // Only valid if it is part of a struct
    DD_SETTINGS_NAME_HASH nameHash;
    DD_SETTINGS_TYPE      type;
    SettingValue          value;
    SettingValidValues    validValues; // Valid values for the setting, if applicable
};

struct SettingComponent
{
    std::string               name;
    std::vector<SettingsData> settings;
};

DD_RESULT ParseSettingsBlobs(const char* pBlobBuffer, size_t bufferSize, std::vector<SettingComponent>& output, DDConnectionId connectionId = DD_SETTINGS_DRIVER_TYPE_UNKNOWN);

std::string SettingTypeToString(DD_SETTINGS_TYPE type);

std::string SettingValueToString(const SettingValue& value, DD_SETTINGS_TYPE type);

SettingValue ConvertValueRef(const DDSettingsValueRef& valueRef);

std::string SettingValueRefToString(const DDSettingsValueRef& valueRef);

} // namespace SettingsUtils

} // namespace DevDriver
