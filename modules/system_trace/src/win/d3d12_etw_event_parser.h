//=============================================================================
/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief Class definition and implementation for Direct3D12 ETW event parsing.
//=============================================================================

#ifndef DEVTOOLS_ROUTER_D3D12_ETW_EVENT_PARSER_H
#define DEVTOOLS_ROUTER_D3D12_ETW_EVENT_PARSER_H

#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <Tdh.h>
#include <evntcons.h>
#include <evntprov.h>
#include <evntrace.h>
#include <objbase.h>
#include <tchar.h>

#include <ModuleLogger.h>
#include <ddApi.h>

#include "ddPlatform.h"
#include "gpuopen.h"
#include "protocols/etwProtocol.h"
#include "util/queue.h"

#include "d3d12_etw_direct3d12_parser.h"
#include "d3d12_etw_dxgk_parser.h"

#ifdef _DEBUG
#define _ENABLE_ETW_LOGGING_
#endif

/// @brief Callback definition used to emit debug name events.
///
/// @param [in] name               The name of the resource.
/// @param [in] key                The correlation key used to match the name with a resource.
/// @param [in] emitter_instance   The instance to handle the event for.
/// @param [in] creation_timestamp The timestamp of the first ETW event creation time.
///
/// @return DD_RESULT_SUCCESS or an error code if the operation is unsuccessful.
using PFNDebugNameEventCallbackType = DD_RESULT (*)(const char* name, ULONGLONG key, void* emitter_instance, const uint64_t creation_timestamp);

/// @brief Callback definition used to emit implicit resource events.
/// @param [in] resource_id        The resource ID (that matches the ID in the RESOURCE_CREATE token).
/// @param [in] emitter_instance   The instance to handle the event for.
/// @param [in] creation_timestamp The timestamp of the first ETW event creation time.
/// @param [in] heap_type          The heap type associated with this resource.
///
/// @return DD_RESULT_SUCCESS or an error code if the operation is unsuccessful.
using PFNImplicitResourceEventCallbackType = DD_RESULT (*)(ULONGLONG      resource_id,
                                                           void*          emitter_instance,
                                                           const uint64_t creation_timestamp,
                                                           const uint8_t  heap_type);

namespace DevDriver
{
    /// @brief The size of the buffer used to read a Tdh property into.
    static constexpr size_t kTdhPropertyBufferSize = 8192;

    /// @brief Constant representing an invalid timestamp.
    static constexpr uint64_t kInvalidTimestamp = UINT64_MAX - 1;

    /// @brief Function that determines if a string has 8-bit or 16-bit wide characters.
    ///
    /// @param [in] buffer        The string buffer to test.
    /// @param [in] buffer_length The number of bytes in the buffer (including zero termination character).
    ///
    /// @return true if the string is Unicode, otherwise returns false.
    inline bool IsUnicode(const char* buffer, int buffer_length)
    {
        return buffer_length > 2 && buffer[1] == 0;
    }

    /// @brief Function object that compares GPUEvents based on their submission time.
    class GpuEventComparison
    {
    public:
        /// @brief Compares two events.
        /// @param lhs The left hand event to compare.
        /// @param rhs The right hand event to compare.
        /// @return true if the lhs event was submitted before the rhs event.
        inline bool operator()(const GpuEvent& lhs, const GpuEvent& rhs)
        {
            return lhs.submissionTime < rhs.submissionTime;
        }
    };

    /// @brief Storage for parsed events. Events are sorted by earliest submission time.
    using ParsedStorage = std::priority_queue<GpuEvent, std::vector<GpuEvent>, GpuEventComparison>;

    /// @brief Contains AssociateDxgSchedulerObjectPacket that have been received and parsed.
    using AssociationStorage = std::vector<dxgk_etw_parser::AssociateDxgSchedulerObjectPacket>;

    /// @brief Type used to associate sync events with correct hardware scheduler
    using HwQueueStorage = std::vector<dxgk_etw_parser::HwQueue>;

    ///  @brief The Resource types associated with name information.
    enum class ResourceNameType
    {
        kResourceNameTypeUnknown = 0,  ///< Resource name that doesn't have an associated resource handle yet.
        kResourceNameTypeImage,        ///< Resource name that is associated with an image resource handle.
        kResourceNameTypeHeap,         ///< Resource name that is associated with a heap resource handle.
        kResourceNameTypeBuffer,       ///< Resource name that is associated with a buffer resource handle.
        kResourceNameTypeCount         ///< The number of enumerated resource name types.
    };

#ifdef _ENABLE_ETW_LOGGING_
    ///  @brief Convert ResourceNameType enum value to a string.
    ///
    /// @param [in] type The resource name type.
    ///
    /// @return The resource name type string.
    static const char* GetResourceNameTypeString(ResourceNameType type)
    {
        switch (type)
        {
        case ResourceNameType::kResourceNameTypeImage:
            return "image";

        case ResourceNameType::kResourceNameTypeHeap:
            return "heap";

        case ResourceNameType::kResourceNameTypeBuffer:
            return "buffer";

        default:
            return "unknown";
        }
    }
#endif

    /// @brief Structure defining the debug name information for a resource object.
    ///
    /// The information is collected from multiple ETW events.  Once all information is available, the object is considered valid.
    /// Specifically, when naming resources in DirectX 12, a user would typically use the ID3D12Resource function SetName(). This comes in
    /// as 2 ETW events which are processed by TraceStorage::StoreDebugNameString and TraceStorage::StoreDebugNameHandle. These events can
    /// arrive in any order and the DebugNameEvent token is only emitted after both ETW events have been processed. If a resource is renamed
    /// (again, using SetName()), only one ETW event is sent, which is processed by TraceStorage::StoreDebugNameString.
    struct DebugNameInfo
    {
        /// @brief Constructor.
        inline DebugNameInfo() = default;

        /// @brief Method used to determine if the DebugNameInfo has valid data.
        ///
        /// @return true to indicate all required data has been collected, false is returned otherwise.
        inline bool IsValid() const
        {
            return ((!name.empty()) && (driver_handle != 0));
        }

        std::string      name;                                                             ///< The resource name.
        ULONGLONG        driver_handle      = 0;                                           ///< The driver handle used to correlate the name with a resource.
        ResourceNameType type               = ResourceNameType::kResourceNameTypeUnknown;  ///< The type of resource that the name is associcated with.
        uint64_t         creation_timestamp = UINT64_MAX;                                  ///< The timestamp when the first ETW event was created.
    };

    /// @brief Storage for the output of a parser.
    struct TraceStorage
    {
        ModuleLogger* logger = nullptr;  ///< The object used to log messages.
        ParsedStorage parsed_events;     ///< Storage for the events that have been parsed.

        /// @brief Parsed Dxgk events.
        ///
        /// Events are first grouped by their context identifier and then their sequence. So to access
        /// a particular event, you would write dxgk_events[context_identifier][sequence].
        dxgk_etw_parser::EventStorage                dxgk_events;
        AssociationStorage                           os_association;               ///< Dxgk scheduler objects associated with the OS.
        HwQueueStorage                               hw_queues;                    ///< HwQueue objects
        ProcessId                                    process_id = 0;               ///< The id of the process that the events belong to.
        std::unordered_map<ULONGLONG, DebugNameInfo> debug_name_map;               ///< Map of collected debug name information from ETW events.
        std::unordered_set<uint32_t>                 debug_correlation_ids;        ///< List of debug name correlation IDs received in ETW events.
        PFNDebugNameEventCallbackType        debug_name_event_callback = nullptr;  ///< Callback function used to emit the token for the debug name event.
        PFNImplicitResourceEventCallbackType implicit_resource_event_callback;     ///< Callback function used to emit the token for the implicit resource.

        void* emitter_instance = nullptr;  ///< The object instance for the token emitter.

        /// @brief Constructor.
        inline TraceStorage() = default;

        /// @brief Destructor.
        inline ~TraceStorage()
        {
            Clear();
        }

        /// @brief Clears any stray events that are stored in this object.
        ///
        /// This is needed to work on RS4 and RS5 with the RS5 work-around issue.
        inline void Clear()
        {
            parsed_events = ParsedStorage();
            dxgk_events.submission_events.clear();
            debug_name_map.clear();
            debug_correlation_ids.clear();
        }

        /// @brief Method to store the debug name string into the map.
        ///
        /// If all data has been collected, this method emits the debug name token.
        ///
        /// @param [in] key                The ID used to correlate name with a resource.
        /// @param [in] debug_object_name  The name of the resource.
        /// @param [in] etw_timestamp      The timestamp when the name string ETW event was created.
        ///
        /// @return DD_RESULT_SUCCESS or an error code if the operation is unsuccessful.
        inline DD_RESULT StoreDebugNameString(ULONGLONG key, std::string& debug_object_name, LARGE_INTEGER etw_timestamp)
        {
            auto& debug_name_info = debug_name_map[key];
            debug_name_info.name  = debug_object_name;

            // If the timestamp is invalid, the resource has probably been renamed, so update it with the current timestamp.
            if (debug_name_info.creation_timestamp == kInvalidTimestamp)
            {
                debug_name_info.creation_timestamp = etw_timestamp.QuadPart;
            }

#ifdef _ENABLE_ETW_LOGGING_
            logger->Verbose(
                "[direct3d12_etw_parser::StoreDebugNameString] Store resource debug name '%s' for ID 0x%llx (%s resource "
                "handle = 0x%x)",
                debug_name_info.name.c_str(),
                key,
                GetResourceNameTypeString(debug_name_info.type),
                debug_name_info.driver_handle);
#endif
            if (debug_name_info.IsValid())
            {
                // The timestamp should be set to the time that the first of the two ETW events was processed.
                DD_ASSERT(debug_name_info.creation_timestamp != UINT64_MAX);

                // Make sure the timestamp reflects the first ETW event that was created.
                debug_name_info.creation_timestamp = std::min<uint64_t>(debug_name_info.creation_timestamp, etw_timestamp.QuadPart);

                debug_name_event_callback(debug_name_info.name.c_str(), debug_name_info.driver_handle, emitter_instance, debug_name_info.creation_timestamp);

                // We still want to keep this map entry alive in case the resource is renamed over its lifetime. Mark the timestamp as invalid so that it
                // is updated if the resource is renamed.
                debug_name_map[key].creation_timestamp = kInvalidTimestamp;
            }
            else
            {
                // If both ETW events haven't been processed, then this is the first event. Update the timestamp.
                debug_name_info.creation_timestamp = etw_timestamp.QuadPart;
            }

            return DD_RESULT::DD_RESULT_SUCCESS;
        }

        /// @brief Forwards the implicit resource to emit the MARK_IMPLICIT_RESOURCE USERDATA token.
        ///
        /// @param [in] resource_id   The ID of the resource to be marked as implicitly created.
        /// @param [in] etw_timestamp The timestamp of when the ETW event was created.
        /// @param [in] heap_type     The HeapType property from the ETW Resource event.
        inline void MarkImplicitResource(ULONGLONG resource_id, LARGE_INTEGER etw_timestamp, const uint8_t heap_type)
        {
            implicit_resource_event_callback(resource_id, emitter_instance, etw_timestamp.QuadPart, heap_type);
        }

        /// @brief Method to Store the debug name handle that is used by RMT parsing tools to associate a name with a resource.
        ///
        /// If all data has been collected, this method emits the debug name token.
        ///
        /// @param [in] key           The ID used to correlate name with a resource.
        /// @param [in] driver_handle The resource handle.
        /// @param [in] resource_type The type of the resource handle.
        /// @param [in] timestamp     The timestamp when the name handle ETW event was created.
        ///
        /// @return DD_RESULT_SUCCESS or an error code if the operation is unsuccessful.
        inline DD_RESULT StoreDebugNameHandle(ULONGLONG key, ULONGLONG driver_handle, ResourceNameType resource_type, LARGE_INTEGER timestamp)
        {
            auto& debug_name_info = debug_name_map[key];
            // Lookup the truncated, 32 bit correlation ID.
            if (debug_correlation_ids.find(static_cast<uint32_t>(driver_handle)) != debug_correlation_ids.end())
            {
                // If the full 64bit correlation IDs match then this is a duplicate ETW event that can be ignored.
                // Otherwise it's a duplicate caused by truncating to 32 bits.
                if ((debug_name_info.driver_handle != 0) && (debug_name_info.driver_handle != driver_handle))
                {
                    return DD_RESULT::DD_RESULT_COMMON_ALREADY_EXISTS;
                }
            }
            debug_correlation_ids.insert(static_cast<uint32_t>(driver_handle));

            std::string name;
            if (debug_name_info.name.empty() == false)
            {
                name = debug_name_info.name;
            }
            else
            {
                name = "<unassigned>";
            }

            debug_name_info.type = resource_type;

#ifdef _ENABLE_ETW_LOGGING_
            logger->Verbose("[direct3d12_etw_parser::StoreDebugNameHandle] Store %s resource handle 0x%llx for ID 0x%llx (name = '%s')",
                            GetResourceNameTypeString(resource_type),
                            driver_handle,
                            key,
                            name.c_str());
#endif
            debug_name_info.driver_handle = driver_handle;
            if (debug_name_info.IsValid())
            {
                // The timestamp should be set to the time that the first of the two ETW events was processed.
                DD_ASSERT(debug_name_info.creation_timestamp != UINT64_MAX);

                // Make sure the timestamp reflects the first ETW event that was created.
                debug_name_info.creation_timestamp = std::min<uint64_t>(debug_name_info.creation_timestamp, timestamp.QuadPart);

                debug_name_event_callback(debug_name_info.name.c_str(), debug_name_info.driver_handle, emitter_instance, debug_name_info.creation_timestamp);

                // We still want to keep this map entry alive in case the resource is renamed over its lifetime. Mark the timestamp as invalid so that it
                // is updated if the resource is renamed.
                debug_name_map[key].creation_timestamp = kInvalidTimestamp;
            }
            else
            {
                // If both ETW events haven't been processed, then this is the first event. Update the timestamp.
                debug_name_info.creation_timestamp = timestamp.QuadPart;
            }

            return DD_RESULT::DD_RESULT_SUCCESS;
        }
    };

    /// @brief Prints the uppercase hex representation of the event userdata split into 32 bit chunks.
    /// @param trace_data The data of the trace (unused).
    /// @param event The event to print the userdata for.
    template <bool Is32Bit>
    inline void PrintPacketData(TraceStorage& trace_data, PEVENT_RECORD event)
    {
        const char*   base_address     = ((const char*)event->UserData);
        const uint32* iterator_pointer = reinterpret_cast<const uint32*>(base_address);
        const uint32* end_pointer      = reinterpret_cast<const uint32*>(base_address + event->UserDataLength);

        while (iterator_pointer < end_pointer)
        {
            printf(" %.8X\n", *iterator_pointer);
            iterator_pointer++;
        }
        printf("\n");
    }

    /// @brief Translates a context identifier into an Os handle by looking
    /// for an associate object with the provided context.
    ///
    /// If no matching associate object is found, then h_context is returned.
    /// @param trace_data The container for associate objects to search in.
    /// @param h_context The context identifier to translate.
    /// @return The translated context identifier.
    inline uint64 TranslateContext(TraceStorage& trace_data, uint64 h_context)
    {
        for (const auto& hwQueue : trace_data.hw_queues)
        {
            if (hwQueue.h_parent_queue == h_context)
            {
                h_context = hwQueue.h_context;
                break;
            }
        }

        for (const auto& associate : trace_data.os_association)
        {
            DD_ASSERT(associate.sch_object != h_context);
            DD_ASSERT(associate.kmd_handle != h_context);
            DD_ASSERT(associate.h_os_handle != h_context);
            if (h_context == associate.dxg_object)
            {
                return associate.h_os_handle;
            }
        }
        return h_context;
    }

    namespace dxgk_etw_parser
    {
        /// @brief Parses a wait packet from the userdata from the event and creates
        /// a QueueSyncSubmissionEvent from it.
        ///
        /// The parsed event is stored in the dxgk_events of the trace_data.
        ///
        /// @tparam true if the system is 32-bit, false otherwise.
        /// @param trace_data The place to store the parsed event.
        /// @param event The event to parse.
        template <bool Is32Bit>
        inline void ProcessWaitQueuePacket(TraceStorage& trace_data, PEVENT_RECORD event)
        {
            using WaitPacket = WaitPacketHeader<Is32Bit>;

            const char*       base_address = ((const char*)event->UserData);
            const WaitPacket* header       = reinterpret_cast<const WaitPacket*>(base_address);

            // create event and copy raw time into it
            QueueSyncSubmissionEvent queue_event = {};
            queue_event.type                     = GpuEventType::QueueWait;

            // TODO: Is this correct? Do we need to modify this at all?
            queue_event.timestamp          = static_cast<uint64>(event->EventHeader.TimeStamp.QuadPart);
            queue_event.context_identifier = TranslateContext(trace_data, header->h_context);
            queue_event.sequence           = header->sequence;
            queue_event.fences.emplace_back(FenceObject({header->h_sync_object, header->fence_value}));

            trace_data.dxgk_events.submission_events[queue_event.context_identifier][queue_event.sequence] = queue_event;
        }

        /// @brief Parses a signal packet from the userdata from the event and creates
        /// a QueueSyncSubmissionEvent from it.
        ///
        /// The parsed event is stored in the dxgk_events of the trace_data.
        ///
        /// @tparam true if the system is 32-bit, false otherwise.
        /// @param trace_data The place to store the parsed event.
        /// @param event The event to parse.
        template <bool Is32Bit>
        inline void ProcessSignalQueuePacket(TraceStorage& trace_data, PEVENT_RECORD event)
        {
            using SignalPacket               = SignalPacketHeader<Is32Bit>;
            const char*         base_address = ((const char*)event->UserData);
            const SignalPacket* header       = reinterpret_cast<const SignalPacket*>(base_address);
            // create event and copy raw time into it
            QueueSyncSubmissionEvent queue_event = {};
            queue_event.type                     = GpuEventType::QueueSignal;

            // TODO: Is this correct? Do we need to modify this at all?
            queue_event.timestamp          = static_cast<uint64>(event->EventHeader.TimeStamp.QuadPart);
            queue_event.context_identifier = TranslateContext(trace_data, header->h_context);
            queue_event.sequence           = header->sequence;

            const uint64* fence_values = (const uint64*)(((const char*)&header->semaphore.values[0]) + sizeof(uint64) * header->semaphore.count);
            for (UINT32 i = 0; i < header->semaphore.count; i++)
            {
                queue_event.fences.emplace_back(FenceObject({header->semaphore.values[i], fence_values[i]}));
            }

            trace_data.dxgk_events.submission_events[queue_event.context_identifier][queue_event.sequence] = queue_event;
        }

        /// @brief Finalizes a QueueSyncSubmissionEvent and creates a GPUEvent from it.
        ///
        /// If an event with the supplied context identifier and sequence was not found,
        /// this will do nothing.
        ///
        /// @param trace_data The data to pull the submission event from and store the GPUEvent in.
        /// @param type The type of the new GPUEvent.
        /// @param context The context identifier of the event to finalize.
        /// @param sequence The sequence of the event to finalize/
        /// @param timestamp The completion time of the new GPUEvent.
        template <bool Is32Bit>
        inline void FinalizeSyncQueuePacket(TraceStorage& trace_data, GpuEventType type, uint64 context, uint32 sequence, uint64 timestamp)
        {
            const auto& find_context = trace_data.dxgk_events.submission_events.find(context);
            if (find_context != trace_data.dxgk_events.submission_events.end())
            {
                const auto& find = find_context->second.find(sequence);
                if (find != find_context->second.end())
                {
                    const QueueSyncSubmissionEvent& submission_event = find->second;
                    if (sequence == submission_event.sequence && type == submission_event.type)
                    {
                        GpuEvent event                = {};
                        event.type                    = type;
                        event.submissionTime          = submission_event.timestamp;
                        event.completionTime          = timestamp;
                        event.queue.contextIdentifier = submission_event.context_identifier;
                        for (auto& fence : submission_event.fences)
                        {
                            event.queue.fenceObject = fence.fence_object;
                            event.queue.fenceValue  = fence.fence_value;
                            trace_data.parsed_events.emplace(event);
                        }
                    }
                }
            }
        }

        /// @brief Parses an end queue from the userdata from the event.
        ///
        /// This finalizes the sync queue packet that is specified in the provided
        /// end queue packet.
        ///
        /// @tparam true if the system is 32-bit, false otherwise.
        /// @param trace_data The place to store the parsed event.
        /// @param event The event to parse.
        template <bool Is32Bit>
        inline void ProcessSyncEndQueuePacket(TraceStorage& trace_data, PEVENT_RECORD event)
        {
            using SyncQueuePacket         = SyncQueuePacketHeader<Is32Bit>;
            const SyncQueuePacket* header = reinterpret_cast<const SyncQueuePacket*>(event->UserData);
            switch (header->packet_type)
            {
            case CommandBufferType::kSignal:
            {
                FinalizeSyncQueuePacket<Is32Bit>(trace_data,
                                                 GpuEventType::QueueSignal,
                                                 TranslateContext(trace_data, header->h_context),
                                                 header->sequence,
                                                 static_cast<uint64>(event->EventHeader.TimeStamp.QuadPart));
                break;
            }
            case CommandBufferType::kWait:
            {
                FinalizeSyncQueuePacket<Is32Bit>(trace_data,
                                                 GpuEventType::QueueWait,
                                                 TranslateContext(trace_data, header->h_context),
                                                 header->sequence,
                                                 static_cast<uint64>(event->EventHeader.TimeStamp.QuadPart));
                break;
            }
            default:
                break;
            }
        }

        /// @brief Parses queue packet depending on the type of packet.
        ///
        /// @tparam true if the system is 32-bit, false otherwise.
        /// @param trace_data The place to store the parsed event.
        /// @param event The event to parse.
        template <bool Is32Bit>
        inline void ParseQueuePacket(TraceStorage& trace_data, PEVENT_RECORD event)
        {
            switch (static_cast<QueuePacketId>(event->EventHeader.EventDescriptor.Id))
            {
            case QueuePacketId::kEnd:
                ProcessSyncEndQueuePacket<Is32Bit>(trace_data, event);
                break;
            case QueuePacketId::kSignal:
                if (event->EventHeader.ProcessId == trace_data.process_id)
                {
                    ProcessSignalQueuePacket<Is32Bit>(trace_data, event);
                }
                break;
            case QueuePacketId::kWait:
                if (event->EventHeader.ProcessId == trace_data.process_id)
                {
                    ProcessWaitQueuePacket<Is32Bit>(trace_data, event);
                }
                break;
            default:
                break;
            }
        }

        template <bool Is32Bit>
        inline void ParseHwQueue(TraceStorage& trace_data, PEVENT_RECORD event)
        {
            if (event->UserDataLength == sizeof(HwQueue))
            {
                auto* packet = static_cast<HwQueue*>(event->UserData);
                DD_PRINT(LogLevel::Debug,
                         "[HwQueue] Registering runtime hContext, hParentDxgHwQueue (0x%llx, 0x%llx) (pid=%lu)",
                         packet->h_context,
                         packet->h_parent_queue,
                         event->EventHeader.ProcessId);
                trace_data.hw_queues.push_back(*packet);
            }
        }

        /// @brief Parses an associated Dxg scheduler object event.
        ///
        /// If the event's userdata is a AssociateDxgSchedulerObjectPacket, adds a new entry to the
        /// os_associations with the information in the packet.
        ///
        /// If the userdata has length sizeof(uint64[3]), the data is logged.
        ///
        /// If the userdata has any other length an error message is logged.
        ///
        /// @tparam true if the system is 32-bit, false otherwise.
        /// @param trace_data The object to store the parsed event in, assuming that the userdata
        ///                   is a AssociateDxgSchedulerObjectPacket.
        /// @param event The event to parse.
        template <bool Is32Bit>
        inline void ParseAssociateDxgSchedulerObject(TraceStorage& trace_data, PEVENT_RECORD event)
        {
            if (event->UserDataLength == sizeof(AssociateDxgSchedulerObjectPacket))
            {
                auto* packet = static_cast<AssociateDxgSchedulerObjectPacket*>(event->UserData);
                DD_PRINT(LogLevel::Debug,
                         "[ParseAssociateDxgSchedulerObject] Registering runtime hContext pair (0x%llx, 0x%llx) (pid=%lu)",
                         packet->dxg_object,
                         packet->h_os_handle,
                         event->EventHeader.ProcessId);
                trace_data.os_association.push_back(*packet);
            }
            else if (event->UserDataLength == (sizeof(uint64[3])))
            {
                // We expect to receive this event on RS4, but we don't use it.
                // If we *don't* get this event, we can't collect ETW trace data. It happens: don't know why.
                uint64 handles[3] = {
                    0x0,  // This is usually some value that looks useful.
                    0x0,  // This is sometimes 0x0 and sometimes something that looks useful.
                    0x0,  // This is reliably 0xcccccccc, so I have no idea what it's used for.
                };
                Platform::Memcpy_s(handles, sizeof(handles), event->UserData, sizeof(handles));
                DD_PRINT(LogLevel::Debug,
                         "[ParseAssociateDxgSchedulerObject] Registering runtime hContext 0x%llx (pid=%lu)",
                         handles[0],
                         event->EventHeader.ProcessId);
            }
            else
            {
                DD_PRINT(
                    LogLevel::Debug,
                    "[ParseAssociateDxgSchedulerObject] AssociateDxgSchedulerObject event with unexpected UserDataLength: Expected %zu or %zu, but got %zu!",
                    sizeof(AssociateDxgSchedulerObjectPacket),
                    sizeof(uint64[3]),
                    (size_t)event->UserDataLength);
            }
        }

        /// @brief Parses a packet - either a queue packet or a eAssociateDxgSchedulerObject packet.
        /// @tparam true if the system is 32-bit, false otherwise.
        /// @param trace_data The storage for the parsed data.
        /// @param event The event to parse.
        template <bool Is32Bit>
        inline void ParsePacket(TraceStorage& trace_data, PEVENT_RECORD event)
        {
            char              buffer[4096];
            PTRACE_EVENT_INFO info        = reinterpret_cast<PTRACE_EVENT_INFO>(&buffer[0]);
            DWORD             buffer_size = sizeof(buffer);

            // Retrieve the required buffer size for the event metadata.
            DWORD result = TdhGetEventInformation(event, 0, nullptr, info, &buffer_size);
            if (result == ERROR_SUCCESS)
            {
                LPWSTR event_string = (LPWSTR)((PBYTE)(info) + info->TaskNameOffset);

                std::wstring str_name = std::wstring(event_string);
                const Event  type     = kObjectTypeMap[str_name];
                switch (type)
                {
                case Event::kQueuePacket:
                {
                    dxgk_etw_parser::ParseQueuePacket<Is32Bit>(trace_data, event);
                    break;
                }
                case Event::kAssociateDxgSchedulerObject:
                {
                    dxgk_etw_parser::ParseAssociateDxgSchedulerObject<Is32Bit>(trace_data, event);
                    break;
                }
                case Event::kHwQueue:
                {
                    dxgk_etw_parser::ParseHwQueue<Is32Bit>(trace_data, event);
                    break;
                }
                default:
                    break;
                }
            }
        }
    }  // namespace dxgk_etw_parser

    namespace direct3d12_etw_parser
    {
        /// @brief Function to parse the ETW Direct3d12 Name event.
        ///
        /// @param [in] trace_data The context for the event being parsed.
        /// @param [in] etw_event  The ETW event record being parsed.
        inline void ParseDebugObjectName(TraceStorage& trace_data, PEVENT_RECORD etw_event)
        {
            char  buffer[kTdhPropertyBufferSize];
            DWORD buffer_size = sizeof(buffer);

            PROPERTY_DATA_DESCRIPTOR descriptor;
            ZeroMemory(&descriptor, sizeof(PROPERTY_DATA_DESCRIPTOR));
            ZeroMemory(&buffer, buffer_size);

            descriptor.PropertyName = reinterpret_cast<ULONGLONG>(kEtwDirect3D12NameEventPObjectProperty);
            descriptor.ArrayIndex   = ULONG_MAX;
            auto status             = TdhGetProperty(etw_event, 0, nullptr, 1, &descriptor, buffer_size, (BYTE*)buffer);
            DD_ASSERT(status == ERROR_SUCCESS);

            if (status == ERROR_SUCCESS)
            {
                ULONGLONG object       = 0;
                void*     buffer_value = reinterpret_cast<void*>(buffer);
                Platform::Memcpy_s(&object, sizeof(object), buffer_value, sizeof(object));

                ZeroMemory(&descriptor, sizeof(PROPERTY_DATA_DESCRIPTOR));
                ZeroMemory(&buffer, buffer_size);
                descriptor.PropertyName = reinterpret_cast<ULONGLONG>(kEtwDirect3D12NameEventNewDebugObjectNameProperty);
                descriptor.ArrayIndex   = ULONG_MAX;

                ULONG property_size = 0;
                status              = TdhGetPropertySize(etw_event, 0, nullptr, 1, &descriptor, &property_size);
                DD_ASSERT(status == ERROR_SUCCESS);

                status = TdhGetProperty(etw_event, 0, nullptr, 1, &descriptor, buffer_size, (BYTE*)buffer);
                DD_ASSERT(status == ERROR_SUCCESS);

                if (status == ERROR_SUCCESS)
                {
                    if (buffer[0] != 0)
                    {
                        std::string debug_object_name;
                        if (IsUnicode(buffer, static_cast<int>(property_size)))
                        {
                            int wlen = static_cast<int>(property_size / sizeof(wchar_t));
                            std::vector<wchar_t> wbuffer;
                            wbuffer.reserve(wlen);

                            // We need to build the wide buffer manually since the input buffer may not be aligned.
                            for (int i = 0; i < wlen; ++i)
                            {
                                const wchar_t wc = static_cast<wchar_t>(static_cast<unsigned char>(buffer[i * 2]) |
                                                                        (static_cast<unsigned char>(buffer[i * 2 + 1]) << 8));
                                wbuffer.push_back(wc);
                            }

                            int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wbuffer.data(), wlen, nullptr, 0, nullptr, nullptr);

                            std::string temp_str;
                            if (utf8_len > 0)
                            {
                                temp_str.resize(utf8_len);
                                WideCharToMultiByte(CP_UTF8, 0, wbuffer.data(), wlen, &temp_str[0], utf8_len, nullptr, nullptr);
                            }

                            // Resize the string to remove any trailing nulls.
                            size_t null_pos = temp_str.find_first_of('\0');
                            if (null_pos != std::string::npos) {
                                temp_str.resize(null_pos);
                            }

                            debug_object_name = temp_str;
                        }
                        else
                        {
                            debug_object_name = buffer;
                        }

                        DD_PRINT(LogLevel::Debug, "[direct3d12_etw_parser::ParseDebugObjectName] name = %s, 0x%x", debug_object_name.c_str(), object);
                        trace_data.StoreDebugNameString(object, debug_object_name, etw_event->EventHeader.TimeStamp);
                    }
                }
            }
        }

        /// @brief Function to parse the ETW Direct3d12 Resource event.
        ///
        /// @param [in] trace_data The context for the event being parsed.
        /// @param [in] etw_event  The ETW event record being parsed.
        inline void ParseResource(TraceStorage& trace_data, PEVENT_RECORD etw_event)
        {
            char  buffer[kTdhPropertyBufferSize];
            DWORD buffer_size = sizeof(buffer);

            PROPERTY_DATA_DESCRIPTOR descriptor;
            ZeroMemory(&descriptor, sizeof(PROPERTY_DATA_DESCRIPTOR));
            ZeroMemory(&buffer, buffer_size);
            descriptor.PropertyName = reinterpret_cast<ULONGLONG>(kEtwDirect3D12ResourceEventHeapTypeProperty);
            descriptor.ArrayIndex   = ULONG_MAX;
            auto status             = TdhGetProperty(etw_event, 0, nullptr, 1, &descriptor, buffer_size, (BYTE*)buffer);
            DD_ASSERT(status == ERROR_SUCCESS);

            if (status == ERROR_SUCCESS)
            {
                UINT32 heap_type    = 0;
                void*  buffer_value = reinterpret_cast<void*>(buffer);
                Platform::Memcpy_s(&heap_type, sizeof(heap_type), buffer_value, sizeof(heap_type));

                ZeroMemory(&descriptor, sizeof(PROPERTY_DATA_DESCRIPTOR));
                ZeroMemory(&buffer, buffer_size);
                descriptor.PropertyName = reinterpret_cast<ULONGLONG>(kEtwDirect3D12ResourceEventHUMResourceProperty);
                descriptor.ArrayIndex   = ULONG_MAX;
                status                  = TdhGetProperty(etw_event, 0, nullptr, 1, &descriptor, buffer_size, (BYTE*)buffer);
                DD_ASSERT(status == ERROR_SUCCESS);

                if (status == ERROR_SUCCESS)
                {
                    ULONGLONG h_um_resource = 0;
                    Platform::Memcpy_s(&h_um_resource, sizeof(h_um_resource), buffer_value, sizeof(h_um_resource));

                    if ((static_cast<HeapType>(heap_type) == HeapType::kHeapTypeImplicitResource) ||
                        (static_cast<HeapType>(heap_type) == HeapType::kHeapTypeImplicitHeap))
                    {
                        // Mark the resource implicit so that it can be detected by other tools.
                        trace_data.MarkImplicitResource(h_um_resource, etw_event->EventHeader.TimeStamp, static_cast<uint8_t>(heap_type));
                    }

                    // Emit a resource debug name.
                    ZeroMemory(&descriptor, sizeof(PROPERTY_DATA_DESCRIPTOR));
                    ZeroMemory(&buffer, buffer_size);
                    descriptor.PropertyName = reinterpret_cast<ULONGLONG>(kEtwDirect3D12ResourceEventPId3D12ResourceProperty);
                    descriptor.ArrayIndex   = ULONG_MAX;
                    status                  = TdhGetProperty(etw_event, 0, nullptr, 1, &descriptor, buffer_size, (BYTE*)buffer);
                    DD_ASSERT(status == ERROR_SUCCESS);

                    if (status == ERROR_SUCCESS)
                    {
                        ULONGLONG id_3d12_resource = 0;
                        buffer_value               = reinterpret_cast<void*>(buffer);
                        Platform::Memcpy_s(&id_3d12_resource, sizeof(id_3d12_resource), buffer_value, sizeof(id_3d12_resource));

                        if (trace_data.StoreDebugNameHandle(
                                id_3d12_resource, h_um_resource, ResourceNameType::kResourceNameTypeImage, etw_event->EventHeader.TimeStamp) ==
                            DD_RESULT::DD_RESULT_COMMON_ALREADY_EXISTS)
                        {
                            trace_data.logger->Warn("[direct3d12_etw_parser::ParseResource] Correlation ID already exists: 0x%llx", h_um_resource);
                        }
                    }
                }
            }
        }

        /// @brief Function to parse the ETW Direct3d12 Heap event.
        ///
        /// @param [in] trace_data The context for the event being parsed.
        /// @param [in] etw_event  The ETW event record being parsed.
        inline void ParseHeap(TraceStorage& trace_data, PEVENT_RECORD etw_event)
        {
            char  buffer[kTdhPropertyBufferSize];
            DWORD buffer_size = sizeof(buffer);

            PROPERTY_DATA_DESCRIPTOR descriptor;
            ZeroMemory(&descriptor, sizeof(PROPERTY_DATA_DESCRIPTOR));
            ZeroMemory(&buffer, buffer_size);
            descriptor.PropertyName = reinterpret_cast<ULONGLONG>(kEtwDirect3D12HeapEvenPId3D12HeapProperty);
            descriptor.ArrayIndex   = ULONG_MAX;
            auto status             = TdhGetProperty(etw_event, 0, nullptr, 1, &descriptor, buffer_size, (BYTE*)buffer);
            DD_ASSERT(status == ERROR_SUCCESS);

            if (status == ERROR_SUCCESS)
            {
                ULONGLONG id_3d12_heap = 0;
                void*     buffer_value = reinterpret_cast<void*>(buffer);
                Platform::Memcpy_s(&id_3d12_heap, sizeof(id_3d12_heap), buffer_value, sizeof(id_3d12_heap));

                ZeroMemory(&descriptor, sizeof(PROPERTY_DATA_DESCRIPTOR));
                ZeroMemory(&buffer, buffer_size);
                descriptor.PropertyName = reinterpret_cast<ULONGLONG>(kEtwDirect3D12HeapEventHKMAllocationProperty);
                descriptor.ArrayIndex   = ULONG_MAX;

                status = TdhGetProperty(etw_event, 0, nullptr, 1, &descriptor, buffer_size, (BYTE*)buffer);
                DD_ASSERT(status == ERROR_SUCCESS);
                if (status == ERROR_SUCCESS)
                {
                    ULONGLONG h_k_m_allocation = 0;
                    Platform::Memcpy_s(&h_k_m_allocation, sizeof(h_k_m_allocation), buffer_value, sizeof(h_k_m_allocation));

                    if (trace_data.StoreDebugNameHandle(
                            id_3d12_heap, h_k_m_allocation, ResourceNameType::kResourceNameTypeHeap, etw_event->EventHeader.TimeStamp) ==
                        DD_RESULT::DD_RESULT_COMMON_ALREADY_EXISTS)
                    {
                        trace_data.logger->Warn("[direct3d12_etw_parser::ParseHeap] Correlation ID already exists: 0x%llx", h_k_m_allocation);
                    }
                }
            }
        }

        /// @brief Function to parse the ETW event.
        ///
        /// @param [in] trace_data The context for the event being parsed.
        /// @param [in] etw_event  The ETW event record being parsed.
        template <bool Is32Bit>
        inline void ParsePacket(TraceStorage& trace_data, PEVENT_RECORD etw_event)
        {
            DWORD             result      = ERROR_SUCCESS;
            DWORD             buffer_size = 0;
            PTRACE_EVENT_INFO event_info  = nullptr;

            // Retrieve the required buffer size for the event metadata.
            result = TdhGetEventInformation(etw_event, 0, NULL, event_info, &buffer_size);

            if (ERROR_INSUFFICIENT_BUFFER == result)
            {
                event_info = (TRACE_EVENT_INFO*)new char[buffer_size];
                if (event_info != nullptr)
                {
                    ZeroMemory(event_info, buffer_size);

                    // Retrieve the event metadata.
                    result = TdhGetEventInformation(etw_event, 0, NULL, event_info, &buffer_size);

                    if (result == ERROR_SUCCESS)
                    {
                        LPWSTR                       event_name = (LPWSTR)((PBYTE)(event_info) + event_info->TaskNameOffset);
                        direct3d12_etw_parser::Event event_type = GetEventType(event_name);

                        switch (event_type)
                        {
                        case direct3d12_etw_parser::Event::kDebugObjectName:
                        {
                            direct3d12_etw_parser::ParseDebugObjectName(trace_data, etw_event);
                            break;
                        }

                        case direct3d12_etw_parser::Event::kResource:
                        {
                            direct3d12_etw_parser::ParseResource(trace_data, etw_event);
                            break;
                        }

                        case direct3d12_etw_parser::Event::kHeap:
                        {
                            direct3d12_etw_parser::ParseHeap(trace_data, etw_event);
                            break;
                        }
                        default:
                            break;
                        }
                    }
                }
            }

            if (event_info != nullptr)
            {
                delete[] event_info;
            }
        }
    }  // namespace direct3d12_etw_parser

    class EtwParser
    {
    public:
        /// @brief Constructor.
        inline EtwParser()
            : trace_data_()
            , dxg_kernel_provider_guid_()
            , direct_3d_12_provider_guid_()
        {
            HRESULT result = CLSIDFromString(kDxgKernelProviderGuid, &dxg_kernel_provider_guid_);
            result         = CLSIDFromString(kDirect3D12ProviderGuid, &direct_3d_12_provider_guid_);
            (void)result;
        }

        /// @brief Starts parsing Etw events for a process with pid.
        ///
        /// If a trace is already in progress, this will return false.
        /// FinishTrace() should be called before calling Start() again.
        ///
        /// @param pid The process id to start tracing for.
        /// @return true if parsing was started, false otherwise.
        inline bool Start(ProcessId pid)
        {
            bool no_trace_running_already = (trace_data_.process_id == 0);
            // Don't double-start parsers.
            DD_ASSERT(no_trace_running_already);
            if (trace_data_.process_id == 0)
            {
                trace_data_.process_id = pid;
            }
            return no_trace_running_already;
        }

        /// @brief Parses an Etw trace event.
        /// @param event The event to parse.
        inline void ParseEvent(PEVENT_RECORD event)
        {
            bool is_32_bit = (event->EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER) != 0;
            if (is_32_bit)
            {
                ParseEventInternal<true>(event);
            }
            else
            {
                ParseEventInternal<false>(event);
            }
        }

        /// @brief No-op.
        inline void ClearEvents()
        {
        }

        /// @brief Clean up trace data.
        ///
        /// @return The number of signal/wait events processed (not used in this context).
        inline size_t FinishTrace()
        {
            trace_data_.Clear();
            trace_data_.process_id = 0;
            return 0;
        }

        /// @brief Stops parsing a trace and pushes all of the parsed events to the msg_queue.
        ///
        /// All of the parsed events are wrapped inside of the events of a TraceDataChunk
        /// ETWMessage.
        ///
        /// @param msg_queue The message queue to send all of the parsed events to.
        /// @return The number of events that were pushed to the message queue.
        inline size_t FinishTrace(Queue<ETWProtocol::ETWPayload>& msg_queue)
        {
            // If we don't see these events, we won't have sync primitives for the trace.
            // This should not be fatal, but the user might want to know.
            if (trace_data_.os_association.empty())
            {
                DD_PRINT(LogLevel::Warn, "[EtwParser::FinishTrace] No association events found - trace may not have sync data.");
            }

            size_t result = 0;
            if (trace_data_.process_id != 0)
            {
                result = trace_data_.parsed_events.size();

                while (trace_data_.parsed_events.size() > 0)
                {
                    DevDriver::ETWProtocol::ETWPayload* payload = msg_queue.AllocateBack();
                    if (payload != nullptr)
                    {
                        payload->command                  = DevDriver::ETWProtocol::ETWMessage::TraceDataChunk;
                        payload->traceDataChunk.numEvents = 0;
                        for (uint32 count = 0; count < DevDriver::ETWProtocol::kMaxEventsPerChunk && trace_data_.parsed_events.size() > 0; count++)
                        {
                            payload->traceDataChunk.events[count] = trace_data_.parsed_events.top();
                            trace_data_.parsed_events.pop();
                            payload->traceDataChunk.numEvents++;
                        }
                    }
                }
                // Somehow, we reliably get events in here. We can't do anything about them, and they muck up our data.
                // So drop them.
                trace_data_.Clear();
                trace_data_.process_id = 0;
            }
            return result;
        }

        /// @brief Set the callback function used to emit the debug name token.
        ///
        /// @param [in] instance The token emitter instance.
        inline void SetTokenEmitterInstance(void* instance)
        {
            trace_data_.emitter_instance = instance;
        }

        /// @brief Set the callback function used to emit the debug name token.
        ///
        /// @param [in] function_callback The callback function to handle emitting the debug name token.
        inline void SetDebugNameEmitterCallback(const PFNDebugNameEventCallbackType function_callback)
        {
            trace_data_.debug_name_event_callback = function_callback;
        }

        /// @brief Set the callback function used to emit the debug name token.
        ///
        /// @param [in] function_callback The callback function to handle emitting the MARK_RESOURCE_IMPLICIT_USERDATA token.
        inline void SetImplicitResourceEmitterCallback(const PFNImplicitResourceEventCallbackType function_callback)
        {
            trace_data_.implicit_resource_event_callback = function_callback;
        }

        /// @brief Sets the logger that this parser should use.
        /// @param logger The logger that this parser should write to.
        inline void SetLogger(ModuleLogger* logger)
        {
            trace_data_.logger = logger;
        }

    private:
        TraceStorage trace_data_;                  ///< Storage for parsed events.
        GUID         dxg_kernel_provider_guid_;    ///< The GUID for the DXGKernel ETW provider.
        GUID         direct_3d_12_provider_guid_;  ///< The GUID for the Direct3D12 ETW provider.

        /// @brief Method to parse ETW events and route for further processing.
        ///
        /// @tparam Is32Bit true if the system is 32-bit, false otherwise.
        /// @param [in] etw_event The event record to parse.
        template <bool Is32Bit>
        inline void ParseEventInternal(PEVENT_RECORD etw_event)
        {
            int is_dxg_kernel_provider_event = IsEqualGUID(etw_event->EventHeader.ProviderId, dxg_kernel_provider_guid_);
            if (is_dxg_kernel_provider_event != 0)
            {
                dxgk_etw_parser::ParsePacket<Is32Bit>(trace_data_, etw_event);
            }
            else
            {
                int isDirect3d12ProviderEvent = IsEqualGUID(etw_event->EventHeader.ProviderId, direct_3d_12_provider_guid_);
                if (isDirect3d12ProviderEvent != 0)
                {
                    direct3d12_etw_parser::ParsePacket<Is32Bit>(trace_data_, etw_event);
                }
            }
        }
    };
}  // namespace DevDriver
#endif
