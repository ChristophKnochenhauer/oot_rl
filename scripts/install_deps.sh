#!/usr/bin/env bash
set -euo pipefail

if ! command -v apt-get >/dev/null 2>&1; then
    echo "error: this script supports Debian/Ubuntu only" >&2
    echo "On other distros, install the equivalents of:" >&2
    grep -E '^\s+lib|^\s+[a-z]' "$0" | grep -v '^\s*#' >&2
    exit 1
fi

sudo apt-get install -y \
    git \
    cmake \
    build-essential \
    gdb \
    ninja-build \
    ccache \
    lsb-release \
    libsdl2-dev \
    libpng-dev \
    libsdl2-net-dev \
    libzip-dev \
    zipcmp \
    zipmerge \
    ziptool \
    nlohmann-json3-dev \
    libtinyxml2-dev \
    libspdlog-dev \
    libopengl-dev \
    libopusfile-dev \
    libvorbis-dev
