// smoke_test — end-to-end sanity check of the SoH library API.
//
// Boots the engine into GamePlay via the native SoH_BootToGameplay() helper,
// then exercises the three pillars the RL env depends on: game-state readout,
// frame capture, and save/load determinism. Returns non-zero on any failure so
// it can be wired as a CTest regression gate.

#include "soh_lib.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>

static int fail(const char* msg) {
    std::printf("[smoke] FAIL: %s\n", msg);
    return 1;
}

int main(int argc, char** argv) {
    SoH_SetHeadless(1);
    SoH_Init(argc, argv);
    SoH_EnableFrameCapture();

    // --- Boot -------------------------------------------------------------
    if (!SoH_BootToGameplay()) {
        SoH_Shutdown();
        return fail("SoH_BootToGameplay did not reach GamePlay");
    }

    // Let the framebuffer settle into rendered content (the first valid
    // frames after boot can still be mid-fade / black).
    for (int i = 0; i < 60; i++) SoH_StepFrame();

    SoH_GameState gs = {};
    SoH_GetGameState(&gs);
    std::printf("[smoke] boot ok: scene=%d pos=(%.1f, %.1f, %.1f) hp=%d/%d\n",
                gs.scene_id, gs.pos_x, gs.pos_y, gs.pos_z, gs.health, gs.max_health);
    if (!gs.valid) {
        SoH_Shutdown();
        return fail("GameState not valid after boot");
    }

    // --- Frame capture ----------------------------------------------------
    int w = 0, h = 0;
    if (SoH_GetFrameDimensions(&w, &h) != 0 || w <= 0 || h <= 0) {
        SoH_Shutdown();
        return fail("GetFrameDimensions returned bad dims");
    }
    unsigned char* buf = (unsigned char*)std::malloc((size_t)w * h * 3);
    unsigned long long sum = 0;
    if (SoH_GetFrame(buf) == 0) {
        for (size_t i = 0; i < (size_t)w * h * 3; i++) sum += buf[i];
    }
    std::free(buf);
    std::printf("[smoke] frame ok: %dx%d pixel_sum=%llu\n", w, h, sum);
    if (sum == 0) {
        SoH_Shutdown();
        return fail("captured frame is all black");
    }

    // --- Save/load determinism -------------------------------------------
    if (SoH_SaveState(0) != 0) {
        SoH_Shutdown();
        return fail("SoH_SaveState(0) failed");
    }
    SoH_GameState before = gs;

    SoH_Input fwd = {};
    fwd.stick_y = 80;
    for (int i = 0; i < 60; i++) {
        SoH_SetInput(0, &fwd);
        SoH_StepFrame();
    }
    SoH_ClearInput(0);

    if (SoH_LoadState(0) != 0) {
        SoH_Shutdown();
        return fail("SoH_LoadState(0) failed");
    }
    SoH_GetGameState(&gs);
    float d = std::sqrt((gs.pos_x - before.pos_x) * (gs.pos_x - before.pos_x) +
                        (gs.pos_y - before.pos_y) * (gs.pos_y - before.pos_y) +
                        (gs.pos_z - before.pos_z) * (gs.pos_z - before.pos_z));
    std::printf("[smoke] reset ok: dist_from_snapshot=%.3f\n", d);
    if (d > 0.01f) {
        SoH_Shutdown();
        return fail("load_state did not restore snapshot position");
    }

    SoH_Shutdown();
    std::printf("[smoke] PASS\n");
    return 0;
}
