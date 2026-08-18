## Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. ##

#.rst:
# FindLibftrace
# -------------
#
# Find libtraceevent and libtracefs libraries, either from the system or
# by downloading pre-built binaries from an internal artifactory (when
# available in non-sanitized builds).
#
# These libraries are used by the SystemTrace module on Linux for kernel
# event tracing.
#
# This module first attempts to locate system-installed libraries using
# pkg-config. If system libraries are not found, it falls back to
# downloading pre-built binaries (internal builds only).
#
# This will define the following variables:
#
# ``Libftrace_FOUND``
#     True if both libtraceevent and libtracefs are found
# ``Libtraceevent_VERSION``
#     The version of libtraceevent
# ``Libtracefs_VERSION``
#     The version of libtracefs
#
# If ``Libftrace_FOUND`` is TRUE, it will also define the following imported
# targets:
#
# ``libtraceevent``
#     The libtraceevent shared library
# ``libtracefs``
#     The libtracefs shared library

include_guard()

if(WIN32)
    message(FATAL_ERROR "FindLibftrace.cmake: libtraceevent/libtracefs are Linux-only libraries.")
endif()

### Try to find system-installed libraries first ###############################################################

# Use pkg-config to locate the libraries
find_package(PkgConfig QUIET)

set(_Libftrace_SYSTEM_FOUND TRUE)

if(PkgConfig_FOUND)
    ### libtraceevent (system) #################################################################################

    pkg_check_modules(PKG_Libtraceevent QUIET libtraceevent)

    find_path(Libtraceevent_INCLUDE_DIR
        NAMES
            event-parse.h
        HINTS
            ${PKG_Libtraceevent_INCLUDE_DIRS}
        PATH_SUFFIXES
            traceevent
    )

    find_library(Libtraceevent_LIBRARY
        NAMES
            traceevent
        HINTS
            ${PKG_Libtraceevent_LIBRARY_DIRS}
    )

    set(Libtraceevent_VERSION ${PKG_Libtraceevent_VERSION})

    ### libtracefs (system) ####################################################################################

    pkg_check_modules(PKG_Libtracefs QUIET libtracefs)

    find_path(Libtracefs_INCLUDE_DIR
        NAMES
            tracefs.h
        HINTS
            ${PKG_Libtracefs_INCLUDE_DIRS}
        PATH_SUFFIXES
            tracefs
    )

    find_library(Libtracefs_LIBRARY
        NAMES
            tracefs
        HINTS
            ${PKG_Libtracefs_LIBRARY_DIRS}
    )

    set(Libtracefs_VERSION ${PKG_Libtracefs_VERSION})

    ### Check if system libraries were found ###################################################################

    if(NOT Libtraceevent_LIBRARY OR NOT Libtraceevent_INCLUDE_DIR OR
       NOT Libtracefs_LIBRARY OR NOT Libtracefs_INCLUDE_DIR)
        set(_Libftrace_SYSTEM_FOUND FALSE)
    endif()
else()
    set(_Libftrace_SYSTEM_FOUND FALSE)
endif()

### Validation #################################################################################################

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Libftrace
    REQUIRED_VARS
        Libtraceevent_LIBRARY
        Libtraceevent_INCLUDE_DIR
        Libtracefs_LIBRARY
        Libtracefs_INCLUDE_DIR
    VERSION_VAR
        Libtraceevent_VERSION
)

### Imported targets ###########################################################################################

if(Libftrace_FOUND AND NOT TARGET libtraceevent)
    add_library(libtraceevent SHARED IMPORTED)
    set_target_properties(libtraceevent PROPERTIES
        IMPORTED_LOCATION "${Libtraceevent_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Libtraceevent_INCLUDE_DIR}"
    )
    if(_Libtraceevent_SONAME)
        set_target_properties(libtraceevent PROPERTIES
            IMPORTED_SONAME "${_Libtraceevent_SONAME}"
        )
    endif()
endif()

if(Libftrace_FOUND AND NOT TARGET libtracefs)
    add_library(libtracefs SHARED IMPORTED)
    set_target_properties(libtracefs PROPERTIES
        IMPORTED_LOCATION "${Libtracefs_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Libtracefs_INCLUDE_DIR}"
    )
    if(_Libtracefs_SONAME)
        set_target_properties(libtracefs PROPERTIES
            IMPORTED_SONAME "${_Libtracefs_SONAME}"
        )
    endif()
endif()

### Install rules ##############################################################################################

if(Libftrace_FOUND AND DD_BP_INSTALL)
    install(FILES "${Libtraceevent_LIBRARY}" DESTINATION bin)
    install(FILES "${Libtracefs_LIBRARY}" DESTINATION bin)
endif()

mark_as_advanced(
    Libtraceevent_LIBRARY
    Libtraceevent_INCLUDE_DIR
    Libtracefs_LIBRARY
    Libtracefs_INCLUDE_DIR
)
