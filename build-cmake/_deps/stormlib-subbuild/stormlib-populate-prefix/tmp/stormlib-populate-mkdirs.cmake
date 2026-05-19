# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/go29net/dev/oot_rl/build-cmake/_deps/stormlib-src"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/stormlib-build"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/stormlib-subbuild/stormlib-populate-prefix"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/stormlib-subbuild/stormlib-populate-prefix/tmp"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/stormlib-subbuild/stormlib-populate-prefix/src/stormlib-populate-stamp"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/stormlib-subbuild/stormlib-populate-prefix/src"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/stormlib-subbuild/stormlib-populate-prefix/src/stormlib-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/go29net/dev/oot_rl/build-cmake/_deps/stormlib-subbuild/stormlib-populate-prefix/src/stormlib-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/go29net/dev/oot_rl/build-cmake/_deps/stormlib-subbuild/stormlib-populate-prefix/src/stormlib-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
