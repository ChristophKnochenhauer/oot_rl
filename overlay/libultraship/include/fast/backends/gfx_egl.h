#pragma once

#ifdef LUS_HAS_EGL

#include "fast/backends/gfx_sdl.h"
#include <EGL/egl.h>

namespace Fast {

class GfxWindowBackendEGL : public GfxWindowBackendSDL2 {
  public:
    GfxWindowBackendEGL() = default;
    ~GfxWindowBackendEGL() override;

    void Init(const char* gameName, const char* apiName, bool startFullScreen,
              uint32_t width, uint32_t height, int32_t posX, int32_t posY) override;
    void SwapBuffersBegin() override;
    void SwapBuffersEnd() override;
    void Destroy() override;
    void HandleEvents() override;
    void GetDimensions(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY) override;

  private:
    EGLDisplay mEglDisplay = EGL_NO_DISPLAY;
    EGLContext mEglContext = EGL_NO_CONTEXT;
    EGLSurface mEglSurface = EGL_NO_SURFACE;
};

} // namespace Fast

#endif // LUS_HAS_EGL