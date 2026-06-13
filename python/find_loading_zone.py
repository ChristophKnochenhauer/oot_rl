import os
import sys
from collections import deque

import numpy as np

os.environ.setdefault("LUS_APP_BUNDLE_PATH", os.getcwd())
import oot_rl as soh

EPISODES        = int(sys.argv[1]) if len(sys.argv) > 1 else 30
MAX_STEPS       = 1000
SAVE_SLOT       = 0
FRAME_SKIP      = 4
POS_HISTORY_LEN = 10
DOOR_LOOKBACK   = 5

ACTIONS = [
    (0,   0,    0),
    (0,   0,   80),  (0,   0,   80),  (0,   0,   80),  (0,   0,   80),
    (0,   0,  -80),
    (0, -80,    0),
    (0,  80,    0),
]


def boot_until_valid(max_frames: int = 800) -> bool:
    for i in range(max_frames):
        soh.step_frame()
        gs = soh.get_game_state()
        if gs.valid:
            for _ in range(60):
                soh.step_frame()
            return True
    return False


def safe_load_state(slot: int, max_attempts: int = 3) -> bool:
    for attempt in range(max_attempts):
        try:
            soh.load_state(slot)
            return True
        except RuntimeError as e:
            print(f"  load_state attempt {attempt+1} failed: {e}")
            soh.set_input(port=0, buttons=soh.BTN_B | soh.BTN_START,
                          stick_x=0, stick_y=0)
            for _ in range(10):
                soh.step_frame()
            soh.clear_input()
            for _ in range(60):
                soh.step_frame()
    return False


def main():
    soh.set_headless(True)
    soh.init()
    soh.enable_frame_capture()

    print("[boot] stepping until valid GameState...")
    if not boot_until_valid():
        print("[boot] never became valid — abort")
        soh.shutdown()
        return
    print(f"[boot] valid. {soh.get_game_state()!r}")

    rng = np.random.default_rng(0)
    door_positions: list[tuple[float, float, float]] = []

    for ep in range(EPISODES):
        if not safe_load_state(SAVE_SLOT):
            print(f"[ep {ep:02d}] could not recover load_state — skipping")
            continue

        soh.clear_input()
        soh.step_frame()
        gs0 = soh.get_game_state()
        initial_scene = gs0.scene_id

        history: deque[tuple[float, float, float]] = deque(maxlen=POS_HISTORY_LEN)
        history.append((gs0.pos_x, gs0.pos_y, gs0.pos_z))

        for step in range(MAX_STEPS):
            btn, sx, sy = ACTIONS[rng.integers(len(ACTIONS))]
            soh.set_input(port=0, buttons=btn, stick_x=sx, stick_y=sy)
            for _ in range(FRAME_SKIP):
                soh.step_frame()

            gs = soh.get_game_state()
            if gs.valid and gs.scene_id != initial_scene:
                idx = -min(DOOR_LOOKBACK, len(history))
                door = history[idx]
                door_positions.append(door)
                print(f"[ep {ep:02d}] step {step:4d}: scene {initial_scene} -> {gs.scene}  "
                      f"door @ ({door[0]:7.1f}, {door[1]:7.1f}, {door[2]:7.1f})")
                break

            if gs.valid and gs.scene_id == initial_scene:
                pos = (gs.pos_x, gs.pos_y, gs.pos_z)
                if pos != (0.0, 0.0, 0.0):
                    history.append(pos)
        else:
            last = history[-1] if history else (None, None, None)
            print(f"[ep {ep:02d}] no transition in {MAX_STEPS} steps; last pos = {last}")

    if door_positions:
        arr = np.array(door_positions)
        print(f"\nFound {len(arr)}/{EPISODES} transitions")
        print(f"  mean   = ({arr[:,0].mean():7.1f}, {arr[:,1].mean():7.1f}, {arr[:,2].mean():7.1f})")
        print(f"  std    = ({arr[:,0].std():7.1f}, {arr[:,1].std():7.1f}, {arr[:,2].std():7.1f})")
        print(f"  median = ({np.median(arr[:,0]):7.1f}, {np.median(arr[:,1]):7.1f}, {np.median(arr[:,2]):7.1f})")
    else:
        print("\nNo transitions found.")

    soh.shutdown()


if __name__ == "__main__":
    main()
