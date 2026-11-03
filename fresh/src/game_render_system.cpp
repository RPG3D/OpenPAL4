#include "pal4/game_render_system.h"

#include <bit>

namespace pal4 {

std::uint32_t GameRenderSystem::ShowGameRaster(
    const std::uint32_t camera, IRenderWarePresentation& renderware) const {
    const std::uint32_t flags =
        state_.Read(8u) != 0u && state_.Read(0x54cu) != 0u ? 1u : 0u;
    return renderware.ShowRaster(camera, state_.Read(0u), flags);
}

std::int32_t GameRenderSystem::SetViewportCenter(
    const float x, const float y, IGameRenderPlatform& platform) {
    if (state_.Read(0x20u) == 0u) {
        return static_cast<std::int32_t>(state_.Address());
    }
    state_.Write(0x2cu, 0u);
    std::int32_t point_x = platform.FloatToLong(x);
    std::int32_t point_y = platform.FloatToLong(y);
    platform.ClientToScreen(state_.Read(0u), point_x, point_y);
    return platform.SetCursorPos(point_x, point_y);
}

std::uint32_t GameRenderSystem::DestroyTexture(
    const std::uint32_t dictionary,
    ITextureDictionaryDestroyRuntime& renderware) const {
    renderware.DestroyTextureDictionary(dictionary);
    return dictionary;
}

std::int32_t GameRenderSystem::ShutdownRenderSystem(
    IRenderWareApi& renderware, IGameDisplayModeRuntime& platform) {
    renderware.CloseCharset();
    renderware.ForEachTextureDictionary(*this, 0u);
    renderware.CloseFileSystemManager();
    renderware.EngineStop();
    renderware.EngineClose();
    renderware.EngineShutdown();
    return platform.ChangeDisplaySettings(0u, 0u);
}

std::uint32_t GameRenderSystem::EndTimePeriod(
    IGameRenderPlatform& platform) const {
    return platform.EndTimePeriod(state_.Read(0x18u));
}

void GameRenderSystem::ResizeRenderScreen(
    std::array<std::int32_t, 4> rectangle, const std::uint32_t camera,
    const std::array<std::uint32_t, 12>& config,
    ICameraViewRuntime& runtime) {
    if (camera != 0u) {
        runtime.AdjustCameraView(camera, rectangle,
                                 std::bit_cast<float>(config[5]), config[4]);
    }
}

}  // namespace pal4
