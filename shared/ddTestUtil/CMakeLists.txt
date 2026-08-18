## Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. ##

### Utility Functionality For Tests ###################################################################################
devdriver_library(ddTestUtil STATIC)

target_link_libraries(ddTestUtil
    PUBLIC
        ddNet
        ddRouter
        ddCommon
        dd_common
        gtest
        ddRpcClient
        ddRpcServer
)

target_compile_definitions(ddTestUtil
    PUBLIC
        # gpuopen.h requires this been defined.
        # This is public so any test taking a dependency on this will have this
        # definition.
        GPUOPEN_CLIENT_INTERFACE_MAJOR_VERSION=43
)

target_include_directories(ddTestUtil
    PUBLIC
        inc
    PRIVATE
        src
)

target_sources(ddTestUtil PRIVATE
    inc/ddTestUtil.h
    src/ddTestUtil.cpp
)
