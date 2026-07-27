## Copyright (C) 2019-2026 Advanced Micro Devices, Inc. All rights reserved. ##

### CommandModuleExt API ##############################################################################################
devdriver_library(CommandModuleExt INTERFACE)
target_link_libraries(CommandModuleExt INTERFACE ddModule)

if (DD_BP_INSTALL)
	install(FILES inc/CommandModuleExt.h DESTINATION inc)
endif()

target_include_directories(CommandModuleExt INTERFACE inc)
