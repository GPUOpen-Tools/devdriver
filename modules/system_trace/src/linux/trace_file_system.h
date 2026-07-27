//=============================================================================
/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief libtracefs dynamic loader definition
//=============================================================================

#pragma once

#include <functional>

#include "tracefs_function_types.h"

class TraceFileSystem
{
public:
    static TraceFileSystem& Get();

    static bool                     Initialize();
    static struct tracefs_instance* InstanceCreate(const char* name);
    static void                     InstanceDestroy(struct tracefs_instance* instance);
    static void                     InstanceFree(struct tracefs_instance* instance);
    static char*                    InstanceGetFile(struct tracefs_instance* instance, const char* file);
    static char*                    GetTracingDirectory(struct tracefs_instance* instance);
    static void                     TraceOn(struct tracefs_instance* instance);
    static void                     TraceOff(struct tracefs_instance* instance);
    static char*                    InstanceFileRead(struct tracefs_instance* instance, const char* file, int* psize);
    static struct tep_handle*       LocalEventsSystem(const char* tracing_directory, const char* const* system_names);
    static void                     PutTracingFile(char* name);
    static int                      IterateRawEvents(struct tep_handle*       tep,
                                                     struct tracefs_instance* instance,
                                                     cpu_set_t*               cpus,
                                                     int                      cpu_size,
                                                     int                      (*callback)(struct tep_event*, struct tep_record*, int, void*),
                                                     void*                    callback_context);

private:
    TraceFileSystem() = default;

    tracefs_get_tracing_file_fn          get_tracing_file_;
    tracefs_put_tracing_file_fn          put_tracing_file_;
    tracefs_tracing_dir_fn               tracing_dir_;
    tracefs_instance_create_fn           instance_create_;
    tracefs_instance_free_fn             instance_free_;
    tracefs_instance_alloc_fn            instance_alloc_;
    tracefs_instance_destroy_fn          instance_destroy_;
    tracefs_instance_is_new_fn           instance_is_new_;
    tracefs_instance_get_name_fn         instance_get_name_;
    tracefs_instance_get_trace_dir_fn    instance_get_trace_dir_;
    tracefs_instance_get_file_fn         instance_get_file_;
    tracefs_instance_get_dir_fn          instance_get_dir_;
    tracefs_instance_file_write_fn       instance_file_write_;
    tracefs_instance_file_append_fn      instance_file_append_;
    tracefs_instance_file_clear_fn       instance_file_clear_;
    tracefs_instance_file_read_fn        instance_file_read_;
    tracefs_instance_file_read_number_fn instance_file_read_number_;
    tracefs_instance_file_open_fn        instance_file_open_;
    tracefs_instances_walk_fn            instances_walk_;
    tracefs_instance_exists_fn           instance_exists_;
    tracefs_file_exists_fn               file_exists_;
    tracefs_dir_exists_fn                dir_exists_;
    tracefs_trace_is_on_fn               trace_is_on_;
    tracefs_trace_on_fn                  trace_on_;
    tracefs_trace_off_fn                 trace_off_;
    tracefs_trace_on_fd_fn               trace_on_fd_;
    tracefs_trace_off_fd_fn              trace_off_fd_;
    tracefs_event_enable_fn              event_enable_;
    tracefs_event_disable_fn             event_disable_;
    tracefs_print_init_fn                print_init_;
    tracefs_printf_fn                    printf_;
    tracefs_vprintf_fn                   vprintf_;
    tracefs_print_close_fn               print_close_;
    tracefs_binary_init_fn               binary_init_;
    tracefs_binary_write_fn              binary_write_;
    tracefs_binary_close_fn              binary_close_;
    tracefs_list_free_fn                 list_free_;
    tracefs_event_systems_fn             event_systems_;
    tracefs_system_events_fn             system_events_;
    tracefs_iterate_raw_events_fn        iterate_raw_events_;
    tracefs_tracers_fn                   tracers_;
    tracefs_local_events_fn              local_events_;
    tracefs_local_events_system_fn       local_events_system_;
    tracefs_fill_local_events_fn         fill_local_events_;
    tracefs_load_cmdlines_fn             load_cmdlines_;
    tracefs_get_clock_fn                 get_clock_;
    tracefs_option_mask_is_set_fn        option_mask_is_set_;
    tracefs_options_get_supported_fn     options_get_supported_;
    tracefs_option_is_supported_fn       option_is_supported_;
    tracefs_options_get_enabled_fn       options_get_enabled_;
    tracefs_option_is_enabled_fn         option_is_enabled_;
    tracefs_option_enable_fn             option_enable_;
    tracefs_option_disable_fn            option_disable_;
    tracefs_option_name_fn               option_name_;
    tracefs_function_filter_fn           function_filter_;
    tracefs_function_notrace_fn          function_notrace_;
    tracefs_set_loglevel_fn              set_loglevel_;
};
