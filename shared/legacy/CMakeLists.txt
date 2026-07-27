## Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. ##

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
include(DevDriver)

# Put ZERO_CHECK, INSTALL, etc default targets in a separate folder in VS solutions
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

### Create GpuOpen Library #############################################################################################
devdriver_library(devdriver STATIC)

# Driver clients link against `gpuopen` instead of `devdriver`, so we need to support this target.
add_library(gpuopen ALIAS devdriver)

# Warn about legacy behavior
if(DEFINED GPUOPEN_LIB_NAME)
    message(WARNING
        "The 'gpuopen' name for the DevDriver component is deprecated and will be removed in the future. "
        "Please stop setting 'GPUOPEN_LIB_NAME' and link against `devdriver` directly instead."
    )
endif()

### Project Build Parameters ###########################################################################################
dd_bp(
    GPUOPEN_CLIENT_INTERFACE_MAJOR_VERSION 43
    MSG  "Major interface version for legacy GPUOPEN library"
    # No one should be setting this to anything other than 43 at this point.
    MODE "VERBOSE"
)

dd_bp(
    DD_BRANCH_STRING "unknown"
    # For driver and internal builds, we want this to be set always.
    # Tools do not need this, so ddLegacy sets a default ontop of this. See ddLegacy/CMakeLists.txt for details.
    MSG  "Forwards the branch name to DevDriver so that it can be queried from drivers by tools. See: AMD_BRDEFS_BRANCH_STRING"
    MODE "WARNING"
)

dd_bp(DD_BP_REMOTE_WIN_TRANSPORT ON
    DEPENDS_ON "WIN32"
    MSG        "Enables building remote transport on Windows. (Linux is unconditional)"
)

### Core Component #####################################################################################################
# WA: Check that this target hasn't already been added (Only until we have a global CMake context)
if(NOT TARGET ddCore)
    add_subdirectory(core)
endif()

target_link_libraries(devdriver
    PUBLIC
        ddCore
        metrohash
        dd_interface
        dd_event_registry
        ddApi
    PRIVATE
        mpack)

# WA: We will move this logic into a toolchain
# Json is not supported in kernel builds
if(NOT DD_TOOLCHAIN_WA_BUILD_KERNELMODE)
    target_link_libraries(devdriver PUBLIC  rapidjson)
endif()

### Build Defines ######################################################################################################
target_compile_definitions(devdriver
    PUBLIC
        GPUOPEN_CLIENT_INTERFACE_MAJOR_VERSION=${GPUOPEN_CLIENT_INTERFACE_MAJOR_VERSION}

    PRIVATE
        DD_BRANCH_STRING=\"${DD_BRANCH_STRING}\"
)

### Include Directories ################################################################################################
target_include_directories(devdriver PUBLIC inc)
target_include_directories(devdriver PRIVATE src)

### Sources ############################################################################################################

### Core Components ###
target_sources(devdriver PRIVATE
    ## Core files
    src/gpuopen.cpp
    src/protocolSession.cpp
    src/session.h
    src/session.cpp

    src/sessionManager.h
    src/sessionManager.cpp

    src/messageChannel.h
    src/messageChannel.inl
    src/messageChannel.cpp

    inc/baseProtocolServer.h
    src/baseProtocolServer.cpp

    inc/legacyProtocolClient.h
    src/legacyProtocolClient.cpp

    inc/ddTransferManager.h
    src/ddTransferManager.cpp

    src/ddClientURIService.h
    src/ddClientURIService.cpp

    src/ddUriInterface.cpp

    inc/ddURIRequestContext.h
    src/ddURIRequestContext.cpp

    ## Protocols
    inc/protocols/ddTransferServer.h
    src/protocols/ddTransferProtocol.cpp
    src/protocols/ddTransferServer.cpp
    src/protocols/ddEventProtocol.cpp

    inc/protocols/ddTransferClient.h
    src/protocols/ddTransferClient.cpp

    inc/protocols/ddURIClient.h
    src/protocols/ddURIClient.cpp

    src/protocols/ddURIServer.h
    src/protocols/ddURIServer.cpp

    inc/protocols/ddEventServer.h
    src/protocols/ddEventServer.cpp

    inc/protocols/ddEventProvider.h
    src/protocols/ddEventProvider.cpp

    src/protocols/systemProtocols.cpp

    src/protocols/ddEventServerSession.h
    src/protocols/ddEventServerSession.cpp

    src/protocols/ddInternalService.h
    src/protocols/ddInternalService.cpp

    ## Utility functions and classes
    inc/util/ddBitSet.h

    inc/util/ddByteReader.h
    src/util/ddByteReader.cpp

    inc/util/ddByteWriter.h

    inc/util/ddByteSwap.h

    inc/util/ddEventTimer.h
    src/util/ddEventTimer.cpp
    src/util/hashFunc.cpp

    inc/util/ddMetroHash.h
    src/util/ddMetroHash.cpp

    inc/util/ddStructuredReader.h
    src/util/ddStructuredReader.cpp

    inc/util/ddTextWriter.h
    src/util/ddTextWriter.cpp

    inc/util/hashBase.h
    inc/util/hashFunc.h
    inc/util/hashMap.h
    inc/util/hashSet.h

    inc/util/kmdEventDefs.h

    inc/util/rmtCommon.h
    inc/util/rmtFileFormat.h
    inc/util/rmtResourceDescriptions.h
    inc/util/rmtTokens.h
    inc/util/rmtWriter.h
    src/util/rmtWriter.cpp

    inc/util/queue.h
    inc/util/sharedptr.h
    src/util/sharedptr.cpp
    inc/util/string.h
    src/util/string.cpp
    inc/util/vector.h
    src/util/vector.cpp
)

target_link_libraries(devdriver PRIVATE dd_common)

# AmdGpuInfo library interface
# GpuInfo is not supported in kernel builds
if(NOT DD_TOOLCHAIN_WA_BUILD_KERNELMODE)
    add_subdirectory(gpuinfo)
endif()

# WA: We will move this logic into a toolchain
if(NOT DD_TOOLCHAIN_WA_BUILD_KERNELMODE)
    target_sources(devdriver PRIVATE
        inc/util/ddJsonWriter.h
        src/util/ddJsonWriter.cpp
    )
endif()

### Transports ###
if(UNIX)
    target_sources(devdriver PRIVATE
        src/posix/ddPosixSocket.cpp
        src/socketMsgTransport.cpp
    )
elseif(WIN32
# WA: We will move this logic into a toolchain
AND NOT DD_TOOLCHAIN_WA_BUILD_KERNELMODE
)
    target_sources(devdriver PRIVATE
        # WA: We will move this logic into a toolchain
        # TODO: Move the local transport + control device to the main source list once it's supported on all platforms.
        src/ddLocalNgMsgTransport.cpp
        src/ddDevModeControlDevice.cpp
        src/ddDevModeQueue.cpp
        src/win/ddWinKmIoCtlDevice.cpp
        src/win/ddWinUmIoCtlDevice.cpp
        src/win/ddWinPipeMsgTransport.cpp
    )
# WA: We will move this logic into a toolchain
elseif (WIN32 AND DD_TOOLCHAIN_WA_BUILD_KERNELMODE)
    target_sources(devdriver PRIVATE
        src/ddLocalNgMsgTransport.cpp
        src/ddDevModeQueue.cpp
    )
endif()

# Build remote transport (Only required for Windows UM since Linux always supports remote and Windows KM does not support remote)
if(DD_BP_REMOTE_WIN_TRANSPORT)
    target_sources(devdriver PRIVATE
        src/win/ddWinSocket.cpp
        src/socketMsgTransport.cpp
    )

    # Pass the build system variable down to the code
    target_compile_definitions(devdriver PUBLIC DEVDRIVER_BUILD_REMOTE_TRANSPORT)
endif()

### Router Components ###
# The utility driver code is both Windows only and closed source for now
if (WIN32 AND DD_BP_ENABLE_ROUTER_CORE)
    target_sources(devdriver PRIVATE
        src/ddRouterInterface.cpp
        src/router/ddMsgRouter.cpp
        src/router/ddRouterContext.cpp
        src/router/ddRouterClientManager.cpp
        src/router/ddRouterBusTransport.cpp
        src/router/ddRouterHostTransport.cpp
    )

    target_link_libraries(devdriver PUBLIC dd_event_registry)
endif()

### Protocols & Helpers ###
target_sources(devdriver
    PRIVATE
        # Helper classes
        src/devDriverServer.cpp
        src/devDriverClient.cpp

        # Driver Control Protocol
        src/protocols/driverControlClient.cpp
        src/protocols/driverControlServer.cpp

        # RGP Protocol
        src/protocols/rgpClient.cpp
        src/protocols/rgpServer.cpp

        # ETW Client (the Driver uses the Client, this is not a typo)
        src/protocols/etwClient.cpp
        # AMDLOG Support via devdriver
        src/ddAmdLogLogger.cpp
)

### Services ###
if (DD_TOOLCHAIN_WA_BUILD_KERNELMODE)
    target_sources(devdriver PRIVATE
        src/protocols/ddInfoService.cpp
    )
else()
    target_sources(devdriver PRIVATE
        src/protocols/ddPipelineUriService.cpp
        src/protocols/ddInfoService.cpp
    )
endif()
