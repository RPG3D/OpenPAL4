#include "pal4/thunder_visual_path.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IThunderPathTransformRuntime {
    std::vector<std::string> calls;
    std::vector<pal4::ThunderVisualSegment> segments{
        {{1, 2, 3}, {4, 5, 6}},
        {{7, 8, 9}, {10, 11, 12}}};
    std::uint32_t FrameTransform(std::uint32_t frame) override {
        Check(frame == 0x800u, "output frame handle");
        calls.push_back("frame-transform");
        return 0x900u;
    }
    std::vector<pal4::ThunderVisualSegment> ReadSegments(
        std::uint32_t begin, std::uint32_t end) override {
        Check(begin == 0x1000u && end == 0x1030u,
              "native 24-byte pair range");
        calls.push_back("read");
        return segments;
    }
    pal4::ThunderVector3 TransformVector(
        pal4::ThunderVector3 value, std::uint32_t transform) override {
        Check(transform == 0x900u, "frame transform forwarded");
        calls.push_back("vector");
        return {value[0] * 2.0f, value[1] * 3.0f,
                value[2] * 4.0f};
    }
    void WriteSegments(
        pal4::ThunderObjectState& state,
        const std::vector<pal4::ThunderVisualSegment>& value) override {
        Check(state.words[47u] == 0x1000u &&
                  state.words[48u] == 0x1030u,
              "array pointer range remains intact");
        calls.push_back("write");
        segments = value;
    }
};

pal4::ThunderObjectState State() {
    pal4::ThunderObjectState state;
    state.words[0u] = 0xdeadbeefu;
    state.words[34u] = 0x800u;
    state.words[47u] = 0x1000u;
    state.words[48u] = 0x1030u;
    state.words[31u] = std::bit_cast<std::uint32_t>(10.0f);
    state.words[32u] = std::bit_cast<std::uint32_t>(20.0f);
    state.words[33u] = std::bit_cast<std::uint32_t>(30.0f);
    return state;
}

}  // namespace

int main() {
    auto state = State();
    Runtime runtime;
    state.words[47u] = 0u;
    pal4::ThunderVisualPath::TransformToWorld(state, runtime);
    Check(runtime.calls == std::vector<std::string>{"frame-transform"},
          "null array still queries frame but skips element work");
    runtime.calls.clear();
    state.words[47u] = 0x1000u;
    state.words[48u] = 0x1000u;
    pal4::ThunderVisualPath::TransformToWorld(state, runtime);
    Check(runtime.calls == std::vector<std::string>{"frame-transform"},
          "empty array skips element work");
    runtime.calls.clear();
    state.words[48u] = 0x1030u;
    pal4::ThunderVisualPath::TransformToWorld(state, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "frame-transform", "read", "vector", "vector",
              "vector", "vector", "write"},
          "both endpoints transformed in pair order");
    Check(runtime.segments[0].first ==
                  pal4::ThunderVector3{12.0f, 26.0f, 42.0f} &&
              runtime.segments[0].second ==
                  pal4::ThunderVector3{18.0f, 35.0f, 54.0f} &&
              runtime.segments[1].first ==
                  pal4::ThunderVector3{24.0f, 44.0f, 66.0f} &&
              runtime.segments[1].second ==
                  pal4::ThunderVector3{30.0f, 53.0f, 78.0f},
          "matrix vector result plus midpoint stored for both pairs");
    Check(state.words[0u] == 0xdeadbeefu,
          "outer Thunder header untouched");

    state = State();
    state.words[47u] = 0u;
    pal4::ThunderPathTransformRuntimeStub stub;
    pal4::ThunderVisualPath::TransformToWorld(state, stub);
}
