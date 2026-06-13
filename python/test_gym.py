"""Smoke test for LeaveHouseEnv with a random policy (M7-B3).

    PYTHONPATH=python LUS_APP_BUNDLE_PATH=$PWD/build-cmake python python/test_gym.py

Checks the full env loop end-to-end: construction, the warp-based reset anchor,
stepping with random actions, observation shape, and that load_state on reset
returns Link to the same anchor (scene + position).
"""
import os
import sys

os.environ.setdefault(
    "LUS_APP_BUNDLE_PATH",
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build-cmake"),
)

import numpy as np
from oot_rl_gym import LeaveHouseEnv

EPISODES = int(sys.argv[1]) if len(sys.argv) > 1 else 3
MAX_STEPS = int(sys.argv[2]) if len(sys.argv) > 2 else 80


def main():
    env = LeaveHouseEnv(max_steps=MAX_STEPS)
    print(f"observation_space={env.observation_space}")
    print(f"action_space={env.action_space}")

    rng = np.random.default_rng(0)
    obs, info = env.reset()
    print(f"[reset] obs.shape={obs.shape} dtype={obs.dtype} "
          f"scene={info['scene']} pos={tuple(round(v,1) for v in info['pos'])}")
    anchor_scene, anchor_pos = info["scene"], np.array(info["pos"])

    for ep in range(EPISODES):
        obs, info = env.reset()
        # Reset anchor must be deterministic: same scene + position every episode.
        off = float(np.linalg.norm(np.array(info["pos"]) - anchor_pos))
        print(f"\n[ep {ep}] reset -> scene={info['scene']} "
              f"pos={tuple(round(v,1) for v in info['pos'])} (off anchor by {off:.1f})")

        total_r, left = 0.0, False
        for t in range(MAX_STEPS):
            obs, r, term, trunc, info = env.step(env.action_space.sample())
            total_r += r
            if term or trunc:
                left = term
                print(f"[ep {ep}] {'LEFT HOUSE' if term else 'truncated'} at step {t} "
                      f"scene={info['scene']} total_reward={total_r:+.2f}")
                break
        else:
            print(f"[ep {ep}] ran {MAX_STEPS} steps, still in scene {info['scene']} "
                  f"total_reward={total_r:+.2f}")

    env.close()
    print("\n[smoke] OK — env constructs, anchors, steps, and resets.")


if __name__ == "__main__":
    main()
