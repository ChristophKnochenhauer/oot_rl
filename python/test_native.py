import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import oot_rl as soh

soh.set_headless(True)
soh.set_app_path("/home/christoph/dev/oot_rl/build-cmake")
soh.init()
soh.enable_frame_capture()

state = None
for i in range(600):
    soh.step_frame()
    state = soh.get_game_state()
    if state.valid:
        break
    print(f"[boot] step={i} {state!r}")

for _ in range(200):
    soh.step_frame()
state = soh.get_game_state()

w, h = soh.get_frame_dimensions()
print(f"[frame] dims={w}x{h}")
frame = soh.get_frame()
print(f"[frame] shape={frame.shape} dtype={frame.dtype} sum={int(frame.sum())}")
print(f"[frame] mean={float(frame.mean()):.2f}  first row 5 px = {frame[0,:5].tolist()}")

soh.save_state(0)
start = (state.pos_x, state.pos_y, state.pos_z)
for _ in range(60):
    soh.set_input(stick_y=127)
    soh.step_frame()
state = soh.get_game_state()
print(f"[A]   forward -> ({state.pos_x:.2f}, {state.pos_y:.2f}, {state.pos_z:.2f})")
soh.load_state(0)
state = soh.get_game_state()
print(f"[reset] back to ({state.pos_x:.2f}, {state.pos_y:.2f}, {state.pos_z:.2f})  (expected {start})")

soh.shutdown()
