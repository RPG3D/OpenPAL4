#pragma once

#include "pal4/effect_render_parameters.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PreteritEffectState {
    std::uint32_t primary_texture{};
    std::array<std::uint32_t, 11> frame_textures{};
    std::uint32_t selected_texture{};
    float frame_interval{};
    float frame_elapsed{};
    float duration{};
    float elapsed{};
    std::uint32_t next_game_state{};
    std::uint32_t active{};
    std::int32_t x{};
    std::int32_t y{};
    std::int32_t width{};
    std::int32_t height{};
};
static_assert(sizeof(PreteritEffectState) == 92u);
static_assert(offsetof(PreteritEffectState, selected_texture) == 48u);
static_assert(offsetof(PreteritEffectState, active) == 72u);
static_assert(offsetof(PreteritEffectState, width) == 84u);

class IPreteritRuntime {
public:
    virtual ~IPreteritRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindTexture(
        std::uint32_t dictionary, std::string_view path) = 0;
    [[nodiscard]] virtual std::int32_t Random() = 0;
    virtual void SetGameState(std::uint32_t value) = 0;
};

class PreteritRuntimeStub final : public IPreteritRuntime {
public:
    [[nodiscard]] std::uint32_t FindTexture(
        std::uint32_t dictionary, std::string_view path) override;
    [[nodiscard]] std::int32_t Random() override;
    void SetGameState(std::uint32_t value) override;
};

class EffectTextureArray final {
public:
    static void Cleanup(PreteritEffectState& state,
                        IEffectRenderRuntime& runtime);
};

class PreteritEffect final {
public:
    [[nodiscard]] static PreteritEffectState& Initialize(
        PreteritEffectState& state, std::int32_t screen_width,
        std::int32_t screen_height);
    static void LoadTextures(
        PreteritEffectState& state, IEffectRenderRuntime& render_runtime,
        IPreteritRuntime& preterit_runtime);
    [[nodiscard]] static std::uint32_t Draw(
        const PreteritEffectState& state, std::uint32_t context,
        IEffectRenderRuntime& runtime);
    [[nodiscard]] static std::uint32_t Advance(
        PreteritEffectState& state, float delta,
        IPreteritRuntime& runtime);
};

}  // namespace pal4
