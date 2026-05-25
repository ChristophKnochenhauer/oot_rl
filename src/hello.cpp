#include "soh_lib.h"
#include <stdio.h>

int main(int argc, char** argv) {
    SoH_SetHeadless(1);
    SoH_Init(argc, argv);

    SoH_Input pressed_a = {
        .buttons = 0x8000,
        .stick_x = 0,
        .stick_y = 0,
        .right_stick_x = 0,
        .right_stick_y = 0,
    };
    SoH_SetInput(0, &pressed_a);

    for (int i = 0; i < 120; i++) {
        SoH_StepFrame();
    }
    
    printf("[hello] 120 frames with synthetic input A. Clearing.\n");
    SoH_ClearInput(0);

    for (int i = 0; i < 60; i++) {
        SoH_StepFrame();
    }

    SoH_Shutdown();
    return 0;
}