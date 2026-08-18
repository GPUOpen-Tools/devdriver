## Copyright (C) 2019-2026 Advanced Micro Devices, Inc. All rights reserved. ##

### MercuryModuleExt API ##############################################################################################
devdriver_library(MercuryModuleExt INTERFACE)
target_link_libraries(MercuryModuleExt INTERFACE ddModule)

if (DD_BP_INSTALL)
	install(FILES inc/MercuryModuleExt.h DESTINATION inc)
endif()

target_include_directories(MercuryModuleExt INTERFACE inc)
