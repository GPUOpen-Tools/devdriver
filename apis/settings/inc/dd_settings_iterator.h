/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_settings_api.h>
#include <dd_settings_rpc_types.h>

namespace DevDriver
{

/// This class helps users iterate through settings components and their values packed in a raw buffer.
class SettingsIterator
{
public:
    struct Component
    {
        // A pointer to null-terminated component name string.
        const char* pName;

        // Hash value of the settings JSON blob of this component.
        uint64_t    blobHash;

        // The number of settings values in this component.
        uint16_t    numValues;

        // An opaque offset representing a settings component. Users must not modify this value.
        size_t      offset;

        Component()
            : pName{nullptr}
            , numValues{0}
            , offset{0}
        {}
    };

    struct Value
    {
        DDSettingsValueRef valueRef;

        // An opaque offset representing a settings component. Users must not modify this value.
        size_t offset;

        Value()
            : valueRef{}
            , offset{0}
        {}
    };

    struct UnsupportedExperiment
    {
        DD_SETTINGS_NAME_HASH hash;

        // An opaque offset representing a settings component. Users must not modify this value.
        size_t offset;

        UnsupportedExperiment()
            : hash{},
              offset{ 0 }
        {
        }
    };

private:
    const uint8_t* m_pBuf;
    size_t         m_bufSize;

    DDSettingsAllComponentsHeader m_allComponentsHeader;

    DD_RESULT m_error;

public:
    /// @param pBuf A pointer to a buffer holding settings data.
    /// @param size The size of the buffer.
    SettingsIterator(const uint8_t* pBuf, size_t size);

    ~SettingsIterator() = default;

    /// Get the next component in the settings data.
    ///
    /// @param[in,out] pComponent A pointer to an existing \ref SettingsIterator.Component to receive the next
    /// component data. To get the first component, the pointed to object must be zero-initialized.
    /// @return true if a valid component is found, false otherwise.
    bool NextComponent(Component* pComponent);

    /// Get the next setting value of the current component in the settings data.
    ///
    /// @param[in,out] pValue A pointer to an existing \ref SettingsIterator.Value to receive the next value data.
    /// To get the first value, the pointed to object must be zero-initialized.
    /// @return true if a valid value is found, false otherwise.
    bool NextValue(const Component& component, Value* pValue);

    /// Get the next experiment support info of the current component in the settings data.
    ///
    /// @param[in,out] pExp A pointer to an existing \ref SettingsIterator.UnsupportedExperiment to receive the next support data.
    /// To get the first value, the pointed to object must be zero-initialized.
    /// @return true if a valid support info is found, false otherwise.
    bool NextUnsupportedExperiment(const Component& component, UnsupportedExperiment* pExp);
};

} // namespace DevDriver
