#ifdef LUS_HAS_EGL

#include "fast/backends/gfx_egl.h"
#include "ship/Context.h"
#include "ship/window/Window.h"
#include "ship/window/gui/Gui.h"

#include <spdlog/spdlog.h>
#include <EGL/eglext.h>

namespace Fast {

void GfxWindowBackendEGL::Init(const char* gameName, const char* gfxApiName,
                                bool startFullScreen, uint32_t width, uint32_t height,
                                int32_t posX, int32_t posY) {
    mWindowWidth = width;
    mWindowHeight = height;

    EGLint major = 0, minor = 0;
    mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mEglDisplay != EGL_NO_DISPLAY && eglInitialize(mEglDisplay, &major, &minor)) {
        SPDLOG_INFO("EGL initialized: {}.{} (default display)", major, minor);
    } else {
        SPDLOG_INFO("Default EGL display unavailable, trying platform-device path");

        auto eglQueryDevicesEXT = reinterpret_cast<PFNEGLQUERYDEVICESEXTPROC>(
            eglGetProcAddress("eglQueryDevicesEXT"));
        auto eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
            eglGetProcAddress("eglGetPlatformDisplayEXT"));

        if (!eglQueryDevicesEXT || !eglGetPlatformDisplayEXT) {
            SPDLOG_ERROR("EGL device extensions not available");
            return;
        }

        EGLDeviceEXT devices[16];
        EGLint numDevices = 0;
        if (!eglQueryDevicesEXT(16, devices, &numDevices) || numDevices < 1) {
            SPDLOG_ERROR("eglQueryDevicesEXT found no devices");
            return;
        }

        mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, devices[0], nullptr);
        if (mEglDisplay == EGL_NO_DISPLAY) {
            SPDLOG_ERROR("eglGetPlatformDisplayEXT failed");
            return;
        }

        if (!eglInitialize(mEglDisplay, &major, &minor)) {
            SPDLOG_ERROR("eglInitialize on device display failed");
            return;
        }
        SPDLOG_INFO("EGL initialized: {}.{} (device 0, {} devices total)",
                    major, minor, numDevices);
    }

    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(mEglDisplay, configAttribs, &config, 1, &numConfigs) || numConfigs < 1) {
        SPDLOG_ERROR("eglChooseConfig failed (no matching config)");
        return;
    }

    if (!eglBindAPI(EGL_OPENGL_API)) {
        SPDLOG_ERROR("eglBindAPI(EGL_OPENGL_API) failed");
        return;
    }

    EGLint pbufferAttribs[] = {
        EGL_WIDTH,  (EGLint)mWindowWidth,
        EGL_HEIGHT, (EGLint)mWindowHeight,
        EGL_NONE
    };
    mEglSurface = eglCreatePbufferSurface(mEglDisplay, config, pbufferAttribs);
    if (mEglSurface == EGL_NO_SURFACE) {
        SPDLOG_ERROR("eglCreatePbufferSurface failed");
        return;
    }

    EGLint contextAttribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 4,
        EGL_CONTEXT_MINOR_VERSION, 1,
        EGL_NONE
    };
    mEglContext = eglCreateContext(mEglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    if (mEglContext == EGL_NO_CONTEXT) {
        SPDLOG_ERROR("eglCreateContext failed");
        return;
    }

    if (!eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
        SPDLOG_ERROR("eglMakeCurrent failed");
        return;
    }

    SPDLOG_INFO("EGL backend initialised: {}x{} PBuffer", mWindowWidth, mWindowHeight);

    Ship::GuiWindowInitData window_impl;
    window_impl.Opengl.Window = nullptr;
    window_impl.Opengl.Context = mEglContext;
    Ship::Context::GetInstance()->GetWindow()->GetGui()->Init(window_impl);
}

void GfxWindowBackendEGL::SwapBuffersBegin() {
    if (mEglDisplay != EGL_NO_DISPLAY && mEglSurface != EGL_NO_SURFACE) {
        eglSwapBuffers(mEglDisplay, mEglSurface);
    }
}

void GfxWindowBackendEGL::SwapBuffersEnd() {
}

void GfxWindowBackendEGL::HandleEvents() {
}

void GfxWindowBackendEGL::GetDimensions(uint32_t* width, uint32_t* height,
                                         int32_t* posX, int32_t* posY) {
    if (width)  *width  = mWindowWidth;
    if (height) *height = mWindowHeight;
    if (posX)   *posX   = 0;
    if (posY)   *posY   = 0;
}

void GfxWindowBackendEGL::Destroy() {
    if (mEglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (mEglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(mEglDisplay, mEglContext);
            mEglContext = EGL_NO_CONTEXT;
        }
        if (mEglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(mEglDisplay, mEglSurface);
            mEglSurface = EGL_NO_SURFACE;
        }
        eglTerminate(mEglDisplay);
        mEglDisplay = EGL_NO_DISPLAY;
    }
}

GfxWindowBackendEGL::~GfxWindowBackendEGL() {
    Destroy();
}

} // namespace Fast

#endif // LUS_HAS_EGL