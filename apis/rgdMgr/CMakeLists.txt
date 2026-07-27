## Copyright (C) 2025-2026 Advanced Micro Devices, Inc. All rights reserved. ##

devdriver_library(rgd_mgr STATIC)

target_include_directories(rgd_mgr PUBLIC inc)

target_sources(rgd_mgr
    PUBLIC
        inc/RgdMgr.h
    PRIVATE
        src/RgdMgr.cpp
)

target_link_libraries(rgd_mgr
    PUBLIC
        ddRouter
        ddCommon
        dd_tools
        ddRdf
        rdf
        devdriver
    PRIVATE
        ddNet
        RouterUtilsModuleStatic
        ddRdfEventStreamer
        SystemTraceModuleStatic
)
