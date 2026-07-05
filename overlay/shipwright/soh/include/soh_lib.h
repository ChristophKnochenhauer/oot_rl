#ifndef SOH_LIB_H
#define SOH_LIB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Return-code convention for the whole C API
 * -----------------------------------------------------------------------------
 * Every function that returns an int rc uses: 0 = success, negative = error.
 * A negative rc identifies the failure (e.g. SoH_SaveState/SoH_LoadState return
 * the negated internal SaveStateReturn code, so -4 == FAIL_WRONG_GAMESTATE / a
 * dialog is open). It never encodes an ordinary game condition: e.g. "no
 * PlayState / Player yet" is NOT an error — SoH_GetGameState still returns 0 and
 * reports that state through the SoH_GameState.valid field. void functions
 * cannot fail. The Python bindings surface any negative rc as a RuntimeError. */

void SoH_Init(int argc, char** argv);
void SoH_StepFrame(void);
void SoH_Shutdown(void);
void SoH_SetHeadless(int enabled);

/* Drop the engine into a defined, controllable gameplay state: load the given
 * entrance/room and place Link at (x, y, z) facing rotY (binary angle). Forwards
 * to SoH's Warp(). This is THE path into controllable gameplay (not the
 * attract-demo auto-boot). Call after at least one stepped frame (needs an
 * active game state). Returns 0 on success. */
int SoH_WarpTo(int entrance_id, int room_num, float x, float y, float z, int rot_y);

/* Like SoH_WarpTo() but spawns Link at the entrance's OWN natural spawn point
 * (always in-bounds — no coordinate guessing). The coarse path into controllable
 * gameplay for M7-B3. Call from the title screen (before the attract demo's
 * PlayState exists) so SoH's Warp() takes its clean, GAMEMODE_NORMAL branch.
 * link_age: 0=adult, 1=child, -1=keep Warp()'s default (adult).
 * Returns 0 on success. */
int SoH_WarpToEntrance(int entrance_id, int link_age);

typedef struct {
    uint16_t buttons;
    int8_t stick_x;
    int8_t stick_y;
    int8_t right_stick_x;
    int8_t right_stick_y;
} SoH_Input;

void SoH_SetInput(int port, const SoH_Input* input);
void SoH_ClearInput(int port);

typedef struct {
    int valid;

    float pos_x;
    float pos_y;
    float pos_z;

    float rot_x;
    float rot_y;
    float rot_z;

    int16_t health;
    int16_t max_health;
    int16_t magic;
    int16_t rupees;

    int16_t scene_id;
    uint16_t day_time;
} SoH_GameState;

/* Fill *out with the current game state. Returns 0 on success (even when no
 * PlayState/Player exists yet — that legitimate condition is reported via
 * out->valid == 0, not via the rc). Returns a negative rc only on a real error
 * (out == NULL). */
int SoH_GetGameState(SoH_GameState* out);

void SoH_EnableDebugView(uint32_t width, uint32_t height);
void SoH_DisableDebugView(void);

/* Returns 0 on success, or the negated internal SaveStateReturn code on failure
 * (e.g. -4 == FAIL_WRONG_GAMESTATE, raised when called with a dialog open). */
int SoH_SaveState(unsigned int slot);
int SoH_LoadState(unsigned int slot);

/* Work around a savestate gap: ActorDB per-type load counters (numLoaded) live
 * outside gSystemHeap, so load_state leaks them and eventually overflows
 * (assert numLoaded < 255 in Actor_Spawn). Snapshot right after SoH_SaveState,
 * restore right after SoH_LoadState. */
void SoH_SnapshotActorCounts(void);
void SoH_RestoreActorCounts(void);

void SoH_EnableFrameCapture(void);
void SoH_DisableFrameCapture(void);
int  SoH_GetFrameDimensions(int* width, int* height);
int  SoH_GetFrame(unsigned char* out);

#ifdef __cplusplus
}
#endif

#endif /* SOH_LIB_H */
