# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

if(EXISTS "/home/go29net/dev/oot_rl/build-cmake/_deps/dr_libs-subbuild/dr_libs-populate-prefix/src/dr_libs-populate-stamp/dr_libs-populate-gitclone-lastrun.txt" AND EXISTS "/home/go29net/dev/oot_rl/build-cmake/_deps/dr_libs-subbuild/dr_libs-populate-prefix/src/dr_libs-populate-stamp/dr_libs-populate-gitinfo.txt" AND
  "/home/go29net/dev/oot_rl/build-cmake/_deps/dr_libs-subbuild/dr_libs-populate-prefix/src/dr_libs-populate-stamp/dr_libs-populate-gitclone-lastrun.txt" IS_NEWER_THAN "/home/go29net/dev/oot_rl/build-cmake/_deps/dr_libs-subbuild/dr_libs-populate-prefix/src/dr_libs-populate-stamp/dr_libs-populate-gitinfo.txt")
  message(STATUS
    "Avoiding repeated git clone, stamp file is up to date: "
    "'/home/go29net/dev/oot_rl/build-cmake/_deps/dr_libs-subbuild/dr_libs-populate-prefix/src/dr_libs-populate-stamp/dr_libs-populate-gitclone-lastrun.txt'"
  )
  return()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E rm -rf "/home/go29net/dev/oot_rl/build-cmake/_deps/dr_libs-src"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: '/home/go29net/dev/oot_rl/build-cmake/_deps/dr_libs-src'")
endif()

# try the clone 3 times in case there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "/usr/bin/git"
            clone --no-checkout --config "advice.detachedHead=false" "https://github.com/mackron/dr_libs.git" "dr_libs-src"
    WORKING_DIRECTORY "/home/go29net/dev/oot_rl/build-cmake/_deps"
    RESULT_VARIABLE error_code
  )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(STATUS "Had to git clone more than once: ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/mackron/dr_libs.git'")
endif()

execute_process(
  COMMAND "/usr/bin/git"
          checkout "da35f9d6c7374a95353fd1df1d394d44ab66cf01" --
  WORKING_DIRECTORY "/home/go29net/dev/oot_rl/build-cmake/_deps/dr_libs-src"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: 'da35f9d6c7374a95353fd1df1d394d44ab66cf01'")
endif()

set(init_submodules TRUE)
if(init_submodules)
  execute_process(
    COMMAND "/usr/bin/git" 
            submodule update --recursive --init 
    WORKING_DIRECTORY "/home/go29net/dev/oot_rl/build-cmake/_deps/dr_libs-src"
    RESULT_VARIABLE error_code
  )
endif()
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: '/home/go29net/dev/oot_rl/build-cmake/_deps/dr_libs-src'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy "/home/go29net/dev/oot_rl/build-cmake/_deps/dr_libs-subbuild/dr_libs-populate-prefix/src/dr_libs-populate-stamp/dr_libs-populate-gitinfo.txt" "/home/go29net/dev/oot_rl/build-cmake/_deps/dr_libs-subbuild/dr_libs-populate-prefix/src/dr_libs-populate-stamp/dr_libs-populate-gitclone-lastrun.txt"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: '/home/go29net/dev/oot_rl/build-cmake/_deps/dr_libs-subbuild/dr_libs-populate-prefix/src/dr_libs-populate-stamp/dr_libs-populate-gitclone-lastrun.txt'")
endif()
