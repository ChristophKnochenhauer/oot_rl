#include "soh_lib.h"
#include <spdlog/spdlog.h>
#include <stdio.h>

int main(int argc, char** argv) {
    SoH_SetHeadless(1);
    SoH_Init(argc, argv);
    SoH_EnableDebugView(640, 480);
    spdlog::set_level(spdlog::level::info);

    SoH_GameState state;
    bool saved = false;
    float saved_x = 0, saved_y = 0, saved_z = 0;

    int save_frame = -1;
    for (int frame = 0; frame < 400; frame++) {
        SoH_StepFrame();
        SoH_GetGameState(&state);

        if (state.valid && !saved) {
            int rc = SoH_SaveState(0);
            saved_x = state.pos_x;
            saved_y = state.pos_y;
            saved_z = state.pos_z;
            saved = true;
            save_frame = frame;
            printf("[hello] frame=%d SAVED slot 0 rc=%d pos=(%.1f,%.1f,%.1f)\n",
                   frame, rc, saved_x, saved_y, saved_z);
        }

        if (saved && frame == save_frame + 60) {
            printf("[hello] frame=%d PRE-LOAD pos=(%.1f,%.1f,%.1f)\n",
                   frame, state.pos_x, state.pos_y, state.pos_z);
            int rc = SoH_LoadState(0);
            SoH_GetGameState(&state);
            printf("[hello] frame=%d LOADED rc=%d pos=(%.1f,%.1f,%.1f) "
                   "(expected %.1f,%.1f,%.1f)\n",
                   frame, rc, state.pos_x, state.pos_y, state.pos_z,
                   saved_x, saved_y, saved_z);
        }
    }

    SoH_DisableDebugView();
    SoH_Shutdown();
    return 0;
}
