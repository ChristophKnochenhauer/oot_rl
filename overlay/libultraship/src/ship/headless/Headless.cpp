#include "ship/headless/Headless.h"
#include <array>

namespace Ship::Headless {

namespace {
    bool s_enabled = false;
    
    constexpr int kMaxPorts = 4;
    std::array<PadOverride, kMaxPorts> s_padOverrides{};
    std::array<bool, kMaxPorts> s_hasOverride{};
}

void SetEnabled(bool enabled) { s_enabled = enabled; }
bool IsEnabled()               { return s_enabled; }

void SetPadOverride(int port, const PadOverride& pad) {
    if (port < 0 || port >= kMaxPorts) return;
    s_padOverrides[port] = pad;
    s_hasOverride[port] = true;
}

void ClearPadOverride(int port) {
    if (port < 0 || port >= kMaxPorts) return;
    s_hasOverride[port] = false;
}

bool HasPadOverride(int port) {
    if (port < 0 || port >= kMaxPorts) return false;
    return s_hasOverride[port];
}

PadOverride GetPadOverride(int port) {
    if (port < 0 || port >= kMaxPorts) return {};
    return s_padOverrides[port];
}

}  // namespace Ship::Headless

extern "C" {

void Ship_Headless_SetEnabled(int enabled) {
    Ship::Headless::SetEnabled(enabled != 0);
}

int Ship_Headless_IsEnabled(void) {
    return Ship::Headless::IsEnabled() ? 1 : 0;
}

void Ship_Headless_SetPadOverride(int port, uint16_t buttons,
                                  int8_t stick_x, int8_t stick_y,
                                  int8_t right_stick_x, int8_t right_stick_y) {
    Ship::Headless::PadOverride o;
    o.buttons = buttons;
    o.stick_x = stick_x;
    o.stick_y = stick_y;
    o.right_stick_x = right_stick_x;
    o.right_stick_y = right_stick_y;
    Ship::Headless::SetPadOverride(port, o);
}

void Ship_Headless_ClearPadOverride(int port) {
    Ship::Headless::ClearPadOverride(port);
}

int Ship_Headless_HasPadOverride(int port) {
    return Ship::Headless::HasPadOverride(port) ? 1 : 0;
}

}
