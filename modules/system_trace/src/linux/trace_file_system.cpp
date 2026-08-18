//=============================================================================
/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */
/// @author AMD Developer Tools Team
/// @file
/// @brief libtracefs dynamic loader implementation
//=============================================================================

#include "trace_file_system.h"

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

TraceFileSystem& TraceFileSystem::Get()
{
    static TraceFileSystem trace_fs;
    return trace_fs;
}

bool TraceFileSystem::Initialize()
{
    auto& instance = Get();

    // Load symbols
    void* handle = dlopen("libtracefs.so.1", RTLD_LAZY);
    if (handle == nullptr)
    {
        return false;
    }

    instance.get_tracing_file_          = LOAD_SYMBOL(handle, tracefs_get_tracing_file);
    instance.put_tracing_file_          = LOAD_SYMBOL(handle, tracefs_put_tracing_file);
    instance.tracing_dir_               = LOAD_SYMBOL(handle, tracefs_tracing_dir);
    instance.instance_create_           = LOAD_SYMBOL(handle, tracefs_instance_create);
    instance.instance_free_             = LOAD_SYMBOL(handle, tracefs_instance_free);
    instance.instance_alloc_            = LOAD_SYMBOL(handle, tracefs_instance_alloc);
    instance.instance_destroy_          = LOAD_SYMBOL(handle, tracefs_instance_destroy);
    instance.instance_is_new_           = LOAD_SYMBOL(handle, tracefs_instance_is_new);
    instance.instance_get_name_         = LOAD_SYMBOL(handle, tracefs_instance_get_name);
    instance.instance_get_trace_dir_    = LOAD_SYMBOL(handle, tracefs_instance_get_trace_dir);
    instance.instance_get_file_         = LOAD_SYMBOL(handle, tracefs_instance_get_file);
    instance.instance_get_dir_          = LOAD_SYMBOL(handle, tracefs_instance_get_dir);
    instance.instance_file_write_       = LOAD_SYMBOL(handle, tracefs_instance_file_write);
    instance.instance_file_append_      = LOAD_SYMBOL(handle, tracefs_instance_file_append);
    instance.instance_file_clear_       = LOAD_SYMBOL(handle, tracefs_instance_file_clear);
    instance.instance_file_read_        = LOAD_SYMBOL(handle, tracefs_instance_file_read);
    instance.instance_file_read_number_ = LOAD_SYMBOL(handle, tracefs_instance_file_read_number);
    instance.instance_file_open_        = LOAD_SYMBOL(handle, tracefs_instance_file_open);
    instance.instances_walk_            = LOAD_SYMBOL(handle, tracefs_instances_walk);
    instance.instance_exists_           = LOAD_SYMBOL(handle, tracefs_instance_exists);
    instance.file_exists_               = LOAD_SYMBOL(handle, tracefs_file_exists);
    instance.dir_exists_                = LOAD_SYMBOL(handle, tracefs_dir_exists);
    instance.trace_is_on_               = LOAD_SYMBOL(handle, tracefs_trace_is_on);
    instance.trace_on_                  = LOAD_SYMBOL(handle, tracefs_trace_on);
    instance.trace_off_                 = LOAD_SYMBOL(handle, tracefs_trace_off);
    instance.trace_on_fd_               = LOAD_SYMBOL(handle, tracefs_trace_on_fd);
    instance.trace_off_fd_              = LOAD_SYMBOL(handle, tracefs_trace_off_fd);
    instance.event_enable_              = LOAD_SYMBOL(handle, tracefs_event_enable);
    instance.event_disable_             = LOAD_SYMBOL(handle, tracefs_event_disable);
    instance.print_init_                = LOAD_SYMBOL(handle, tracefs_print_init);
    instance.printf_                    = LOAD_SYMBOL(handle, tracefs_printf);
    instance.vprintf_                   = LOAD_SYMBOL(handle, tracefs_vprintf);
    instance.print_close_               = LOAD_SYMBOL(handle, tracefs_print_close);
    instance.binary_init_               = LOAD_SYMBOL(handle, tracefs_binary_init);
    instance.binary_write_              = LOAD_SYMBOL(handle, tracefs_binary_write);
    instance.binary_close_              = LOAD_SYMBOL(handle, tracefs_binary_close);
    instance.list_free_                 = LOAD_SYMBOL(handle, tracefs_list_free);
    instance.event_systems_             = LOAD_SYMBOL(handle, tracefs_event_systems);
    instance.system_events_             = LOAD_SYMBOL(handle, tracefs_system_events);
    instance.iterate_raw_events_        = LOAD_SYMBOL(handle, tracefs_iterate_raw_events);
    instance.tracers_                   = LOAD_SYMBOL(handle, tracefs_tracers);
    instance.local_events_              = LOAD_SYMBOL(handle, tracefs_local_events);
    instance.local_events_system_       = LOAD_SYMBOL(handle, tracefs_local_events_system);
    instance.fill_local_events_         = LOAD_SYMBOL(handle, tracefs_fill_local_events);
    instance.load_cmdlines_             = LOAD_SYMBOL(handle, tracefs_load_cmdlines);
    instance.get_clock_                 = LOAD_SYMBOL(handle, tracefs_get_clock);
    instance.option_mask_is_set_        = LOAD_SYMBOL(handle, tracefs_option_mask_is_set);
    instance.options_get_supported_     = LOAD_SYMBOL(handle, tracefs_options_get_supported);
    instance.option_is_supported_       = LOAD_SYMBOL(handle, tracefs_option_is_supported);
    instance.options_get_enabled_       = LOAD_SYMBOL(handle, tracefs_options_get_enabled);
    instance.option_is_enabled_         = LOAD_SYMBOL(handle, tracefs_option_is_enabled);
    instance.option_enable_             = LOAD_SYMBOL(handle, tracefs_option_enable);
    instance.option_disable_            = LOAD_SYMBOL(handle, tracefs_option_disable);
    instance.option_name_               = LOAD_SYMBOL(handle, tracefs_option_name);
    instance.function_filter_           = LOAD_SYMBOL(handle, tracefs_function_filter);
    instance.function_notrace_          = LOAD_SYMBOL(handle, tracefs_function_notrace);
    instance.set_loglevel_              = LOAD_SYMBOL(handle, tracefs_set_loglevel);

    return true;
}

struct tracefs_instance* TraceFileSystem::InstanceCreate(const char* name)
{
    const auto& file_system = TraceFileSystem::Get();
    if (file_system.instance_create_ != nullptr)
    {
        return file_system.instance_create_(name);
    }

    return nullptr;
}

char* TraceFileSystem::InstanceGetFile(struct tracefs_instance* instance, const char* file)
{
    const auto& file_system = TraceFileSystem::Get();
    if (file_system.instance_get_file_ != nullptr)
    {
        return file_system.instance_get_file_(instance, file);
    }

    return nullptr;
}

void TraceFileSystem::InstanceFree(struct tracefs_instance* instance)
{
    const auto& file_system = TraceFileSystem::Get();
    if (file_system.instance_free_ != nullptr)
    {
        file_system.instance_free_(instance);
    }
}

void TraceFileSystem::InstanceDestroy(struct tracefs_instance* instance)
{
    const auto& file_system = TraceFileSystem::Get();
    if (file_system.instance_destroy_ != nullptr)
    {
        file_system.instance_destroy_(instance);
    }
}

void TraceFileSystem::TraceOn(struct tracefs_instance* instance)
{
    const auto& file_system = TraceFileSystem::Get();
    if (file_system.trace_on_ != nullptr)
    {
        file_system.trace_on_(instance);
    }
}

void TraceFileSystem::TraceOff(struct tracefs_instance* instance)
{
    const auto& file_system = TraceFileSystem::Get();
    if (file_system.trace_off_ != nullptr)
    {
        file_system.trace_off_(instance);
    }
}

char* TraceFileSystem::GetTracingDirectory(struct tracefs_instance* instance)
{
    const auto& file_system = TraceFileSystem::Get();
    if (file_system.instance_get_dir_ != nullptr)
    {
        return file_system.instance_get_dir_(instance);
    }

    return nullptr;
}

void TraceFileSystem::PutTracingFile(char* name)
{
    const auto& file_system = TraceFileSystem::Get();
    if (file_system.put_tracing_file_ != nullptr)
    {
        file_system.put_tracing_file_(name);
    }
}

char* TraceFileSystem::InstanceFileRead(struct tracefs_instance* instance, const char* file, int* psize)
{
    const auto& file_system = TraceFileSystem::Get();
    if (file_system.instance_file_read_ != nullptr)
    {
        return file_system.instance_file_read_(instance, file, psize);
    }

    return nullptr;
}

int TraceFileSystem::IterateRawEvents(struct tep_handle*       tep,
                                      struct tracefs_instance* instance,
                                      cpu_set_t*               cpus,
                                      int                      cpu_size,
                                      int                      (*callback)(struct tep_event*, struct tep_record*, int, void*),
                                      void*                    callback_context)
{
    const auto& file_system = TraceFileSystem::Get();
    if (file_system.iterate_raw_events_ != nullptr)
    {
        return file_system.iterate_raw_events_(tep, instance, cpus, cpu_size, callback, callback_context);
    }

    return -1;
}

struct tep_handle* TraceFileSystem::LocalEventsSystem(const char* tracing_directory, const char* const* system_names)
{
    const auto& file_system = TraceFileSystem::Get();
    if (file_system.local_events_system_ != nullptr)
    {
        return file_system.local_events_system_(tracing_directory, system_names);
    }

    return nullptr;
}
