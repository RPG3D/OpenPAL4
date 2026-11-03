#include "pal4/preterit_effect.h"

#include <bit>
#include <cstdio>

namespace pal4 {

std::uint32_t PreteritRuntimeStub::FindTexture(
    std::uint32_t, std::string_view) { return 0u; }
std::int32_t PreteritRuntimeStub::Random() { return 0; }
void PreteritRuntimeStub::SetGameState(std::uint32_t) {}

void EffectTextureArray::Cleanup(
    PreteritEffectState& state,
    IEffectRenderRuntime& runtime) {
    auto release = [&](std::uint32_t& texture) {
        if (texture != 0u) {
            static_cast<void>(runtime.DestroyTexture(texture));
            texture = 0u;
        }
    };
    release(state.primary_texture);
    for (std::size_t index = 0; index < 10u; ++index) {
        release(state.frame_textures[index]);
    }
}

PreteritEffectState& PreteritEffect::Initialize(
    PreteritEffectState& state, std::int32_t screen_width,
    std::int32_t screen_height) {
    state = {};
    state.width = screen_width;
    state.height = screen_height;
    return state;
}

void PreteritEffect::LoadTextures(
    PreteritEffectState& state, IEffectRenderRuntime& render_runtime,
    IPreteritRuntime& preterit_runtime) {
    if (state.primary_texture != 0u) return;
    const auto dictionary = render_runtime.TextureDictionary();
    state.primary_texture = render_runtime.CreateTexture(
        dictionary, 32u, 32u, 1280u, 0xffffffffu);
    for (std::size_t index = 0; index < state.frame_textures.size(); ++index) {
        char path[256]{};
        std::snprintf(path, sizeof(path),
                      "gamedata\\2D\\ScreenEffect\\Preterit\\preteritBK%02d.png",
                      static_cast<int>(index));
        state.frame_textures[index] = preterit_runtime.FindTexture(
            render_runtime.TextureDictionary(), path);
    }
}

std::uint32_t PreteritEffect::Draw(
    const PreteritEffectState& state, std::uint32_t context,
    IEffectRenderRuntime& runtime) {
    if (state.active == 0u || context == 0u) return state.active;
    static_cast<void>(runtime.DrawTexturedRectangle(
        context, state.x, state.y, state.width, state.height,
        state.selected_texture, 255u, 255u, 255u, 255));
    return runtime.DrawTexturedRectangle(
        context, state.x, state.y, state.width, state.height,
        state.primary_texture, 255u, 255u, 255u, 255);
}

std::uint32_t PreteritEffect::Advance(
    PreteritEffectState& state, float delta,
    IPreteritRuntime& runtime) {
    if (state.active == 0u) return 0u;
    state.frame_elapsed += delta;
    if (state.frame_elapsed >= state.frame_interval) {
        state.frame_elapsed = 0.0f;
        const auto index = runtime.Random() % 11;
        state.selected_texture = state.frame_textures[static_cast<std::size_t>(index)];
    }
    state.elapsed += delta;
    if (!(state.elapsed >= state.duration)) return 0x100u;
    state.elapsed = state.duration;
    state.active = 0u;
    runtime.SetGameState(state.next_game_state);
    return std::bit_cast<std::uint32_t>(state.duration);
}

}  // namespace pal4
