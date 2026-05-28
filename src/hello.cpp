#include "soh_lib.h"
#include "libultraship/libultra/controller.h"
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <math.h>

static const SoH_Input NEUTRAL     = { 0,         0,   0,   0, 0 };
static const SoH_Input PRESS_A     = { BTN_A,     0,   0,   0, 0 };
static const SoH_Input PRESS_START = { BTN_START, 0,   0,   0, 0 };
static const SoH_Input HOLD_FWD    = { 0,         0,   127, 0, 0 };
static const SoH_Input HOLD_RIGHT  = { 0,         127, 0,   0, 0 };

typedef struct { float x, y, z; } Vec3;

static SoH_GameState g_state;
static int g_frame = 0;

static void step(SoH_Input in) {
    SoH_SetInput(0, &in);
    SoH_StepFrame();
    SoH_GetGameState(&g_state);
    g_frame++;
}

static void step_n(SoH_Input in, int n) {
    for (int i = 0; i < n; i++) step(in);
}

static void boot_to_savefile(void) {
    step_n(NEUTRAL, 259);
    step(PRESS_A);
    step_n(NEUTRAL, 29);
    step(PRESS_START);
    step_n(NEUTRAL, 89);
    step(PRESS_A);
    step_n(NEUTRAL, 29);
    step(PRESS_A);
    step_n(NEUTRAL, 29);
}

static Vec3 run_trajectory(SoH_Input action, int n) {
    step_n(action, n);
    return (Vec3){ g_state.pos_x, g_state.pos_y, g_state.pos_z };
}

static float dist(Vec3 a, Vec3 b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}
static int exact_eq(Vec3 a, Vec3 b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

int main(int argc, char** argv) {
    SoH_SetHeadless(1);
    SoH_Init(argc, argv);
    spdlog::set_level(spdlog::level::info);
    SoH_EnableDebugView(640, 480);

    boot_to_savefile();
    printf("[boot] frame=%d valid=%d scene=%d pos=(%.2f,%.2f,%.2f)\n",
           g_frame, g_state.valid, g_state.scene_id,
           g_state.pos_x, g_state.pos_y, g_state.pos_z);

    int rc = SoH_SaveState(0);
    Vec3 start = { g_state.pos_x, g_state.pos_y, g_state.pos_z };
    printf("[save] rc=%d start=(%.2f,%.2f,%.2f)\n", rc, start.x, start.y, start.z);

    const int N = 60;

    Vec3 endA = run_trajectory(HOLD_FWD, N);
    printf("[A ] forward -> (%.2f,%.2f,%.2f)  moved %.2f\n",
           endA.x, endA.y, endA.z, dist(start, endA));

    SoH_LoadState(0); SoH_GetGameState(&g_state);
    Vec3 endB = run_trajectory(HOLD_RIGHT, N);
    printf("[B ] right   -> (%.2f,%.2f,%.2f)  moved %.2f\n",
           endB.x, endB.y, endB.z, dist(start, endB));

    SoH_LoadState(0); SoH_GetGameState(&g_state);
    Vec3 endA2 = run_trajectory(HOLD_FWD, N);
    printf("[A'] forward -> (%.2f,%.2f,%.2f)\n", endA2.x, endA2.y, endA2.z);

    printf("\n=== RESULTS ===\n");
    float ab = dist(endA, endB);
    printf("M4  input affects behavior (A vs B):   dist=%.3f  -> %s\n",
           ab, ab > 1.0f ? "PASS" : "FAIL (input had no/equal effect)");
    int det = exact_eq(endA, endA2);
    printf("M6  reset is deterministic (A vs A'):  %s\n",
           det ? "PASS (bit-identical)" : "FAIL (diverged)");
    if (!det) {
        printf("    A =(%.6f,%.6f,%.6f)\n    A'=(%.6f,%.6f,%.6f)  dist=%.6f\n",
               endA.x, endA.y, endA.z, endA2.x, endA2.y, endA2.z, dist(endA, endA2));
    }

    SoH_DisableDebugView();
    SoH_Shutdown();
    return 0;
}