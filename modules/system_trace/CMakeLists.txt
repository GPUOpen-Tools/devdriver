## Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. ##

### Module ############################################################################################################
devdriver_library(SystemTraceModule STATIC)

target_link_libraries(SystemTraceModule
        PUBLIC
        ddModule
        PRIVATE
        devdriver
        ddCommon
        ddModuleUtil
        ddEventServer
        ddGpuInfo
        ddRpcServer
        )

target_include_directories(SystemTraceModule
        PRIVATE
        src
        )

target_sources(SystemTraceModule PRIVATE
        src/system_trace_module_connection_context.h
        src/system_trace_module_connection_context.cpp
        src/SystemTraceModuleConfig.h
        src/SystemTraceModuleConfig.cpp
        src/rmt_token_emitter.h
        src/rmt_token_emitter.cpp
        )

# SystemTrace module is enabled on Windows and Linux platforms.
#
# libtraceevent is a library SystemTrace module uses on Linux for kernel
# event tracing. Due to a limitation in libtraceevent's Makefile, kernel event
# tracing is not available on x86 Linux.
set(STM_ENABLE_KERNEL_TRACING OFF)

if (WIN32)
    # Windows supports kernel event tracing via KMD
    set(STM_ENABLE_KERNEL_TRACING ON)

    target_sources(SystemTraceModule PRIVATE
            src/win/d3d12_etw_direct3d12_parser.h
            src/win/d3d12_etw_dxgk_parser.h
            src/win/d3d12_etw_event_parser.h
            src/win/dd_etw_debug_name_server_session.h
            src/win/dd_win_etw_server_session.h
            src/win/trace_session.h
            src/win/trace_session.cpp
            src/win/etw_server.h
            src/win/etw_server.cpp
            src/win/debug_name_token_emitter.h
            src/win/debug_name_token_emitter.cpp
            )

    # The Tdh library is required for ETW (which is part of SystemTrace module on
    # Windows) support
    target_link_libraries(SystemTraceModule
            PRIVATE
            Tdh
            )
elseif ((CMAKE_SIZEOF_VOID_P EQUAL 8) AND (${CMAKE_SYSTEM_NAME} STREQUAL "Linux"))
    # 64-bit Linux/Android supports kernel tracing via libtraceevent, but 32-bit does not.
    set(STM_ENABLE_KERNEL_TRACING ON)

    if (STM_ENABLE_KERNEL_TRACING)
        target_sources(SystemTraceModule PRIVATE
                src/linux/rmt_ftrace.h
                src/linux/rmt_ftrace.cpp
                src/linux/page_table_update_token_emitter.h
                src/linux/page_table_update_token_emitter.cpp
                src/linux/trace_file_system.h
                src/linux/trace_file_system.cpp
                src/linux/trace_event_parser.h
                src/linux/trace_event_parser.cpp
                )

        find_package(Libftrace REQUIRED)

        target_link_libraries(SystemTraceModule
                PRIVATE
                    libtraceevent
                    libtracefs)

        ### Unit tests ###################################################################################
        if (DD_OPT_ENABLE_TESTS)
            devdriver_executable(tests_LinuxRMT
                    src/linux/rmt_ftrace.cpp
                    src/linux/rmt_ftrace.h
                    src/linux/tests/rmt_tests.cpp
                    )

            target_include_directories(tests_LinuxRMT PRIVATE
                    src/linux
                    src/linux/tests
                    )

            target_link_libraries(tests_LinuxRMT
                    PUBLIC
                    ddCommon
                    gtest
                    )

            set_target_properties(tests_LinuxRMT
                    PROPERTIES
                    FOLDER "${DEVDRIVER_TESTS_ROOT}"
                    )

            devdriver_discover_tests(tests_LinuxRMT)
        endif ()
    endif ()
endif ()

target_compile_definitions(SystemTraceModule
        PRIVATE
        STM_ENABLE_KERNEL_TRACING=$<BOOL:${STM_ENABLE_KERNEL_TRACING}>
        )

# Generate module boilerplate code for our target
dd_generate_boilerplate_module_code(
        TARGET_NAME SystemTraceModule
        MODULE_NAME "SystemTrace"
        MODULE_DESCRIPTION "Module that is loaded by the router and exposes SystemTrace related functionalities."
        MODULE_VERSION_MAJOR 0
        MODULE_VERSION_MINOR 1
        MODULE_VERSION_PATCH 0
        MODULE_NAMESPACE "SystemTraceModule"
        CONNECTION_CONTEXT_CLASS_NAME "ModuleConnectionContext"
)

