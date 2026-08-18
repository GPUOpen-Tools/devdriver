//=============================================================================
/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief libtracefs entrypoint definitions
//=============================================================================

#pragma once

#include "tracefs.h"

typedef char*                              (*tracefs_get_tracing_file_fn)(const char*);
typedef void                               (*tracefs_put_tracing_file_fn)(char* name);
typedef const char*                        (*tracefs_tracing_dir_fn)();
typedef struct tracefs_instance*           (*tracefs_instance_create_fn)(const char*);
typedef struct tracefs_instance*           (*tracefs_instance_alloc_fn)(const char*, const char*);
typedef void                               (*tracefs_instance_free_fn)(struct tracefs_instance*);
typedef int                                (*tracefs_instance_destroy_fn)(struct tracefs_instance*);
typedef bool                               (*tracefs_instance_is_new_fn)(struct tracefs_instance*);
typedef const char*                        (*tracefs_instance_get_name_fn)(struct tracefs_instance*);
typedef const char*                        (*tracefs_instance_get_trace_dir_fn)(struct tracefs_instance*);
typedef char*                              (*tracefs_instance_get_file_fn)(struct tracefs_instance*, const char*);
typedef char*                              (*tracefs_instance_get_dir_fn)(struct tracefs_instance*);
typedef int                                (*tracefs_instance_file_write_fn)(struct tracefs_instance*, const char*, const char*);
typedef int                                (*tracefs_instance_file_append_fn)(struct tracefs_instance*, const char*, const char*);
typedef int                                (*tracefs_instance_file_clear_fn)(struct tracefs_instance*, const char*);
typedef char*                              (*tracefs_instance_file_read_fn)(struct tracefs_instance*, const char*, int*);
typedef int                                (*tracefs_instance_file_read_number_fn)(struct tracefs_instance*, const char*, long long*);
typedef int                                (*tracefs_instance_file_open_fn)(struct tracefs_instance*, const char*, int);
typedef int                                (*tracefs_instances_walk_fn)(int (*)(const char*, void*), void*);
typedef bool                               (*tracefs_instance_exists_fn)(const char*);
typedef bool                               (*tracefs_file_exists_fn)(struct tracefs_instance*, const char*);
typedef bool                               (*tracefs_dir_exists_fn)(struct tracefs_instance*, const char*);
typedef int                                (*tracefs_trace_is_on_fn)(struct tracefs_instance*);
typedef int                                (*tracefs_trace_on_fn)(struct tracefs_instance*);
typedef int                                (*tracefs_trace_off_fn)(struct tracefs_instance*);
typedef int                                (*tracefs_trace_on_fd_fn)(int);
typedef int                                (*tracefs_trace_off_fd_fn)(int);
typedef int                                (*tracefs_event_enable_fn)(struct tracefs_instance*, const char*, const char*);
typedef int                                (*tracefs_event_disable_fn)(struct tracefs_instance*, const char*, const char*);
typedef int                                (*tracefs_print_init_fn)(struct tracefs_instance*);
typedef int                                (*tracefs_printf_fn)(struct tracefs_instance*, const char*, ...);
typedef int                                (*tracefs_vprintf_fn)(struct tracefs_instance*, const char*, va_list);
typedef void                               (*tracefs_print_close_fn)(struct tracefs_instance*);
typedef int                                (*tracefs_binary_init_fn)(struct tracefs_instance*);
typedef int                                (*tracefs_binary_write_fn)(struct tracefs_instance*);
typedef void                               (*tracefs_binary_close_fn)(struct tracefs_instance*);
typedef void                               (*tracefs_list_free_fn)(char**);
typedef char**                             (*tracefs_event_systems_fn)(const char*);
typedef char**                             (*tracefs_system_events_fn)(const char*, const char*);
typedef int                                (*tracefs_iterate_raw_events_fn)(struct tep_handle*,
                                             struct tracefs_instance*,
                                             cpu_set_t*,
                                             int,
                                             int (*)(struct tep_event*, struct tep_record*, int, void*),
                                             void*);
typedef char**                             (*tracefs_tracers_fn)(const char*);
typedef struct tep_handle*                 (*tracefs_local_events_fn)(const char*);
typedef struct tep_handle*                 (*tracefs_local_events_system_fn)(const char*, const char* const*);
typedef int                                (*tracefs_fill_local_events_fn)(const char*, struct tep_handle*, int*);
typedef int                                (*tracefs_load_cmdlines_fn)(const char*, struct tep_handle*);
typedef char*                              (*tracefs_get_clock_fn)(struct tracefs_instance*);
typedef bool                               (*tracefs_option_mask_is_set_fn)(const struct tracefs_options_mask*, enum tracefs_option_id);
typedef const struct tracefs_options_mask* (*tracefs_options_get_supported_fn)(struct tracefs_instance*);
typedef bool                               (*tracefs_option_is_supported_fn)(struct tracefs_instance*, enum tracefs_option_id);
typedef const struct tracefs_options_mask* (*tracefs_options_get_enabled_fn)(struct tracefs_instance*);
typedef bool                               (*tracefs_option_is_enabled_fn)(struct tracefs_instance*, enum tracefs_option_id);
typedef int                                (*tracefs_option_enable_fn)(struct tracefs_instance*, enum tracefs_option_id);
typedef int                                (*tracefs_option_disable_fn)(struct tracefs_instance*, enum tracefs_option_id);
typedef const char*                        (*tracefs_option_name_fn)(enum tracefs_option_id);
typedef enum tracefs_option_id             (*tracefs_option_id_fn)(const char*);
typedef int                                (*tracefs_function_filter_fn)(struct tracefs_instance*, const char*, const char*, unsigned int);
typedef int                                (*tracefs_function_notrace_fn)(struct tracefs_instance*, const char*, const char*, unsigned int);
typedef void                               (*tracefs_set_loglevel_fn)(enum tep_loglevel);
