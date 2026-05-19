# Install script for directory: /home/go29net/dev/oot_rl/external/Shipwright

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/go29net/dev/oot_rl/build-cmake/libultraship/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/go29net/dev/oot_rl/build-cmake/ZAPD/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/go29net/dev/oot_rl/build-cmake/external/Shipwright/OTRExporter/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/go29net/dev/oot_rl/build-cmake/external/Shipwright/soh/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "ship" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE FILE FILES "/home/go29net/dev/oot_rl/build-cmake/soh/soh.o2r")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "extractor" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/./assets/extractor/ZAPD.out" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/./assets/extractor/ZAPD.out")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/./assets/extractor/ZAPD.out"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/./assets/extractor" TYPE EXECUTABLE FILES "/home/go29net/dev/oot_rl/build-cmake/ZAPD/ZAPD.out")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/./assets/extractor/ZAPD.out" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/./assets/extractor/ZAPD.out")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/./assets/extractor/ZAPD.out")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "extractor" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/go29net/dev/oot_rl/build-cmake/ZAPD/CMakeFiles/ZAPD.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "extractor" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/./assets" TYPE DIRECTORY FILES "/home/go29net/dev/oot_rl/soh/assets/extractor/")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "extractor" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/./assets/xml" TYPE DIRECTORY FILES "/home/go29net/dev/oot_rl/soh/assets/xml/")
endif()

