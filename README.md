# oot_rl

Reinforcement Learning Suite for The Legend of Zelda: Ocarina of Time, based on
the wonderful [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright)
(SoH) project.

oot_rl builds SoH as a **headless library** and exposes it to Python through a
[pybind11](https://pybind11.readthedocs.io/) module (`oot_rl`), so RL agents can
step the game frame-by-frame, read game state, capture frames, and snapshot /
restore via savestates.

## Requirements

- Ubuntu 24.04 (or compatible), GCC 13+
- CMake ≥ 3.26, Ninja, ccache
- Python 3.11 (Conda recommended) with `pybind11`, `numpy`, `gymnasium`
- Native dev libraries (EGL, SDL2, libpng, …) — installed by `scripts/install_deps.sh`
- Your own legally-owned OoT N64 ROM, extracted to `oot.o2r` (one-time)

## Build Instructions

```bash
# 1. Clone with submodules
git clone --recurse-submodules -j8 https://github.com/ChristophKnochenhauer/oot_rl.git
cd oot_rl

# 2. Place your extracted ROM as data/oot.o2r
#    (extract from your own ROM; not distributed here)

# 3. System dependencies (Ubuntu/Debian)
./scripts/install_deps.sh

# 4. Python environment — MUST be active for the build (pybind11 discovery)
conda create -n oot-rl python=3.11
conda activate oot-rl
pip install pybind11 numpy gymnasium

# 5. Build
./scripts/build.sh
```

`scripts/build.sh` applies the SoH patches (`scripts/apply_patches.sh`), then
configures and builds with CMake + Ninja. Extra arguments are forwarded to the
build, e.g.:

```bash
./scripts/build.sh -j8                 # parallelism
./scripts/build.sh --target smoke_test # build a single target
```

> **Note:** the active Conda environment must be the one used at configure time —
> the top-level `CMakeLists.txt` locates pybind11 via `python -m pybind11
> --cmakedir`. The first build is long (SoH is compiled in full); ccache makes
> later builds fast.

### Build outputs

- `python/oot_rl.cpython-*.so` — the importable Python module
- `build-cmake/smoke_test` — native end-to-end sanity-check executable

## Running

### Smoke test (native)

Boots the engine, captures a frame, and verifies save/load determinism. Wired as
a CTest:

```bash
ctest --test-dir build-cmake -R smoke_test --output-on-failure
# or directly:
./build-cmake/smoke_test
```

### From Python

The module needs `LUS_APP_BUNDLE_PATH` pointed at the build directory (so SoH
finds `oot.o2r`) and `python/` on the import path:

```bash
export LUS_APP_BUNDLE_PATH=$PWD/build-cmake
export PYTHONPATH=$PWD/python
python python/test_native.py
```

```python
import oot_rl as soh
soh.set_headless(True)
soh.init()
soh.enable_frame_capture()

# Step until the engine reaches a valid GamePlay state, then read state/frames.
while not soh.get_game_state().valid:
    soh.step_frame()

gs = soh.get_game_state()        # pos, scene_id, health, ...
frame = soh.get_frame()          # (480, 640, 3) uint8 RGB
```

See `HANDOVER.md` §5 for the full native-module API. (The native
`SoH_BootToGameplay()` helper exists in the C API but is not yet exposed through
the Python module.)

## Development Mode (editing SoH)

SoH and libultraship live as git submodules under `external/`. **Do not commit
into the submodules directly.** Our changes live in two places (details in
`patches/README.md`):

- **`overlay/`** — our *own* new files (e.g. `soh_api.c`, `FrameCapture.cpp`),
  symlinked into the submodules at build time. **To change our code, just edit
  `overlay/` and rebuild — no dev mode, no patch step.**
- **`patches/`** — minimal edits to *upstream* files, re-applied onto the pinned
  commits. Only these need dev mode to edit.

The normal (non-dev) build applies patches as plain working-tree changes and
symlinks the overlay. To *edit an upstream file*, enter dev mode, which checks
out a `oot-rl-dev` branch in each submodule with the patches applied as real
commits:

```bash
# Enter dev mode (applies patches as commits, tags a baseline)
./scripts/start_dev.sh

# Edit + commit the UPSTREAM change inside the submodule, then build
$EDITOR external/Shipwright/soh/src/...
( cd external/Shipwright && git add -A && git commit -m "..." )
cmake --build build-cmake --target smoke_test

# Export your new commits back into patches/ (auto-numbered)
./scripts/export_patches.sh
#   inspect, then commit in the main repo:
git add patches/ && git commit

# Return to the clean applied state (discards dev branches, re-applies patches)
./scripts/leave_dev.sh
```

| Script | Purpose |
|--------|---------|
| `scripts/build.sh` | Apply patches, configure, build |
| `scripts/apply_patches.sh` | Apply patch series onto pinned commits + symlink overlay |
| `scripts/link_overlay.sh` | Symlink `overlay/` into the submodules (idempotent) |
| `scripts/start_dev.sh` | Enter dev mode (patches as commits on `oot-rl-dev`) |
| `scripts/export_patches.sh` | Export new submodule commits into `patches/` |
| `scripts/leave_dev.sh` | Leave dev mode, restore clean applied state |

When bumping a submodule to a newer upstream, re-apply the patches one by one in
dev mode and resolve conflicts. Only the upstream-file patches can conflict — our
own code lives in `overlay/` and stays out of the conflict path entirely.
Details: `HANDOVER.md` §9.
