/*
 * soh_api.c — Public C API for the headless / RL library build (SOH_AS_LIB).
 *
 * This file is OURS, not part of upstream Ship of Harkinian. It is added by the
 * oot_rl patch series and is never touched by upstream, so it carries almost the
 * entire RL-facing API surface. Only code that is inseparable from main.c's
 * file-static bootstrap state (Main_LibInit/Shutdown, SoH_Init/Shutdown) and the
 * one-line SoH_StepFrame hook in graph.c live outside this file.
 *
 * Keep upstream-file edits minimal: when adding new API, add it here.
 */

#ifdef SOH_AS_LIB

#include "global.h"
#include "soh_lib.h"

#include "ship/headless/Headless.h"
#include "ship/headless/DebugView.h"
#include "ship/headless/FrameCapture.h"

/* ------------------------------------------------------------------ */
/* Runtime mode                                                       */
/* ------------------------------------------------------------------ */

void SoH_SetHeadless(int enabled) {
    Ship_Headless_SetEnabled(enabled);
}

/* ------------------------------------------------------------------ */
/* Input                                                              */
/* ------------------------------------------------------------------ */

void SoH_SetInput(int port, const SoH_Input* input) {
    if (input == NULL) {
        Ship_Headless_ClearPadOverride(port);
        return;
    }
    Ship_Headless_SetPadOverride(port,
                                 input->buttons,
                                 input->stick_x,
                                 input->stick_y,
                                 input->right_stick_x,
                                 input->right_stick_y);
}

void SoH_ClearInput(int port) {
    Ship_Headless_ClearPadOverride(port);
}

/* ------------------------------------------------------------------ */
/* Game state readout                                                 */
/* ------------------------------------------------------------------ */

int SoH_GetGameState(SoH_GameState* out) {
    if (out == NULL) return 0;

    out->health     = gSaveContext.health;
    out->max_health = gSaveContext.healthCapacity;
    out->magic      = gSaveContext.magic;
    out->rupees     = gSaveContext.rupees;
    out->day_time   = gSaveContext.dayTime;

    if (gPlayState == NULL) {
        out->valid       = 0;
        out->scene_id    = -1;
        out->pos_x = out->pos_y = out->pos_z = 0.0f;
        out->rot_y       = 0;
        return 0;
    }

    out->scene_id = gPlayState->sceneNum;

    Player* link = GET_PLAYER(gPlayState);
    if (link == NULL) {
        out->valid = 0;
        out->pos_x = out->pos_y = out->pos_z = 0.0f;
        out->rot_y = 0;
        return 0;
    }

    out->valid = 1;
    out->pos_x = link->actor.world.pos.x;
    out->pos_y = link->actor.world.pos.y;
    out->pos_z = link->actor.world.pos.z;

    out->rot_x = link->actor.world.rot.x;
    out->rot_y = link->actor.world.rot.y;
    out->rot_z = link->actor.world.rot.z;

    return 1;
}

/* ------------------------------------------------------------------ */
/* Debug view (on-screen mirror of the headless framebuffer)          */
/* ------------------------------------------------------------------ */

void SoH_EnableDebugView(uint32_t width, uint32_t height) {
    Ship_Headless_EnableDebugView(width, height);
}

void SoH_DisableDebugView(void) {
    Ship_Headless_DisableDebugView();
}

/* ------------------------------------------------------------------ */
/* Frame capture (RGB readback of the game framebuffer)               */
/* ------------------------------------------------------------------ */

void SoH_EnableFrameCapture(void) {
    Ship_Headless_EnableFrameCapture();
}

void SoH_DisableFrameCapture(void) {
    Ship_Headless_DisableFrameCapture();
}

int SoH_GetFrameDimensions(int* w, int* h) {
    return Ship_Headless_GetFrameDimensions(w, h);
}

int SoH_GetFrame(unsigned char* out) {
    return Ship_Headless_GetFrame(out);
}

/* ------------------------------------------------------------------ */
/* Boot                                                               */
/*                                                                    */
/* The save anchor (data/file1.sav) auto-loads on boot, so the engine */
/* enters GamePlay mode on its own after a few hundred frames. We just */
/* step with neutral input until a PlayState with a spawned Player     */
/* exists, giving every caller (smoke test, Python envs) one shared    */
/* boot primitive instead of re-implementing the loop.                */
/*                                                                    */
/* SCOPE: a true return means "GamePlay mode reached, save_state works".*/
/* It does NOT yet distinguish the title-screen attract demo from a    */
/* controllable save-file load — that is the open M7 boot-sequence     */
/* problem and is intentionally left to a dedicated effort. Do not      */
/* treat a true return as "the agent now controls Link".               */
/* ------------------------------------------------------------------ */

int SoH_BootToGameplay(void) {
    SoH_GameState gs;
    int i;

    SoH_ClearInput(0);
    for (i = 0; i < 1200; i++) {
        SoH_StepFrame();
        if (SoH_GetGameState(&gs) && gs.valid) {
            return 1;
        }
    }
    return 0;
}

#endif /* SOH_AS_LIB */
