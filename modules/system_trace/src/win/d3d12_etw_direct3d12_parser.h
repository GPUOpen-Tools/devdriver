//=============================================================================
/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @file
/// @brief ETW Parser for Microsoft-Windows-Direct3D12 provider.
//=============================================================================

#pragma once

#include <unordered_map>

namespace DevDriver
{
    /// @brief The "Microsoft-Windows-Direct3D12" ETW provider's GUID.
    static constexpr const wchar_t* kDirect3D12ProviderGuid = L"{5d8087dd-3a9b-4f56-90df-49196cdc4f11}";

    /// @brief Structure representing the named object.
    struct DebugObjectNameEvent
    {
        /// @brief Default constructor.
        DebugObjectNameEvent() = default;

        /// @brief Constructor.
        ///
        /// @param [in] name           The object_name The name of the object.
        /// @param [in] key_identifier The object identifier.
        DebugObjectNameEvent(std::string& name, ULONGLONG key_identifier)
            : object_name(name)
            , key_identifier(key_identifier)
        {
        }

        std::string object_name;         ///< The name string for the object.
        ULONGLONG   key_identifier = 0;  ///< The object identifier for the name.
    };

    namespace direct3d12_etw_parser
    {
        /// @brief The "pObject" property for the ETW Name event.
        static constexpr const wchar_t* kEtwDirect3D12NameEventPObjectProperty = L"pObject";

        /// @brief The "NewDebugObjectName" property for the ETW Name event.
        static constexpr const wchar_t* kEtwDirect3D12NameEventNewDebugObjectNameProperty = L"NewDebugObjectName";

        /// @brief The "pID3D12Resource" property for the ETW Resource event.
        static constexpr const wchar_t* kEtwDirect3D12ResourceEventPId3D12ResourceProperty = L"pID3D12Resource";

        /// @brief The "HeapType" property for the ETW Resource event.
        static constexpr const wchar_t* kEtwDirect3D12ResourceEventHeapTypeProperty = L"HeapType";

        /// @brief The "hUMResource" property for the ETW Resource event.
        static constexpr const wchar_t* kEtwDirect3D12ResourceEventHUMResourceProperty = L"hUMResource";

        /// @brief The "hKMAllocation" property for the ETW Heap event.
        static constexpr const wchar_t* kEtwDirect3D12HeapEventHKMAllocationProperty = L"hKMAllocation";

        /// @brief The "pID3D12Heap" property for the ETW Heap event.
        static constexpr const wchar_t* kEtwDirect3D12HeapEvenPId3D12HeapProperty = L"pID3D12Heap";

        /// @brief The "Type" property for the ETW Heap event.
        static constexpr const wchar_t* kEtwDirect3D12HeapEventTypeProperty = L"Type";

        ///  @brief Heap Types for the D3D12Resource ETW event.
        enum class HeapType
        {
            kHeapTypeImmutableHeap    = 0,
            kHeapTypeImplicitHeap     = 1,
            kHeapTypeImplicitResource = 2,
            kHeapTypeReservedResource = 3
        };

        /// @brief Template structure used to signify the pointer size for ETW properties (64bit or 32 bit)
        template <bool Is32Bit>
        struct PointerSize
        {
        };

        /// @brief Specialized pointer size template for indicating 64bit values.
        template <>
        struct PointerSize<false>
        {
            using Type = ULONGLONG;
        };

        /// @brief Specialized pointer size template for indicating 32bit values.
        template <>
        struct PointerSize<true>
        {
            using Type = ULONG;
        };

        /// @brief Sets the Pointer type to be a PointerSize<true> struct type for 32bit,
        /// or PointerSize<false> struct type for 64bit.
        template <bool Is32Bit>
        using Pointer = typename PointerSize<Is32Bit>::type;

        /// @brief Enum defining the ETW event type.
        enum class Event
        {
            kUnknown,          ///< Unsupported event type.
            kDebugObjectName,  ///< Object Name event type.
            kResource,         ///< Resource event type.
            kHeap              ///< Heap event type.
        };

        // Microsoft-Windows-Direct3D12 ETW event types.

        static constexpr WCHAR kEtwDx12DebugObjectEventNameString[] = L"Name";      ///< Event name for debug object events.
        static constexpr WCHAR kEtwDx12ResourceEventString[]        = L"Resource";  ///< Event name for a resource event.
        static constexpr WCHAR kEtwDx12HeapEventString[]            = L"Heap";      ///< Event name for a heap event.

        /// @brief Convert a Microsoft-Windows-Direct3D12 ETW event type string to an enumerated value.
        ///
        /// @param [in] type_string The ETW event string to convert.
        ///
        /// @return The ETW event type enumeration.
        inline enum Event GetEventType(const wchar_t* type_string)
        {
            if (wcsncmp(type_string, kEtwDx12DebugObjectEventNameString, sizeof(kEtwDx12DebugObjectEventNameString)) == 0)
            {
                return Event::kDebugObjectName;
            }

            if (wcsncmp(type_string, kEtwDx12ResourceEventString, sizeof(kEtwDx12ResourceEventString)) == 0)
            {
                return Event::kResource;
            }

            if (wcsncmp(type_string, kEtwDx12HeapEventString, sizeof(kEtwDx12HeapEventString)) == 0)
            {
                return Event::kHeap;
            }

            return Event::kUnknown;
        }

    }  // namespace direct3d12_etw_parser
}  // namespace DevDriver
