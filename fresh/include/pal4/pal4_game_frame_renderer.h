#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IPal4GameFrameRuntime {
public:
    virtual ~IPal4GameFrameRuntime() = default;
    [[nodiscard]] virtual std::int32_t HandleVideoModeChange() = 0;
    [[nodiscard]] virtual std::uint32_t InitCameraSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCamera(
        std::uint32_t subsystem) = 0;
    [[nodiscard]] virtual std::uint32_t InternalIdSlot(
        std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t GameState() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    virtual void UpdateAudio(std::uint32_t manager, float elapsed) = 0;
    virtual void UpdateGameObject(std::uint32_t object, float elapsed,
                                  std::uint32_t camera_id,
                                  std::uint32_t override_camera) = 0;
    virtual void SetRenderOption(std::uint32_t option,
                                 std::uint32_t value) = 0;
    virtual void RenderGameObject(std::uint32_t object,
                                  std::uint32_t camera_id_slot) = 0;
    virtual void CameraClear(std::uint32_t camera, std::uint32_t color,
                             std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t CameraBeginUpdate(
        std::uint32_t camera) = 0;
    virtual void CameraEndUpdate(std::uint32_t camera) = 0;
    virtual void ShowGameRaster(std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t RenderSystem() = 0;
    virtual void ClearRenderCollection(std::uint32_t render_system) = 0;
    virtual void DrawTexturedRectangle(
        std::uint32_t camera_id_slot, std::uint32_t texture,
        std::uint32_t x, std::uint32_t y,
        std::uint32_t width, std::uint32_t height,
        const std::array<std::uint8_t, 4>& color) = 0;
    virtual void DrawTextCollection(std::uint32_t render_system,
                                    std::uint32_t camera_id_slot) = 0;
    [[nodiscard]] virtual std::uint32_t FrameCallback() = 0;
    virtual void ApplyTextureProperties(std::uint32_t texture_object,
                                        float elapsed,
                                        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::int32_t HandleDeviceLoss() = 0;

    [[nodiscard]] virtual std::uint32_t EffectManager() = 0;
    virtual void UpdateRenderParams(std::uint32_t manager,
                                    double elapsed) = 0;
    [[nodiscard]] virtual std::uint32_t CameraContainer(
        std::uint32_t camera) = 0;
    virtual void PrepareRenderFrame(std::uint32_t frame,
                                    std::uint32_t container) = 0;
    virtual void EndRenderFrame(std::uint32_t frame) = 0;
    virtual void RenderScene(std::uint32_t frame) = 0;
    virtual void RenderEffectPasses(std::uint32_t manager,
                                    std::uint32_t camera) = 0;
};

class Pal4GameFrameRenderer final {
public:
    // IDA 0x5c8034..0x5c804e: back-face culling, depth test, depth write.
    // Reapply after BeginUpdate: UI/movie passes may leave 2D states active.
    static constexpr std::array<std::array<std::uint32_t, 2>, 3>
        kSceneRenderOptions{{{20u, 2u}, {6u, 1u}, {8u, 1u}}};
    static constexpr std::uint32_t kGameObjectOffset = 0x18cu;
    static constexpr std::uint32_t kTextureOffset = 0x184u;
    static constexpr std::uint32_t kTextureObjectOffset = 0x178u;
    static constexpr std::uint32_t kScaleOffset = 0x57cu;
    static constexpr std::uint32_t kTextOverlayOffset = 0x554u;
    static constexpr std::uint32_t kTextureOverlayOffset = 0x588u;
    static constexpr std::uint32_t kEffectFrameOffset = 0x104u;
    static constexpr std::uint32_t kClearColor = 0x907928u;

    // IDA 0x5c7f30 / 0x5c8150.
    [[nodiscard]] static std::int32_t Render(
        double elapsed, std::uint32_t override_camera,
        IPal4GameFrameRuntime& runtime);
    [[nodiscard]] static std::int32_t RenderWithEffects(
        double elapsed, std::uint32_t override_camera,
        IPal4GameFrameRuntime& runtime);
};

}  // namespace pal4
