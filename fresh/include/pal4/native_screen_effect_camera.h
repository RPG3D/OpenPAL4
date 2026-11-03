#pragma once
#include "pal4/screen_effect_camera.h"
#include <memory>

namespace pal4 {
// Owns a capture camera/texture/depth/frame; the bound game camera is borrowed.
// Unbind before destroying its world. Destroy this owner before RW shutdown.
// Dimensions and clip planes come from the game configuration/state, as in IDA41F480.
class NativeScreenEffectCamera final {
public:
    NativeScreenEffectCamera(int width, int height, float near_clip, float far_clip);
    // External game-allocated image must outlive this GPU-resource owner.
    NativeScreenEffectCamera(int width, int height, float near_clip, float far_clip,
                             ScreenEffectCameraRecord& storage);
    ~NativeScreenEffectCamera();
    NativeScreenEffectCamera(const NativeScreenEffectCamera&) = delete;
    NativeScreenEffectCamera& operator=(const NativeScreenEffectCamera&) = delete;
    void Bind(std::uint32_t game_camera);
    void Unbind();
    bool Initialize();
    void Cleanup();
    void SyncWorld();
    // Shared dispatcher copies the borrowed camera frame LTM, clears and begins capture.
    void Begin();
    void End();
    // Shared41BB30 quad, with RW texture-raster binding and public D3D9 draw APIs.
    // Caller begins the destination camera (and, for shader rendering, its effect pass).
    void Draw(float x, float y, float width, float height);
    const ScreenEffectCameraRecord& State() const noexcept;
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
