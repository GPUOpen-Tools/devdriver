## Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. ##

devdriver_library(ddRdfEventStreamer STATIC)

target_sources(ddRdfEventStreamer PRIVATE
    inc/RdfEventStreamer.h
    src/RdfEventStreamer.cpp
)

target_include_directories(ddRdfEventStreamer
    PUBLIC inc
    PRIVATE src
)

target_link_libraries(ddRdfEventStreamer
    PUBLIC
        ddEventClient
        ddEventParser
    PRIVATE
        ddCommon
        dd_event_registry
        amdrdf
)
