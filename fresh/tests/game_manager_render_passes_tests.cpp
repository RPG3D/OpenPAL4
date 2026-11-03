#include "pal4/game_manager_render_passes.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameManagerRenderPassRuntime {
public:
    std::int32_t PassCount(const std::uint32_t handler,
                           const std::uint32_t frame_state) override {
        Check(handler == 0x6000u && frame_state == 0x7000u,
              "count receives handler and frame state");
        events.push_back("count");
        return count;
    }
    void SelectPass(const std::uint32_t handler,
                    const std::int32_t index) override {
        Check(handler == 0x6000u && index >= 0 && index < count,
              "pass selection receives handler and index");
        selected_index = index;
        events.push_back("select " + std::to_string(index));
    }
    std::uint8_t RenderPass(
        const std::uint32_t handler, const std::uint32_t argument,
        const pal4::ScreenEffectDispatcherState& state) override {
        Check(handler == 0x6000u && argument == 0x1234u &&
                  state.frame_state == 0x7000u,
              "render receives handler, caller argument and manager");
        events.push_back("render " + std::to_string(selected_index));
        return selected_index == 0 ? first_pass_result : 1u;
    }
    void FinishPass(const std::uint32_t handler) override {
        Check(handler == 0x6000u, "finish targets handler");
        events.push_back("finish " + std::to_string(selected_index));
    }
    void EndPasses(const std::uint32_t handler) override {
        Check(handler == 0x6000u, "end targets handler");
        events.push_back("end");
    }
    pal4::GameRenderDimensions ScreenDimensions() override {
        events.push_back("dimensions");
        return {640, 480};
    }
    std::uint32_t FrameTexture(const std::uint32_t frame_state) override {
        Check(frame_state == 0x7000u,
              "quad texture is read from frame-state pointer +4");
        events.push_back("texture");
        return 0x8000u;
    }
    void DrawQuad(const std::uint32_t texture, const float x,
                  const float y, const float width, const float height,
                  const std::int32_t color) override {
        Check(texture == 0x8000u && x == 0.0f && y == 0.0f &&
                  width == 640.0f && height == 480.0f && color == -1,
              "fallback quad uses full window rectangle and white color");
        events.push_back("draw");
    }

    std::int32_t count{2};
    std::int32_t selected_index{-1};
    std::uint8_t first_pass_result{};
    std::vector<std::string> events;
};

pal4::ScreenEffectDispatcherState InitialState() {
    pal4::ScreenEffectDispatcherState state{};
    state.enabled = 1u;
    state.active = 1;
    state.current_key = 3;
    state.current_handler = 0x6000u;
    state.frame_state = 0x7000u;
    return state;
}

void DisabledAndDirectFallback() {
    auto state = InitialState();
    Runtime runtime;
    state.enabled = 0u;
    pal4::GameManagerRenderPasses::Render(state, 0x1234u, runtime);
    Check(runtime.events.empty(), "disabled manager does not render");
    state.enabled = 1u;
    for (const auto key : {0, 16, 3}) {
        runtime.events.clear();
        state.current_key = key;
        state.active = key == 3 ? 0 : 1;
        pal4::GameManagerRenderPasses::Render(state, 0x1234u, runtime);
        Check(runtime.events == std::vector<std::string>({
                  "dimensions", "texture", "draw"}),
              "inactive, zero-key and key-16 cases draw one fallback quad");
    }
}

void NonpositiveCountStillEndsPasses() {
    auto state = InitialState();
    Runtime runtime;
    for (const auto count : {0, -2}) {
        runtime.count = count;
        runtime.events.clear();
        pal4::GameManagerRenderPasses::Render(state, 0x1234u, runtime);
        Check(runtime.events == std::vector<std::string>({
                  "count", "end", "dimensions", "texture", "draw"}),
              "nonpositive pass count ends handler and draws fallback once");
    }
}

void PerPassFallbackAndFinishOrder() {
    const auto state = InitialState();
    Runtime runtime;
    pal4::GameManagerRenderPasses::Render(state, 0x1234u, runtime);
    Check(runtime.events == std::vector<std::string>({
              "count", "select 0", "render 0", "dimensions", "texture",
              "draw", "finish 0", "select 1", "render 1", "finish 1", "end"}),
          "failed pass draws before finish and positive count suppresses final fallback");
    runtime.first_pass_result = 1u;
    runtime.events.clear();
    pal4::GameManagerRenderPasses::Render(state, 0x1234u, runtime);
    Check(runtime.events == std::vector<std::string>({
              "count", "select 0", "render 0", "finish 0",
              "select 1", "render 1", "finish 1", "end"}),
          "successful passes skip quad drawing");
    pal4::GameManagerRenderPassRuntimeStub stub;
    pal4::GameManagerRenderPasses::Render(state, 0x1234u, stub);
}

}  // namespace

int main() {
    DisabledAndDirectFallback();
    NonpositiveCountStillEndsPasses();
    PerPassFallbackAndFinishOrder();
    std::cout << "3 game manager render-pass test cases passed\n";
}
