## Copyright (C) 2019-2026 Advanced Micro Devices, Inc. All rights reserved. ##

include(DevDriver)

### Create ddCore Library #############################################################################################
devdriver_library(ddCore STATIC)

### Cached Project Options #############################################################################################
set(DD_OPT_LOG_LEVEL ${DD_OPT_LOG_LEVEL} CACHE STRING "Developer driver mode log level? [0-4]")

if(UNIX)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(ddCore PRIVATE Threads::Threads)
endif()

# Support for dynamic library loading
target_link_libraries(ddCore PRIVATE ${CMAKE_DL_LIBS})

# Portable printf
target_link_libraries(ddCore PRIVATE stb_sprintf)

# Common Hashing Utility
# This is used in headers and needs needs to be public
target_link_libraries(ddCore PUBLIC metrohash)

# CRC32
target_link_libraries(ddCore PUBLIC dd_crc32)

### Build Defines ######################################################################################################

# Also enable these features from CMake
option(DD_OPT_ASSERTS_DEBUGBREAK "Trigger a debug break when asserts fail - implies DD_OPT_ASSERTS_ENABLE")

# Enable features with debug builds unconditionally
target_compile_definitions(ddCore PUBLIC
        $<$<CONFIG:Debug>:DD_OPT_ASSERTS_ENABLE>
        $<$<CONFIG:Debug>:DD_OPT_ASSERTS_DEBUGBREAK>)

if (DD_OPT_ASSERTS_DEBUGBREAK)
    set(DD_OPT_ASSERTS_ENABLE ON)
    target_compile_definitions(ddCore PUBLIC DD_OPT_ASSERTS_DEBUGBREAK)
endif()

if (DD_OPT_ASSERTS_ENABLE)
    target_compile_definitions(ddCore PUBLIC DD_OPT_ASSERTS_ENABLE)
endif()

if(DEFINED DD_OPT_LOG_LEVEL AND NOT "${DD_OPT_LOG_LEVEL}" STREQUAL "")
    target_compile_definitions(ddCore PUBLIC DD_OPT_LOG_LEVEL=${DD_OPT_LOG_LEVEL} DD_OPT_LOG_LEVEL_SET=1)
endif()

### Include Directories ################################################################################################
# target_include_directories(ddCore PUBLIC inc)
# Workaround for ATIMake compatibility: We aren't able to change our include directories from the gpuopen directory
# because they've been hardcoded in ATIMake components. We can replace this with the include directive above once
# we move away from ATIMake.
target_include_directories(ddCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(ddCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../inc)
target_include_directories(ddCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(ddCore PRIVATE src)

### Sources ############################################################################################################
target_sources(ddCore
    PRIVATE
        src/ddcPlatform.cpp
        src/ddcMemory.cpp
)
target_sources(ddCore
    PRIVATE
        inc/ddcDefs.h
        inc/ddcPlatform.h
        inc/ddcTemplate.h
)

### Platform ###
if (DD_TOOLCHAIN_WA_BUILD_KERNELMODE)

    target_compile_definitions(ddCore
        PUBLIC
            "-DDD_PLATFORM_STRING=\"${CMAKE_SYSTEM_NAME} Kernel Mode\""
    )

else()

    target_compile_definitions(ddCore
        PUBLIC
            "-DDD_PLATFORM_STRING=\"${CMAKE_SYSTEM_NAME} User Mode\""
    )

endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")

    target_compile_definitions(ddCore
        PUBLIC
            "DD_PLATFORM_LINUX_UM=1"
            "DD_PLATFORM_IS_UM=1"
            "DD_PLATFORM_IS_GNU=1"
            "DD_PLATFORM_IS_POSIX=1")
    target_sources(ddCore
        PRIVATE
            src/platforms/ddcPosixPlatform.cpp
            inc/platforms/ddcPosixPlatform.h)

elseif(CMAKE_SYSTEM_NAME MATCHES "Android")

    # Android is Linux, so we reuse the Linux platform.
    # Note however, that it is not GNU, so some GNU-things are not available on Android.
    target_compile_definitions(ddCore
        PUBLIC
            "DD_PLATFORM_LINUX_UM=1"
            "DD_PLATFORM_IS_UM=1"
            "DD_PLATFORM_IS_POSIX=1")
    target_sources(ddCore
        PRIVATE
            src/platforms/ddcPosixPlatform.cpp
            inc/platforms/ddcPosixPlatform.h)

elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")

    target_compile_definitions(ddCore
        PUBLIC
            "DD_PLATFORM_DARWIN_UM=1"
            "DD_PLATFORM_IS_UM=1"
            "DD_PLATFORM_IS_POSIX=1")
    target_sources(ddCore
        PRIVATE
            src/platforms/ddcPosixPlatform.cpp
            inc/platforms/ddcPosixPlatform.h)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")

    if(DD_TOOLCHAIN_WA_BUILD_KERNELMODE)

        target_compile_definitions(ddCore
            PUBLIC
                "DD_PLATFORM_WINDOWS_KM=1"
                "DD_PLATFORM_IS_KM=1")
        target_sources(ddCore
            PRIVATE
                src/platforms/ddcWinKernelPlatform.cpp
                inc/platforms/ddcWinKernelPlatform.h)

    else()

        target_compile_definitions(ddCore
            PUBLIC
                "DD_PLATFORM_WINDOWS_UM=1"
                "DD_PLATFORM_IS_UM=1")
        target_sources(ddCore
            PRIVATE
                src/platforms/ddcWinPlatform.cpp
                inc/platforms/ddcWinPlatform.h)

    endif()

else()
    message(FATAL_ERROR "Unrecognized platform: ${CMAKE_SYSTEM_NAME} - cannot build platform abstraction")
endif()
