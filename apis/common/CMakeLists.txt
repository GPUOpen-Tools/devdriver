## Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. ##

devdriver_library(dd_common STATIC)
target_include_directories(dd_common PUBLIC inc)
target_sources(dd_common
    PUBLIC
        inc/dd_timeout_constants.h
    PRIVATE
        inc/dd_assert.h
        inc/dd_constants.h
        inc/dd_dynamic_buffer.h
        inc/dd_integer.h
        inc/dd_memory.h
        inc/dd_mutex.h
        inc/dd_new.h
        inc/dd_optional.h
        inc/dd_platform_info.h
        inc/dd_result.h
        inc/dd_ring_buffer.h
        inc/dd_thread.h
        src/dd_dynamic_buffer.cpp
        src/dd_logger.cpp
        src/dd_new.cpp
        src/dd_memory.cpp
        src/dd_result.cpp
        src/dd_ring_buffer.cpp
        src/dd_timeout_constants.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_sources(dd_common
        PRIVATE
            src/dd_memory_windows.cpp
            src/dd_mutex_windows.cpp
            src/dd_platform_info_windows.cpp
            src/dd_result_windows.cpp
            src/dd_thread_windows.cpp
            src/dd_registry_utils.cpp
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(dd_common
        PRIVATE
            src/dd_memory_linux.cpp
            src/dd_mutex_linux.cpp
            src/dd_platform_info_linux.cpp
            src/dd_thread_linux.cpp
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_sources(dd_common
        PRIVATE
            src/dd_mutex_linux.cpp
    )
endif()

target_link_libraries(dd_common
    PUBLIC
        dd_interface
    PRIVATE
        stb_sprintf
        devdriver
)

if (WIN32)
    target_link_libraries(dd_common
        PRIVATE
            onecore # Windows library needed for VirtualAlloc2 and MapViewOfFile3
    )
endif()

if (DD_OPT_ASSERTS_ENABLE)
    target_compile_definitions(dd_common PUBLIC DD_OPT_ASSERTS_ENABLE)
endif()
