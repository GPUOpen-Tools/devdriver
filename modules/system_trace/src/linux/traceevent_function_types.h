//=============================================================================
/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief libtraceevent entrypoint definitions
//=============================================================================

#pragma once

#include <event-parse.h>

typedef int                     (*tep_add_plugin_path_fn)(struct tep_handle*, char*, enum tep_plugin_load_priority);
typedef struct tep_plugin_list* (*tep_load_plugins_fn)(struct tep_handle*);
typedef void                    (*tep_unload_plugins_fn)(struct tep_plugin_list*, struct tep_handle*);
typedef void              (*tep_load_plugins_hook_fn)(struct tep_handle*, const char*, void (*)(struct tep_handle*, const char*, const char*, void*), void*);
typedef char**            (*tep_plugin_list_options_fn)();
typedef void              (*tep_plugin_free_options_list_fn)(char**);
typedef int               (*tep_plugin_add_options_fn)(const char*, struct tep_plugin_option*);
typedef int               (*tep_plugin_add_option_fn)(const char*, const char*);
typedef void              (*tep_plugin_remove_options_fn)(struct tep_plugin_option*);
typedef void              (*tep_plugin_print_options_fn)(struct tep_plugin_option*);
typedef void              (*tep_print_plugins_fn)(struct trace_seq*, const char*, const char*, const struct tep_plugin_list*);
typedef void              (*tep_set_flag_fn)(struct tep_handle*, int);
typedef void              (*tep_clear_flag_fn)(struct tep_handle*, enum tep_flag);
typedef bool              (*tep_test_flag_fn)(struct tep_handle*, enum tep_flag);
typedef int               (*tep_set_function_resolver_fn)(struct tep_handle*, tep_func_resolver_t*, void*);
typedef void              (*tep_reset_function_resolver_fn)(struct tep_handle*);
typedef int               (*tep_register_comm_fn)(struct tep_handle*, const char*, int);
typedef int               (*tep_override_comm_fn)(struct tep_handle*, const char*, int);
typedef int               (*tep_parse_saved_cmdlines_fn)(struct tep_handle*, const char*);
typedef int               (*tep_parse_kallsyms_fn)(struct tep_handle*, const char*);
typedef int               (*tep_register_function_fn)(struct tep_handle*, char*, unsigned long long, char*);
typedef int               (*tep_parse_printk_formats_fn)(struct tep_handle*, const char*);
typedef int               (*tep_register_print_string_fn)(struct tep_handle*, const char*, unsigned long long);
typedef bool              (*tep_is_pid_registered_fn)(struct tep_handle*, int);
typedef struct tep_event* (*tep_get_event_fn)(struct tep_handle*, int);
typedef void              (*tep_print_event_fn)(struct tep_handle*, struct trace_seq*, struct tep_record*, const char*, ...);
typedef int               (*tep_parse_header_page_fn)(struct tep_handle*, char*, unsigned long, int);
typedef enum tep_errno    (*tep_parse_event_fn)(struct tep_handle*, const char*, unsigned long, const char*);
typedef enum tep_errno    (*tep_parse_format_fn)(struct tep_handle*, struct tep_event**, const char*, unsigned long, const char*);
typedef void*             (*tep_get_field_raw_fn)(struct trace_seq*, struct tep_event*, const char*, struct tep_record*, int*, int);
typedef int               (*tep_get_field_val_fn)(struct trace_seq*, struct tep_event*, const char*, struct tep_record*, unsigned long long*, int);
typedef int               (*tep_get_common_field_val_fn)(struct trace_seq*, struct tep_event*, const char*, struct tep_record*, unsigned long long*, int);
typedef int               (*tep_get_any_field_val_fn)(struct trace_seq*, struct tep_event*, const char*, struct tep_record*, unsigned long long*, int);
typedef int               (*tep_print_num_field_fn)(struct trace_seq*, const char*, struct tep_event*, const char*, struct tep_record*, int);
typedef int               (*tep_print_func_field_fn)(struct trace_seq*, const char*, struct tep_event*, const char*, struct tep_record*, int);
typedef int               (*tep_register_event_handler_fn)(struct tep_handle*, int, const char*, const char*, tep_event_handler_func, void*);
typedef int               (*tep_unregister_event_handler_fn)(struct tep_handle*, int, const char*, const char*, tep_event_handler_func, void*);
typedef int               (*tep_register_print_function_fn)(struct tep_handle*, tep_func_handler, enum tep_func_arg_type, char*, ...);
typedef int               (*tep_unregister_print_function_fn)(struct tep_handle*, tep_func_handler, char*);
typedef struct tep_format_field*  (*tep_find_common_field_fn)(struct tep_event*, const char*);
typedef struct tep_format_field*  (*tep_find_field_fn)(struct tep_event*, const char*);
typedef struct tep_format_field*  (*tep_find_any_field_fn)(struct tep_event*, const char*);
typedef const char*               (*tep_find_function_fn)(struct tep_handle*, unsigned long long);
typedef unsigned long long        (*tep_find_function_address_fn)(struct tep_handle*, unsigned long long);
typedef unsigned long long        (*tep_read_number_fn)(struct tep_handle*, const void*, int);
typedef int                       (*tep_read_number_field_fn)(struct tep_format_field*, const void*, unsigned long long*);
typedef struct tep_event*         (*tep_get_first_event_fn)(struct tep_handle*);
typedef int                       (*tep_get_events_count_fn)(struct tep_handle*);
typedef struct tep_event*         (*tep_find_event_fn)(struct tep_handle*, int);
typedef struct tep_event*         (*tep_find_event_by_name_fn)(struct tep_handle*, const char*, const char*);
typedef struct tep_event*         (*tep_find_event_by_record_fn)(struct tep_handle*, struct tep_record*);
typedef int                       (*tep_data_type_fn)(struct tep_handle*, struct tep_record*);
typedef int                       (*tep_data_pid_fn)(struct tep_handle*, struct tep_record*);
typedef int                       (*tep_data_preempt_count_fn)(struct tep_handle*, struct tep_record*);
typedef int                       (*tep_data_flags_fn)(struct tep_handle*, struct tep_record*);
typedef const char*               (*tep_data_comm_from_pid_fn)(struct tep_handle*, int);
typedef struct tep_cmdline*       (*tep_data_pid_from_comm_fn)(struct tep_handle*, const char*, struct tep_cmdline*);
typedef int                       (*tep_cmdline_pid_fn)(struct tep_handle*, struct tep_cmdline*);
typedef void                      (*tep_print_field_fn)(struct trace_seq*, void*, struct tep_format_field*);
typedef void                      (*tep_record_print_fields_fn)(struct trace_seq*, struct tep_record*, struct tep_event*);
typedef void                      (*tep_record_print_selected_fields_fn)(struct trace_seq*, struct tep_record*, struct tep_event*, unsigned long long);
typedef void                      (*tep_print_fields_fn)(struct trace_seq*, void*, int __maybe_unused, struct tep_event*);
typedef int                       (*tep_strerror_fn)(struct tep_handle*, enum tep_errno, char*, size_t);
typedef struct tep_event**        (*tep_list_events_fn)(struct tep_handle*, enum tep_event_sort_type);
typedef struct tep_event**        (*tep_list_events_copy_fn)(struct tep_handle*, enum tep_event_sort_type);
typedef struct tep_format_field** (*tep_event_common_fields_fn)(struct tep_event*);
typedef struct tep_format_field** (*tep_event_fields_fn)(struct tep_event*);
typedef int                       (*tep_get_cpus_fn)(struct tep_handle*);
typedef void                      (*tep_set_cpus_fn)(struct tep_handle*, int);
typedef int                       (*tep_get_long_size_fn)(struct tep_handle*);
typedef void                      (*tep_set_long_size_fn)(struct tep_handle*, int);
typedef int                       (*tep_get_page_size_fn)(struct tep_handle*);
typedef void                      (*tep_set_page_size_fn)(struct tep_handle*, int);
typedef bool                      (*tep_is_file_bigendian_fn)(struct tep_handle*);
typedef void                      (*tep_set_file_bigendian_fn)(struct tep_handle*, enum tep_endian);
typedef bool                      (*tep_is_local_bigendian_fn)(struct tep_handle*);
typedef void                      (*tep_set_local_bigendian_fn)(struct tep_handle*, enum tep_endian);
typedef int                       (*tep_get_header_page_size_fn)(struct tep_handle*);
typedef int                       (*tep_get_header_timestamp_size_fn)(struct tep_handle*);
typedef bool                      (*tep_is_old_format_fn)(struct tep_handle*);
typedef void                      (*tep_set_test_filters_fn)(struct tep_handle*, int);
typedef struct tep_handle*        (*tep_alloc_fn)(void);
typedef void                      (*tep_free_fn)(struct tep_handle*);
typedef void                      (*tep_ref_fn)(struct tep_handle*);
typedef void                      (*tep_unref_fn)(struct tep_handle*);
typedef int                       (*tep_get_ref_fn)(struct tep_handle*);
typedef void                      (*tep_print_funcs_fn)(struct tep_handle*);
typedef void                      (*tep_print_printk_fn)(struct tep_handle*);
typedef struct tep_event_filter*  (*tep_filter_alloc_fn)(struct tep_handle*);
typedef enum tep_errno            (*tep_filter_add_filter_str_fn)(struct tep_event_filter*, const char*);
typedef enum tep_errno            (*tep_filter_match_fn)(struct tep_event_filter*, struct tep_record*);
typedef int                       (*tep_filter_strerror_fn)(struct tep_event_filter*, enum tep_errno, char*, size_t);
typedef int                       (*tep_event_filtered_fn)(struct tep_event_filter*, int);
typedef void                      (*tep_filter_reset_fn)(struct tep_event_filter*);
typedef void                      (*tep_filter_free_fn)(struct tep_event_filter*);
typedef char*                     (*tep_filter_make_string_fn)(struct tep_event_filter*, int);
typedef int                       (*tep_filter_remove_event_fn)(struct tep_event_filter*, int);
typedef int                       (*tep_filter_copy_fn)(struct tep_event_filter*, struct tep_event_filter*);
typedef int                       (*tep_filter_compare_fn)(struct tep_event_filter*, struct tep_event_filter*);
typedef void                      (*tep_set_loglevel_fn)(enum tep_loglevel);
