from __future__ import annotations

import numpy as np
import gymnasium as gym
from gymnasium import spaces

import oot_rl as soh

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


class LeaveHouseEnv(gym.Env):
    """Single-room navigation env around Link's House."""

    metadata = {"render_modes": ["rgb_array"], "render_fps": 15}

    def __init__(
        self,
        loading_zone_pos: tuple[float, float, float],
        *,
        save_slot: int = 0,
        frame_skip: int = 4,
        max_steps: int = 500,
        terminal_bonus: float = 100.0,
        distance_scale: float = 1000.0,
    ):
        super().__init__()
        self.loading_zone_pos = np.asarray(loading_zone_pos, dtype=np.float32)
        self.save_slot = save_slot
        self.frame_skip = frame_skip
        self.max_steps = max_steps
        self.terminal_bonus = terminal_bonus
        self.distance_scale = distance_scale

        h, w = soh.get_frame_dimensions()
        self.observation_space = spaces.Box(0, 255, (h, w, 3), dtype=np.uint8)
        self.action_space = spaces.Discrete(len(_ACTIONS))

        self._steps = 0
        self._initial_scene = -1

    def reset(self, *, seed=None, options=None):
        super().reset(seed=seed)

        soh.load_state(self.save_slot)
        soh.clear_input()
        soh.step_frame()

        gs = soh.get_game_state()
        self._initial_scene = int(gs.scene)
        self._steps = 0

        obs = soh.get_frame()
        return obs, self._info(gs, action_name="<reset>")

    def step(self, action: int):
        name, btn, sx, sy = _ACTIONS[int(action)]
        soh.set_input(port=0, buttons=btn, stick_x=sx, stick_y=sy)
        for _ in range(self.frame_skip):
            soh.step_frame()

        gs = soh.get_game_state()
        pos = np.array([gs.pos_x, gs.pos_y, gs.pos_z], dtype=np.float32)
        dist = float(np.linalg.norm(pos - self.loading_zone_pos))

        reward = -dist / self.distance_scale

        self._steps += 1
        terminated = bool(gs.valid) and int(gs.scene) != self._initial_scene
        truncated  = (self._steps >= self.max_steps) and not terminated

        if terminated:
            reward += self.terminal_bonus

        obs = soh.get_frame()
        return obs, reward, terminated, truncated, self._info(gs, action_name=name, dist=dist)

    def render(self):
        return soh.get_frame()

    def close(self):
        pass

    def _info(self, gs, *, action_name: str, dist: float | None = None) -> dict:
        info = {
            "scene": int(gs.scene),
            "pos":   (float(gs.pos_x), float(gs.pos_y), float(gs.pos_z)),
            "hp":    (int(gs.hp), int(gs.max_hp)),
            "action": action_name,
        }
        if dist is not None:
            info["dist_to_target"] = dist
        return info
    