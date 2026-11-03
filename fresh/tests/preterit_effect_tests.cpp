#include "pal4/preterit_effect.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct RenderRuntime final : pal4::IEffectRenderRuntime {
    explicit RenderRuntime(std::vector<std::string>& trace) : events(trace) {}
    std::uint32_t DestroyTexture(std::uint32_t texture) override {
        destroyed.push_back(texture);
        events.emplace_back("destroy");
        return 1u;
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
              "preterit primary texture dimensions and format");
        events.emplace_back("create");
        return 0x9000u;
    }
    std::int32_t FloatToLong(float) override { return 0; }
    std::uint32_t DrawTexturedRectangle(
        std::uint32_t context, std::int32_t x, std::int32_t y,
        std::int32_t width, std::int32_t height, std::uint32_t texture,
        std::uint8_t red, std::uint8_t green, std::uint8_t blue,
        std::int32_t alpha) override {
        Check(context == 0x5000u && x == 5 && y == 7 &&
                  width == 800 && height == 600 &&
                  red == 255u && green == 255u && blue == 255u &&
                  alpha == 255, "preterit draw rectangle and white color");
        drawn.push_back(texture);
        events.emplace_back("draw");
        return static_cast<std::uint32_t>(0xa000u + drawn.size());
    }
    float Interpolate(float, float, float, std::uint32_t) override {
        return 0.0f;
    }
    std::vector<std::string>& events;
    std::vector<std::uint32_t> drawn;
    std::vector<std::uint32_t> destroyed;
};

struct PreteritRuntime final : pal4::IPreteritRuntime {
    explicit PreteritRuntime(std::vector<std::string>& trace) : events(trace) {}
    std::uint32_t FindTexture(std::uint32_t dictionary,
                               std::string_view path) override {
        Check(dictionary == 0x7000u, "frame texture dictionary");
        const std::string expected =
            "gamedata\\2D\\ScreenEffect\\Preterit\\preteritBK" +
            std::string(lookup_count < 10u ? "0" : "") +
            std::to_string(lookup_count) + ".png";
        Check(path == expected, "eleven ordered frame paths");
        events.emplace_back("find");
        return 0x1000u + static_cast<std::uint32_t>(lookup_count++);
    }
    std::int32_t Random() override {
        events.emplace_back("random");
        ++random_count;
        return random_value;
    }
    void SetGameState(std::uint32_t value) override {
        Check(value == 0x44u, "completion writes next game state");
        events.emplace_back("game state");
    }
    std::vector<std::string>& events;
    std::size_t lookup_count{};
    std::size_t random_count{};
    std::int32_t random_value{23};
};

void InitializeAndLoad() {
    std::vector<std::string> events;
    RenderRuntime render(events);
    PreteritRuntime preterit(events);
    pal4::PreteritEffectState state{};
    state.primary_texture = 0xdeadbeefu;
    state.frame_textures[10] = 0x1111u;
    state.active = 7u;
    Check(&pal4::PreteritEffect::Initialize(state, 800, 600) == &state &&
              state.primary_texture == 0u && state.frame_textures[10] == 0u &&
              state.selected_texture == 0u && state.active == 0u &&
              state.width == 800 && state.height == 600,
          "92-byte Preterit initializer clears state except dimensions");
    pal4::PreteritEffect::LoadTextures(state, render, preterit);
    Check(state.primary_texture == 0x9000u && preterit.lookup_count == 11u &&
              state.frame_textures.front() == 0x1000u &&
              state.frame_textures.back() == 0x100au &&
              events.size() == 24u && events[0] == "dictionary" &&
              events[1] == "create" && events[2] == "dictionary" &&
              events[3] == "find" && events.back() == "find",
          "loader creates primary then looks up all eleven frames");
    events.clear();
    pal4::PreteritEffect::LoadTextures(state, render, preterit);
    Check(events.empty() && preterit.lookup_count == 11u,
          "non-null primary gates repeated loading");
}

void TextureArrayCleanup() {
    std::vector<std::string> events;
    RenderRuntime render(events);
    pal4::PreteritEffectState state{};
    state.primary_texture = 0x101u;
    state.frame_textures[4] = 0x505u;
    state.frame_textures[9] = 0xa0au;
    state.frame_textures[10] = 0xb0bu;
    pal4::EffectTextureArray::Cleanup(state, render);
    Check(render.destroyed == std::vector<std::uint32_t>({0x101u, 0x505u,
                                                          0xa0au}) &&
              state.primary_texture == 0u && state.frame_textures[4] == 0u &&
              state.frame_textures[9] == 0u &&
              state.frame_textures[10] == 0xb0bu,
          "cleanup loops over primary and first ten frames only");
    render.destroyed.clear();
    pal4::EffectTextureArray::Cleanup(state, render);
    Check(render.destroyed.empty(), "second cleanup has no releases");
}

void DrawAndAdvance() {
    std::vector<std::string> events;
    RenderRuntime render(events);
    PreteritRuntime preterit(events);
    pal4::PreteritEffectState state{};
    state.primary_texture = 0x9000u;
    state.selected_texture = 0x1002u;
    state.x = 5;
    state.y = 7;
    state.width = 800;
    state.height = 600;
    Check(pal4::PreteritEffect::Draw(state, 0x5000u, render) == 0u &&
              events.empty(), "inactive draw returns active value");
    state.active = 3u;
    Check(pal4::PreteritEffect::Draw(state, 0u, render) == 3u &&
              events.empty(), "null context returns active value");
    Check(pal4::PreteritEffect::Draw(state, 0x5000u, render) == 0xa002u &&
              render.drawn == std::vector<std::uint32_t>({0x1002u, 0x9000u}),
          "draws selected frame then primary texture");
    events.clear();
    state.frame_textures[1] = 0x1001u;
    state.frame_interval = 1.0f;
    state.duration = 2.0f;
    state.next_game_state = 0x44u;
    Check(pal4::PreteritEffect::Advance(state, 0.5f, preterit) == 0x100u &&
              state.frame_elapsed == 0.5f && state.elapsed == 0.5f &&
              state.selected_texture == 0x1002u && events.empty(),
          "animation below frame interval keeps selected frame");
    Check(pal4::PreteritEffect::Advance(state, 0.6f, preterit) == 0x100u &&
              state.frame_elapsed == 0.0f && state.elapsed == 1.1f &&
              state.selected_texture == 0x1001u &&
              preterit.random_count == 1u &&
              events == std::vector<std::string>({"random"}),
          "frame threshold resets timer and chooses rand modulo eleven");
    events.clear();
    Check(pal4::PreteritEffect::Advance(state, 1.0f, preterit) ==
              std::bit_cast<std::uint32_t>(2.0f) &&
              state.elapsed == 2.0f && state.active == 0u &&
              events == std::vector<std::string>({"random", "game state"}),
          "completion clamps elapsed and writes game-state field");
    events.clear();
    Check(pal4::PreteritEffect::Advance(state, 1.0f, preterit) == 0u &&
              events.empty(), "inactive animation has no work");
    state.active = 1u;
    state.elapsed = 0.0f;
    state.duration = std::numeric_limits<float>::quiet_NaN();
    state.frame_interval = 10.0f;
    Check(pal4::PreteritEffect::Advance(state, 0.1f, preterit) == 0x100u &&
              state.active == 1u && events.empty(),
          "unordered duration comparison leaves animation active");
}

}  // namespace

int main() {
    InitializeAndLoad();
    TextureArrayCleanup();
    DrawAndAdvance();
    std::cout << "Preterit effect tests passed\n";
}
