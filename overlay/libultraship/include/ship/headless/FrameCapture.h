#pragma once

#include <stdint.h>

#ifdef __cplusplus
namespace Ship::Headless {

void SetFrameCaptureEnabled(bool enabled);
bool IsFrameCaptureEnabled();

} // namespace Ship::Headless
#endif  // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

void Ship_Headless_EnableFrameCapture(void);
void Ship_Headless_DisableFrameCapture(void);
int Ship_Headless_GetFrameDimensions(int* width, int* height);
int Ship_Headless_GetFrame(unsigned char* out);

#ifdef __cplusplus
}
#endif
