/* Copyright (C) 2025-2026 Advanced Micro Devices, Inc. All rights reserved. */
#include <dd_settings_utils.h>
#include <dd_settings_blob.h>
#include <unordered_map>

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wpragmas"
    #pragma clang diagnostic ignored "-Wunknown-warning-option"
    #pragma clang diagnostic ignored "-Wunused-local-typedef"
#elif defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpragmas"
#endif

#include "rapidjson/document.h"
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#if defined(__clang__)
    #pragma clang diagnostic pop
#elif defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

namespace DevDriver
{
namespace SettingsUtils
{

// Helper function to safely get string value from JSON member
static const char* SafeGetStringValue(const rapidjson::Value& obj, const char* memberName, const char* defaultValue = "")
{
    const auto member = obj.FindMember(memberName);
    if (member != obj.MemberEnd() && member->value.IsString())
    {
        return member->value.GetString();
    }
    return defaultValue;
}

static DD_SETTINGS_TYPE SettingTypeFromString(const std::string& typeStr)
{
    static const std::unordered_map<std::string, DD_SETTINGS_TYPE> kSettingsTypeMappings = {
        { "bool", DD_SETTINGS_TYPE_BOOL },     { "float", DD_SETTINGS_TYPE_FLOAT },
        { "int8", DD_SETTINGS_TYPE_INT8 },     { "int16", DD_SETTINGS_TYPE_INT16 },
        { "int32", DD_SETTINGS_TYPE_INT32 },   { "int64", DD_SETTINGS_TYPE_INT64 },
        { "uint8", DD_SETTINGS_TYPE_UINT8 },   { "uint16", DD_SETTINGS_TYPE_UINT16 },
        { "uint32", DD_SETTINGS_TYPE_UINT32 }, { "uint64", DD_SETTINGS_TYPE_UINT64 },
        { "string", DD_SETTINGS_TYPE_STRING }, { "enum", DD_SETTINGS_TYPE_UINT32 }
    };

    if (kSettingsTypeMappings.count(typeStr) == 0)
    {
        printf("Invalid type found = %s\n", typeStr.c_str());
        return DD_SETTINGS_TYPE_BOOL;
    }

    return kSettingsTypeMappings.at(typeStr);
}

std::string SettingTypeToString(DD_SETTINGS_TYPE type)
{
    switch (type)
    {
        case DD_SETTINGS_TYPE_BOOL: return "bool";
        case DD_SETTINGS_TYPE_FLOAT: return "float";
        case DD_SETTINGS_TYPE_INT8: return "int8";
        case DD_SETTINGS_TYPE_INT16: return "int16";
        case DD_SETTINGS_TYPE_INT32: return "int32";
        case DD_SETTINGS_TYPE_INT64: return "int64";
        case DD_SETTINGS_TYPE_UINT8: return "uint8";
        case DD_SETTINGS_TYPE_UINT16: return "uint16";
        case DD_SETTINGS_TYPE_UINT32: return "uint32";
        case DD_SETTINGS_TYPE_UINT64: return "uint64";
        case DD_SETTINGS_TYPE_STRING: return "string";
        default: return "unknown";
    }
}

std::string SettingValueToString(const SettingValue& value, DD_SETTINGS_TYPE type)
{
    if (value.isOptional)
    {
        return "";
    }

    switch (type)
    {
        case DD_SETTINGS_TYPE_BOOL: return value.numVal.b ? "true" : "false";
        case DD_SETTINGS_TYPE_INT8: return std::to_string(value.numVal.i8);
        case DD_SETTINGS_TYPE_UINT8: return std::to_string(value.numVal.u8);
        case DD_SETTINGS_TYPE_INT16: return std::to_string(value.numVal.i16);
        case DD_SETTINGS_TYPE_UINT16: return std::to_string(value.numVal.u16);
        case DD_SETTINGS_TYPE_INT32: return std::to_string(value.numVal.i32);
        case DD_SETTINGS_TYPE_UINT32: return std::to_string(value.numVal.u32);
        case DD_SETTINGS_TYPE_INT64: return std::to_string(value.numVal.i64);
        case DD_SETTINGS_TYPE_UINT64: return std::to_string(value.numVal.u64);
        case DD_SETTINGS_TYPE_FLOAT: return std::to_string(value.numVal.f);
        case DD_SETTINGS_TYPE_STRING: return value.strVal;
        default: return "unknown";
    }
}

// Helper function to assign a value from JSON to a SettingValue based on type
static void AssignValueByType(SettingValue* pValue, DD_SETTINGS_TYPE type, const rapidjson::Value& jsonValue)
{
    switch (type)
    {
        case DD_SETTINGS_TYPE_BOOL: pValue->numVal.b = jsonValue.GetBool(); break;
        case DD_SETTINGS_TYPE_INT8: pValue->numVal.i8 = static_cast<int8_t>(jsonValue.GetInt()); break;
        case DD_SETTINGS_TYPE_UINT8: pValue->numVal.u8 = static_cast<uint8_t>(jsonValue.GetUint()); break;
        case DD_SETTINGS_TYPE_INT16: pValue->numVal.i16 = static_cast<int16_t>(jsonValue.GetInt()); break;
        case DD_SETTINGS_TYPE_UINT16: pValue->numVal.u16 = static_cast<uint16_t>(jsonValue.GetUint()); break;
        case DD_SETTINGS_TYPE_INT32: pValue->numVal.i32 = jsonValue.GetInt(); break;
        case DD_SETTINGS_TYPE_UINT32:
            pValue->numVal.u32 = 0;
            if (jsonValue.IsUint())
            {
                pValue->numVal.u32 = jsonValue.GetUint();
            }
            else if (jsonValue.IsString())
            {
                // Todo: Properly handle strings like "0xFFFF"
                // For example "PrimCompressionFlags" in DXCP
            }
            else if (jsonValue.IsInt())
            {
                // Get as an int to workaround default of -1
                pValue->numVal.u32 = jsonValue.GetInt();
            }
            break;
        case DD_SETTINGS_TYPE_INT64: pValue->numVal.i64 = jsonValue.GetInt64(); break;
        case DD_SETTINGS_TYPE_UINT64: pValue->numVal.u64 = jsonValue.GetUint64(); break;
        case DD_SETTINGS_TYPE_FLOAT: pValue->numVal.f = jsonValue.GetFloat(); break;
        case DD_SETTINGS_TYPE_STRING: pValue->strVal = jsonValue.GetString(); break;
        default: printf("Invalid Type\n");
    }
}

// Helper function to fill setting values from a JSON object that contains defaults
static void FillSettingsValue(SettingValue* pValue, DD_SETTINGS_TYPE type, const rapidjson::Value& valueObj)
{
    if (valueObj.HasMember("Defaults"))
    {
        const auto defaultsField   = valueObj.FindMember("Defaults");
        const auto defaultValField = defaultsField->value.FindMember("Default");
        if (defaultValField != defaultsField->value.MemberEnd())
        {
            AssignValueByType(pValue, type, defaultValField->value);
            return;
        }
    }
    else if (valueObj.HasMember("Default"))
    {
        const auto defaultValField = valueObj.FindMember("Default");
        AssignValueByType(pValue, type, defaultValField->value);
        return;
    }

    // If no default is present, it is assumed to be optional
    pValue->isOptional = true;
}

// Helper function to fill valid values from a JSON object that contains a "Value" field
static void FillValidValue(SettingValue* pValue, DD_SETTINGS_TYPE type, const rapidjson::Value& valueObj)
{
    if (valueObj.HasMember("Value"))
    {
        const auto valueField = valueObj.FindMember("Value");
        AssignValueByType(pValue, type, valueField->value);
    }
    else
    {
        // If no value is present, mark as optional
        pValue->isOptional = true;
    }
}

void UpdateSetting(rapidjson::Value::ConstValueIterator itr, SettingsData* pData)
{
    const auto nameField = itr->FindMember("Name");
    pData->name          = nameField->value.GetString();

    const auto descriptionField = itr->FindMember("Description");
    pData->description          = descriptionField->value.GetString();

    const auto nameHashField = itr->FindMember("NameHash");
    pData->nameHash          = nameHashField->value.GetUint();

    const auto typeField = itr->FindMember("Type");
    pData->type          = SettingTypeFromString(typeField->value.GetString());

    FillSettingsValue(&pData->value, pData->type, *itr);

    if (itr->HasMember("ValidValues"))
    {
        const auto validValuesField = itr->FindMember("ValidValues");

        SettingValidValues validValues = {};
        validValues.name               = SafeGetStringValue(*itr, "Name");
        validValues.desc               = SafeGetStringValue(*itr, "Description");
        validValues.isEnum             = (itr->FindMember("Type")->value.GetString() == std::string("enum"));

        if (validValuesField->value.HasMember("Values"))
        {
            validValues.hasValidValues = true;
            const auto valuesField     = validValuesField->value.FindMember("Values");

            if (valuesField->value.IsArray())
            {
                // Handle array of values
                for (const auto& value : valuesField->value.GetArray())
                {
                    SettingValidValue validValue = {};
                    if (value.IsObject())
                    {
                        validValue.name              = SafeGetStringValue(value, "Name");
                        validValue.desc              = SafeGetStringValue(value, "Description");
                        validValue.logicOp           = SafeGetStringValue(value, "LogicOp");

                        FillValidValue(&validValue.value, pData->type, value);
                    }
                    else
                    {
                        //raw values
                        AssignValueByType(&validValue.value, pData->type, value);
                    }

                    validValues.values.push_back(validValue);
                }
            }
            else
            {
                // Handle single value object
                const auto& value = valuesField->value;
                SettingValidValue validValue = {};

                if (value.IsObject())
                {
                    validValue.name              = value.FindMember("Name")->value.GetString();
                    validValue.desc              = value.FindMember("Description")->value.GetString();
                    validValue.logicOp           = value.FindMember("LogicOp")->value.GetString();

                    FillValidValue(&validValue.value, pData->type, value);
                }
                else
                {
                    //raw value
                    AssignValueByType(&validValue.value, pData->type, value);
                }

                validValues.values.push_back(validValue);
            }
        }

        pData->validValues = validValues;

    }
}

DD_RESULT ParseSettingsBlobs(const char* pBlobBuffer, size_t bufferSize, std::vector<SettingComponent>& output, DDConnectionId connectionId)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;
    const SettingsBlobsAll* pSettingsBlobAllHeader = nullptr;
    if ((pBlobBuffer != nullptr) && (bufferSize > 0))
    {
        // The settings blob all header starts after the settings path size and path.
        if (connectionId != DD_SETTINGS_DRIVER_TYPE_KMD)
        {
            uint32_t offset = *reinterpret_cast<const uint16_t*>(pBlobBuffer);
            pSettingsBlobAllHeader = reinterpret_cast<const SettingsBlobsAll*>(pBlobBuffer + offset + 2);
        }
        else
        {
            //for KMD we do not provide the settings path size or path so header starts at offset 0
            uint32_t offset = 0;
            pSettingsBlobAllHeader = reinterpret_cast<const SettingsBlobsAll*>(pBlobBuffer);
        }

        if (pSettingsBlobAllHeader->nblobs > 0)
        {
            // Skip past the header to get the first blob.
            const SettingsBlob* pBlob = reinterpret_cast<const SettingsBlob*>(pSettingsBlobAllHeader + 1);
            for (uint32_t blob = 0; blob < pSettingsBlobAllHeader->nblobs; ++blob)
            {
                if (pBlob->blobSize == 0)
                {
                    continue;
                }

                rapidjson::Document document;
                document.Parse((const char*)pBlob->blob, pBlob->blobSize);

                const rapidjson::Value& componentName = document["ComponentName"];
                SettingComponent        component     = {};
                if (componentName.IsString())
                {
                    component.name = componentName.GetString();
                }

                const rapidjson::Value& settings = document["Settings"];

                rapidjson::Value::ConstValueIterator itr = settings.Begin();
                for (; itr != settings.End(); ++itr)
                {
                    if (itr->HasMember("Structure") == false)
                    {
                        SettingsData setting = {};
                        UpdateSetting(itr, &setting);
                        component.settings.push_back(setting);
                    }
                    else
                    {
                        // For structures, each member has its own hash, value, etc
                        // @ToDo: Should we group these some how?
                        const rapidjson::Value& strct      = itr->FindMember("Structure")->value;
                        auto                    strItr     = strct.GetArray().Begin();
                        const auto              nameField  = itr->FindMember("Name");
                        std::string             structName = nameField->value.GetString();
                        for (; strItr != strct.End(); strItr++)
                        {
                            SettingsData setting = {};
                            setting.structName   = structName;
                            UpdateSetting(strItr, &setting);
                            component.settings.push_back(setting);
                        }
                    }
                }
                pBlob = reinterpret_cast<const DevDriver::SettingsBlob*>(reinterpret_cast<const uint8_t*>(pBlob) +
                                                                         pBlob->size);
                output.push_back(component);
            }

            result = DD_RESULT_SUCCESS;
        }
        else
        {
            printf("Driver settings were successfully queried but the queried data didn't contain any blobs.");
            result = DD_RESULT_SETTINGS_SERVICE_INVALID_SETTING_DATA;
        }
    }

    return result;
}

SettingValue ConvertValueRef(const DDSettingsValueRef& valueRef)
{
    SettingValue settingValue = {};
    settingValue.isOptional = valueRef.isOptional;

    if (valueRef.isOptional && (valueRef.pValue == nullptr))
    {
        // Optional setting with no value
        return settingValue;
    }

    switch (static_cast<DD_SETTINGS_TYPE>(valueRef.type))
    {
        case DD_SETTINGS_TYPE_BOOL:
            settingValue.numVal.b = *static_cast<const bool*>(valueRef.pValue);
            break;
        case DD_SETTINGS_TYPE_INT8:
            settingValue.numVal.i8 = *static_cast<const int8_t*>(valueRef.pValue);
            break;
        case DD_SETTINGS_TYPE_UINT8:
            settingValue.numVal.u8 = *static_cast<const uint8_t*>(valueRef.pValue);
            break;
        case DD_SETTINGS_TYPE_INT16:
            settingValue.numVal.i16 = *static_cast<const int16_t*>(valueRef.pValue);
            break;
        case DD_SETTINGS_TYPE_UINT16:
            settingValue.numVal.u16 = *static_cast<const uint16_t*>(valueRef.pValue);
            break;
        case DD_SETTINGS_TYPE_INT32:
            settingValue.numVal.i32 = *static_cast<const int32_t*>(valueRef.pValue);
            break;
        case DD_SETTINGS_TYPE_UINT32:
            settingValue.numVal.u32 = *static_cast<const uint32_t*>(valueRef.pValue);
            break;
        case DD_SETTINGS_TYPE_INT64:
            settingValue.numVal.i64 = *static_cast<const int64_t*>(valueRef.pValue);
            break;
        case DD_SETTINGS_TYPE_UINT64:
            settingValue.numVal.u64 = *static_cast<const uint64_t*>(valueRef.pValue);
            break;
        case DD_SETTINGS_TYPE_FLOAT:
            settingValue.numVal.f = *static_cast<const float*>(valueRef.pValue);
            break;
        case DD_SETTINGS_TYPE_STRING:
            if (valueRef.pValue != nullptr)
            {
                settingValue.strVal = static_cast<const char*>(valueRef.pValue);
            }
            break;
        default:
            // Unknown or unsupported type
            break;
    }

    return settingValue;
}

std::string SettingValueRefToString(const DDSettingsValueRef& valueRef)
{
    SettingValue value = ConvertValueRef(valueRef);
    return SettingValueToString(value, static_cast<DD_SETTINGS_TYPE>(valueRef.type));
}

} // namespace SettingsUtils

} // namespace DevDriver
