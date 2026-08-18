//=============================================================================
/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief Implementation for using ftrace.
//=============================================================================
#include "rmt_ftrace.h"
#include <ddCommon.h>
#include <util/rmtTokens.h>
#include <array>

#include "trace_event_parser.h"
#include "trace_file_system.h"

namespace rmt_ftrace
{
    /// @brief The name for the instance of ftrace that we use to capture kernel events.
    DD_STATIC_CONST const char* kFtraceInstanceName = "amd_rmv";

    /// @brief The array of systems whose events we're interested in capturing.
    ///
    /// The NULL at the end is required and serves as an end sentinel.
    DD_STATIC_CONST std::array<const char*, 2> kEventSystems = {"amdgpu", nullptr};

    /// @brief The amdgpu events to be captured.
    DD_STATIC_CONST std::array<const char*, 1> kAmdgpuEvents = {"amdgpu_vm_update_ptes"};

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief This function is similar to `tep_get_field_raw` which returns a raw pointer
    /// to the requested field.
    ///
    /// Except that this function accepts the format data of
    /// the requested field directly from a `tep_format_field` argument, instead of
    /// searching for it by a name string. When the field is an array,
    /// `out_byte_size` represents the total amount of bytes of all elements in the
    /// array.
    ///
    /// @param field The field to get the size of.
    /// @param record The record to read from.
    /// @param out_byte_size The output for the total amount of bytes in field.
    /// @return The location of the start of the field in the record data.
    static void* GetFieldRaw(tep_format_field* field, tep_record* record, size_t* out_byte_size)
    {
        uint8_t* data = static_cast<uint8_t*>(record->data);
        size_t   offset;
        size_t   dummy;

        // Allow `out_byte_size` to be NULL.
        if (out_byte_size == nullptr)
        {
            out_byte_size = &dummy;
        }

        offset = field->offset;
        if ((field->flags & TEP_FIELD_IS_DYNAMIC) != 0U)
        {
            // If `field` points to an array, we need to extract byte-size and the real
            // offset separately.

            offset = TraceEventParser::ReadNumber(field->event->tep, data + offset, field->size);

            // The high 16-bit represents the total byte size of the array field points to,
            // while the low 16-bit represents the real offset of the array.
            *out_byte_size = offset >> 16;
            offset &= 0xffff;
        }
        else
        {
            *out_byte_size = field->size;
        }

        return data + offset;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief This function is specifically for extracting the physical addresses from the
    /// amdgpu_vm_update_ptes event.
    ///
    /// It allocates memory to copy the addresses over. The returned memory needs to be freed by the caller.
    /// @param field The field to read the data for.
    /// @param record The record to read from.
    /// @param out_phy_addr_array The output for the physical address array.
    /// @param alloc_cb Allocator and de-allocator callbacks used for managing memory.
    /// @return DD_RESULT_SUCCESS if the array is successfully read; an error code otherwise.
    static DD_RESULT GetPhysicalAddressArray(tep_format_field* field, tep_record* record, uint64_t** out_phy_addr_array, DevDriver::AllocCb alloc_cb)
    {
        DD_RESULT result = DD_RESULT_SUCCESS;

        size_t    array_byte_size = 0;
        uint64_t* tep_field_start = static_cast<uint64_t*>(GetFieldRaw(field, record, &array_byte_size));

        if (tep_field_start != nullptr)
        {
            *out_phy_addr_array = static_cast<uint64_t*>(alloc_cb.Alloc(array_byte_size, false));
            if (*out_phy_addr_array != nullptr)
            {
                // Need to copy addresses over to get parsed later.
                DevDriver::Platform::Memcpy_s(*out_phy_addr_array, array_byte_size, tep_field_start, array_byte_size);
            }
            else
            {
                result = DD_RESULT_COMMON_OUT_OF_HEAP_MEMORY;
            }
        }
        else
        {
            result = DD_RESULT_UNKNOWN;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Populate a single PageTableUpdateEvent from a raw event record.
    /// @param tep_record The record to read from.
    /// @param field_formats the pointer offset, data size and other information about a field in an event.
    /// @param out_event The output for the read event.
    /// @param dd_alloc_cb  Allocator and de-allocator callbacks used for managing memory.
    static void ParsePageTableUpdateEventRecord(tep_record*                    tep_record,
                                                const tep_format_field* const* field_formats,
                                                PageTableUpdateEvent*          out_event,
                                                DevDriver::AllocCb             dd_alloc_cb)
    {
        using LogLevel = DevDriver::LogLevel;

        out_event->valid = true;

        unsigned long long val = 0;

        // `tep_read_number_field` takes non-const pointers, so convert `ppFieldFormats` here.
        tep_format_field* const* formats = const_cast<tep_format_field* const*>(field_formats);

        int field_not_found = TraceEventParser::ReadNumberField(formats[PageTableUpdateEventField::kStart], tep_record->data, &val);
        out_event->start    = static_cast<uint64_t>(val);
        if (field_not_found != 0)
        {
            out_event->valid = false;
            DD_PRINT(LogLevel::Error, "[ParsePageTableUpdateEventRecord] PageTableUpdate event field \"start\" not found.");
        }

        if (out_event->valid)
        {
            field_not_found = TraceEventParser::ReadNumberField(formats[PageTableUpdateEventField::kEnd], tep_record->data, &val);
            out_event->end  = static_cast<uint64_t>(val);
            if (field_not_found != 0)
            {
                out_event->valid = false;
                DD_PRINT(LogLevel::Error, "[ParsePageTableUpdateEventRecord] PageTableUpdate event field \"end\" not found.");
            }
        }

        if (out_event->valid)
        {
            field_not_found  = TraceEventParser::ReadNumberField(formats[PageTableUpdateEventField::kFlags], tep_record->data, &val);
            out_event->flags = static_cast<uint64_t>(val);
            if (field_not_found != 0)
            {
                out_event->valid = false;
                DD_PRINT(LogLevel::Error, "[ParsePageTableUpdateEventRecord] PageTableUpdate event field \"flags\" not found.");
            }
        }

        if (out_event->valid)
        {
            field_not_found  = TraceEventParser::ReadNumberField(formats[PageTableUpdateEventField::kNptes], tep_record->data, &val);
            out_event->nptes = static_cast<uint32_t>(val);
            if (field_not_found != 0)
            {
                out_event->valid = false;
                DD_PRINT(LogLevel::Error, "[ParsePageTableUpdateEventRecord] PageTableUpdate event field \"nptes\" not found.");
            }
        }

        if (out_event->valid)
        {
            field_not_found = TraceEventParser::ReadNumberField(formats[PageTableUpdateEventField::kIncr], tep_record->data, &val);
            out_event->incr = static_cast<uint32_t>(val);
            if (field_not_found != 0)
            {
                out_event->valid = false;
                DD_PRINT(LogLevel::Error, "[ParsePageTableUpdateEventRecord] PageTableUpdate event field \"incr\" not found.");
            }
        }

        if (out_event->valid)
        {
            // TODO: pid is an int type, hand-roll our own code to get pid from a void pointer.
            field_not_found = TraceEventParser::ReadNumberField(formats[PageTableUpdateEventField::kPid], tep_record->data, &val);
            out_event->pid  = static_cast<pid_t>(val);
            if (field_not_found != 0)
            {
                out_event->valid = false;
                DD_PRINT(LogLevel::Error, "[ParsePageTableUpdateEventRecord] PageTableUpdate event field \"pid\" not found.");
            }
        }

        if (out_event->valid)
        {
            const DD_RESULT result = GetPhysicalAddressArray(formats[PageTableUpdateEventField::kDst], tep_record, &out_event->dst, dd_alloc_cb);

            if (result != DD_RESULT_SUCCESS)
            {
                out_event->valid = false;
                out_event->dst   = nullptr;

                DD_PRINT(LogLevel::Error,
                         "[ParsePageTableUpdateEventRecord] Failed to retrieve PageTableUpdate event field \"dst\" with error: %s.",
                         ddApiResultToString(result));
            }
        }

        // Currently, when kernel emits a page-table-update event, it right-shifts off the last
        // bits of the virtual addresses by `virtual_address /= AMDGPU_GPU_PAGE_SIZE`. However,
        // the page-table-update token constructor expects virtual addresses un-shifted. So here
        // we restore it to its original value. `AMDGPU_GPU_PAGE_SIZE` is set to the minimum
        // page size which is 4k.
        out_event->start *= RMT_4KB;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief A callback passed into `tracefs_iterate_raw_events` to parse the raw event
    /// records.
    ///
    /// Note, this function can be invoked multiple times to parse all remaining events left on ftrace buffers.
    /// @param event Unused.
    /// @param tep_record The record that the events are read from.
    /// @param cpu Unused.
    /// @param userdata An EventRecord that ptu events are read from.
    /// @return 0.
    static int EventIteratorCallback(tep_event* event, tep_record* tep_record, int cpu, void* userdata)
    {
        using LogLevel = DevDriver::LogLevel;

        DD_UNUSED(event);
        DD_UNUSED(cpu);

        EventRecord*                   record  = static_cast<EventRecord*>(userdata);
        const tep_format_field* const* formats = record->formats;

        DevDriver::Vector<PageTableUpdateEvent, 128>& ptu_events = record->ptu_events;

        if (ptu_events.PushBack())
        {
            ParsePageTableUpdateEventRecord(tep_record, formats, &(ptu_events[ptu_events.Size() - 1]), record->dd_alloc_cb);
        }
        else
        {
            DD_PRINT(LogLevel::Error, "[EventIteratorCallback] Failed to PushBack elements to PageTableUpdate event array.");
        }

        record->is_new_event_polled = true;

        // Returning non-zero will stop event iterator from polling and parsing the next event record.
        // Always return 0, so we keep going.
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <size_t BufSize>
    static DD_RESULT MakeEventEnableFilePath(std::array<char, BufSize>(&filepath_buf), const char* event_system, const char* event_name)
    {
        using DevDriver::Platform::Snprintf;

        const uint32_t len = Snprintf(filepath_buf.data(), BufSize, "events/%s/%s/enable", event_system, event_name);
        if ((len > 0) && (static_cast<size_t>(len) <= BufSize))
        {
            return DD_RESULT_SUCCESS;
        }

        return DD_RESULT_DD_GENERIC_INVALID_PARAMETER;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    EventRecord::EventRecord(const tep_format_field* const* field_formats, DevDriver::AllocCb alloc_cb)
        : ptu_events(alloc_cb)
        , is_new_event_polled(false)
        , formats(field_formats)
        , dd_alloc_cb(alloc_cb)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    EventRecord::~EventRecord()
    {
        for (size_t i = 0; i < ptu_events.Size(); ++i)
        {
            dd_alloc_cb.Free(ptu_events[i].dst);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PageTableUpdatePhyAddrCoalesceIterator::PageTableUpdatePhyAddrCoalesceIterator(const PageTableUpdateEvent& event)
        : virtual_address_start_(event.start)
        , physical_address_array_(event.dst)
        , page_size_(event.incr)
        , num_total_pages_(event.nptes)
        , page_mapping_index_(0)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool PageTableUpdatePhyAddrCoalesceIterator::Next(PageTableUpdatePhyAddrCoalesceIterator::VirPhyAddressPair* out_addr_pair)
    {
        using LogLevel = DevDriver::LogLevel;

        bool still_continue = false;

        if (page_mapping_index_ < num_total_pages_)
        {
            // Virtual addresses are always contiguous, so we just advance
            // the pointer linearly.
            out_addr_pair->virtual_address  = virtual_address_start_ + (page_mapping_index_ * page_size_);
            out_addr_pair->physical_address = physical_address_array_[page_mapping_index_];

            uint32_t num_coalesced_pages = 1;
            for (uint32_t i = page_mapping_index_; i < (num_total_pages_ - 1); ++i)
            {
                // Check if the next physical address is not `page_size_` away from
                // the current one. If so, stop coalescing.
                uint64_t const stride = physical_address_array_[i + 1] - physical_address_array_[i];
                if (stride <= 0)
                {
                    DD_PRINT(LogLevel::Warn, "[PageTableUpdatePhyAddrCoalesceIterator::Next] Addresses of contiguous physical pages are in descending order.");
                }

                if (stride != page_size_)
                {
                    break;
                }
                num_coalesced_pages++;
            }
            page_mapping_index_ += num_coalesced_pages;
            out_addr_pair->num_pages = num_coalesced_pages;

            still_continue = true;
        }

        return still_continue;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DD_RESULT FTraceContext::Initialize()
    {
        DD_RESULT result = DD_RESULT_SUCCESS;

        // Create a ftrace instance and parse the header page to get metadata
        // about events and their fields, which will be used to extract
        // values from actual individual events.

        if (!TraceFileSystem::Initialize() || !TraceEventParser::Initialize())
        {
            return result;
        }

        using LogLevel = DevDriver::LogLevel;

        tracing_inst_ = TraceFileSystem::InstanceCreate(kFtraceInstanceName);

        char* tracing_inst_dir = TraceFileSystem::GetTracingDirectory(tracing_inst_);
        if (tracing_inst_dir != nullptr)
        {
            // Set up Trace Event Parser (tep) context.
            tep_context_ = TraceFileSystem::LocalEventsSystem(tracing_inst_dir, kEventSystems.data());
            TraceFileSystem::PutTracingFile(tracing_inst_dir);  // frees tracing_inst_dir

            if (tep_context_ != nullptr)
            {
                // We don't support big endian.
                TraceEventParser::SetFileBigEndian(tep_context_, TEP_LITTLE_ENDIAN);
                TraceEventParser::SetLocalBigEndian(tep_context_, TEP_LITTLE_ENDIAN);

                int   bytes_read      = 0;
                char* header_page_buf = TraceFileSystem::InstanceFileRead(tracing_inst_, "events/header_page", &bytes_read);
                if (header_page_buf != nullptr)
                {
                    // Parse the header page which stores information about events and their fields.
                    int const err = TraceEventParser::ParseHeaderPage(tep_context_, header_page_buf, bytes_read, sizeof(unsigned long));
                    free(header_page_buf);

                    if (err < 0)
                    {
                        result = DD_RESULT_PARSING_UNKNOWN;
                        DD_PRINT(LogLevel::Error, "[FTraceContext::Initialize] Failed to parse header page.");
                    }
                    else
                    {
                        result = InitPageTableUpdateEventFormats();
                        if (result != DD_RESULT_SUCCESS)
                        {
                            DD_PRINT(LogLevel::Error, "[FTraceContext::Initialize] Failed to initialize event field formats.");
                        }
                    }
                }
                else
                {
                    result = DD_RESULT_FS_UNKNOWN;
                    DD_PRINT(LogLevel::Error, "[FTraceContext::Initialize] Failed to read file events/header_page.");
                }
            }
            else
            {
                result = DD_RESULT_UNKNOWN;
                DD_PRINT(LogLevel::Error, "[FTraceContext::Initialize] Failed to create Trace Event Parser (tep) handle.");
            }
        }
        else
        {
            result = DD_RESULT_UNKNOWN;
            DD_PRINT(LogLevel::Error, "[FTraceContext::Initialize] Failed to create ftrace instance named %s.", kFtraceInstanceName);
        }

        if (result != DD_RESULT_SUCCESS)
        {
            Destroy();
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void FTraceContext::Destroy()
    {
        if (tep_context_ != nullptr)
        {
            TraceEventParser::Free(tep_context_);
            tep_context_ = nullptr;
        }

        if (tracing_inst_ != nullptr)
        {
            TraceFileSystem::InstanceDestroy(tracing_inst_);
            TraceFileSystem::InstanceFree(tracing_inst_);

            tracing_inst_ = nullptr;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void FTraceContext::Enable()
    {
        SetTracingEventsEnabled(true);
        TraceFileSystem::TraceOn(tracing_inst_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void FTraceContext::Disable()
    {
        TraceFileSystem::TraceOff(tracing_inst_);
        SetTracingEventsEnabled(false);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void FTraceContext::PollEvents(EventRecord* out_record)
    {
        out_record->ptu_events.Clear();
        out_record->is_new_event_polled = false;

        TraceFileSystem::IterateRawEvents(tep_context_, tracing_inst_, nullptr, 0, EventIteratorCallback, out_record);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const tep_format_field* const* FTraceContext::PageTableUpdateEventFieldFormats()
    {
        return const_cast<const tep_format_field* const*>(formats_.data());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void FTraceContext::SetTracingEventsEnabled(bool enabled)
    {
        // To enable/disable tracing of an event, we just write "1"/"0" to the
        // corresponding file at '/sys/kernel/tracing/<instance-name>/events/<event-name>/enable'.

        using DevDriver::LogLevel;

        for (const auto* amdgpu_event : kAmdgpuEvents)
        {
            std::array<char, FILENAME_MAX> relative_enable_file_path_buf{};
            const DD_RESULT                result = MakeEventEnableFilePath(relative_enable_file_path_buf, kEventSystems[0], amdgpu_event);

            if (result == DD_RESULT_SUCCESS)
            {
                char* enable_file_full_path = TraceFileSystem::InstanceGetFile(tracing_inst_, relative_enable_file_path_buf.data());
                if (enable_file_full_path != nullptr)
                {
                    FILE* enable_file = fopen(enable_file_full_path, "w");
                    TraceFileSystem::PutTracingFile(enable_file_full_path);  // free the string `enable_file_full_path`
                    if (enable_file != nullptr)
                    {
                        char enable = enabled ? '1' : '0';

                        size_t const err = fwrite(&enable, 1, 1, enable_file);
                        fclose(enable_file);

                        if (err != 1)
                        {
                            DD_PRINT(LogLevel::Error,
                                     "[FTraceContext::SetTracingEventsEnabled] Failed to write to file %s, error(%d): %s",
                                     enable_file_full_path,
                                     errno,
                                     strerror(errno));
                        }
                    }
                    else
                    {
                        DD_PRINT(LogLevel::Error,
                                 "[FTraceContext::SetTracingEventsEnabled] Failed to open file %s, error(%d): %s",
                                 enable_file_full_path,
                                 errno,
                                 strerror(errno));
                    }
                }
            }
            else
            {
                DD_PRINT(LogLevel::Error, "[FTraceContext::SetTracingEventsEnabled] MakeEventEnableFilePath failed.");
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DD_RESULT FTraceContext::InitPageTableUpdateEventFormats()
    {
        DD_RESULT result = DD_RESULT_SUCCESS;

        tep_event* event = TraceEventParser::FindEventByName(tep_context_, "amdgpu", "amdgpu_vm_update_ptes");
        if (event != nullptr)
        {
            formats_[PageTableUpdateEventField::kStart] = TraceEventParser::FindField(event, "start");
            formats_[PageTableUpdateEventField::kEnd]   = TraceEventParser::FindField(event, "end");
            formats_[PageTableUpdateEventField::kFlags] = TraceEventParser::FindField(event, "flags");
            formats_[PageTableUpdateEventField::kNptes] = TraceEventParser::FindField(event, "nptes");
            formats_[PageTableUpdateEventField::kIncr]  = TraceEventParser::FindField(event, "incr");
            formats_[PageTableUpdateEventField::kPid]   = TraceEventParser::FindField(event, "pid");
            formats_[PageTableUpdateEventField::kVmCtx] = TraceEventParser::FindField(event, "vm_ctx");
            formats_[PageTableUpdateEventField::kDst]   = TraceEventParser::FindField(event, "dst");

            for (auto& format : formats_)
            {
                if (format == nullptr)
                {
                    result = DD_RESULT_UNKNOWN;
                    break;
                }
            }
        }
        else
        {
            result = DD_RESULT_UNKNOWN;
        }

        return result;
    }

}  // namespace rmt_ftrace
