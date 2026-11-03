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

void Byte(pal4::ThunderObjectState& state,
          std::uint32_t offset, std::uint8_t value) {
    offset += 60u;
    const auto shift = 8u * (offset % 4u);
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

void Float(pal4::ThunderObjectState& state,
           std::uint32_t offset, float value) {
    state.words[(offset + 60u) / 4u] =
        std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IThunderVisualPathRuntime {
    std::vector<pal4::ThunderVisualSegment> segments;
    std::vector<std::string> calls;
    std::vector<std::vector<pal4::ThunderVector3>> controls;
    std::vector<float> samples;
    std::uint32_t random_calls{};
    bool fail_second{};
    std::uint32_t Random() override {
        calls.push_back("random");
        ++random_calls;
        return 0u;
    }
    void WriteSegments(
        pal4::ThunderObjectState& state,
        const std::vector<pal4::ThunderVisualSegment>& value) override {
        calls.push_back("write:" + std::to_string(value.size()));
        segments = value;
        Check(state.words[47u] == 0x1000u,
              "visual array begin uses owner +188");
        state.words[48u] = state.words[47u] +
                           static_cast<std::uint32_t>(value.size() * 24u);
    }
    std::uint32_t CreateSpline(
        const std::vector<pal4::ThunderVector3>& points,
        std::uint32_t type) override {
        Check(type == 1u, "native spline type");
        calls.push_back("create");
        controls.push_back(points);
        return fail_second && controls.size() == 2u
                   ? 0u
                   : (controls.size() == 1u ? 0x2000u : 0x3000u);
    }
    pal4::ThunderVector3 SplinePosition(
        std::uint32_t spline, std::uint32_t mode,
        float position) override {
        Check((spline == 0x2000u || spline == 0x3000u) && mode == 11u,
              "sample spline and native mode");
        calls.push_back("sample");
        samples.push_back(position);
        return {position, static_cast<float>(spline), 0.0f};
    }
    std::uint8_t DestroySpline(std::uint32_t spline) override {
        calls.push_back("destroy:" + std::to_string(spline));
        return 0x5au;
    }
};

pal4::ThunderObjectState State(float length) {
    pal4::ThunderObjectState state;
    state.words[0u] = 0xdeadbeefu;
    state.words[47u] = 0x1000u;
    state.words[48u] = 0x1100u;
    Float(state, 60u, length);
    Float(state, 100u, 0.0f);
    Float(state, 104u, 0.0f);
    Float(state, 108u, 0.2f);
    Float(state, 112u, 0.4f);
    Float(state, 116u, 0.5f);
    Float(state, 120u, 0.5f);
    return state;
}

void InactiveLength() {
    auto state = State(0.0f);
    Runtime runtime;
    Check(pal4::ThunderVisualPath::Build(state, runtime) == 0u &&
              runtime.segments.empty() &&
              state.words[48u] == state.words[47u] &&
              runtime.random_calls == 0u,
          "nonpositive length clears old segment array without random draws");
    Check(state.words[0u] == 0xdeadbeefu,
          "outer Thunder header untouched");
}

void StalledLength() {
    auto state = State(10.0f);
    Byte(state, 4u, 1u);
    Byte(state, 5u, 1u);
    Float(state, 116u, 1.0e-30f);
    Float(state, 120u, 1.0e-30f);
    Runtime runtime;
    Check(pal4::ThunderVisualPath::Build(state, runtime) == 0u &&
              runtime.segments.size() == 1u &&
              runtime.random_calls == 1u,
          "sub-ulp section length stops when stored remaining cannot advance");
}

void RandomWidthAndLockedEnd() {
    auto state = State(10.0f);
    Byte(state, 3u, 1u);
    Runtime runtime;
    Check(pal4::ThunderVisualPath::Build(state, runtime) == 0u,
          "non-sampled path returns sample flag");
    Check(runtime.segments.size() == 3u && runtime.random_calls == 10u,
          "two sections with width/range/step draws and locked final point");
    const auto& path = runtime.segments;
    Check(path[0].first[0] == -1.0f &&
              path[0].second[0] == 1.0f &&
              path[0].first[1] == -5.0f &&
              path[1].first[1] == 0.0f &&
              path[2].first[1] == 5.0f,
          "initial width and longitudinal section positions");
    Check(path[2].first[2] == 0.0f &&
              path[2].second[2] == 0.0f,
          "locked endpoint has no transverse random offset");
    Check(state.words[48u] == 0x1000u + 3u * 24u &&
              state.words[0u] == 0xdeadbeefu,
          "native array end and outer header");
}

void GradualWidthAndRange() {
    auto state = State(10.0f);
    Byte(state, 3u, 1u);
    Byte(state, 4u, 1u);
    Byte(state, 5u, 1u);
    Float(state, 104u, 0.2f);
    Runtime runtime;
    (void)pal4::ThunderVisualPath::Build(state, runtime);
    Check(runtime.segments.size() == 3u && runtime.random_calls == 6u,
          "fixed width/range draw section, middle offsets and disturbance");
    const auto& path = runtime.segments;
    Check(path[0].first[0] == -2.0f &&
              path[0].second[0] == 2.0f &&
              path[2].first[0] == -1.0f &&
              path[2].second[0] == 1.0f,
          "width tapers from max to min");
    Check(path[1].first[2] < 0.0f &&
              path[1].first[2] == path[1].second[2] &&
              path[2].first[2] == 0.0f,
          "range disturbance shifts both interior endpoints but not locked end");
}

void SplineSuccessAndFailure() {
    auto state = State(8.0f);
    Float(state, 116u, 0.25f);
    Float(state, 120u, 0.25f);
    Byte(state, 3u, 1u);
    Byte(state, 4u, 1u);
    Byte(state, 5u, 1u);
    Byte(state, 6u, 1u);
    state.words[17u] = 2u;
    Runtime runtime;
    Check(pal4::ThunderVisualPath::Build(state, runtime) == 0x5au,
          "second spline destruction result forwarded");
    Check(runtime.controls.size() == 2u &&
              runtime.controls[0].size() == 4u &&
              runtime.controls[1].size() == 4u &&
              runtime.segments.size() == 10u &&
              runtime.samples.size() == 20u,
          "spline controls exclude first pair and sampling doubles pair count");
    Check(runtime.samples.front() == 0.0f &&
              runtime.samples[runtime.samples.size() - 1u] == 1.0f &&
              runtime.segments.front().first[0] == 0.0f &&
              runtime.segments.front().second[1] == 12288.0f,
          "both splines sampled at normalized endpoints");
    Check(runtime.calls[runtime.calls.size() - 2u] == "destroy:8192" &&
              runtime.calls.back() == "destroy:12288" &&
              state.words[48u] == 0x1000u + 10u * 24u,
          "scratch splines released and sampled array stored");

    state = State(8.0f);
    Float(state, 116u, 0.25f);
    Float(state, 120u, 0.25f);
    Byte(state, 4u, 1u);
    Byte(state, 5u, 1u);
    Byte(state, 6u, 1u);
    state.words[17u] = 2u;
    Runtime failed;
    failed.fail_second = true;
    Check(pal4::ThunderVisualPath::Build(state, failed) == 0u &&
              failed.segments.size() == 5u &&
              failed.calls.back() == "destroy:8192",
          "failed second spline preserves original path and frees first");
}

}  // namespace

int main() {
    pal4::ThunderVisualSegment source[2]{{{1.0f, 2.0f, 3.0f},
                                           {4.0f, 5.0f, 6.0f}},
                                          {{7.0f, 8.0f, 9.0f},
                                           {10.0f, 11.0f, 12.0f}}};
    pal4::ThunderVisualSegment destination[3]{};
    Check(pal4::ThunderSegmentRecords::Copy(source + 2, source + 2,
                                              destination) == destination &&
              destination[0].first[0] == 0.0f,
          "empty 24-byte range returns unchanged destination");
    Check(pal4::ThunderSegmentRecords::Copy(source, source + 2,
                                              destination + 1) == destination + 3 &&
              destination[0].first[0] == 0.0f &&
              destination[1].first == source[0].first &&
              destination[1].second == source[0].second &&
              destination[2].first == source[1].first &&
              destination[2].second == source[1].second,
          "two complete 24-byte records copied in source order");
    InactiveLength();
    StalledLength();
    RandomWidthAndLockedEnd();
    GradualWidthAndRange();
    SplineSuccessAndFailure();
    auto state = State(0.0f);
    pal4::ThunderVisualPathRuntimeStub stub;
    (void)pal4::ThunderVisualPath::Build(state, stub);
}
