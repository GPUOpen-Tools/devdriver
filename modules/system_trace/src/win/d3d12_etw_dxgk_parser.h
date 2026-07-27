//=============================================================================
/* Copyright (C) 2017-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief  Definitions for DirectX Graphics Kernel ETW event parsing.
//=============================================================================

#pragma once

#include <map>
#include <queue>
#include <set>
#include <unordered_map>

#include "ddPlatform.h"
#include "gpuopen.h"
#include "protocols/etwProtocol.h"

namespace DevDriver
{
    static constexpr const wchar_t* kDxgKernelProviderGuid = L"{802ec45a-1e99-4b83-9920-87c98277ba9d}";  ///< The DXGKernel ETW provider's GUID.

    namespace dxgk_etw_parser
    {
        /// @brief The common fields that each queue event has.
        struct CommonQueueEvent
        {
            uint64       timestamp;           ///< The timestamp of the event.
            uint64       context_identifier;  ///< The identifier of the event's context.
            uint32       sequence;            ///< The event sequence number.
            GpuEventType type;                ///< The type of this event.
        };

        /// @brief Functor for comparing CommonQueueEvent.
        class EventLess
        {
        public:
            /// @brief Compares two CommonQueueEvent.
            /// @param left The first event to compare.
            /// @param right The second event to compare.
            /// @return true if the timestamp on the left event is before the right's timestamp.
            bool operator()(const CommonQueueEvent& left, const CommonQueueEvent& right)
            {
                return left.timestamp < right.timestamp;
            }
        };

        /// @brief The description of a fence object.
        struct FenceObject
        {
            uint64 fence_object;  ///< The fence object that the value is for.
            uint64 fence_value;   ///< The value of the fence object.
        };

        /// @brief Data for a sync submission event.
        struct QueueSyncSubmissionEvent : public CommonQueueEvent
        {
            std::vector<FenceObject> fences;  ///< Information about several fence objects.
        };

        /// @brief Data for a sync completion event.
        struct QueueSyncCompletionEvent : public CommonQueueEvent
        {
        };

        /// @brief Container for storing QueueSyncSubmissionEvents.
        struct EventStorage
        {
            /// @brief The storage for QueueSyncSubmissionEvents.
            ///
            /// The keys for the first layer of maps are context_identifiers. The keys for the second
            /// layer are sequence numbers.
            std::unordered_map<uint64, std::unordered_map<uint32, QueueSyncSubmissionEvent>> submission_events;
        };

#pragma warning(push)
        // enable MSVC specific extension that allows zero sized arrays in structs
#pragma warning(disable : 4200)
#pragma pack(push)
#pragma pack(1)

        /// @brief The data structure used to encode arrays in packets.
        /// @tparam T The type of element that the array is for.
        template <typename T>
        struct ArrayHeader
        {
            UINT32 count;     ///< The number of items in the array.
            T      values[];  ///< The values of the array (should have the same length as count).
        };

        /// @brief Contains Type which stores the length of a pointer on either a 64 or 32-bit system.
        /// @tparam Is32Bit true if the Type should be the size of a pointer
        ///         on a 32-bit system. false otherwise.
        template <bool Is32Bit>
        struct PointerSize
        {
        };

        /// @brief Pointer size for 32-bit systems.
        template <>
        struct PointerSize<false>
        {
            using Type = ULONGLONG;
        };

        /// @brief Pointer size for 64-bit systems.
        template <>
        struct PointerSize<true>
        {
            using Type = ULONG;
        };

        /// @brief Declaration for a pointer on either a 64-bit or 32-bit system.
        /// @tparam Is32Bit true if Pointer should be a pointer on a 32-bit system.
        ///                 false otherwise.
        template <bool Is32Bit>
        using Pointer = typename PointerSize<Is32Bit>::Type;

        /// @brief The different types of sync queue events.
        enum struct CommandBufferType : UINT32
        {
            kRender   = 0,
            kMmioFlip = 3,
            kWait     = 4,
            kSignal   = 5,
            kDevice   = 6,
            kSoftware = 7,
            kPaging   = 8,
        };

        /// @brief The basic header for a QueueFence event.
        /// @tparam Is32Bit Whether or not the header is for a 32 or a 64-bit system.
        template <bool Is32Bit>
        struct QueueFenceHeader
        {
            Pointer<Is32Bit> h_context;  ///< The context for the event this header belongs to.
            UINT32           sequence;   ///< The sequence number for th  this header belongs to.
            UINT32           flags;      ///< The flag for the event this header belongs to.
        };

        /// @brief The header for a wait packet.
        /// @tparam Is32Bit Whether or not the header is for a 32 or a 64-bit system.
        template <bool Is32Bit>
        struct WaitPacketHeader : QueueFenceHeader<Is32Bit>
        {
            Pointer<Is32Bit> h_sync_object;  ///< Pointer to the synchronization object.
            UINT64           fence_value;    ///< The value of the fence object.
        };

        /// @brief The header for a signal event.
        /// @tparam Is32Bit Whether or not the header is for a 32 or a 64-bit system.
        template <bool Is32Bit>
        struct SignalPacketHeader : QueueFenceHeader<Is32Bit>
        {
            /// @brief Fences.
            ArrayHeader<Pointer<Is32Bit>> semaphore;
        };

        /// @brief The header for a sync queue event.
        /// @tparam Is32Bit Whether or not the header is for a 32 or a 64-bit system.
        template <bool Is32Bit>
        struct SyncQueuePacketHeader
        {
            Pointer<Is32Bit>  h_context;    ///< The context for the event this header belongs to.
            CommandBufferType packet_type;  ///< The sequence number for the this header belongs to.
            UINT32            sequence;     ///< The type of the event this header belongs to.
        };

        // technically, the end packets have extra values after the common header. We don't actually use them though.
        //struct EndQueuePacket : SyncQueuePacketHeader
        //{
        //    UINT32 preempted;
        //    UINT32 timeout;
        //};

        /// @brief A packet for an associated Dxg scheduler object.
        struct AssociateDxgSchedulerObjectPacket
        {
            uint64 dxg_object  = 0;  ///< Identifier for the Dxg object.
            uint64 sch_object  = 0;  ///< The identifier for the scheduler object.
            uint64 kmd_handle  = 0;  ///< Handle of the kernel mode driver.
            uint64 h_os_handle = 0;  ///< The os handle.
        };

        /// @brief A packet for matching sync events to appropriate schedulers
        struct HwQueue
        {
            uint64 h_context      = 0;  ///< OS handle
            uint64 h_queue        = 0;  ///< Queue
            uint64 h_parent_queue = 0;  ///< Parent scheduler handle
        };

#pragma pack(pop)
#pragma warning(pop)
    }  // namespace dxgk_etw_parser

    /// @brief The different types of Etw trace events.
    enum struct Event
    {
        kUnknown,
        kQueuePacket,
        kAssociateDxgSchedulerObject,
        kHwQueue
    };

#pragma warning(push)
    // MSVC has a known issue where it triggers warnings for initializing static objects with literals.
#pragma warning(disable : 4592)
    // we define a static object to force the compiler to handle string hashing for us.
    // this should be const, but there are MSVC compiler bugs that prevent that.
    /// @brief Maps event names to event types.
    static std::unordered_map<std::wstring, Event> kObjectTypeMap = {{L"QueuePacket", Event::kQueuePacket},
                                                                     {L"AssociateDxgSchedulerObject", Event::kAssociateDxgSchedulerObject},
                                                                     {L"HwQueue", Event::kHwQueue}};
#pragma warning(pop)

    /// @brief The id of the event descriptor for queue packets.
    enum struct QueuePacketId : UINT32
    {
        kUnknown = 0,
        kInfo    = 0x00b3,
        kEnd     = 0x00b4,
        kWait    = 0x00f4,
        kSignal  = 0x00f5,
    };
}  // namespace DevDriver
