# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/go29net/dev/oot_rl/build-cmake/_deps/libgfxd-src"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/libgfxd-build"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/libgfxd-subbuild/libgfxd-populate-prefix"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/libgfxd-subbuild/libgfxd-populate-prefix/tmp"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/libgfxd-subbuild/libgfxd-populate-prefix/src/libgfxd-populate-stamp"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/libgfxd-subbuild/libgfxd-populate-prefix/src"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/libgfxd-subbuild/libgfxd-populate-prefix/src/libgfxd-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/go29net/dev/oot_rl/build-cmake/_deps/libgfxd-subbuild/libgfxd-populate-prefix/src/libgfxd-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/go29net/dev/oot_rl/build-cmake/_deps/libgfxd-subbuild/libgfxd-populate-prefix/src/libgfxd-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
