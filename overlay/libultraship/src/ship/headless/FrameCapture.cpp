#include "ship/headless/FrameCapture.h"

#ifdef LUS_HAS_EGL
#include <EGL/egl.h>
#include <GL/gl.h>
#endif

#include "ship/Context.h"
#include "fast/Fast3dWindow.h"
#include <memory>

namespace Ship::Headless {

static bool sFrameCaptureEnabled = false;

void SetFrameCaptureEnabled(bool enabled) { sFrameCaptureEnabled = enabled; }
bool IsFrameCaptureEnabled() { return sFrameCaptureEnabled; }

static unsigned int GetGameTexture(uint32_t* w, uint32_t* h) {
#ifdef LUS_HAS_EGL
    auto context = Ship::Context::GetInstance();
    if (!context) return 0;
    auto window = context->GetWindow();
    if (!window) return 0;
    auto fast3d = std::dynamic_pointer_cast<Fast::Fast3dWindow>(window);
    if (!fast3d) return 0;
    if (!fast3d->IsRenderingToFramebuffer()) return 0;
    GLuint tex = static_cast<GLuint>(fast3d->GetGameFramebufferTexture());
    if (!tex) return 0;

    glBindTexture(GL_TEXTURE_2D, tex);
    GLint tw = 0, th = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tw);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &th);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (w) *w = (uint32_t)tw;
    if (h) *h = (uint32_t)th;
    return tex;
#else
    return 0;
#endif
}

} // namespace Ship::Headless

extern "C" {

void Ship_Headless_EnableFrameCapture(void) {
    Ship::Headless::SetFrameCaptureEnabled(true);
}

void Ship_Headless_DisableFrameCapture(void) {
    Ship::Headless::SetFrameCaptureEnabled(false);
}

int Ship_Headless_GetFrameDimensions(int* width, int* height) {
#ifdef LUS_HAS_EGL
    uint32_t w = 0, h = 0;
    if (!Ship::Headless::GetGameTexture(&w, &h)) return -1;
    if (width)  *width  = (int)w;
    if (height) *height = (int)h;
    return 0;
#else
    return -1;
#endif
}

int Ship_Headless_GetFrame(unsigned char* out) {
#ifdef LUS_HAS_EGL
    uint32_t w = 0, h = 0;
    GLuint tex = Ship::Headless::GetGameTexture(&w, &h);
    if (!tex) return -1;
    glBindTexture(GL_TEXTURE_2D, tex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, out);  // 3 bytes/pixel
    glBindTexture(GL_TEXTURE_2D, 0);
    return 0;
#else
    return -1;
#endif
}

} // extern "C"
