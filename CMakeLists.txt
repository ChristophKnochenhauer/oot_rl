cmake_minimum_required(VERSION 3.26)
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
