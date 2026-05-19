# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/go29net/dev/oot_rl/build-cmake/_deps/threadpool-src"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/threadpool-build"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/threadpool-subbuild/threadpool-populate-prefix"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/threadpool-subbuild/threadpool-populate-prefix/tmp"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/threadpool-subbuild/threadpool-populate-prefix/src/threadpool-populate-stamp"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/threadpool-subbuild/threadpool-populate-prefix/src"
  "/home/go29net/dev/oot_rl/build-cmake/_deps/threadpool-subbuild/threadpool-populate-prefix/src/threadpool-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/go29net/dev/oot_rl/build-cmake/_deps/threadpool-subbuild/threadpool-populate-prefix/src/threadpool-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/go29net/dev/oot_rl/build-cmake/_deps/threadpool-subbuild/threadpool-populate-prefix/src/threadpool-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
