## Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. ##

devdriver_library(ddYaml STATIC)

target_include_directories(ddYaml PUBLIC inc)

target_sources(ddYaml PRIVATE
    inc/ddYaml.h
    src/ddYaml.cpp
)

target_link_libraries(ddYaml
    PUBLIC
        dd_libyaml
    PRIVATE
        devdriver
)
