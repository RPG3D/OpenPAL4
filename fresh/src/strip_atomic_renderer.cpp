#include "pal4/strip_atomic_renderer.h"

namespace pal4 {

std::uint32_t StripAtomicRenderer::RenderBasic(
    std::uint32_t atomic, IStripAtomicRenderRuntime& runtime) {
    std::uint32_t old_state_7{};
    std::uint32_t old_state_14{};
    std::uint32_t old_device_20{};
    runtime.GetRenderState(7u, old_state_7);
    runtime.GetRenderState(14u, old_state_14);
    runtime.SetRenderState(7u, 1u);
    runtime.SetRenderState(14u, 0u);
    runtime.GetDeviceRenderState(20u, old_device_20);
    runtime.SetDeviceRenderState(20u, 1u);
    const auto result = runtime.RenderOriginalAtomic(atomic);
    runtime.SetDeviceRenderState(20u, old_device_20);
    runtime.SetRenderState(7u, old_state_7);
    runtime.SetRenderState(14u, old_state_14);
    return result;
}

std::uint32_t StripAtomicRenderer::RenderBlended(
    std::uint32_t atomic, IStripAtomicRenderRuntime& runtime) {
    std::uint32_t old_state_27{};
    std::uint32_t old_state_19{};
    std::uint32_t old_state_20{};
    std::uint32_t old_state_7{};
    std::uint32_t old_state_14{};
    std::uint32_t old_device_20{};
    runtime.GetRenderState(27u, old_state_27);
    runtime.GetRenderState(19u, old_state_19);
    runtime.GetRenderState(20u, old_state_20);
    runtime.SetRenderState(27u, 1u);
    runtime.SetRenderState(19u, 5u);
    runtime.SetRenderState(20u, 2u);
    runtime.GetRenderState(7u, old_state_7);
    runtime.GetRenderState(14u, old_state_14);
    runtime.SetRenderState(7u, 1u);
    runtime.SetRenderState(14u, 0u);
    runtime.GetDeviceRenderState(20u, old_device_20);
    runtime.SetDeviceRenderState(20u, 1u);
    const auto result = runtime.RenderOriginalAtomic(atomic);
    runtime.SetDeviceRenderState(20u, old_device_20);
    runtime.SetRenderState(7u, old_state_7);
    runtime.SetRenderState(14u, old_state_14);
    runtime.SetRenderState(27u, old_state_27);
    runtime.SetRenderState(19u, old_state_19);
    runtime.SetRenderState(20u, old_state_20);
    return result;
}

}  // namespace pal4
