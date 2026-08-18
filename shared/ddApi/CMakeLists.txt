## Copyright (C) 2019-2026 Advanced Micro Devices, Inc. All rights reserved. ##

### ddApi #########################################################################################################
devdriver_library(ddApi STATIC)

target_sources(ddApi
    PRIVATE
        inc/ddApi.h
        src/ddApi.cpp
)

if (DD_BP_INSTALL)
	install(FILES inc/ddApi.h DESTINATION inc)
	install(TARGETS ddApi DESTINATION lib)
endif()

target_include_directories(ddApi PUBLIC inc)

# dd_common_api
devdriver_library(dd_common_api INTERFACE)
target_sources(dd_common_api INTERFACE inc/dd_common_api.h)
target_include_directories(dd_common_api INTERFACE inc)
