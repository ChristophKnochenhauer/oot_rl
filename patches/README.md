# SoH Integration: patches/ + overlay/

Turning upstream **Ship of Harkinian** into the headless, scriptable library the
RL environment links against needs two kinds of change. We keep them in two
separate places, because they have very different maintenance costs:

| Mechanism | Holds | Why |
|-----------|-------|-----|
| **`patches/`** | edits to *upstream* files | A diff has to match upstream context, so it can conflict on a submodule bump. Kept minimal, single-concern, single-file. |
| **`overlay/`** | our *own* new files | No upstream counterpart → can never conflict. Symlinked into the submodule, so they are normal tracked files in this repo and edits need **no** patch regeneration. |

**Pins** (commits the patches are written against):

| Submodule                          | Pin commit | Upstream version       |
|------------------------------------|------------|------------------------|
| `external/Shipwright`              | `cb71e22a` | SoH 9.2.3 Ackbar Delta |
| `external/Shipwright/libultraship` | `fdcaf633` | (pinned by SoH)        |

## Maintenance principle

Minimize edits to upstream files. If you are **adding** code, add a new file
under `overlay/` (mirroring the submodule path) — never as a patch. If you are
**modifying** upstream behavior, that is a patch; keep it as small and as
single-file as possible (ideally a one-line hook that jumps into overlay code).

- Edit our code: `$EDITOR overlay/...` then rebuild. No dev mode, no export.
- Edit upstream: `scripts/start_dev.sh` → edit + commit in the submodule →
  `scripts/export_patches.sh` → commit `patches/` in the main repo →
  `scripts/leave_dev.sh`. See `HANDOVER.md` §9.

`scripts/link_overlay.sh` symlinks `overlay/` into the submodules (run
automatically by `apply_patches.sh` and `start_dev.sh`) and git-excludes the
symlinks so they never show as untracked or get committed into the submodule.

---

## `overlay/` — our own source (11 files)

These are 100% ours; upstream never sees them. Symlinked into place at build time.

| Overlay file | Symlinked into | Purpose |
|--------------|----------------|---------|
| `shipwright/soh/include/soh_lib.h` | `soh/include/` | Public C API header |
| `shipwright/soh/src/soh_lib/soh_api.c` | `soh/src/soh_lib/` | The entire RL API: input, `GetGameState`, debug view, frame-capture forwards, warp helpers |
| `libultraship/include/ship/headless/Headless.h` `…/src/ship/headless/Headless.cpp` | `ship/headless/` | Runtime headless flag + programmatic pad override |
| `libultraship/include/ship/headless/DebugView.{h,cpp}` | `ship/headless/` | Optional on-screen mirror of the FBO |
| `libultraship/include/ship/headless/FrameCapture.{h,cpp}` | `ship/headless/` | RGB readback of the game framebuffer |
| `libultraship/include/fast/backends/gfx_egl.{h,cpp}` | `fast/backends/` | EGL off-screen rendering backend |
| `libultraship/cmake/dependencies/linux-oot-rl.cmake` | `cmake/dependencies/` | EGL/GLES dependency wiring |

## `shipwright/` — SoH game-layer patches (4, upstream edits only)

| # | Patch | Upstream file(s) | Purpose | Re-apply risk |
|---|-------|------------------|---------|---------------|
| 0001 | cmake: embed + BUILD_AS_LIB | `CMakeLists.txt`, `soh/CMakeLists.txt`, `soh/src/boot/build.c.in` | Build SoH as a static lib in the parent build; fix embedded paths/version vars | Medium |
| 0002 | main: library-mode bootstrap | `soh/src/code/main.c` | `SoH_Init`/`Shutdown`/`Main_LibInit` + `#ifdef SOH_AS_LIB` guards (bound to this file's static state) | Medium |
| 0003 | graph: SoH_StepFrame hook | `soh/src/code/graph.c` | One-frame step entrypoint (`RunFrame` is `static`, so the hook stays here) | Low |
| 0004 | savestates: expose save/load | `soh/soh/Enhancements/savestates.cpp` | `SoH_SaveState`/`SoH_LoadState` wrappers | Low |

## `libultraship/` — renderer/audio/input patches (11, upstream edits only)

| # | Patch | Purpose | Re-apply risk |
|---|-------|---------|---------------|
| 0001 | headless runtime flag | Wire the global headless on/off switch into LUS | Low–Medium |
| 0002 | audio: skip SDL device when headless | No audio device in headless | Low |
| 0003 | build: EGL dependency | Pull in `linux-oot-rl.cmake` (overlay) for off-screen GL | Low–Medium |
| 0004 | gfx_sdl: allow subclassing | Hook point for the headless backend | Medium |
| 0005 | Fast3dWindow: select EGL when headless | Pick the `gfx_egl` backend (overlay) in headless mode | Medium |
| 0006 | skip ImGui rendering | Drop GUI overhead in headless | Medium |
| 0007 | input controls | Programmatic pad override (RL input) | Medium |
| 0008 | debug view | Wire the optional on-screen FBO mirror | Medium |
| 0009 | fix debug window not rendering game | VAO bind + force-FBO fixes | Medium |
| 0010 | honor `LUS_APP_BUNDLE_PATH` | Override app-bundle path so embedding finds `oot.o2r` (HANDOVER §8) | Low |
| 0011 | headless frame capture | 1-line `interpreter.cpp` hook that force-renders to FBO when capture is on | Low |

The libultraship patches modify rendering internals by nature, so they carry more
inherent risk on an upstream bump — that is expected; the goal is that *all of our
own code* (overlay) stays out of the conflict path entirely.
