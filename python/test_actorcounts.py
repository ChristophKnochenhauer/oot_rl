"""Verify the ActorDB numLoaded leak fix.

Exercises exactly the crashing path: repeatedly load a heavy-actor scene (Kokiri
Forest), then load_state() back to a light scene (Link's House). Each cycle would
leak the Kokiri actors' numLoaded counters (load_state overwrites them without
freeing) and, without restore_actor_counts(), trip
`assert(numLoaded < 255)` in Actor_Spawn after ~30 cycles.

    PYTHONPATH=python LUS_APP_BUNDLE_PATH=$PWD/build-cmake python python/test_actorcounts.py [cycles]
"""
import os
import sys

os.environ.setdefault(
    "LUS_APP_BUNDLE_PATH",
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build-cmake"),
)

import oot_rl as soh

ENTR_LINKS_HOUSE_CHILD_SPAWN = 0x00BB
ENTR_KOKIRI_FOREST_0 = 0x00EE
LINK_AGE_CHILD = 1
CYCLES = int(sys.argv[1]) if len(sys.argv) > 1 else 50


def step(n):
    for _ in range(n):
        soh.step_frame()


def main():
    soh.set_headless(True)
    soh.init()
    soh.enable_frame_capture()

    step(50)  # title
    soh.warp_to_entrance(ENTR_LINKS_HOUSE_CHILD_SPAWN, LINK_AGE_CHILD)
    step(160)
    gs = soh.get_game_state()
    print(f"[anchor] scene={gs.scene_id} valid={bool(gs.valid)}")
    soh.save_state(0)
    soh.snapshot_actor_counts()

    for i in range(CYCLES):
        soh.warp_to_entrance(ENTR_KOKIRI_FOREST_0, LINK_AGE_CHILD)  # heavy actors
        step(130)
        soh.load_state(0)               # back to house -> would leak Kokiri counts
        soh.restore_actor_counts()      # the fix
        step(5)
        if (i + 1) % 10 == 0:
            gs = soh.get_game_state()
            print(f"[cycle {i+1:3d}] survived; back in scene={gs.scene_id} valid={bool(gs.valid)}")

    print(f"\n[ok] survived {CYCLES} Kokiri<->House load_state cycles without overflow")
    soh.shutdown()


if __name__ == "__main__":
    main()
