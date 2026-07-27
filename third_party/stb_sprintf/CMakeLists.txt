### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

add_library(stb_sprintf STATIC)

target_sources(stb_sprintf
    PRIVATE
        inc/stb_sprintf.h
        src/stb_sprintf.cpp)

target_include_directories(stb_sprintf PUBLIC inc)

# Set warning level 3
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(stb_sprintf PRIVATE /W3)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(stb_sprintf PRIVATE -Wall)
endif()