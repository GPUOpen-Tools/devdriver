## Copyright (C) 2019-2026 Advanced Micro Devices, Inc. All rights reserved. ##

### ddModule API ######################################################################################################
devdriver_library(ddModule INTERFACE)
target_link_libraries(ddModule
    INTERFACE
        ddApi
)

if (DD_BP_INSTALL)
	install(FILES inc/ddModule.h DESTINATION inc)
endif()

target_include_directories(ddModule INTERFACE inc)
