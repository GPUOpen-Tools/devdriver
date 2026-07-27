//=============================================================================
/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief Class declaration for using ftrace.
//=============================================================================

#pragma once

#include <ddApi.h>
#include <ddPlatform.h>
#include <util/vector.h>
#include <array>

extern "C" {
#include <tracefs.h>
}

namespace rmt_ftrace
{
    /// @brief Serve as an index into each field of page-table-update event.
    enum PageTableUpdateEventField : size_t
    {
        kStart = 0,
        kEnd,
        kFlags,
        kNptes,
        kIncr,
        kPid,
        kVmCtx,
        kDst,
        kCount  ///< The total count of fields in this enum.
    };

    /// @brief This struct represents a page-table-update event record and the field names
    /// and types follow the format verbatim (except the last field `valid`) as it's
    /// specified in: "sys/kernel/tracing/events/amdgpu/amdgpu_vm_update_ptes/format".
    struct PageTableUpdateEvent
    {
        uint64_t  start;   ///< The starting address in virtual memory.
        uint64_t  end;     ///< the ending address in virtual memory.
        uint64_t  flags;   ///< The flags for the event.
        uint32_t  nptes;   ///< the total pages updated in the event.
        uint64_t  incr;    ///< The size of each page.
        pid_t     pid;     ///< The process identifier the event is for.
        uint64_t  vm_ctx;  ///< Current RMT spec doesn't use this field.
        uint64_t* dst;     ///< The location of the virtual to physical page mapping table.
        bool      valid;   ///< Indicate whether this struct has been populated properly.
    };

    /// @brief Storage for page table update events.
    struct EventRecord
    {
        /// @brief Constructor.
        /// @param formats An array of the formats of the fields in PageTableUpdateEvent. Should
        ///                  have length PageTableUpdateEventField::Count.
        /// @param alloc_cb Allocator and deallocator callbacks used for managing memory `ptu_event.dst`.
        EventRecord(const tep_format_field* const* formats, DevDriver::AllocCb alloc_cb);

        /// @brief Destructor.
        ~EventRecord();

        /// @brief A dynamically allocated array storing all the page-table-update events
        /// recorded in a event-polling.
        DevDriver::Vector<PageTableUpdateEvent, 128> ptu_events;

        /// @brief Indicate whether the last event-polling iteration writes a new record to
        /// this struct.
        bool is_new_event_polled;

        /// @brief An array of the formats of the fields in PageTableUpdateEvent.
        ///
        /// The length of the array is PageTableUpdateEventField::Count.
        const tep_format_field* const* formats;

        /// @brief Allocator and deallocator callbacks used for managing memory `ptu_event.dst`.
        DevDriver::AllocCb dd_alloc_cb;
    };

    /// @brief This class helps iterate through physical addresses in continuous chunks.
    ///
    /// To save memory bandwidth, we generate a single token for virtual pages that
    /// are mapped onto a continuous range of physical addresses.
    class PageTableUpdatePhyAddrCoalesceIterator
    {
    public:
        /// @brief A mapping from virtual memory to physical memory.
        struct VirPhyAddressPair
        {
            uint64_t virtual_address;   ///< The starting address of virtual pages.
            uint64_t physical_address;  ///< The starting address of physical pages.
            uint32_t num_pages;         ///< The number of pages that mapped onto the continuous range
                                        ///< of physical addresses.
        };

        /// @brief Constructor.
        /// @param event The event to iterate through. The pages updated by the event
        ///              will be what this iterator iterates through.
        explicit PageTableUpdatePhyAddrCoalesceIterator(const PageTableUpdateEvent& event);

        /// @brief If the return value is true, `out_addr_pair` presents a valid mapping of
        /// pages from `PageTableUpdateEvent`.
        ///
        /// If false, all page mappings have been
        /// iterated, and `out_addr_pair` is invalid.
        /// @param out_addr_pair The output for a valid mapping of pages from the update event.
        /// @return true if out_addr_pair if there are more pages mappings to iterate over, false otherwise.
        bool Next(VirPhyAddressPair* out_addr_pair);

    private:
        uint64_t  virtual_address_start_;   ///< The starting address in virtual memory.
        uint64_t* physical_address_array_;  ///< The location of the virtual to physical page mapping table.
        uint64_t  page_size_;               ///< The length of each page.
        uint32_t  num_total_pages_;         ///< The total number of pages to iterate through.
        uint32_t  page_mapping_index_;      ///< The current page index.
    };

    /// @brief An encapsulation around an ftrace instance.
    class FTraceContext
    {
    public:
        /// @brief Constructor.
        FTraceContext() = default;

        /// @brief Destructor.
        ~FTraceContext() = default;

        /// @brief Creates a new ftrace instance.
        /// @return DD_RESULT_SUCCESS if the initialization was a success, an error code if it was not.
        DD_RESULT Initialize();

        /// @brief Destroys this context.
        void Destroy();

        /// @brief Enables event tracing.
        void Enable();

        /// @brief Disables event tracing.
        void Disable();

        /// @brief This function populates `out_record` for new events if there is any on
        /// ftrace buffers.
        ///
        /// The events are polled from the earliest to the latest in time.
        /// @param out_record [out] The location to read the events into.
        void PollEvents(EventRecord* out_record);

        /// @brief Getter for formats.
        /// @return formats.
        const tep_format_field* const* PageTableUpdateEventFieldFormats();

    private:
        /// @brief Enables or disables tracing events by writing to the file at
        /// /sys/kernel/tracing/<instance-name>/events/<event-name>/enable.
        ///
        /// @param enabled true if event tracing should be enabled, false otherwise.
        void SetTracingEventsEnabled(bool enabled);

        /// @brief Initializes the formats table by looking up the amdgpu_vm_update_ptes event.
        /// @return DD_RESULT_SUCCESS if the initialization was a success, an error code otherwise.
        DD_RESULT InitPageTableUpdateEventFormats();

        /// @brief A `tracefs_instance` is an instance of tracefs whose backing buffers and
        /// event toggles are independent of the main tracefs system.
        ///
        /// This allows us
        /// to trace events without interfering with the other tracing processes.
        tracefs_instance* tracing_inst_ = nullptr;

        /// @brief Trace Event Parser (tep) context, used for parsing event records.
        tep_handle* tep_context_ = nullptr;

        /// @brief a `tep_format_field` stores pointer offset, data size and other
        /// information about a field in an event.
        ///
        /// Format data are owned by `tep_handle`, so no need to free them.
        std::array<const tep_format_field*, PageTableUpdateEventField::kCount> formats_{};
    };

}
