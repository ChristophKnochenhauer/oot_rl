# oot_rl
Reinforcement Learning Suite for The Legend of Zelda: Ocarina of Time based on the wonderful [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) project.

## Build Instructions

Ubuntu (24.04): First, run
```
git clone --recurse-submodules -j8 https://github.com/ChristophKnochenhauer/oot_rl.git
```
Now place your own copy of an extracted ROM with filename `oot.o2r` in the `oot_rl/data` folder. Afterwards, run
```
cd oot_rl
./scripts/install_deps.sh
./scripts/build.sh
```
