## Copyright (C) 2019-2026 Advanced Micro Devices, Inc. All rights reserved. ##

### ddRouter API ######################################################################################################
devdriver_library(ddRouter STATIC)
target_link_libraries(ddRouter
    PUBLIC
        ddApi
    PRIVATE
        ddCommon
        ddModuleLoader
        ddNet
        ddRpcServer
        ddEventServer
        devdriver
)

if (DD_BP_INSTALL)
	install(FILES   inc/ddRouterApi.h DESTINATION inc)
	install(TARGETS ddRouter          DESTINATION lib)
endif()

target_include_directories(ddRouter PUBLIC  inc)
target_include_directories(ddRouter PRIVATE src)

target_sources(ddRouter PRIVATE
    inc/ddRouterApi.h
    inc/ddRouter.h
    src/ddRouter.cpp
    src/routerContext.h
    src/routerContext.cpp

    src/listener/listener.h
    src/listener/listener.cpp
    src/listener/listenerCore.h
    src/listener/listenerCore.cpp
    src/listener/routerCore.h
    src/listener/routerCore.cpp
    src/listener/transportThread.h
    src/listener/transportThread.cpp

    src/listener/transports/abstractListenerTransport.h
    src/listener/transports/socketTransport.h
    src/listener/transports/socketTransport.cpp

    src/listener/clientmanagers/abstractClientManager.h
    src/listener/clientmanagers/listenerClientManager.h
    src/listener/clientmanagers/listenerClientManager.cpp
)

if(WIN32)
    if (DD_AUTHENTICATED_USER_SECURITY)
        target_compile_definitions(ddRouter PRIVATE DD_AUTHENTICATED_USER_SECURITY)
    endif()
    target_sources(ddRouter
        PRIVATE
            # The listener core requires a special pipe transport implementation on Windows
            src/listener/transports/winPipeTransport.h
            src/listener/transports/winPipeTransport.cpp

            src/listener/transports/ddLocalNgListenerTransport.h
            src/listener/transports/ddLocalNgListenerTransport.cpp
    )

endif()
