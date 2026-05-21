# oot_rl
Reinforcement Learning Suite for The Legend of Zelda: Ocarina of Time.

## Build Instructions

Make sure to place your own copy of an extracted ROM in the data folder.

Ubuntu (24.04): First, run
```
git clone --recurse-submodules -j8 https://github.com/ChristophKnochenhauer/oot_rl.git
```
Now place your own copy of an extracted ROM in the `oot_rl/data` folder. Afterwards, run
```
cd oot_rl
./scripts/install_deps.sh
./scripts/build.sh
```
