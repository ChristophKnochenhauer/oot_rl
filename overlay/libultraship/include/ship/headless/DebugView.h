#pragma once

#include <stdint.h>

#ifdef __cplusplus
namespace Ship::Headless {

class DebugView {
  public:
    static DebugView* GetInstance();

    void Enable(uint32_t width, uint32_t height);
    void Disable();
    bool IsEnabled() const { return mEnabled; }

    void CaptureFrame();

  private:
    DebugView() = default;
    ~DebugView();

    bool mEnabled = false;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;

    void* mWindow = nullptr;
    void* mRenderer = nullptr;
    void* mTexture = nullptr;

    uint8_t* mPixels = nullptr;
};

}  // namespace Ship::Headless
#endif  // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

void Ship_Headless_EnableDebugView(uint32_t width, uint32_t height);
void Ship_Headless_DisableDebugView(void);
int  Ship_Headless_IsDebugViewEnabled(void);

#ifdef __cplusplus
}
#endif
