"""
Coarse M7-B3 probe: does soh.warp_to() drop us into CONTROLLABLE gameplay?

We can't tell "valid gameplay" from the title attract demo by gs.valid alone
(see HANDOVER §6). The discriminator used here is *control*: under a held
movement input Link's position should move coherently in one direction; under
neutral input it should stay ~put. The attract demo ignores our input and/or
teleports Link around, so it fails both checks.

Usage:
    PYTHONPATH=python LUS_APP_BUNDLE_PATH=$PWD/build-cmake python python/test_warp.py [early|demo|viz]

  early  warp from the title (gPlayState NULL -> Warp() clean branch), then run
         the automated control + savestate probes (headless, prints a verdict)
  demo   boot until valid (in the attract demo) then warp (in-game branch; crashes)
  viz    open a live DebugView window and drive Link on a scripted loop so you
         can watch and verify control yourself. Needs a display (WSLg/X11).
         Optional 2nd arg = frames-to-warp-at (default 50), 3rd = fps (default 30).
  record headless: write the warp + scripted control run to /tmp/warp_run.mp4
         (no display needed). Args: record <warp_at> <fps> <cycles>.
"""
import os
import sys
import math

os.environ.setdefault("LUS_APP_BUNDLE_PATH",
                       os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build-cmake"))

import oot_rl as soh

MODE = sys.argv[1] if len(sys.argv) > 1 else "demo"

# Entrances (see soh/include/tables/entrance_table.h).
ENTR_KOKIRI_FOREST_0 = 0x00EE          # -> scene 0x55 (85)
ENTR_LINKS_HOUSE_CHILD_SPAWN = 0x00BB  # -> scene 0x34 (52), child spawn in the house
# The attract demo, despite gs.valid, actually runs in Hyrule Field (scene 81).

TARGET_ENTRANCE = ENTR_LINKS_HOUSE_CHILD_SPAWN
LINK_AGE_CHILD = 1   # Warp() forces adult (0); our envs want child.
WALK = 80


def pos():
    g = soh.get_game_state()
    return (g.pos_x, g.pos_y, g.pos_z), g.scene_id, bool(g.valid)


def dist(a, b):
    return math.sqrt(sum((p - q) ** 2 for p, q in zip(a, b)))


def step(n, buttons=0, sx=0, sy=0):
    for _ in range(n):
        soh.set_input(port=0, buttons=buttons, stick_x=sx, stick_y=sy)
        soh.step_frame()


def control_probe(label):
    soh.clear_input()
    step(30)
    base, scene, valid = pos()
    print(f"  [{label}] start pos={tuple(round(v,1) for v in base)} scene={scene} valid={valid}")

    p0, _, _ = pos()
    step(40, sy=WALK)              # hold forward
    p_fwd, _, _ = pos()
    step(40, sy=-WALK)             # hold back
    p_back, _, _ = pos()
    soh.clear_input()
    step(40)                       # neutral
    p_idle, _, _ = pos()

    d_fwd = dist(p0, p_fwd)
    d_back = dist(p_fwd, p_back)
    d_idle = dist(p_back, p_idle)
    print(f"  [{label}] move(forward)={d_fwd:7.1f}  move(back)={d_back:7.1f}  drift(idle)={d_idle:7.1f}")
    controllable = d_fwd > 5.0 and d_idle < d_fwd * 0.5
    print(f"  [{label}] => {'CONTROLLABLE' if controllable else 'NOT controllable (demo? frozen?)'}")
    return controllable


def run_viz():
    """Open a live window and drive Link on a loop so a human can watch/verify."""
    import time
    warp_at = int(sys.argv[2]) if len(sys.argv) > 2 else 50
    fps = float(sys.argv[3]) if len(sys.argv) > 3 else 30.0
    delay = 1.0 / fps if fps > 0 else 0.0

    # WSLg shows SDL software-renderer windows as invisible "[Copy Mode]" under
    # the Wayland backend. Forcing XWayland (x11) makes them visible. Override
    # with SDL_VIDEODRIVER=wayland if x11 is unavailable. Must be set before init.
    os.environ.setdefault("SDL_VIDEODRIVER", "x11")
    print(f"[viz] SDL_VIDEODRIVER={os.environ.get('SDL_VIDEODRIVER')}")

    soh.set_headless(True)
    soh.init()
    soh.enable_frame_capture()
    soh.enable_debug_view(640, 480)
    print("[viz] DebugView window open. Booting to title ...")
    for _ in range(warp_at):
        soh.step_frame()
        time.sleep(delay)

    print(f"[viz] warp_to_entrance(0x{TARGET_ENTRANCE:X} (child))")
    soh.warp_to_entrance(TARGET_ENTRANCE, LINK_AGE_CHILD)
    for _ in range(160):            # scene load + fade-in
        soh.step_frame()
        time.sleep(delay)

    g = soh.get_game_state()
    print(f"[viz] scene={g.scene_id} valid={bool(g.valid)} — driving Link (Ctrl-C to stop)")

    script = [
        ("FORWARD", 0,  WALK, 60),
        ("IDLE",    0,  0,    25),
        ("BACK",    0, -WALK, 60),
        ("IDLE",    0,  0,    25),
        ("LEFT",  -WALK, 0,   45),
        ("RIGHT",  WALK, 0,   45),
        ("IDLE",    0,  0,    25),
    ]
    try:
        cycle = 0
        while True:
            cycle += 1
            for name, sx, sy, n in script:
                for _ in range(n):
                    soh.set_input(port=0, stick_x=sx, stick_y=sy)
                    soh.step_frame()
                    time.sleep(delay)
                gp = soh.get_game_state()
                print(f"[viz] cycle {cycle:2d} {name:7} -> "
                      f"pos=({gp.pos_x:7.1f},{gp.pos_y:7.1f},{gp.pos_z:7.1f})")
    except KeyboardInterrupt:
        print("\n[viz] stopped.")
    finally:
        soh.shutdown()


def run_record():
    """Headless: capture the warp + scripted control run straight to an MP4.

    No display needed — frames come from get_frame() as numpy arrays and stream
    to ffmpeg. Open the result in Windows. Args: record <warp_at> <fps> <cycles>.
    """
    import imageio
    warp_at = int(sys.argv[2]) if len(sys.argv) > 2 else 50
    fps = int(float(sys.argv[3])) if len(sys.argv) > 3 else 30
    cycles = int(sys.argv[4]) if len(sys.argv) > 4 else 2
    out = "/tmp/warp_run.mp4"

    soh.set_headless(True)
    soh.init()
    soh.enable_frame_capture()
    writer = imageio.get_writer(out, fps=fps)

    n_frames = 0

    def rec(n, sx=0, sy=0, btn=0):
        nonlocal n_frames
        for _ in range(n):
            soh.set_input(port=0, buttons=btn, stick_x=sx, stick_y=sy)
            soh.step_frame()
            writer.append_data(soh.get_frame())
            n_frames += 1

    print("[rec] booting to title ...")
    rec(warp_at)
    print(f"[rec] warp_to_entrance(0x{TARGET_ENTRANCE:X} (child))")
    soh.warp_to_entrance(TARGET_ENTRANCE, LINK_AGE_CHILD)
    rec(160)  # scene load + fade-in
    g = soh.get_game_state()
    print(f"[rec] scene={g.scene_id} valid={bool(g.valid)} — recording {cycles} control cycle(s)")

    script = [
        ("FORWARD", 0,  WALK, 60),
        ("IDLE",    0,  0,    20),
        ("BACK",    0, -WALK, 60),
        ("IDLE",    0,  0,    20),
        ("LEFT",  -WALK, 0,   45),
        ("RIGHT",  WALK, 0,   45),
        ("IDLE",    0,  0,    20),
    ]
    for c in range(cycles):
        for name, sx, sy, n in script:
            rec(n, sx=sx, sy=sy)
            gp = soh.get_game_state()
            print(f"[rec] cycle {c+1} {name:7} pos=({gp.pos_x:7.1f},{gp.pos_y:7.1f},{gp.pos_z:7.1f})")

    writer.close()
    soh.shutdown()

    print(f"\n[rec] wrote {n_frames} frames -> {out}")
    import shutil
    import subprocess
    if shutil.which("wslpath") and shutil.which("explorer.exe"):
        try:
            win = subprocess.check_output(["wslpath", "-w", out]).decode().strip()
            print(f"[rec] opening in Windows: {win}")
            subprocess.run(["explorer.exe", win], check=False)
        except Exception as e:
            print(f"[rec] (auto-open failed: {e}; open {out} manually)")


def main():
    if MODE == "viz":
        run_viz()
        return
    if MODE == "record":
        run_record()
        return

    soh.set_headless(True)
    soh.init()
    soh.enable_frame_capture()

    target_pos = (0.0, 0.0, 0.0)

    if MODE == "demo":
        print("[boot] stepping until valid (attract demo) ...")
        for i in range(800):
            soh.step_frame()
            if soh.get_game_state().valid:
                break
        (target_pos, scene, _) = pos()
        print(f"[boot] valid at scene={scene} pos={tuple(round(v,1) for v in target_pos)}")
        print("[probe] BEFORE warp (this is the demo):")
        control_probe("pre-warp")
    else:
        # Step enough for the title GameState to init, but FEWER than the ~230
        # frames it takes the attract demo's PlayState to spin up. While we're at
        # the title, gPlayState is NULL -> Warp() takes the clean branch
        # (Sram_InitDebugSave + GAMEMODE_NORMAL) -> a consistent, controllable game.
        WARP_AT = int(sys.argv[2]) if len(sys.argv) > 2 else 40
        print(f"[boot] early mode: stepping {WARP_AT} frames (title), then warp")
        for _ in range(WARP_AT):
            soh.step_frame()
        _, scene, valid = pos()
        print(f"[boot] pre-warp state: scene={scene} valid={valid} "
              f"(valid=False is GOOD here -> title, clean branch)")

    print(f"[warp] warp_to_entrance(0x{TARGET_ENTRANCE:X} (child)) — natural spawn, no pos override")
    soh.warp_to_entrance(TARGET_ENTRANCE, LINK_AGE_CHILD)

    print("[warp] stepping 150 frames for scene load ...")
    step(150)
    _, scene, valid = pos()
    print(f"[warp] after load: scene={scene} valid={valid}")

    print("[probe] AFTER warp:")
    controllable = control_probe("post-warp")

    # The other half of an env: a working RAM reset anchor at this state.
    if controllable:
        print("[savestate] save_state(0) at the warped state ...")
        soh.clear_input()
        step(5)
        anchor, _, _ = pos()
        try:
            soh.save_state(0)
        except RuntimeError as e:
            print(f"[savestate] save_state FAILED: {e}")
        else:
            step(40, sy=WALK)                  # walk away
            moved, _, _ = pos()
            soh.load_state(0)
            step(2)
            restored, _, _ = pos()
            print(f"[savestate] anchor={tuple(round(v,1) for v in anchor)}")
            print(f"[savestate] after walk={tuple(round(v,1) for v in moved)} (moved {dist(anchor,moved):.1f})")
            print(f"[savestate] after load ={tuple(round(v,1) for v in restored)} (off anchor by {dist(anchor,restored):.1f})")
            print(f"[savestate] => {'RESET OK' if dist(anchor,restored) < 5.0 else 'reset drift?'}")

    soh.shutdown()


if __name__ == "__main__":
    main()
