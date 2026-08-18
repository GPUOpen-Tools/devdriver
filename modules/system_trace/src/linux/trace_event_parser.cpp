//=============================================================================
/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief libtraceevent dynamic loader definition
//=============================================================================

#include "trace_event_parser.h"

#include <dlfcn.h>

namespace
{
    template <typename T>
    T LoadSymbol(void* handle, const char* symbol_name)
    {
        return reinterpret_cast<T>(dlsym(handle, symbol_name));
    }
}  // namespace

#define LOAD_SYMBOL(handle, x) LoadSymbol<x##_fn>(handle, #x)

TraceEventParser& TraceEventParser::Get()
{
    static TraceEventParser parser;
    return parser;
}

unsigned long long TraceEventParser::ReadNumber(struct tep_handle* handle, const void* ptr, int size)
{
    const auto& parser = TraceEventParser::Get();
    if (parser.read_number_ != nullptr)
    {
        return parser.read_number_(handle, ptr, size);
    }

    return 0;
}

int TraceEventParser::ReadNumberField(struct tep_format_field* field, const void* data, unsigned long long* value)
{
    const auto& parser = TraceEventParser::Get();
    if (parser.read_number_field_ != nullptr)
    {
        return parser.read_number_field_(field, data, value);
    }

    return -1;
}

void TraceEventParser::SetFileBigEndian(struct tep_handle* handle, enum tep_endian endian)
{
    const auto& parser = TraceEventParser::Get();
    if (parser.set_file_bigendian_ != nullptr)
    {
        parser.set_file_bigendian_(handle, endian);
    }
}

void TraceEventParser::SetLocalBigEndian(struct tep_handle* handle, enum tep_endian endian)
{
    const auto& parser = TraceEventParser::Get();
    if (parser.set_local_bigendian_ != nullptr)
    {
        parser.set_local_bigendian_(handle, endian);
    }
}

int TraceEventParser::ParseHeaderPage(struct tep_handle* handle, char* buf, unsigned long size, int long_size)
{
    const auto& parser = TraceEventParser::Get();
    if (parser.parse_header_page_ != nullptr)
    {
        return parser.parse_header_page_(handle, buf, size, long_size);
    }

    return -1;
}

struct tep_format_field* TraceEventParser::FindField(struct tep_event* event, const char* name)
{
    const auto& parser = TraceEventParser::Get();
    if (parser.find_field_ != nullptr)
    {
        return parser.find_field_(event, name);
    }

    return nullptr;
}

struct tep_event* TraceEventParser::FindEventByName(struct tep_handle* handle, const char* sys, const char* name)
{
    const auto& parser = TraceEventParser::Get();
    if (parser.find_event_by_name_ != nullptr)
    {
        return parser.find_event_by_name_(handle, sys, name);
    }

    return nullptr;
}

void TraceEventParser::Free(struct tep_handle* handle)
{
    const auto& parser = TraceEventParser::Get();
    if (parser.free_ != nullptr)
    {
        parser.free_(handle);
    }
}

bool TraceEventParser::Initialize()
{
    auto& parser = TraceEventParser::Get();

    void* handle = dlopen("libtraceevent.so.1", RTLD_LAZY);
    if (handle == nullptr)
    {
        return false;
    }

    parser.add_plugin_path_              = LOAD_SYMBOL(handle, tep_add_plugin_path);
    parser.load_plugins_                 = LOAD_SYMBOL(handle, tep_load_plugins);
    parser.unload_plugins_               = LOAD_SYMBOL(handle, tep_unload_plugins);
    parser.load_plugins_hook_            = LOAD_SYMBOL(handle, tep_load_plugins_hook);
    parser.plugin_list_options_          = LOAD_SYMBOL(handle, tep_plugin_list_options);
    parser.plugin_free_options_list_     = LOAD_SYMBOL(handle, tep_plugin_free_options_list);
    parser.plugin_add_options_           = LOAD_SYMBOL(handle, tep_plugin_add_options);
    parser.plugin_add_option_            = LOAD_SYMBOL(handle, tep_plugin_add_option);
    parser.plugin_remove_options_        = LOAD_SYMBOL(handle, tep_plugin_remove_options);
    parser.plugin_print_options_         = LOAD_SYMBOL(handle, tep_plugin_print_options);
    parser.print_plugins_                = LOAD_SYMBOL(handle, tep_print_plugins);
    parser.set_flag_                     = LOAD_SYMBOL(handle, tep_set_flag);
    parser.clear_flag_                   = LOAD_SYMBOL(handle, tep_clear_flag);
    parser.test_flag_                    = LOAD_SYMBOL(handle, tep_test_flag);
    parser.set_function_resolver_        = LOAD_SYMBOL(handle, tep_set_function_resolver);
    parser.reset_function_resolver_      = LOAD_SYMBOL(handle, tep_reset_function_resolver);
    parser.register_comm_                = LOAD_SYMBOL(handle, tep_register_comm);
    parser.override_comm_                = LOAD_SYMBOL(handle, tep_override_comm);
    parser.parse_saved_cmdlines_         = LOAD_SYMBOL(handle, tep_parse_saved_cmdlines);
    parser.parse_kallsyms_               = LOAD_SYMBOL(handle, tep_parse_kallsyms);
    parser.register_function_            = LOAD_SYMBOL(handle, tep_register_function);
    parser.parse_printk_formats_         = LOAD_SYMBOL(handle, tep_parse_printk_formats);
    parser.register_print_string_        = LOAD_SYMBOL(handle, tep_register_print_string);
    parser.is_pid_registered_            = LOAD_SYMBOL(handle, tep_is_pid_registered);
    parser.get_event_                    = LOAD_SYMBOL(handle, tep_get_event);
    parser.print_event_                  = LOAD_SYMBOL(handle, tep_print_event);
    parser.parse_header_page_            = LOAD_SYMBOL(handle, tep_parse_header_page);
    parser.parse_event_                  = LOAD_SYMBOL(handle, tep_parse_event);
    parser.parse_format_                 = LOAD_SYMBOL(handle, tep_parse_format);
    parser.get_field_raw_                = LOAD_SYMBOL(handle, tep_get_field_raw);
    parser.get_field_val_                = LOAD_SYMBOL(handle, tep_get_field_val);
    parser.get_common_field_val_         = LOAD_SYMBOL(handle, tep_get_common_field_val);
    parser.get_any_field_val_            = LOAD_SYMBOL(handle, tep_get_any_field_val);
    parser.print_num_field_              = LOAD_SYMBOL(handle, tep_print_num_field);
    parser.print_func_field_             = LOAD_SYMBOL(handle, tep_print_func_field);
    parser.register_event_handler_       = LOAD_SYMBOL(handle, tep_register_event_handler);
    parser.unregister_event_handler_     = LOAD_SYMBOL(handle, tep_unregister_event_handler);
    parser.register_print_function_      = LOAD_SYMBOL(handle, tep_register_print_function);
    parser.unregister_print_function_    = LOAD_SYMBOL(handle, tep_unregister_print_function);
    parser.find_common_field_            = LOAD_SYMBOL(handle, tep_find_common_field);
    parser.find_field_                   = LOAD_SYMBOL(handle, tep_find_field);
    parser.find_any_field_               = LOAD_SYMBOL(handle, tep_find_any_field);
    parser.find_function_                = LOAD_SYMBOL(handle, tep_find_function);
    parser.find_function_address_        = LOAD_SYMBOL(handle, tep_find_function_address);
    parser.read_number_                  = LOAD_SYMBOL(handle, tep_read_number);
    parser.read_number_field_            = LOAD_SYMBOL(handle, tep_read_number_field);
    parser.get_first_event_              = LOAD_SYMBOL(handle, tep_get_first_event);
    parser.get_events_count_             = LOAD_SYMBOL(handle, tep_get_events_count);
    parser.find_event_                   = LOAD_SYMBOL(handle, tep_find_event);
    parser.find_event_by_name_           = LOAD_SYMBOL(handle, tep_find_event_by_name);
    parser.find_event_by_record_         = LOAD_SYMBOL(handle, tep_find_event_by_record);
    parser.data_type_                    = LOAD_SYMBOL(handle, tep_data_type);
    parser.data_pid_fn_                  = LOAD_SYMBOL(handle, tep_data_pid);
    parser.data_preempt_count_           = LOAD_SYMBOL(handle, tep_data_preempt_count);
    parser.data_flags_                   = LOAD_SYMBOL(handle, tep_data_flags);
    parser.data_comm_from_pid_           = LOAD_SYMBOL(handle, tep_data_comm_from_pid);
    parser.data_pid_from_comm_           = LOAD_SYMBOL(handle, tep_data_pid_from_comm);
    parser.cmdline_pid_                  = LOAD_SYMBOL(handle, tep_cmdline_pid);
    parser.print_field_                  = LOAD_SYMBOL(handle, tep_print_field);
    parser.record_print_fields_          = LOAD_SYMBOL(handle, tep_record_print_fields);
    parser.record_print_selected_fields_ = LOAD_SYMBOL(handle, tep_record_print_selected_fields);
    parser.print_fields_                 = LOAD_SYMBOL(handle, tep_print_fields);
    parser.strerror_                     = LOAD_SYMBOL(handle, tep_strerror);
    parser.list_events_                  = LOAD_SYMBOL(handle, tep_list_events);
    parser.list_events_copy_             = LOAD_SYMBOL(handle, tep_list_events_copy);
    parser.event_common_fields_          = LOAD_SYMBOL(handle, tep_event_common_fields);
    parser.event_fields_                 = LOAD_SYMBOL(handle, tep_event_fields);
    parser.get_cpus_                     = LOAD_SYMBOL(handle, tep_get_cpus);
    parser.set_cpus_                     = LOAD_SYMBOL(handle, tep_set_cpus);
    parser.get_long_size_                = LOAD_SYMBOL(handle, tep_get_long_size);
    parser.set_long_size_                = LOAD_SYMBOL(handle, tep_set_long_size);
    parser.get_page_size_                = LOAD_SYMBOL(handle, tep_get_page_size);
    parser.set_page_size_                = LOAD_SYMBOL(handle, tep_set_page_size);
    parser.is_file_bigendian_            = LOAD_SYMBOL(handle, tep_is_file_bigendian);
    parser.set_file_bigendian_           = LOAD_SYMBOL(handle, tep_set_file_bigendian);
    parser.is_local_bigendian_           = LOAD_SYMBOL(handle, tep_is_local_bigendian);
    parser.set_local_bigendian_          = LOAD_SYMBOL(handle, tep_set_local_bigendian);
    parser.get_header_page_size_         = LOAD_SYMBOL(handle, tep_get_header_page_size);
    parser.get_header_timestamp_size_    = LOAD_SYMBOL(handle, tep_get_header_timestamp_size);
    parser.is_old_format_                = LOAD_SYMBOL(handle, tep_is_old_format);
    parser.set_test_filters_             = LOAD_SYMBOL(handle, tep_set_test_filters);
    parser.alloc_                        = LOAD_SYMBOL(handle, tep_alloc);
    parser.free_                         = LOAD_SYMBOL(handle, tep_free);
    parser.ref_                          = LOAD_SYMBOL(handle, tep_ref);
    parser.unref_                        = LOAD_SYMBOL(handle, tep_unref);
    parser.get_ref_                      = LOAD_SYMBOL(handle, tep_get_ref);
    parser.print_funcs_                  = LOAD_SYMBOL(handle, tep_print_funcs);
    parser.print_printk_                 = LOAD_SYMBOL(handle, tep_print_printk);
    parser.filter_alloc_                 = LOAD_SYMBOL(handle, tep_filter_alloc);
    parser.filter_add_filter_str_        = LOAD_SYMBOL(handle, tep_filter_add_filter_str);
    parser.filter_match_                 = LOAD_SYMBOL(handle, tep_filter_match);
    parser.filter_strerror_              = LOAD_SYMBOL(handle, tep_filter_strerror);
    parser.event_filtered_fn_            = LOAD_SYMBOL(handle, tep_event_filtered);
    parser.filter_reset_                 = LOAD_SYMBOL(handle, tep_filter_reset);
    parser.filter_free_                  = LOAD_SYMBOL(handle, tep_filter_free);
    parser.filter_make_string_           = LOAD_SYMBOL(handle, tep_filter_make_string);
    parser.filter_remove_event_          = LOAD_SYMBOL(handle, tep_filter_remove_event);
    parser.filter_copy_                  = LOAD_SYMBOL(handle, tep_filter_copy);
    parser.filter_compare_               = LOAD_SYMBOL(handle, tep_filter_compare);
    parser.set_loglevel_                 = LOAD_SYMBOL(handle, tep_set_loglevel);

    return true;
}
