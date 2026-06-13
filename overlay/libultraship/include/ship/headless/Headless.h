#pragma once

#include <stdint.h>

#ifdef __cplusplus
namespace Ship::Headless {
    void SetEnabled(bool enabled);
    bool IsEnabled();

    struct PadOverride {
        uint16_t buttons;
        int8_t stick_x;
        int8_t stick_y;
        int8_t right_stick_x;
        int8_t right_stick_y;
    };

    void SetPadOverride(int port, const PadOverride& pad);
    void ClearPadOverride(int port);
    bool HasPadOverride(int port);
    PadOverride GetPadOverride(int port);
}
#endif  // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

void Ship_Headless_SetEnabled(int enabled);
int  Ship_Headless_IsEnabled(void);

void Ship_Headless_SetPadOverride(int port, uint16_t buttons,
                                  int8_t stick_x, int8_t stick_y,
                                  int8_t right_stick_x, int8_t right_stick_y);
void Ship_Headless_ClearPadOverride(int port);
int  Ship_Headless_HasPadOverride(int port);

#ifdef __cplusplus
}
#endif