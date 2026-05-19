# CMake generated Testfile for 
# Source directory: /home/go29net/dev/oot_rl/build-cmake/_deps/prism-src
# Build directory: /home/go29net/dev/oot_rl/build-cmake/_deps/prism-build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(prism "prism" "../examples/script.opengl.fs")
set_tests_properties(prism PROPERTIES  _BACKTRACE_TRIPLES "/home/go29net/dev/oot_rl/build-cmake/_deps/prism-src/CMakeLists.txt;135;add_test;/home/go29net/dev/oot_rl/build-cmake/_deps/prism-src/CMakeLists.txt;0;")
subdirs("../gsl-build")
