## Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. ##

devdriver_library(ddRmtEventTracer STATIC)

target_sources(ddRmtEventTracer PRIVATE
    inc/RmtEventTracer.h
    src/RmtEventStreamer.cpp
    src/RmtEventStreamer.h
    src/RmtEventTracer.cpp
)

target_include_directories(ddRmtEventTracer
    PUBLIC inc
    PRIVATE src
)

target_link_libraries(ddRmtEventTracer
    PUBLIC
        system_info
        ddRdf
    PRIVATE
        ddCommon
        ddEventClient
        ddEventParser
        amdrdf
)
