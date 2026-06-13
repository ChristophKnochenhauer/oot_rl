"""LeaveHouseEnv — the first coarse RL env for oot_rl (M7-B3).

Goal: child Link, starting inside Link's House, walks out the door. Reaching the
exit triggers a scene transition (house -> Kokiri Forest), which is the episode's
success condition.

Reset anchor: we don't rely on a pre-existing savestate. On the first reset the
env boots to the title, warps into the house as child (SoH's clean GAMEMODE_NORMAL
branch), settles, and takes save_state(slot) as the per-episode anchor. Every later
reset just load_state(slot) — a fast, in-RAM restore.

Caveats (coarse phase; see the clean-authoring milestone): Link carries SoH's full
debug inventory (Sram_InitDebugSave), and RNG/camera state isn't captured by the
savestate, so episodes are NOT bit-reproducible yet.

NOTE: `oot_rl` is the native pybind module (the .so); this package is deliberately
named `oot_rl_gym` to avoid shadowing it on the import path.
"""
from __future__ import annotations

import numpy as np
import gymnasium as gym
from gymnasium import spaces

import oot_rl as soh

# Link's House child spawn -> scene 0x34 (52). See entrance_table.h.
ENTR_LINKS_HOUSE_CHILD_SPAWN = 0x00BB
LINK_AGE_CHILD = 1

_WALK = 80

_ACTIONS = [
    ("noop",      0,           0,      0),
    ("forward",   0,           0,  _WALK),
    ("backward",  0,           0, -_WALK),
    ("left",      0,      -_WALK,      0),
    ("right",     0,       _WALK,      0),
    ("a",         soh.BTN_A,   0,      0),
    ("forward_a", soh.BTN_A,   0,  _WALK),
]

# SoH is a process-global singleton: init() may run only once per process.
_SOH_INITED = False


def _init_soh(headless: bool) -> None:
    global _SOH_INITED
    if _SOH_INITED:
        return
    soh.set_headless(headless)
    soh.init()
    soh.enable_frame_capture()
    _SOH_INITED = True


class LeaveHouseEnv(gym.Env):
    """Child Link navigates out of Link's House. Leaving (scene change) = success."""

    metadata = {"render_modes": ["rgb_array"], "render_fps": 15}

    def __init__(
        self,
        *,
        entrance_id: int = ENTR_LINKS_HOUSE_CHILD_SPAWN,
        link_age: int = LINK_AGE_CHILD,
        loading_zone_pos: tuple[float, float, float] | None = None,
        save_slot: int = 0,
        frame_skip: int = 4,
        max_steps: int = 500,
        title_frames: int = 50,
        warp_settle_frames: int = 160,
        terminal_bonus: float = 100.0,
        step_penalty: float = 0.01,
        distance_scale: float = 1000.0,
        headless: bool = True,
    ):
        super().__init__()
        self.entrance_id = entrance_id
        self.link_age = link_age
        self.loading_zone_pos = (
            None if loading_zone_pos is None
            else np.asarray(loading_zone_pos, dtype=np.float32)
        )
        self.save_slot = save_slot
        self.frame_skip = frame_skip
        self.max_steps = max_steps
        self.title_frames = title_frames
        self.warp_settle_frames = warp_settle_frames
        self.terminal_bonus = terminal_bonus
        self.step_penalty = step_penalty
        self.distance_scale = distance_scale

        _init_soh(headless)  # needed for the frame dimensions below

        # Frame dimensions only exist once a frame has actually been rendered to
        # the FBO; step a few frames until the query succeeds. These early title
        # frames are harmless (well before the ~frame-230 attract demo).
        w = h = 0
        for _ in range(30):
            try:
                w, h = soh.get_frame_dimensions()  # returns (width, height)
                break
            except RuntimeError:
                soh.step_frame()
        else:
            raise RuntimeError("no rendered frame after 30 steps; frame capture off?")
        # get_frame() is (height, width, 3), so the Box must be (h, w, 3).
        self.observation_space = spaces.Box(0, 255, (h, w, 3), dtype=np.uint8)
        self.action_space = spaces.Discrete(len(_ACTIONS))

        self._steps = 0
        self._initial_scene = -1
        self._anchored = False

    def _build_anchor(self) -> None:
        """One-time: boot to title, warp into the house as child, snapshot the anchor."""
        soh.clear_input()
        for _ in range(self.title_frames):
            soh.step_frame()
        soh.warp_to_entrance(self.entrance_id, self.link_age)
        soh.clear_input()
        for _ in range(self.warp_settle_frames):
            soh.step_frame()
        gs = soh.get_game_state()
        if not gs.valid:
            raise RuntimeError("warp did not reach a valid GamePlay state")
        self._initial_scene = int(gs.scene_id)
        soh.save_state(self.save_slot)
        self._anchored = True

    def reset(self, *, seed=None, options=None):
        super().reset(seed=seed)
        if not self._anchored:
            self._build_anchor()
        else:
            soh.load_state(self.save_slot)
        soh.clear_input()
        soh.step_frame()
        self._steps = 0
        gs = soh.get_game_state()
        return soh.get_frame(), self._info(gs, action_name="<reset>")

    def step(self, action: int):
        name, btn, sx, sy = _ACTIONS[int(action)]
        soh.set_input(port=0, buttons=btn, stick_x=sx, stick_y=sy)
        for _ in range(self.frame_skip):
            soh.step_frame()

        gs = soh.get_game_state()
        self._steps += 1

        left_house = bool(gs.valid) and int(gs.scene_id) != self._initial_scene
        terminated = left_house
        truncated = (self._steps >= self.max_steps) and not terminated

        reward = -self.step_penalty
        dist = None
        if self.loading_zone_pos is not None:
            pos = np.array([gs.pos_x, gs.pos_y, gs.pos_z], dtype=np.float32)
            dist = float(np.linalg.norm(pos - self.loading_zone_pos))
            reward -= dist / self.distance_scale
        if terminated:
            reward += self.terminal_bonus

        return (soh.get_frame(), reward, terminated, truncated,
                self._info(gs, action_name=name, dist=dist))

    def render(self):
        return soh.get_frame()

    def close(self):
        pass

    def _info(self, gs, *, action_name: str, dist: float | None = None) -> dict:
        info = {
            "scene": int(gs.scene_id),
            "pos": (float(gs.pos_x), float(gs.pos_y), float(gs.pos_z)),
            "hp": (int(gs.health), int(gs.max_health)),
            "action": action_name,
        }
        if dist is not None:
            info["dist_to_target"] = dist
        return info
