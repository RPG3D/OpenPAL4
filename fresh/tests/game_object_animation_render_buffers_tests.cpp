#include "pal4/game_object_embedded_animation.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::IGameObjectAnimationRenderBufferRuntime {
public:
    std::vector<std::string> calls;
    std::int32_t count = 3;
    std::vector<std::uint32_t> allocations = {0x5000u, 0x6000u, 0x7000u};
    std::size_t next_allocation = 0;

    std::int32_t FrameCount(std::uint32_t source) override {
        Check(source == 0x1000u, "reads source count at +4");
        calls.emplace_back("count");
        return count;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("alloc:" + std::to_string(bytes));
        Check(next_allocation < allocations.size(), "three allocations only");
        return allocations[next_allocation++];
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
    void WriteHalf(std::uint32_t address, std::uint16_t value) override {
        calls.emplace_back("half:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("word:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
};

void GatesAndReplacement() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState state;
    state.words[28] = 0x100u;
    Check(!pal4::GameObjectEmbeddedAnimation::InitializeRenderBuffers(
              state, 0, runtime) && state.words[28] == 0x100u &&
              runtime.calls.empty(), "null source preserves prior buffers");
    state.words[29] = 0x200u;
    state.words[30] = 0x300u;
    runtime.count = 0;
    Check(!pal4::GameObjectEmbeddedAnimation::InitializeRenderBuffers(
              state, 0x1000u, runtime) &&
              state.words[28] == 0 && state.words[29] == 0 &&
              state.words[30] == 0 && state.words[25] == 0 &&
              runtime.calls == std::vector<std::string>{
                  "free:256", "free:512", "free:768", "count"},
          "old buffers free in slot order before nonpositive count gate");
    runtime.calls.clear();
    runtime.count = -2;
    Check(!pal4::GameObjectEmbeddedAnimation::InitializeRenderBuffers(
              state, 0x1000u, runtime) &&
              state.words[25] == static_cast<std::uint32_t>(-2) &&
              runtime.calls == std::vector<std::string>{"count"},
          "negative count is stored before rejection");
}

void LayoutAndAllocationFailure() {
    Runtime runtime;
    pal4::GameObjectEmbeddedAnimationState state;
    Check(pal4::GameObjectEmbeddedAnimation::InitializeRenderBuffers(
              state, 0x1000u, runtime) &&
              state.words[25] == 3 && state.words[28] == 0x7000u &&
              state.words[29] == 0x5000u && state.words[30] == 0x6000u &&
              runtime.calls == std::vector<std::string>{
                  "count", "alloc:108", "alloc:12", "alloc:4",
                  "half:24576:0", "half:24578:1", "word:20504:4294967295",
                  "half:24580:0", "half:24582:2", "word:20540:4294967295",
                  "half:24584:0", "half:24586:3", "word:20576:4294967295"},
          "36-byte vertex and four-byte index strides match source");

    Runtime failed;
    failed.allocations[1] = 0;
    Check(!pal4::GameObjectEmbeddedAnimation::InitializeRenderBuffers(
              state, 0x1000u, failed) &&
              state.words[28] == 0 && state.words[29] == 0 &&
              state.words[30] == 0 &&
              failed.calls == std::vector<std::string>{
                  "free:28672", "free:20480", "free:24576", "count",
                  "alloc:108", "alloc:12", "alloc:4",
                  "free:28672", "free:20480"},
          "allocation failure releases successful new buffers");

    Runtime one;
    one.count = 1;
    one.allocations[2] = 0;
    Check(pal4::GameObjectEmbeddedAnimation::InitializeRenderBuffers(
              state, 0x1000u, one) &&
              state.words[28] == 0 &&
              one.calls[3] == "alloc:0",
          "one-frame zero-sized half-index allocation may return null");
}

}  // namespace

int main() {
    GatesAndReplacement();
    LayoutAndAllocationFailure();
    std::cout << "game_object_animation_render_buffers_tests passed\n";
}
