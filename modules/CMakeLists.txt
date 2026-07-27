## Copyright (C) 2019-2026 Advanced Micro Devices, Inc. All rights reserved. ##

set(CMAKE_FOLDER "${CMAKE_FOLDER}/modules")

add_subdirectory_if_exist(routerutils)
add_subdirectory_if_exist(siphon)
add_subdirectory_if_exist(system_trace)
