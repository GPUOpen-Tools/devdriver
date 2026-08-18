## Copyright (C) 2021-2025 Advanced Micro Devices, Inc. All rights reserved. ##

add_executable(rdf.Test)
target_sources(rdf.Test PRIVATE
    inc/test_rdf.h
    src/IO_test.cpp
    src/RDF_test.cpp
    src/test_file.c
    src/main.cpp
)

target_link_libraries(rdf.Test PRIVATE rdf catch2)
target_include_directories(rdf.Test PRIVATE inc)
add_test(NAME rdf.Test COMMAND rdf.Test)

if (WIN32)
    target_compile_definitions(rdf.Test PRIVATE RDF_PLATFORM_WINDOWS=1)
endif()
if (UNIX)
    target_compile_definitions(rdf.Test PRIVATE RDF_PLATFORM_UNIX=1)
endif()

if(RDF_BUILD_INSTALL)
    install(TARGETS rdf.Test DESTINATION bin)
endif()
