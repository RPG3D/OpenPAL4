#include "pal4/effect_render_parameters.h"
#include "pal4/screen_fade_state.h"

#include <cmath>
#include <limits>

namespace pal4 {

std::uint32_t EffectRenderRuntimeStub::DestroyTexture(std::uint32_t) {
    return 0u;
}
std::uint32_t EffectRenderRuntimeStub::TextureDictionary() { return 0u; }
std::uint32_t EffectRenderRuntimeStub::CreateTexture(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t) { return 0u; }
std::int32_t EffectRenderRuntimeStub::FloatToLong(float value) {
    if (!std::isfinite(value) ||
        value >= 2147483648.0f || value < -2147483648.0f) {
        return std::numeric_limits<std::int32_t>::min();
    }
    return static_cast<std::int32_t>(value);
}
std::uint32_t EffectRenderRuntimeStub::DrawTexturedRectangle(
    std::uint32_t, std::int32_t, std::int32_t, std::int32_t,
    std::int32_t, std::uint32_t, std::uint8_t, std::uint8_t,
    std::uint8_t, std::int32_t) { return 0u; }
float EffectRenderRuntimeStub::Interpolate(
    float start, float end, float progress, std::uint32_t) {
    return start + (end - start) * progress;
}

EffectRenderParameters& EffectRenderControl::Initialize(
    EffectRenderParameters& state, std::int32_t screen_width,
    std::int32_t screen_height) {
    state.texture = 0u;
    state.x = 0;
    state.y = 0;
    state.width = screen_width;
    state.height = screen_height;
    state.elapsed = 0.0f;
    state.duration = 0.0f;
    state.alpha = 0.0f;
    state.alpha_start = 255.0f;
    state.alpha_end = 0.0f;
    state.alpha_on_finish = 0.0f;
    state.phase = 1u;
    state.keep_active = 0u;
    state.active = 0u;
    return state;
}

std::uint32_t EffectRenderControl::DestroyTexture(
    EffectRenderParameters& state, IEffectRenderRuntime& runtime) {
    if (state.texture == 0u) return 0u;
    const auto result = runtime.DestroyTexture(state.texture);
    state.texture = 0u;
    return result;
}

void EffectRenderControl::ResetAnimation(EffectRenderParameters& state) {
    state.alpha_start = 255.0f;
    state.elapsed = 0.0f;
    state.duration = 0.0f;
    state.red = 0u;
    state.green = 0u;
    state.blue = 0u;
    state.alpha = 0.0f;
    state.alpha_end = 0.0f;
    state.alpha_on_finish = 0.0f;
    state.phase = 1u;
    state.keep_active = 0u;
    state.active = 0u;
}

void EffectRenderControl::InitializeTexture(
    EffectRenderParameters& state, IEffectRenderRuntime& runtime) {
    if (state.texture != 0u) return;
    const auto dictionary = runtime.TextureDictionary();
    state.texture = runtime.CreateTexture(
        dictionary, 32u, 32u, 1280u, 0xffffffffu);
}

std::uint32_t EffectRenderControl::Draw(
    const EffectRenderParameters& state, std::uint32_t context,
    IEffectRenderRuntime& runtime) {
    if (state.active == 0u || context == 0u) return state.active;
    const auto alpha = runtime.FloatToLong(state.alpha);
    return runtime.DrawTexturedRectangle(
        context, state.x, state.y, state.width, state.height,
        state.texture, state.red, state.green, state.blue, alpha);
}

void EffectRenderControl::AdvanceFade(
    EffectRenderParameters& state, float delta,
    IEffectRenderRuntime& runtime) {
    AdvanceScreenFade(state,delta,[&](float start,float end,float progress) {
        return runtime.Interpolate(start,end,progress,0u);
    });
}

}  // namespace pal4
