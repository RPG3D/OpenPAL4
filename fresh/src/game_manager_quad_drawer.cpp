#include "pal4/game_manager_quad_drawer.h"

namespace pal4 {

std::uint32_t GameManagerQuadRuntimeStub::CurrentDevice() { return 0u; }
std::uint32_t GameManagerQuadRuntimeStub::TextureHandle(
    std::uint32_t /*texture_pointer*/) { return 0u; }
void GameManagerQuadRuntimeStub::BindTexture(
    std::uint32_t /*stage*/, std::uint32_t /*handle*/) {}
std::uint32_t GameManagerQuadRuntimeStub::SetDeviceState(
    std::uint32_t /*device*/, std::uint32_t /*state*/,
    std::uint32_t /*value*/) { return 0u; }
void GameManagerQuadRuntimeStub::SetFvf(
    std::uint32_t /*device*/, std::uint32_t /*fvf*/) {}
void GameManagerQuadRuntimeStub::DrawPrimitiveUp(
    std::uint32_t /*device*/, std::uint32_t /*primitive*/,
    std::uint32_t /*count*/,
    const std::array<CeguiRenderVertex, 4>& /*vertices*/,
    std::uint32_t /*stride*/) {}

std::uint32_t GameManagerQuadDrawer::Draw(
    const std::uint8_t enabled, const std::uint32_t incoming_eax,
    const std::uint32_t texture_pointer, const float x, const float y,
    const float width, const float height, const std::uint32_t color,
    IGameManagerQuadRuntime& runtime) {
    if (enabled == 0u) {
        return incoming_eax & 0xffffff00u;
    }
    const auto device = runtime.CurrentDevice();
    const auto right = x + width;
    const auto bottom = y + height;
    const std::array<CeguiRenderVertex, 4> vertices{{
        {x, y, 0.5f, 1.0f, color, 0.0f, 0.0f},
        {x, bottom, 0.5f, 1.0f, color, 0.0f, 1.0f},
        {right, y, 0.5f, 1.0f, color, 1.0f, 0.0f},
        {right, bottom, 0.5f, 1.0f, color, 1.0f, 1.0f},
    }};
    const auto texture = runtime.TextureHandle(texture_pointer);
    runtime.BindTexture(1u, texture);
    (void)runtime.SetDeviceState(device, 15u, 0u);
    (void)runtime.SetDeviceState(device, 27u, 1u);
    (void)runtime.SetDeviceState(device, 19u, 5u);
    (void)runtime.SetDeviceState(device, 20u, 6u);
    (void)runtime.SetDeviceState(device, 7u, 1u);
    (void)runtime.SetDeviceState(device, 14u, 0u);
    runtime.SetFvf(device, 324u);
    runtime.DrawPrimitiveUp(device, 5u, 2u, vertices, 28u);
    runtime.BindTexture(1u, 0u);
    (void)runtime.SetDeviceState(device, 7u, 1u);
    return runtime.SetDeviceState(device, 14u, 1u);
}

}  // namespace pal4
