cmake_minimum_required(VERSION 3.26)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

project(oot_rl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{CONDA_PREFIX})
    list(APPEND CMAKE_IGNORE_PATH "$ENV{CONDA_PREFIX}")
    message(STATUS "Ignoring Conda prefix: $ENV{CONDA_PREFIX}")
endif()
foreach(_conda_dir "$ENV{HOME}/miniconda3" "$ENV{HOME}/anaconda3" "/opt/conda")
    if(EXISTS "${_conda_dir}")
        list(APPEND CMAKE_IGNORE_PATH "${_conda_dir}")
    endif()
endforeach()

set(BUILD_AS_LIB ON CACHE BOOL "Build SoH as a library" FORCE)

add_subdirectory(external/Shipwright EXCLUDE_FROM_ALL)

add_executable(hello src/hello.cpp)
target_link_libraries(hello PRIVATE libultraship soh)

set(_soh_assets_src ${CMAKE_SOURCE_DIR}/external/Shipwright/soh/assets/extractor)
set(_soh_o2r_src    ${CMAKE_SOURCE_DIR}/external/Shipwright/soh/soh.o2r)
set(_oot_o2r_src    ${CMAKE_SOURCE_DIR}/data/oot.o2r)

file(CREATE_LINK ${_soh_assets_src} ${CMAKE_BINARY_DIR}/assets SYMBOLIC)
file(CREATE_LINK ${_soh_o2r_src} ${CMAKE_BINARY_DIR}/soh.o2r SYMBOLIC)
add_dependencies(hello GenerateSohOtr)

if(EXISTS ${_oot_o2r_src})
    file(CREATE_LINK ${_oot_o2r_src} ${CMAKE_BINARY_DIR}/oot.o2r SYMBOLIC)
else()
    message(WARNING
        "\n"
        "  oot.o2r not found at:\n"
        "    ${_oot_o2r_src}\n"
        "\n"
        "  You need to extract it from your own legally-owned OoT ROM.\n"
        "\n"
        "  hello will not run correctly until oot.o2r is present.\n"
    )
endif()
