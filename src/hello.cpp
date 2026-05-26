#include "soh_lib.h"
#include <spdlog/spdlog.h>
#include <stdio.h>

int main(int argc, char** argv) {
    SoH_SetHeadless(1);
    SoH_Init(argc, argv);
    spdlog::set_level(spdlog::level::info);

    SoH_GameState state;
    
    SoH_Input input = {};
    bool input_set = false;

    for (int frame = 0; frame < 1200; frame++) {
        SoH_StepFrame();
        SoH_GetGameState(&state);

        if (state.valid && !input_set) {
            input.buttons = 0;
            input.stick_x = 0;
            input.stick_y = 127;
            SoH_SetInput(0, &input);
            input_set = true;
            printf("[hello] frame=%d INPUT SET: stick_y=127 (full forward)\n", frame);
        }

        if (frame % 30 == 0) {
            printf("[hello] frame=%d valid=%d scene=%d pos=(%.1f,%.1f,%.1f) rot_y=%.2f hp=%d/%d\n",
                   frame, state.valid, state.scene_id,
                   state.pos_x, state.pos_y, state.pos_z, state.rot_y,
                   state.health, state.max_health);
        }
    }

    SoH_Shutdown();
    return 0;
}
