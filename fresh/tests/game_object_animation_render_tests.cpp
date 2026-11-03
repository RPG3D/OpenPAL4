#include "pal4/game_object_animation_render.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::IGameObjectAnimationRenderRuntime {
public:
    std::vector<std::string> calls;
    std::vector<std::uint32_t> flags;
    bool transform_result = true;

    pal4::AnimationFramePosition FramePosition(
        std::uint32_t source, std::int32_t index) override {
        Check(source == 0x1000u && index >= 0 &&
                  static_cast<std::size_t>(index) < flags.size(),
              "frame position index follows source bounds");
        calls.emplace_back("pos:" + std::to_string(index));
        const auto base = static_cast<std::uint32_t>(index) * 10u;
        return {base + 1u, base + 2u, base + 3u};
    }
    std::uint32_t FrameFlags(
        std::uint32_t source, std::int32_t index) override {
        Check(source == 0x1000u && index >= 0 &&
                  static_cast<std::size_t>(index) < flags.size(),
              "frame flag index follows source bounds");
        calls.emplace_back("flag:" + std::to_string(index));
        return flags[static_cast<std::size_t>(index)];
    }
    void WriteVertexPosition(
        std::uint32_t vertices, std::uint32_t index,
        const pal4::AnimationFramePosition& position) override {
        Check(vertices == 0x5000u, "writes animation vertex buffer");
        calls.emplace_back("vertex:" + std::to_string(index) + ":" +
                           std::to_string(position[0]) + ":" +
                           std::to_string(position[1]) + ":" +
                           std::to_string(position[2]));
    }
    std::uint32_t SetRenderState(
        std::uint32_t state, std::uint32_t value) override {
        calls.emplace_back("state:" + std::to_string(state) + ":" +
                           std::to_string(value));
        return state == 6 && value == 1 ? 0x77u : 0;
    }
    bool Im3DTransform(std::uint32_t vertices, std::uint32_t count,
                       std::uint32_t matrix, std::uint32_t transform_flags) override {
        Check(vertices == 0x5000u && matrix == 0 &&
                  transform_flags == 26,
              "Im3D transform receives native arguments");
        calls.emplace_back("transform:" + std::to_string(count));
        return transform_result;
    }
    void RenderIndexedPrimitive(
        std::uint32_t primitive_type, std::uint32_t indices,
        std::uint32_t index_count) override {
        Check(primitive_type == 1 && indices == 0x6000u,
              "indexed primitive uses animation index buffer");
        calls.emplace_back("render:" + std::to_string(index_count));
    }
    void Im3DEnd() override { calls.emplace_back("end"); }
};

pal4::GameObjectEmbeddedAnimationState Ready(std::uint32_t frame_count) {
    pal4::GameObjectEmbeddedAnimationState state;
    state.words[25] = frame_count;
    state.words[29] = 0x5000u;
    state.words[30] = 0x6000u;
    return state;
}

void GatesAndDirectClosure() {
    Runtime runtime;
    runtime.flags = {1, 0};
    auto state = Ready(2);
    state.words[15] = 0x99u;
    Check(pal4::GameObjectAnimationRender::Process(
              state, 0x1000u, 0, runtime) == 0x99u &&
              runtime.calls.empty(), "active blend returns raw state word");
    state.words[15] = 0;
    Check(pal4::GameObjectAnimationRender::Process(
              state, 0, 0, runtime) == 0 && runtime.calls.empty(),
          "null source returns zero");
    state.words[29] = 0;
    Check(pal4::GameObjectAnimationRender::Process(
              state, 0x1000u, 0, runtime) == 0 && runtime.calls.empty(),
          "missing vertex buffer is rejected before native calls");
    state.words[29] = 0x5000u;
    Check(pal4::GameObjectAnimationRender::Process(
              state, 0x1000u, 2, runtime) == 0 && runtime.calls.empty(),
          "out-of-range frame index is rejected");

    Check(pal4::GameObjectAnimationRender::Process(
              state, 0x1000u, 0, runtime) == 0x77u &&
              state.words[27] == 2 &&
              runtime.calls == std::vector<std::string>{
                  "pos:0", "vertex:0:1:2:3", "flag:0", "vertex:1:1:2:3",
                  "state:6:0", "state:1:0", "transform:2", "render:2",
                  "end", "state:6:1"},
          "closing frame duplicates position and restores render state");
}

void LinearAndNestedChains() {
    Runtime runtime;
    runtime.flags = {0, 0, 0};
    auto state = Ready(3);
    runtime.transform_result = false;
    Check(pal4::GameObjectAnimationRender::Process(
              state, 0x1000u, 0, runtime) == 0x77u &&
              state.words[27] == 2 &&
              runtime.calls == std::vector<std::string>{
                  "pos:0", "vertex:0:1:2:3", "flag:0", "flag:1",
                  "pos:1", "vertex:1:11:12:13", "state:6:0",
                  "state:1:0", "transform:2", "state:6:1"},
          "linear path stops at non-nested next frame and failed transform skips draw");

    runtime.calls.clear();
    runtime.flags = {0, 2, 2, 1, 1, 1};
    runtime.transform_result = true;
    state = Ready(6);
    Check(pal4::GameObjectAnimationRender::Process(
              state, 0x1000u, 0, runtime) == 0x77u &&
              state.words[27] == 3 &&
              runtime.calls == std::vector<std::string>{
                  "pos:0", "vertex:0:1:2:3", "flag:0", "flag:1",
                  "pos:1", "vertex:1:11:12:13", "flag:2", "flag:3",
                  "flag:4", "flag:5", "pos:5", "vertex:2:51:52:53",
                  "state:6:0", "state:1:0", "transform:3", "render:4",
                  "end", "state:6:1"},
          "nested flags skip inner positions and append final outer position");

    pal4::GameObjectAnimationRenderRuntimeStub stub;
    state = Ready(2);
    Check(pal4::GameObjectAnimationRender::Process(
              state, 0x1000u, 0, stub) == 0 && state.words[27] == 2,
          "RenderWare stub is linkable and completes control flow");
}

}  // namespace

int main() {
    GatesAndDirectClosure();
    LinearAndNestedChains();
    std::cout << "game_object_animation_render_tests passed\n";
}
