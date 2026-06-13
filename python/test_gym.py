import os
os.environ.setdefault("LUS_APP_BUNDLE_PATH", "/home/christoph/dev/oot_rl/build-cmake")

import oot_rl as soh
from gymnasium.wrappers import FrameStackObservation, ResizeObservation
from oot_rl.envs.leave_house import LeaveHouseEnv

soh.set_headless(True)
soh.init()
soh.enable_frame_capture()

# Plug in the value the find_loading_zone helper printed:
LOADING_ZONE_POS = (0.0, 0.0, 200.0)  # <- replace with helper output

env = LeaveHouseEnv(loading_zone_pos=LOADING_ZONE_POS)
env = ResizeObservation(env, shape=(84, 84))    # 480x640x3 -> 84x84x3
env = FrameStackObservation(env, stack_size=4)  # (84,84,3) -> (4,84,84,3)

obs, info = env.reset()
print("obs.shape =", obs.shape)  # (4, 84, 84, 3)
print("info =", info)

for _ in range(20):
    obs, r, term, trunc, info = env.step(env.action_space.sample())
    print(f"r={r:+.4f}  scene={info['scene']}  dist={info.get('dist_to_target', 0):.1f}")
    if term or trunc:
        print("episode ended -", "TERM" if term else "TRUNC")
        env.reset()