## Copyright (C) 2019-2026 Advanced Micro Devices, Inc. All rights reserved. ##

set(CMAKE_FOLDER "${CMAKE_FOLDER}/apis")

### ddNet #############################################################################################################
add_subdirectory(ddNet)

### ddRouter API ######################################################################################################
if (DD_BP_ENABLE_TOOL_LIBRARIES)
    add_subdirectory(ddRouter)
endif()

### ddSocket ##########################################################################################################
add_subdirectory(ddSocket)

### ddRpc #############################################################################################################
add_subdirectory(ddRpc)

### ddEvent ###########################################################################################################
add_subdirectory(ddEvent)

### Rgd Mgr API #######################################################################################################
if (DD_BP_ENABLE_RGD_MGR)
    add_subdirectory(rgdMgr)
endif()

add_subdirectory(common)
add_subdirectory(events)

if (DD_BP_ENABLE_TOOL_LIBRARIES)
    add_subdirectory_if_exist(tools)
endif()

add_subdirectory_if_exist(settings)
