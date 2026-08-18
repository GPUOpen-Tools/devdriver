## Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. ##

devdriver_library(ddGpuDetectiveEventStreamer STATIC)

target_sources(ddGpuDetectiveEventStreamer PRIVATE
    inc/GpuDetectiveEventStreamer.h
    src/GpuDetectiveEventStreamer.cpp
)

target_include_directories(ddGpuDetectiveEventStreamer
    PUBLIC inc
    PRIVATE src
)

target_link_libraries(ddGpuDetectiveEventStreamer
    PUBLIC
        ddEventClient
        ddEventParser
    PRIVATE
        ddCommon
        dd_event_registry
        amdrdf
)
