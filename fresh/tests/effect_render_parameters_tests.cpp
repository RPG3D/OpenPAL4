#include "pal4/effect_render_parameters.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectRenderRuntime {
    std::uint32_t DestroyTexture(std::uint32_t texture) override {
        Check(texture == 0x9000u, "destroy observed texture");
        events.emplace_back("destroy");
        return 0x1234u;
    }
    std::uint32_t TextureDictionary() override {
        events.emplace_back("dictionary");
        return 0x7000u;
    }
    std::uint32_t CreateTexture(
        std::uint32_t dictionary, std::uint32_t width,
        std::uint32_t height, std::uint32_t format,
        std::uint32_t color) override {
        Check(dictionary == 0x7000u && width == 32u && height == 32u &&
                  format == 1280u && color == 0xffffffffu,
              "create observed texture parameters");
        events.emplace_back("create");
        return 0x9000u;
    }
    std::int32_t FloatToLong(float value) override {
        Check(value == 126.75f, "alpha is converted by CRT helper");
        events.emplace_back("ftol");
        return 126;
    }
    std::uint32_t DrawTexturedRectangle(
        std::uint32_t context, std::int32_t x, std::int32_t y,
        std::int32_t width, std::int32_t height,
        std::uint32_t texture, std::uint8_t red, std::uint8_t green,
        std::uint8_t blue, std::int32_t alpha) override {
        Check(context == 0x5000u && x == 5 && y == 7 &&
                  width == 800 && height == 600 && texture == 0x9000u &&
                  red == 12u && green == 23u && blue == 34u && alpha == 126,
              "draw forwards rectangle, texture and colors");
        events.emplace_back("draw");
        return 0x5678u;
    }
    float Interpolate(float start, float end, float progress,
                      std::uint32_t mode) override {
        Check(start == 255.0f && end == 15.0f && mode == 0u,
              "fade interpolation endpoints and mode");
        events.emplace_back("interpolate");
        last_progress = progress;
        return 255.0f + (15.0f - 255.0f) * progress;
    }
    std::vector<std::string> events;
    float last_progress{};
};

void InitializePreservesUnwrittenColor() {
    pal4::EffectRenderParameters state{};
    state.texture = 0x9000u;
    state.red = 12u;
    state.green = 23u;
    state.blue = 34u;
    state.padding = 0xabu;
    state.x = 5;
    state.y = 7;
    state.active = 9u;
    Check(&pal4::EffectRenderControl::Initialize(state, 800, 600) == &state &&
              state.texture == 0u && state.x == 0 && state.y == 0 &&
              state.width == 800 && state.height == 600 &&
              state.red == 12u && state.green == 23u &&
              state.blue == 34u && state.padding == 0xabu &&
              state.elapsed == 0.0f && state.alpha_start == 255.0f &&
              state.phase == 1u && state.active == 0u,
          "initializer writes observed fields and leaves color bytes alone");
}

void ResetAndTextureLifetime() {
    Runtime runtime;
    pal4::EffectRenderParameters state{};
    state.texture = 0x9000u;
    state.padding = 0xadu;
    state.x = 5;
    state.y = 7;
    state.width = 800;
    state.height = 600;
    state.red = 12u;
    state.green = 23u;
    state.blue = 34u;
    state.active = 1u;
    pal4::EffectRenderControl::ResetAnimation(state);
    Check(state.texture == 0x9000u && state.x == 5 && state.y == 7 &&
              state.width == 800 && state.height == 600 &&
              state.padding == 0xadu && state.red == 0u &&
              state.green == 0u && state.blue == 0u &&
              state.alpha_start == 255.0f && state.phase == 1u &&
              state.active == 0u, "reset preserves texture and geometry");
    pal4::EffectRenderControl::InitializeTexture(state, runtime);
    Check(runtime.events.empty(), "non-null texture skips creation");
    Check(pal4::EffectRenderControl::DestroyTexture(state, runtime) ==
              0x1234u && state.texture == 0u &&
              runtime.events == std::vector<std::string>({"destroy"}),
          "destroy returns RenderWare result and clears pointer");
    runtime.events.clear();
    Check(pal4::EffectRenderControl::DestroyTexture(state, runtime) == 0u &&
              runtime.events.empty(), "null texture destroy is inert");
    pal4::EffectRenderControl::InitializeTexture(state, runtime);
    Check(state.texture == 0x9000u &&
              runtime.events == std::vector<std::string>({"dictionary", "create"}),
          "lazy white 32x32 texture creation");
}

void DrawAndFade() {
    Runtime runtime;
    pal4::EffectRenderParameters state{};
    state.texture = 0x9000u;
    state.x = 5;
    state.y = 7;
    state.width = 800;
    state.height = 600;
    state.red = 12u;
    state.green = 23u;
    state.blue = 34u;
    state.alpha = 126.75f;
    Check(pal4::EffectRenderControl::Draw(state, 0x5000u, runtime) == 0u &&
              runtime.events.empty(), "inactive draw returns active flag");
    state.active = 3u;
    Check(pal4::EffectRenderControl::Draw(state, 0u, runtime) == 3u &&
              runtime.events.empty(), "null context returns active flag");
    Check(pal4::EffectRenderControl::Draw(state, 0x5000u, runtime) ==
              0x5678u &&
              runtime.events == std::vector<std::string>({"ftol", "draw"}),
          "draw forwards CRT-converted alpha and return");
    runtime.events.clear();
    state.phase = 1u;
    state.elapsed = 0.0f;
    state.duration = 2.0f;
    state.alpha_start = 255.0f;
    state.alpha_end = 15.0f;
    pal4::EffectRenderControl::AdvanceFade(state, 0.5f, runtime);
    Check(state.elapsed == 0.0f && runtime.events.empty(),
          "phase gate prevents fade update");
    state.phase = 0u;
    pal4::EffectRenderControl::AdvanceFade(state, 0.5f, runtime);
    Check(state.elapsed == 0.5f && runtime.last_progress == 0.25f &&
              state.alpha == 195.0f && state.active == 3u &&
              runtime.events == std::vector<std::string>({"interpolate"}),
          "fade interpolates while under duration");
    runtime.events.clear();
    state.alpha_on_finish = 200.0f;
    pal4::EffectRenderControl::AdvanceFade(state, 2.0f, runtime);
    Check(state.elapsed == 2.0f && state.alpha == 15.0f &&
              state.phase == 1u && state.active == 0u &&
              runtime.last_progress == 1.0f &&
              runtime.events == std::vector<std::string>({"interpolate"}),
          "completion clamps elapsed and still interpolates final alpha");
    state.active = 1u;
    state.phase = 0u;
    state.keep_active = 1u;
    state.elapsed = 0.0f;
    pal4::EffectRenderControl::AdvanceFade(state, 2.0f, runtime);
    Check(state.active == 1u && state.phase == 1u,
          "keep-active flag preserves active state after completion");
}

}  // namespace

int main() {
    InitializePreservesUnwrittenColor();
    ResetAndTextureLifetime();
    DrawAndFade();
    std::cout << "effect render parameter tests passed\n";
}
