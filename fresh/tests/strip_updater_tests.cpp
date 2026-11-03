#include "pal4/strip_updater.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

float Float(const pal4::StripObjectState& state, std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}

struct Runtime final : pal4::IStripUpdateRuntime {
    pal4::StripPlaybackSample sample{10, 2u, 3u, 4u};
    std::vector<std::string> events;
    std::array<std::uint8_t, 4> playback{};
    std::array<float, 4> uv{};
    std::uint32_t model = 0u;
    pal4::StripPlaybackSample ReadPlayback(
        pal4::StripObjectState&, std::uint32_t facet_offset) override {
        Check(facet_offset == 44u, "playback read facet offset");
        events.emplace_back("read");
        return sample;
    }
    std::int32_t FloatToInt(double value) override {
        events.emplace_back("ftol");
        return static_cast<std::int32_t>(value);
    }
    void WritePlayback(pal4::StripObjectState&,
                       std::uint32_t facet_offset,
                       const std::array<std::uint8_t, 4>& bytes) override {
        Check(facet_offset == 44u, "playback write facet offset");
        playback = bytes;
        events.emplace_back("write");
    }
    void UpdateGeometryVertices(
        std::uint32_t value, const std::array<float, 4>& vertices) override {
        model = value;
        uv = vertices;
        events.emplace_back("vertices");
    }
};

pal4::StripObjectState State() {
    pal4::StripObjectState state{};
    state.words[1u] = 0x00010001u;
    state.words[2u] = 0x01000000u;
    state.words[35u] = 1u;
    state.words[36u] = Bits(2.0f);
    state.words[38u] = 0x3000u;
    state.words[40u] = 1u;
    state.words[41u] = Bits(0.5f);
    state.words[42u] = 2u;
    state.words[43u] = 2u;
    state.words[44u] = 0u;
    state.words[45u] = 1u;
    state.words[46u] = Bits(0.5f);
    state.words[47u] = Bits(0.5f);
    return state;
}

void TestGatesAndPlayback() {
    auto state = State();
    Runtime runtime;
    state.words[1u] = 0u;
    pal4::StripUpdater::Advance(state, 0.5, runtime);
    Check(runtime.events.empty(), "inactive updater does no work");
    state.words[1u] = 1u;
    pal4::StripUpdater::Advance(state, 0.5, runtime);
    Check(runtime.events.empty(), "unstarted updater does no work");

    state = State();
    state.words[35u] = 0u;
    pal4::StripUpdater::Advance(state, 0.5, runtime);
    Check(runtime.events == std::vector<std::string>{
              "read", "ftol", "write"} &&
              runtime.playback == std::array<std::uint8_t, 4>{2, 3, 4, 9},
          "timed fade preserves RGB and decreases alpha");
    runtime.events.clear();
    runtime.sample.remaining = 0;
    pal4::StripUpdater::Advance(state, 5.0, runtime);
    Check(runtime.playback == std::array<std::uint8_t, 4>{2, 3, 4, 0} &&
              runtime.events == std::vector<std::string>{
                  "read", "ftol", "write"},
          "negative fade result clamps to zero");
}

void TestSequence() {
    auto state = State();
    Runtime runtime;
    state.words[2u] = 0u;
    pal4::StripUpdater::Advance(state, 0.25, runtime);
    Check(runtime.events.empty() && Float(state, 192u) == 0.25f &&
              state.words[44u] == 0u,
          "sub-frame time accumulates without advancing");
    pal4::StripUpdater::Advance(state, 0.25, runtime);
    Check(runtime.events == std::vector<std::string>{"vertices"} &&
              runtime.model == 0x3000u &&
              runtime.uv == std::array{0.5f, 0.5f, 1.0f, 1.0f} &&
              state.words[44u] == 1u && state.words[45u] == 1u &&
              Float(state, 192u) == 0.0f,
          "one frame updates UV and resets elapsed time");
    runtime.events.clear();
    pal4::StripUpdater::Advance(state, 2.0, runtime);
    Check(runtime.events == std::vector<std::string>{"vertices"} &&
              runtime.uv == std::array{0.0f, 0.0f, 0.5f, 0.5f} &&
              state.words[44u] == 0u && state.words[45u] == 0u,
          "large delta advances one frame and wraps column");
    runtime.events.clear();
    state.words[44u] = 1u;
    pal4::StripUpdater::Advance(state, 0.5, runtime);
    Check(state.words[44u] == 0u && state.words[45u] == 1u,
          "row wraps to final row after crossing zero");
    runtime.events.clear();
    state.words[48u] = Bits(std::numeric_limits<float>::quiet_NaN());
    pal4::StripUpdater::Advance(state, 0.5, runtime);
    Check(runtime.events.empty() && state.words[44u] == 0u,
          "unordered elapsed comparison skips frame advancement");
}

}  // namespace

int main() {
    TestGatesAndPlayback();
    TestSequence();
}
