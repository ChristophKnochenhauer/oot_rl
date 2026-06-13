/*
 * SoH_Warp.cpp — OUR overlay file (not upstream Ship of Harkinian).
 *
 * Thin extern "C" bridge to SoH's Warp() so the RL library can drop the engine
 * into a defined, controllable gameplay state at an arbitrary entrance + exact
 * position, instead of relying on the attract-demo boot path.
 *
 * Lives under soh/soh/ (not soh/src/) on purpose: the soh CMake target globs
 * soh/soh/ for both .c AND .cpp, but soh/src/ only for .c — and this shim must
 * be C++ to call the (C++) Warp() function.
 *
 * Warp() and WarpPoint live in soh/soh/Enhancements/Warping.cpp with no public
 * header, so we redeclare WarpPoint here (layout copied verbatim) and forward
 * to the tested implementation rather than reimplementing the warp logic.
 *
 * NOTE: z64math.h is included as plain C++ (NOT inside extern "C"). It pulls
 * libultraship headers that define C++ templates, which are illegal inside an
 * extern "C" block. Under C++ it also macro-renames Vec3f -> Vec3f_, exactly as
 * Warping.cpp sees it, so the WarpPoint layout stays in sync.
 */

#ifdef SOH_AS_LIB

#include "z64math.h" // Vec3f + s8/s16/s32 (via libultraship/libultra.h)

// Mirror of the .cpp-local definition in Warping.cpp. Layout must stay in sync.
typedef struct WarpPoint {
    s32 entranceId;
    s8 roomNum;
    Vec3f pos;
    s16 rotY;
    bool bootToPoint;
} WarpPoint;

// Defined in soh/soh/Enhancements/Warping.cpp (external linkage, no header).
void Warp(WarpPoint& warpPoint);

// Defined in soh_api.c — drops Warp()'s forced respawn position.
extern "C" void SoH_ResetRespawnOverride(void);
// Defined in soh_api.c — overrides Link's age (Warp() forces adult).
extern "C" void SoH_SetLinkAge(int age);

extern "C" int SoH_WarpTo(int entranceId, int roomNum,
                          float x, float y, float z, int rotY) {
    WarpPoint wp = {};
    wp.entranceId = (s32)entranceId;
    wp.roomNum = (s8)roomNum;
    wp.pos.x = x;
    wp.pos.y = y;
    wp.pos.z = z;
    wp.rotY = (s16)rotY;
    wp.bootToPoint = false;
    Warp(wp);
    return 0;
}

extern "C" int SoH_WarpToEntrance(int entranceId, int linkAge) {
    // Reuse Warp() for the full (tested) clean-game setup, then drop its forced
    // respawn position so Link spawns at the entrance's own natural spawn point
    // — guaranteed in-bounds, no coordinate guessing.
    WarpPoint wp = {};
    wp.entranceId = (s32)entranceId;
    wp.roomNum = 0;
    wp.pos.x = wp.pos.y = wp.pos.z = 0.0f;
    wp.rotY = 0;
    wp.bootToPoint = false;
    Warp(wp);
    SoH_ResetRespawnOverride();
    if (linkAge >= 0) {  // -1 = keep Warp()'s default (adult)
        SoH_SetLinkAge(linkAge);
    }
    return 0;
}

#endif /* SOH_AS_LIB */
