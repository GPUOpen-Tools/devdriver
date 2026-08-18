//=============================================================================
/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief libtraceevent dynamic loader definition
//=============================================================================

#pragma once

#include "traceevent_function_types.h"

class TraceEventParser
{
public:
    static TraceEventParser& Get();

    static bool Initialize();

    static unsigned long long       ReadNumber(struct tep_handle* handle, const void* ptr, int size);
    static int                      ReadNumberField(struct tep_format_field* field, const void* data, unsigned long long* value);
    static void                     SetFileBigEndian(struct tep_handle* handle, enum tep_endian endian);
    static void                     SetLocalBigEndian(struct tep_handle* handle, enum tep_endian endian);
    static int                      ParseHeaderPage(struct tep_handle* handle, char* buf, unsigned long size, int long_size);
    static void                     Free(struct tep_handle* handle);
    static struct tep_event*        FindEventByName(struct tep_handle* handle, const char* sys, const char* name);
    static struct tep_format_field* FindField(struct tep_event* event, const char* name);

private:
    TraceEventParser() = default;

    tep_add_plugin_path_fn              add_plugin_path_;
    tep_load_plugins_fn                 load_plugins_;
    tep_unload_plugins_fn               unload_plugins_;
    tep_load_plugins_hook_fn            load_plugins_hook_;
    tep_plugin_list_options_fn          plugin_list_options_;
    tep_plugin_free_options_list_fn     plugin_free_options_list_;
    tep_plugin_add_options_fn           plugin_add_options_;
    tep_plugin_add_option_fn            plugin_add_option_;
    tep_plugin_remove_options_fn        plugin_remove_options_;
    tep_plugin_print_options_fn         plugin_print_options_;
    tep_print_plugins_fn                print_plugins_;
    tep_set_flag_fn                     set_flag_;
    tep_clear_flag_fn                   clear_flag_;
    tep_test_flag_fn                    test_flag_;
    tep_set_function_resolver_fn        set_function_resolver_;
    tep_reset_function_resolver_fn      reset_function_resolver_;
    tep_register_comm_fn                register_comm_;
    tep_override_comm_fn                override_comm_;
    tep_parse_saved_cmdlines_fn         parse_saved_cmdlines_;
    tep_parse_kallsyms_fn               parse_kallsyms_;
    tep_register_function_fn            register_function_;
    tep_parse_printk_formats_fn         parse_printk_formats_;
    tep_register_print_string_fn        register_print_string_;
    tep_is_pid_registered_fn            is_pid_registered_;
    tep_get_event_fn                    get_event_;
    tep_print_event_fn                  print_event_;
    tep_parse_header_page_fn            parse_header_page_;
    tep_parse_event_fn                  parse_event_;
    tep_parse_format_fn                 parse_format_;
    tep_get_field_raw_fn                get_field_raw_;
    tep_get_field_val_fn                get_field_val_;
    tep_get_common_field_val_fn         get_common_field_val_;
    tep_get_any_field_val_fn            get_any_field_val_;
    tep_print_num_field_fn              print_num_field_;
    tep_print_func_field_fn             print_func_field_;
    tep_register_event_handler_fn       register_event_handler_;
    tep_unregister_event_handler_fn     unregister_event_handler_;
    tep_register_print_function_fn      register_print_function_;
    tep_unregister_print_function_fn    unregister_print_function_;
    tep_find_common_field_fn            find_common_field_;
    tep_find_field_fn                   find_field_;
    tep_find_any_field_fn               find_any_field_;
    tep_find_function_fn                find_function_;
    tep_find_function_address_fn        find_function_address_;
    tep_read_number_fn                  read_number_;
    tep_read_number_field_fn            read_number_field_;
    tep_get_first_event_fn              get_first_event_;
    tep_get_events_count_fn             get_events_count_;
    tep_find_event_fn                   find_event_;
    tep_find_event_by_name_fn           find_event_by_name_;
    tep_find_event_by_record_fn         find_event_by_record_;
    tep_data_type_fn                    data_type_;
    tep_data_pid_fn                     data_pid_fn_;
    tep_data_preempt_count_fn           data_preempt_count_;
    tep_data_flags_fn                   data_flags_;
    tep_data_comm_from_pid_fn           data_comm_from_pid_;
    tep_data_pid_from_comm_fn           data_pid_from_comm_;
    tep_cmdline_pid_fn                  cmdline_pid_;
    tep_print_field_fn                  print_field_;
    tep_record_print_fields_fn          record_print_fields_;
    tep_record_print_selected_fields_fn record_print_selected_fields_;
    tep_print_fields_fn                 print_fields_;
    tep_strerror_fn                     strerror_;
    tep_list_events_fn                  list_events_;
    tep_list_events_copy_fn             list_events_copy_;
    tep_event_common_fields_fn          event_common_fields_;
    tep_event_fields_fn                 event_fields_;
    tep_get_cpus_fn                     get_cpus_;
    tep_set_cpus_fn                     set_cpus_;
    tep_get_long_size_fn                get_long_size_;
    tep_set_long_size_fn                set_long_size_;
    tep_get_page_size_fn                get_page_size_;
    tep_set_page_size_fn                set_page_size_;
    tep_is_file_bigendian_fn            is_file_bigendian_;
    tep_set_file_bigendian_fn           set_file_bigendian_;
    tep_is_local_bigendian_fn           is_local_bigendian_;
    tep_set_local_bigendian_fn          set_local_bigendian_;
    tep_get_header_page_size_fn         get_header_page_size_;
    tep_get_header_timestamp_size_fn    get_header_timestamp_size_;
    tep_is_old_format_fn                is_old_format_;
    tep_set_test_filters_fn             set_test_filters_;
    tep_alloc_fn                        alloc_;
    tep_free_fn                         free_;
    tep_ref_fn                          ref_;
    tep_unref_fn                        unref_;
    tep_get_ref_fn                      get_ref_;
    tep_print_funcs_fn                  print_funcs_;
    tep_print_printk_fn                 print_printk_;
    tep_filter_alloc_fn                 filter_alloc_;
    tep_filter_add_filter_str_fn        filter_add_filter_str_;
    tep_filter_match_fn                 filter_match_;
    tep_filter_strerror_fn              filter_strerror_;
    tep_event_filtered_fn               event_filtered_fn_;
    tep_filter_reset_fn                 filter_reset_;
    tep_filter_free_fn                  filter_free_;
    tep_filter_make_string_fn           filter_make_string_;
    tep_filter_remove_event_fn          filter_remove_event_;
    tep_filter_copy_fn                  filter_copy_;
    tep_filter_compare_fn               filter_compare_;
    tep_set_loglevel_fn                 set_loglevel_;
};
