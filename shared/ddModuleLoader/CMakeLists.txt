## Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. ##

### ddModuleLoader API ################################################################################################

devdriver_library(ddModuleLoader STATIC)

target_link_libraries(ddModuleLoader
    PUBLIC
        ddApi
        ddModule
    PRIVATE
        devdriver
        ddCommon
)

target_include_directories(ddModuleLoader PUBLIC  inc)
target_include_directories(ddModuleLoader PRIVATE src)

target_sources(ddModuleLoader
    PRIVATE
        inc/toolModule.h
        src/toolModule.cpp
)
