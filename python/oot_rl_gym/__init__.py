"""Gymnasium environments for oot_rl.

Named `oot_rl_gym` (not `oot_rl`) so it doesn't shadow the native pybind module
`oot_rl` on the import path.
"""
from oot_rl_gym.leave_house import LeaveHouseEnv

__all__ = ["LeaveHouseEnv"]
