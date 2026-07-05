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

/* rc convention: 0 = success (out filled), negative = error. "No PlayState /
 * Player yet" is a legitimate state, NOT an error — it is reported via
 * out->valid == 0 while the rc stays 0. The only error is out == NULL. */
int SoH_GetGameState(SoH_GameState* out) {
    if (out == NULL) return -1;

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

    return 0;
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

/* Internal helper for SoH_WarpToEntrance() (implemented in SoH_Warp.cpp).
 * SoH's Warp() always forces a fixed respawn position; clearing respawnFlag
 * makes the upcoming entrance load fall back to the entrance's own natural,
 * always-in-bounds spawn point instead. Lives here because gSaveContext is a C
 * symbol most cleanly touched from this (C) translation unit. */
void SoH_ResetRespawnOverride(void) {
    gSaveContext.respawnFlag = 0;
}

/* Override Link's age after a warp. SoH's Warp() hardcodes adult (linkAge=0);
 * our early-game envs want child (linkAge=1). 0=adult, 1=child. Must be set
 * after Warp() and before the scene actually loads. */
void SoH_SetLinkAge(int age) {
    gSaveContext.linkAge = age;
}

#endif /* SOH_AS_LIB */
