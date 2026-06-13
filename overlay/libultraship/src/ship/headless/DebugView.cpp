#include "ship/headless/DebugView.h"

#include <SDL2/SDL.h>
#include <spdlog/spdlog.h>

#ifdef LUS_HAS_EGL
#include <EGL/egl.h>
#include <GL/gl.h>

#include "fast/Fast3dWindow.h"
#include "ship/Context.h"
#include "ship/window/Window.h"
#endif

namespace Ship::Headless {

DebugView* DebugView::GetInstance() {
    static DebugView instance;
    return &instance;
}

DebugView::~DebugView() {
    Disable();
}

void DebugView::Enable(uint32_t width, uint32_t height) {
    if (mEnabled && width == mWidth && height == mHeight) {
        return;
    }
    if (mEnabled) {
        Disable();
    }

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        SPDLOG_ERROR("[DebugView] SDL_InitSubSystem(VIDEO) failed: {}", SDL_GetError());
        return;
    }

    auto* window = SDL_CreateWindow("SoH Debug View",
                                    100, 100,
                                    width, height,
                                    SDL_WINDOW_SHOWN);
    
    if (!window) {
        SPDLOG_ERROR("[DebugView] SDL_CreateWindow failed: {}", SDL_GetError());
        return;
    }

    auto* renderer = SDL_CreateRenderer(static_cast<SDL_Window*>(window),
                                        -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        SPDLOG_ERROR("[DebugView] SDL_CreateRenderer failed: {}", SDL_GetError());
        SDL_DestroyWindow(static_cast<SDL_Window*>(window));
        return;
    }

    auto* texture = SDL_CreateTexture(static_cast<SDL_Renderer*>(renderer),
                                      SDL_PIXELFORMAT_ABGR8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      width, height);
    if (!texture) {
        SPDLOG_ERROR("[DebugView] SDL_CreateTexture failed: {}", SDL_GetError());
        SDL_DestroyRenderer(static_cast<SDL_Renderer*>(renderer));
        SDL_DestroyWindow(static_cast<SDL_Window*>(window));
        return;
    }

    mWindow = window;
    mRenderer = renderer;
    mTexture = texture;
    mWidth = width;
    mHeight = height;
    mPixels = new uint8_t[width * height * 4];
    mEnabled = true;

    SPDLOG_INFO("[DebugView] Enabled ({}x{})", width, height);
}

void DebugView::Disable() {
    if (!mEnabled) return;

    if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) {
        if (mTexture)  SDL_DestroyTexture(static_cast<SDL_Texture*>(mTexture));
        if (mRenderer) SDL_DestroyRenderer(static_cast<SDL_Renderer*>(mRenderer));
        if (mWindow)   SDL_DestroyWindow(static_cast<SDL_Window*>(mWindow));
    }

    delete[] mPixels;

    mTexture = mRenderer = mWindow = nullptr;
    mPixels = nullptr;
    mEnabled = false;
    SPDLOG_INFO("[DebugView] Disabled");
}

void DebugView::CaptureFrame() {
    if (!mEnabled) return;

#ifdef LUS_HAS_EGL
    auto context = Ship::Context::GetInstance();
    if (!context) return;
    auto window = context->GetWindow();
    if (!window) return;

    auto fast3d = std::dynamic_pointer_cast<Fast::Fast3dWindow>(window);
    if (!fast3d) return;

    const uint32_t row_bytes = mWidth * 4;
    bool needsFlip = false;
    if (fast3d->IsRenderingToFramebuffer()) {
        GLuint tex = static_cast<GLuint>(fast3d->GetGameFramebufferTexture());
        if (!tex) return;
        glBindTexture(GL_TEXTURE_2D, tex);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, mPixels);
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        glReadBuffer(GL_BACK);
        glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, mPixels);
        needsFlip = true;
    }

    if (needsFlip) {
        for (uint32_t y = 0; y < mHeight / 2; y++) {
            uint8_t* top = mPixels + y * row_bytes;
            uint8_t* bot = mPixels + (mHeight - 1 - y) * row_bytes;
            for (uint32_t b = 0; b < row_bytes; b++) {
                std::swap(top[b], bot[b]);
            }
        }
    }

    SDL_UpdateTexture(static_cast<SDL_Texture*>(mTexture), nullptr, mPixels, row_bytes);
    SDL_RenderClear(static_cast<SDL_Renderer*>(mRenderer));
    SDL_RenderCopy(static_cast<SDL_Renderer*>(mRenderer),
                   static_cast<SDL_Texture*>(mTexture), nullptr, nullptr);
    SDL_RenderPresent(static_cast<SDL_Renderer*>(mRenderer));

    SDL_PumpEvents();
#endif
}

}  // namespace Ship::Headless

extern "C" {

void Ship_Headless_EnableDebugView(uint32_t width, uint32_t height) {
    Ship::Headless::DebugView::GetInstance()->Enable(width, height);
}

void Ship_Headless_DisableDebugView(void) {
    Ship::Headless::DebugView::GetInstance()->Disable();
}

int Ship_Headless_IsDebugViewEnabled(void) {
    return Ship::Headless::DebugView::GetInstance()->IsEnabled() ? 1 : 0;
}

}
