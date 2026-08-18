## Copyright (C) 2019-2026 Advanced Micro Devices, Inc. All rights reserved. ##

include(templates/ddGenerateBoilerPlateModuleCode.cmake)

devdriver_library(ddModuleUtil STATIC)

target_link_libraries(ddModuleUtil
    PUBLIC
        ddModule
        ddCommon
        CommandModuleExt
        InfoClient
    PRIVATE
        devdriver
)

target_include_directories(ddModuleUtil
    PUBLIC
        inc
    PRIVATE
        src
)

target_sources(ddModuleUtil PRIVATE
    inc/BaseModuleSystemContext.h
    src/BaseModuleSystemContext.cpp

    inc/BaseModuleClientContext.h
    src/BaseModuleClientContext.cpp

    inc/BaseModuleCommandContext.h
    src/BaseModuleCommandContext.cpp

    inc/BaseModuleDataContext.h
    src/BaseModuleDataContext.cpp

    inc/BaseModuleConnectionContext.h
    src/BaseModuleConnectionContext.cpp

    inc/ModuleLogger.h
    src/ModuleLogger.cpp
)
