"""Config 4: LeaveHouseEnv end-to-end with a random policy (M8 baseline).

Own process (SoH singleton). env.step includes frame_skip=4 step_frames + one
get_frame per step. Resets are timed separately and reported apart from steps.

    PYTHONPATH=python LUS_APP_BUNDLE_PATH=$PWD/build-cmake \
        conda run -n oot-rl python python/bench/bench_env.py [steps]
"""
import os
import sys
import time

os.environ.setdefault(
    "LUS_APP_BUNDLE_PATH",
    os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "build-cmake")),
)

import numpy as np
from oot_rl_gym import LeaveHouseEnv


def main():
    target_steps = int(sys.argv[1]) if len(sys.argv) > 1 else 500

    # large max_steps so episodes rarely truncate mid-benchmark
    env = LeaveHouseEnv(max_steps=100000)
    rng = np.random.default_rng(0)

    # first reset builds the anchor (boot+warp); excluded from timing
    env.reset()
    # warmup steps
    for _ in range(50):
        env.step(env.action_space.sample())

    step_time = 0.0
    reset_time = 0.0
    n_steps = 0
    n_resets = 0
    frame_skip = env.frame_skip

    while n_steps < target_steps:
        t = time.perf_counter()
        _, _, term, trunc, _ = env.step(env.action_space.sample())
        step_time += time.perf_counter() - t
        n_steps += 1
        if term or trunc:
            t = time.perf_counter()
            env.reset()
            reset_time += time.perf_counter() - t
            n_resets += 1

    steps_per_s = n_steps / step_time
    frames_per_s = steps_per_s * frame_skip
    print(f"RESULT config=4 steps={n_steps} step_seconds={step_time:.4f} "
          f"steps_per_s={steps_per_s:.2f} frame_skip={frame_skip} "
          f"frames_per_s={frames_per_s:.2f} resets={n_resets} "
          f"reset_seconds={reset_time:.4f}")

    env.close()


if __name__ == "__main__":
    main()
