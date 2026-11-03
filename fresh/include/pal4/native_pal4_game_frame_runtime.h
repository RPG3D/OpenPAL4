#pragma once

#include "pal4/camera_scene_frame_native.h"
#include "pal4/game_render_system.h"
#include "pal4/pal4_game_frame_renderer.h"

namespace pal4 {

// Supplies the native RenderWare boundary used by Game_RenderFrame.
// Game-state, audio, UI and device-loss services remain with the caller.
class NativePal4GameFrameRuntime : public IPal4GameFrameRuntime {
public:
    NativePal4GameFrameRuntime(GameRenderState& game,
                               std::uint32_t game_state,
                               IRenderWarePresentation& presentation)
        : game_(game), game_state_(game_state),
          presentation_(presentation) {}

    [[nodiscard]] std::uint32_t GameInstance() override;
    [[nodiscard]] std::uint32_t GameState() override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    [[nodiscard]] float ReadFloat(std::uint32_t address) override;
    void SetRenderOption(std::uint32_t option, std::uint32_t value) override;
    void CameraClear(std::uint32_t camera, std::uint32_t color,
                     std::uint32_t flags) override;
    [[nodiscard]] std::uint32_t CameraBeginUpdate(std::uint32_t camera) override;
    void CameraEndUpdate(std::uint32_t camera) override;
    void ShowGameRaster(std::uint32_t camera) override;
    [[nodiscard]] std::uint32_t RenderSystem() override;

private:
    GameRenderState& game_;
    std::uint32_t game_state_;
    IRenderWarePresentation& presentation_;
    NativeCameraSceneFrameRuntime camera_;
};

}  // namespace pal4
