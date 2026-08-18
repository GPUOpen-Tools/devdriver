## Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved. ##

add_library(dd_crc32 STATIC src/dd_crc32.cpp)
target_include_directories(dd_crc32 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
