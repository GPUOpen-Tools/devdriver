## Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. ##

set(CMAKE_FOLDER ${CMAKE_FOLDER}/imported)

# RDF
if (DD_BUILD_RDF)
    set(RDF_ENABLE_CXX_BINDINGS OFF)
    set(RDF_STATIC ON)

    if (NOT TARGET amdrdf)
        add_subdirectory(rdf)
    endif ()
endif ()

# system_info_utils
if (DD_BP_ENABLE_TOOL_LIBRARIES)
    if (NOT TARGET system_info)
        set(SYSTEM_INFO_BUILD_WRITER ON)
        set(SYSTEM_INFO_BUILD_RDF_INTERFACES ON)
        if (SYSTEM_INFO_UTILS_DIR)
            add_subdirectory(${SYSTEM_INFO_UTILS_DIR} sys_info_utils)
        else ()
            add_subdirectory(sys_info_utils)
        endif ()
    endif ()
endif ()

