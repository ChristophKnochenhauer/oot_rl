"""Single-process throughput baseline for the oot_rl step path (M8 baseline / M8.6 ref).

SoH is a process-global singleton: init() may run only ONCE per process. Therefore
each measurement configuration MUST run in its own process. Invoke as:

    PYTHONPATH=python LUS_APP_BUNDLE_PATH=$PWD/build-cmake \
        conda run -n oot-rl python python/bench/bench_step.py <config> [frames]

configs:
  1  engine step only, frame capture NOT enabled
  2  engine step, frame capture enabled, no get_frame calls
  3  engine step + get_frame() every 4th frame (frame_skip=4 observation cadence)
  4  handled by bench_env.py (LeaveHouseEnv end-to-end)

Measures steady-state frames/s after warp anchor + warmup. Prints a one-line
RESULT record that the runner scrapes.
"""
import os
import sys
import time

os.environ.setdefault(
    "LUS_APP_BUNDLE_PATH",
    os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "build-cmake")),
)

import oot_rl as soh

ENTR_LINKS_HOUSE_CHILD_SPAWN = 0x00BB
LINK_AGE_CHILD = 1

TITLE_FRAMES = 50
SETTLE_FRAMES = 160
WARMUP_FRAMES = 200


def build_anchor():
    soh.clear_input()
    for _ in range(TITLE_FRAMES):
        soh.step_frame()
    soh.warp_to_entrance(ENTR_LINKS_HOUSE_CHILD_SPAWN, LINK_AGE_CHILD)
    soh.clear_input()
    for _ in range(SETTLE_FRAMES):
        soh.step_frame()
    gs = soh.get_game_state()
    if not gs.valid:
        raise RuntimeError("warp did not reach a valid GamePlay state")


def main():
    config = int(sys.argv[1])
    frames = int(sys.argv[2]) if len(sys.argv) > 2 else 2000

    soh.set_headless(True)
    soh.init()

    capture = config in (2, 3)
    if capture:
        soh.enable_frame_capture()

    build_anchor()

    # keep Link moving forward so we exercise a realistic gameplay frame
    soh.set_input(port=0, buttons=0, stick_x=0, stick_y=80)

    # warmup (excluded from timing)
    for _ in range(WARMUP_FRAMES):
        soh.step_frame()

    n_getframe = 0
    t0 = time.perf_counter()
    for i in range(frames):
        soh.step_frame()
        if config == 3 and (i % 4) == 0:
            soh.get_frame()
            n_getframe += 1
    dt = time.perf_counter() - t0

    fps = frames / dt
    print(f"RESULT config={config} frames={frames} seconds={dt:.4f} "
          f"fps={fps:.2f} getframe_calls={n_getframe}")

    soh.shutdown()


if __name__ == "__main__":
    main()
