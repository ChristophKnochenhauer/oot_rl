#=================== EGL (for headless rendering) ===================
find_package(OpenGL COMPONENTS EGL)
if (TARGET OpenGL::EGL)
    target_link_libraries(libultraship PUBLIC OpenGL::EGL)
    target_compile_definitions(libultraship PUBLIC LUS_HAS_EGL=1)
    message(STATUS "EGL found — headless rendering enabled")
else()
    message(STATUS "EGL not found — headless rendering will not be available")
endif()
