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

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
if(NOT DEFINED pybind11_DIR OR "${pybind11_DIR}" STREQUAL "")
    execute_process(
        COMMAND ${Python_EXECUTABLE} -m pybind11 --cmakedir
        OUTPUT_VARIABLE pybind11_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _pybind11_query_rc
    )
    if(NOT _pybind11_query_rc EQUAL 0)
        message(FATAL_ERROR
            "pybind11 not importable from the active Python interpreter "
            "(${Python_EXECUTABLE}). Install it: "
            "pip install pybind11  or  conda install -c conda-forge pybind11")
    endif()
endif()
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(oot_rl src/python_bindings.cpp)
target_link_libraries(oot_rl PRIVATE libultraship soh)
add_dependencies(oot_rl GenerateSohOtr)
set_target_properties(oot_rl PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/python
)

add_executable(smoke_test src/smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE libultraship soh)
add_dependencies(smoke_test GenerateSohOtr)

enable_testing()
add_test(NAME smoke_test
    COMMAND smoke_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(smoke_test PROPERTIES
    TIMEOUT 600
    ENVIRONMENT "LUS_APP_BUNDLE_PATH=${CMAKE_BINARY_DIR}")

set(_soh_assets_src ${CMAKE_SOURCE_DIR}/external/Shipwright/soh/assets/extractor)
set(_soh_o2r_src    ${CMAKE_SOURCE_DIR}/external/Shipwright/soh/soh.o2r)
set(_oot_o2r_src    ${CMAKE_SOURCE_DIR}/data/oot.o2r)

file(CREATE_LINK ${_soh_assets_src} ${CMAKE_BINARY_DIR}/assets SYMBOLIC)
file(CREATE_LINK ${_soh_o2r_src} ${CMAKE_BINARY_DIR}/soh.o2r SYMBOLIC)

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
        "  smoke_test will not run correctly until oot.o2r is present.\n"
    )
endif()
