# oot_rl
Reinforcement Learning Suite for The Legend of Zelda: Ocarina of Time.

## Installation

Ubuntu:
```
sudo apt-get install \
    git \
    cmake \
    build-essential \
    gdb \
    ninja-build \
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
git clone https://github.com/ChristophKnochenhauer/oot_rl.git
cd oot_rl
git submodule update --init --recursive
cmake -H. -Bbuild-cmake -GNinja
cmake --build build-cmake --target soh
```
