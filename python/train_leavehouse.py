"""M8 prototyping harness: train PPO on LeaveHouseEnv with Stable-Baselines3.

This is a THROWAWAY prototyping script, not the project's RL implementation — the
env stays framework-agnostic (gym.Env) so the real, hand-written algorithms can
drop in later. SB3 here just proves the pipeline end-to-end (wrappers -> CNN PPO
-> logging -> eval video) and gives a single-process throughput baseline for M9.

Reward is deliberately sparse (see the M8 roadmap): this validates the loop, it
does not aim to learn the task well.

    PYTHONPATH=python LUS_APP_BUNDLE_PATH=$PWD/build-cmake \
        python python/train_leavehouse.py --timesteps 5000 [--rgb] [--record]

SoH is a process-global singleton, so this trains a SINGLE env in-process
(DummyVecEnv). Multi-process scaling is M9.
"""
from __future__ import annotations

import argparse
import os
import time

os.environ.setdefault(
    "LUS_APP_BUNDLE_PATH",
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build-cmake"),
)

import numpy as np
from gymnasium.wrappers import ResizeObservation, GrayscaleObservation

from stable_baselines3 import PPO
from stable_baselines3.common.vec_env import DummyVecEnv, VecFrameStack, VecMonitor

import oot_rl as soh
from oot_rl_gym import LeaveHouseEnv


def build_venv(grayscale: bool, n_stack: int, max_steps: int):
    """LeaveHouseEnv -> resize 84x84 -> (grayscale) -> DummyVecEnv -> frame-stack.

    SB3 then sees a (84, 84, C*n_stack) uint8 image and auto-applies
    VecTransposeImage for its CnnPolicy.
    """
    def make_env():
        env = LeaveHouseEnv(max_steps=max_steps)
        env = ResizeObservation(env, (84, 84))
        if grayscale:
            env = GrayscaleObservation(env, keep_dim=True)  # (84, 84, 1)
        return env

    venv = DummyVecEnv([make_env])      # singleton engine -> exactly one env
    venv = VecFrameStack(venv, n_stack=n_stack)
    venv = VecMonitor(venv)             # episode return / length logging
    return venv


def record_eval(model, venv, out: str, max_steps: int, fps: int = 15):
    import imageio
    obs = venv.reset()
    writer = imageio.get_writer(out, fps=fps)
    frames = 0
    for _ in range(max_steps):
        action, _ = model.predict(obs, deterministic=True)
        obs, _, done, _ = venv.step(action)
        writer.append_data(soh.get_frame())   # raw engine frame, not the 84x84 obs
        frames += 1
        if done[0]:
            break
    writer.close()
    print(f"[eval] wrote {frames} frames -> {out}")
    import shutil
    import subprocess
    if shutil.which("wslpath") and shutil.which("explorer.exe"):
        try:
            win = subprocess.check_output(["wslpath", "-w", out]).decode().strip()
            subprocess.run(["explorer.exe", win], check=False)
        except Exception:
            pass


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--timesteps", type=int, default=5000)
    ap.add_argument("--n-steps", type=int, default=1024, help="PPO rollout length")
    ap.add_argument("--n-stack", type=int, default=4)
    ap.add_argument("--max-steps", type=int, default=500, help="env episode cap")
    ap.add_argument("--rgb", action="store_true", help="keep RGB instead of grayscale")
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--logdir", default="/tmp/oot_rl_tb")
    ap.add_argument("--save", default="/tmp/leavehouse_ppo.zip")
    ap.add_argument("--record", action="store_true", help="record an eval episode to MP4 after training")
    args = ap.parse_args()

    venv = build_venv(grayscale=not args.rgb, n_stack=args.n_stack, max_steps=args.max_steps)
    print(f"[train] obs space (post-wrappers) = {venv.observation_space}")

    model = PPO(
        "CnnPolicy",
        venv,
        n_steps=args.n_steps,
        batch_size=256,
        seed=args.seed,
        tensorboard_log=args.logdir,
        verbose=1,
        device="auto",
    )

    t0 = time.time()
    model.learn(total_timesteps=args.timesteps, progress_bar=False)
    dt = time.time() - t0
    sps = args.timesteps / dt
    print(f"\n[train] {args.timesteps} steps in {dt:.1f}s "
          f"=> {sps:.1f} env-steps/s single-process (M9 baseline)")

    model.save(args.save)
    print(f"[train] saved model -> {args.save}")

    if args.record:
        record_eval(model, venv, "/tmp/leavehouse_eval.mp4", max_steps=args.max_steps)

    venv.close()


if __name__ == "__main__":
    main()
