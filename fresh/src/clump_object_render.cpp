#include "pal4/clump_object_render.h"

namespace pal4 {

std::uint8_t ClumpObject::RenderClump(
    const ClumpObjectState& state,
    [[maybe_unused]] const std::uint32_t unused_argument,
    IClumpObjectRenderRuntime& runtime) {
    const auto active = static_cast<std::uint8_t>(state.words[1u]);
    if (active == 0u)
        return active;
    const auto render_enabled = static_cast<std::uint8_t>(state.words[1u] >> 24u);
    if (render_enabled == 0u)
        return render_enabled;

    const auto clump = runtime.ReadWord(state.words[18u] + 20u);
    const auto state_27 = runtime.GetRenderState(27u);
    const auto state_19 = runtime.GetRenderState(19u);
    const auto state_20 = runtime.GetRenderState(20u);
    static_cast<void>(runtime.SetRenderState(27u, 1u));
    static_cast<void>(runtime.SetRenderState(19u, 5u));
    static_cast<void>(runtime.SetRenderState(20u, 6u));
    const auto state_7 = runtime.GetRenderState(7u);
    const auto state_14 = runtime.GetRenderState(14u);
    static_cast<void>(runtime.SetRenderState(7u, 1u));
    static_cast<void>(runtime.SetRenderState(14u, 1u));
    const auto stage_20 = runtime.GetTextureStageState(20u);
    runtime.SetTextureStageState(20u, 2u);
    runtime.Render(clump);
    runtime.SetTextureStageState(20u, stage_20);
    static_cast<void>(runtime.SetRenderState(7u, state_7));
    static_cast<void>(runtime.SetRenderState(14u, state_14));
    static_cast<void>(runtime.SetRenderState(27u, state_27));
    static_cast<void>(runtime.SetRenderState(19u, state_19));
    return runtime.SetRenderState(20u, state_20);
}

}  // namespace pal4
