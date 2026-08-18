## Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. ##

### ddRdf #########################################################################################################
devdriver_library(ddRdf INTERFACE)

if (DD_BP_INSTALL)
	install(FILES inc/ddRdf.h DESTINATION inc)
endif()

target_include_directories(ddRdf INTERFACE inc)

