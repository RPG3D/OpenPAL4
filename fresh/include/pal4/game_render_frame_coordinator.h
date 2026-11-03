#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IGameFrameRuntime {
public:
    virtual ~IGameFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual int HandleVideoModeChange() = 0;
    [[nodiscard]] virtual std::uint32_t InitCameraSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCamera(
        std::uint32_t camera_manager) = 0;
    [[nodiscard]] virtual std::uint32_t InternalIdSlot(
        std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t GameState() = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t CombatCamera(
        std::uint32_t combat) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    virtual void UpdateAudio(std::uint32_t audio, float elapsed) = 0;
    virtual void UpdateGameObject(std::uint32_t object, float elapsed,
                                  std::uint32_t camera_id,
                                  std::uint32_t override_camera) = 0;
    virtual void UpdateRenderParams(std::uint32_t combat_camera,
                                    double elapsed) = 0;
    [[nodiscard]] virtual std::uint32_t CameraContainer(
        std::uint32_t camera) = 0;
    virtual void PrepareRenderFrame(std::uint32_t frame,
                                    std::uint32_t container) = 0;
    virtual void SetRenderOption(std::uint32_t option,
                                 std::uint32_t value) = 0;
    virtual void RenderGameObject(std::uint32_t object,
                                  std::uint32_t camera_id_slot) = 0;
    virtual void DrawTexturedRectangle(std::uint32_t camera_id_slot,
                                       std::uint32_t texture,
                                       std::uint32_t x, std::uint32_t y,
                                       std::uint32_t width,
                                       std::uint32_t height,
                                       const std::array<std::uint8_t, 4>& color) = 0;
    [[nodiscard]] virtual std::uint32_t RenderSystem() = 0;
    virtual void DrawTextCollection(std::uint32_t render_system,
                                    std::uint32_t camera_id_slot) = 0;
    virtual void EndRenderFrame(std::uint32_t frame) = 0;
    virtual void RenderScene(std::uint32_t frame) = 0;
    virtual void CameraClear(std::uint32_t camera, std::uint32_t color,
                             std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t CameraBeginUpdate(
        std::uint32_t camera) = 0;
    virtual void RenderPasses(std::uint32_t combat_camera,
                              std::uint32_t camera) = 0;
    virtual void CameraEndUpdate(std::uint32_t camera) = 0;
    virtual void ShowGameRaster(std::uint32_t camera) = 0;
    virtual void ClearRenderCollection(std::uint32_t render_system) = 0;
    [[nodiscard]] virtual std::uint32_t TexturePropertyGlobal() = 0;
    virtual void ApplyTextureProperties(std::uint32_t game_property,
                                        float elapsed,
                                        std::uint32_t property) = 0;
    [[nodiscard]] virtual int HandleDeviceLoss() = 0;
};

class GameRenderFrameCoordinator final {
public:
    static constexpr std::uint32_t kGameObjectOffset = 396u;
    static constexpr std::uint32_t kTextureOffset = 388u;
    static constexpr std::uint32_t kTexturePropertyOffset = 376u;
    static constexpr std::uint32_t kScaleOffset = 1404u;
    static constexpr std::uint32_t kTextOverlayOffset = 1364u;
    static constexpr std::uint32_t kTextureOverlayOffset = 1416u;
    static constexpr std::uint32_t kRenderFrameOffset = 260u;
    static constexpr std::uint32_t kClearColor = 0x8f94b0u;

    [[nodiscard]] static int Update(double elapsed,
                                    std::uint32_t override_camera,
                                    IGameFrameRuntime& runtime);
};

}  // namespace pal4
