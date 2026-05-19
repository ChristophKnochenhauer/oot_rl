include(CMakePrintHelpers)
cmake_print_variables(CPACK_TEMPORARY_DIRECTORY)
cmake_print_variables(CPACK_TOPLEVEL_DIRECTORY)
cmake_print_variables(CPACK_PACKAGE_DIRECTORY)
cmake_print_variables(CPACK_PACKAGE_FILE_NAME)

find_program(LINUXDEPLOY_EXECUTABLE
  NAMES linuxdeploy linuxdeploy-x86_64.AppImage
  PATHS ${CPACK_PACKAGE_DIRECTORY}/linuxdeploy)

if (NOT LINUXDEPLOY_EXECUTABLE)
  message(STATUS "Downloading linuxdeploy")
  set(LINUXDEPLOY_EXECUTABLE ${CPACK_PACKAGE_DIRECTORY}/linuxdeploy/linuxdeploy)
  file(DOWNLOAD 
      https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20240109-1/linuxdeploy-x86_64.AppImage
      ${LINUXDEPLOY_EXECUTABLE}
      INACTIVITY_TIMEOUT 10
      LOG ${CPACK_PACKAGE_DIRECTORY}/linuxdeploy/download.log
      STATUS LINUXDEPLOY_DOWNLOAD)
  execute_process(COMMAND chmod +x ${LINUXDEPLOY_EXECUTABLE} COMMAND_ECHO STDOUT)
endif()

execute_process(
  COMMAND
    ${CMAKE_COMMAND} -E env
      OUTPUT=${CPACK_PACKAGE_FILE_NAME}.appimage
      VERSION=${CPACK_PACKAGE_VERSION}
    ${LINUXDEPLOY_EXECUTABLE}
    --appimage-extract-and-run
    --appdir=${CPACK_TEMPORARY_DIRECTORY}
    --executable=/home/go29net/dev/oot_rl/build-cmake/external/Shipwright/soh/soh.elf
    --desktop-file=/home/go29net/dev/oot_rl/scripts/linux/appimage/soh.desktop
    --icon-file=/home/go29net/dev/oot_rl/build-cmake/sohIcon.png
    --output=appimage
    # --verbosity=2
)
