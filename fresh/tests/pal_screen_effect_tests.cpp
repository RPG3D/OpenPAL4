#include "pal4/pal_screen_effect.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct ObjectRuntime final : pal4::IPalScreenEffectRuntime {
    std::uint32_t GetSingletonManager() override {
        events.emplace_back("manager");
        return 0x7000u;
    }
    std::uint32_t Register(
        std::uint32_t manager, std::string_view name,
        std::uint32_t id, std::uint32_t factory,
        std::uint32_t destroy) override {
        Check(manager == 0x7000u && name == "palScreenEffect" &&
                  id == 1018u && factory == 0x41f770u &&
                  destroy == 0x41f7d0u, "singleton registration contract");
        events.emplace_back("register");
        return 0x9000u;
    }
    pal4::PalScreenEffectState* Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x98u, "screen effect allocation size");
        events.emplace_back("allocate");
        return allocate_succeeds ? &storage : nullptr;
    }
    std::uint32_t Free(pal4::PalScreenEffectState* state) override {
        Check(state == &storage, "screen effect free address");
        events.emplace_back("free");
        return 0x1234u;
    }
    std::int32_t ScreenWidth() override {
        events.emplace_back("width");
        if (throw_width) throw std::runtime_error("dimension failed");
        return 800;
    }
    std::int32_t ScreenHeight() override {
        events.emplace_back("height");
        return 600;
    }
    void EmptyDestructor(const void* subobject) override {
        Check(subobject == &storage.preterit ||
                  subobject == &storage.render, "nullsub on observed subobject");
        events.push_back(subobject == &storage.preterit ?
                             "destruct preterit" : "destruct render");
    }
    void Log(std::string_view format, std::string_view source_file,
             std::uint32_t source_line) override {
        Check(source_file ==
                  "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\PAL4Game\\ScreenEffect\\palScreenEffect.cpp",
              "original source path in log call");
        if (source_line == 33u) {
            Check(format.find("palScreenEffect initializing begin") !=
                      std::string_view::npos, "initialization begin log");
            events.emplace_back("log begin");
        } else {
            Check(source_line == 39u &&
                      format.find("palScreenEffect initialize ok") !=
                          std::string_view::npos, "initialization complete log");
            events.emplace_back("log complete");
        }
    }
    pal4::PalScreenEffectState storage{};
    std::vector<std::string> events;
    bool allocate_succeeds{true};
    bool throw_width{};
};

struct RenderRuntime final : pal4::IEffectRenderRuntime {
    explicit RenderRuntime(std::vector<std::string>& trace) : events(trace) {}
    std::uint32_t DestroyTexture(std::uint32_t texture) override {
        destroyed.push_back(texture);
        events.emplace_back("destroy texture");
        return 0x7777u;
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
              "effect texture creation arguments");
        events.emplace_back("create texture");
        return ++created_count == 1u ? 0x1111u : 0x2222u;
    }
    std::int32_t FloatToLong(float value) override {
        Check(value == 128.0f, "render alpha conversion");
        events.emplace_back("ftol");
        return 128;
    }
    std::uint32_t DrawTexturedRectangle(
        std::uint32_t context, std::int32_t x, std::int32_t y,
        std::int32_t width, std::int32_t height, std::uint32_t texture,
        std::uint8_t red, std::uint8_t green, std::uint8_t blue,
        std::int32_t alpha) override {
        Check(context == 0x5000u && x == 0 && y == 0 &&
                  width == 800 && height == 600,
              "composite draw geometry");
        if (texture == 0x1111u) {
            Check(red == 1u && green == 2u && blue == 3u && alpha == 128,
                  "fade color and alpha");
        } else {
            Check(red == 255u && green == 255u && blue == 255u &&
                      alpha == 255, "preterit opaque white draw");
        }
        drawn.push_back(texture);
        events.emplace_back("draw");
        return static_cast<std::uint32_t>(0xa000u + drawn.size());
    }
    float Interpolate(float start, float end, float progress,
                      std::uint32_t mode) override {
        Check(start == 255.0f && end == 0.0f && mode == 0u,
              "fade interpolation arguments");
        events.emplace_back("interpolate");
        return start + (end - start) * progress;
    }
    std::vector<std::string>& events;
    std::vector<std::uint32_t> destroyed;
    std::vector<std::uint32_t> drawn;
    std::size_t created_count{};
};

struct PreteritRuntime final : pal4::IPreteritRuntime {
    explicit PreteritRuntime(std::vector<std::string>& trace) : events(trace) {}
    std::uint32_t FindTexture(std::uint32_t dictionary,
                               std::string_view path) override {
        Check(dictionary == 0x7000u &&
                  path.find("gamedata\\2D\\ScreenEffect\\Preterit\\preteritBK") == 0,
              "Preterit frame path and dictionary");
        events.emplace_back("find texture");
        return 0x3000u + static_cast<std::uint32_t>(find_count++);
    }
    std::int32_t Random() override {
        events.emplace_back("random");
        return 12;
    }
    void SetGameState(std::uint32_t value) override {
        Check(value == 0x44u, "composite animation next state");
        events.emplace_back("game state");
    }
    std::vector<std::string>& events;
    std::size_t find_count{};
};

void SingletonAndFactory() {
    ObjectRuntime runtime;
    pal4::PalScreenEffectAccessState access{0x80u, 0u};
    Check(pal4::PalScreenEffect::GetSingleton(access, runtime) == 0x9000u &&
              access.guard == 0x81u && access.instance == 0x9000u &&
              runtime.events == std::vector<std::string>({"manager", "register"}),
          "singleton registers once with observed ID and callbacks");
    runtime.events.clear();
    Check(pal4::PalScreenEffect::GetSingleton(access, runtime) == 0x9000u &&
              runtime.events.empty(), "singleton guard returns cached instance");
    runtime.allocate_succeeds = false;
    Check(pal4::PalScreenEffect::Create(runtime) == nullptr &&
              runtime.events == std::vector<std::string>({"allocate"}),
          "failed allocation does not construct object");
    runtime.events.clear();
    runtime.allocate_succeeds = true;
    runtime.storage.render.red = 9u;
    auto* state = pal4::PalScreenEffect::Create(runtime);
    Check(state == &runtime.storage && state->render.red == 9u &&
              state->render.width == 800 && state->render.height == 600 &&
              state->preterit.width == 800 && state->preterit.height == 600 &&
              runtime.events == std::vector<std::string>({"allocate", "width",
                  "height", "width", "height"}),
          "constructor builds 60-byte and 92-byte subobjects in order");
    runtime.events.clear();
    pal4::PalScreenEffect::Destruct(*state, runtime);
    Check(runtime.events == std::vector<std::string>({"destruct preterit",
                                               "destruct render"}),
          "destructor calls both empty subobject destructors");
    runtime.events.clear();
    Check(pal4::PalScreenEffect::Destroy(state, 0u, runtime) == 0x1234u &&
              runtime.events == std::vector<std::string>({"destruct preterit",
                  "destruct render", "free"}),
          "destroy forwards allocator result after destructor");
    runtime.events.clear();
    Check(pal4::PalScreenEffect::Destroy(nullptr, 0x7788u, runtime) ==
              0x7788u && runtime.events.empty(),
          "null destroy preserves incoming EAX");
    runtime.throw_width = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::PalScreenEffect::Create(runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<std::string>({"allocate",
            "width", "free"}), "factory frees allocation after ctor failure");
}

void ResourcesAndFade() {
    ObjectRuntime object;
    std::vector<std::string> events;
    RenderRuntime render(events);
    PreteritRuntime preterit(events);
    auto& state = object.storage;
    static_cast<void>(pal4::PalScreenEffect::Construct(state, object));
    state.render.texture = 0u;
    Check(pal4::PalScreenEffect::StartFade(
              state, 2.0f, 1u, 1u, 2u, 3u,
              255.0f, 0.0f, 200.0f) == 0u &&
              state.render.active == 0u,
          "fade setup requires a texture");
    pal4::PalScreenEffect::InitializeResources(
        state, object, render, preterit);
    Check(state.render.texture == 0x1111u &&
              state.preterit.primary_texture == 0x2222u &&
              preterit.find_count == 11u && object.events.size() == 6u &&
              object.events[4] == "log begin" &&
              object.events[5] == "log complete" &&
              events.size() == 26u && events[0] == "dictionary" &&
              events[1] == "create texture" &&
              events[2] == "dictionary" &&
              events[3] == "create texture",
          "resource initialization logs and loads both effect textures");
    events.clear();
    object.events.clear();
    pal4::PalScreenEffect::InitializeResources(
        state, object, render, preterit);
    Check(events.empty() && object.events ==
              std::vector<std::string>({"log begin", "log complete"}),
          "resource init still logs when textures already exist");
    state.render.alpha = 128.0f;
    Check(pal4::PalScreenEffect::StartFade(
              state, 2.0f, 1u, 1u, 2u, 3u,
              255.0f, 0.0f, 200.0f) == 1u &&
              state.render.duration == 2.0f && state.render.elapsed == 0.0f &&
              state.render.red == 1u && state.render.green == 2u &&
              state.render.blue == 3u && state.render.alpha == 128.0f &&
              state.render.alpha_start == 255.0f &&
              state.render.alpha_end == 0.0f &&
              state.render.alpha_on_finish == 200.0f &&
              state.render.phase == 0u && state.render.keep_active == 1u &&
              state.render.active == 1u,
          "fade setup writes only observed fields and returns hold flag");
    Check(pal4::PalScreenEffect::ResetParameters(state) == 0u &&
              state.render.texture == 0x1111u && state.render.active == 0u &&
              state.render.phase == 1u,
          "thunk resets effect parameters and returns zero");
    state.preterit.frame_textures[10] = 0xb0bu;
    events.clear();
    pal4::PalScreenEffect::ReleaseResources(state, render);
    Check(render.destroyed.front() == 0x1111u &&
              render.destroyed[1] == 0x2222u &&
              render.destroyed.size() == 12u &&
              state.render.texture == 0u &&
              state.preterit.primary_texture == 0u &&
              state.preterit.frame_textures[10] == 0xb0bu,
          "resource release skips last Preterit frame as original loop does");
}

void DrawAndAdvance() {
    ObjectRuntime object;
    std::vector<std::string> events;
    RenderRuntime render(events);
    PreteritRuntime preterit(events);
    auto& state = object.storage;
    static_cast<void>(pal4::PalScreenEffect::Construct(state, object));
    state.render.active = 1u;
    state.render.texture = 0x1111u;
    state.render.red = 1u;
    state.render.green = 2u;
    state.render.blue = 3u;
    state.render.alpha = 128.0f;
    state.preterit.active = 1u;
    state.preterit.primary_texture = 0x2222u;
    state.preterit.selected_texture = 0x3000u;
    Check(pal4::PalScreenEffect::Draw(state, 0x5000u, render) == 0xa003u &&
              render.drawn == std::vector<std::uint32_t>({0x1111u, 0x3000u,
                                                           0x2222u}) &&
              events == std::vector<std::string>({"ftol", "draw", "draw", "draw"}),
          "composite draw runs fade then both Preterit layers");
    events.clear();
    render.drawn.clear();
    state.render.active = 0u;
    state.preterit.active = 3u;
    Check(pal4::PalScreenEffect::Draw(state, 0u, render) == 3u &&
              events.empty(), "null draw context returns Preterit gate");
    state.render.active = 1u;
    state.render.phase = 0u;
    state.render.alpha_start = 255.0f;
    state.render.alpha_end = 0.0f;
    state.render.duration = 2.0f;
    state.preterit.frame_interval = 0.5f;
    state.preterit.duration = 3.0f;
    state.preterit.frame_textures[1] = 0x3001u;
    Check(pal4::PalScreenEffect::Advance(state, 0.5f, render, preterit) ==
              0x100u && state.render.elapsed == 0.5f &&
              state.preterit.elapsed == 0.5f &&
              state.preterit.selected_texture == 0x3001u &&
              events == std::vector<std::string>({"interpolate", "random"}),
          "composite update advances fade before Preterit animation");
}

}  // namespace

int main() {
    SingletonAndFactory();
    ResourcesAndFade();
    DrawAndAdvance();
    std::cout << "palScreenEffect tests passed\n";
}
