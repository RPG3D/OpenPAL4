#include "pal4/sequence_anim_updater.h"

#include <array>
#include <bit>
#include <cmath>
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
void PutFloat(pal4::SequenceAnimObjectState& state,
              std::uint32_t offset, float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}
float GetFloat(const pal4::SequenceAnimObjectState& state,
               std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}

struct Runtime final : pal4::ISequenceAnimUpdateRuntime {
    std::vector<std::string> events;
    std::uint32_t color = 0x64010203u;
    double ftol_input = 0.0;
    double update_elapsed = 0.0;
    std::array<std::uint32_t, 4> corners{};
    std::uint32_t ReadColor(
        const pal4::SequenceAnimObjectState& state) override {
        Check((state.words[2u] >> 24u) != 0u,
              "color query only in fading mode");
        events.emplace_back("read-color");
        return color;
    }
    std::int32_t Ftol(double value) override {
        events.emplace_back("ftol");
        ftol_input = value;
        return static_cast<std::int32_t>(value);
    }
    void SetColor(pal4::SequenceAnimObjectState&,
                  std::uint32_t rgba) override {
        events.emplace_back("set-color");
        color = rgba;
    }
    std::uint32_t Geometry(std::uint32_t resource) override {
        Check(resource == 0x3000u, "geometry resource");
        events.emplace_back("geometry");
        return 0x5000u;
    }
    void UpdateGeometry(std::uint32_t geometry,
                        const std::array<std::uint32_t, 4>& values) override {
        Check(geometry == 0x5000u, "geometry handle");
        events.emplace_back("update-geometry");
        corners = values;
    }
    std::uint32_t SystemResource(std::uint32_t system) override {
        Check(system == 0x6000u, "system pointer at owner +16");
        events.emplace_back("system-resource");
        return 0x7000u;
    }
    void UpdateState(std::uint32_t resource, double elapsed,
                     std::uint32_t system_resource) override {
        Check(resource == 0x3000u && system_resource == 0x7000u,
              "native state update arguments");
        events.emplace_back("update-state");
        update_elapsed = elapsed;
    }
};

pal4::SequenceAnimObjectState ReadyState() {
    pal4::SequenceAnimObjectState state{};
    state.words[1u] = 0x00010001u;
    state.words[4u] = 0x6000u;
    state.words[14u] = 4u;
    state.words[15u] = 2u;
    state.words[16u] = 1u;
    state.words[17u] = 1u;
    state.words[30u] = 0x3000u;
    PutFloat(state, 52u, 0.1f);
    PutFloat(state, 72u, 0.25f);
    PutFloat(state, 76u, 0.5f);
    PutFloat(state, 116u, 2.0f);
    return state;
}

void TestGates() {
    Runtime runtime;
    auto state = ReadyState();
    state.words[1u] = 0x00010000u;
    pal4::SequenceAnimUpdater::Update(state, 0.2, runtime);
    Check(runtime.events.empty(), "inactive owner gate");
    state.words[1u] = 0x00000001u;
    pal4::SequenceAnimUpdater::Update(state, 0.2, runtime);
    Check(runtime.events.empty(), "running byte gate");
    state.words[1u] = 0x00010001u;
    for (const auto interval : {0.0f, -1.0f,
                                std::numeric_limits<float>::quiet_NaN()}) {
        PutFloat(state, 52u, interval);
        pal4::SequenceAnimUpdater::Update(state, 0.2, runtime);
        Check(runtime.events.empty(), "positive interval gate");
    }
}

void TestFadeAndAdvance() {
    Runtime runtime;
    auto state = ReadyState();
    state.words[2u] = 0x01000000u;
    state.words[21u] = 11u;
    state.words[22u] = 12u;
    state.words[23u] = 13u;
    state.words[24u] = 14u;
    PutFloat(state, 80u, 0.05f);
    pal4::SequenceAnimUpdater::Update(state, 0.2, runtime);
    Check(runtime.ftol_input == 99.6 && runtime.color == 0x63010203u,
          "preserve RGB and truncate faded alpha");
    Check(state.words[16u] == 2u && state.words[17u] == 1u &&
              GetFloat(state, 84u) == 0.5f &&
              GetFloat(state, 88u) == 0.5f &&
              GetFloat(state, 92u) == 0.75f &&
              GetFloat(state, 96u) == 1.0f &&
              state.words[20u] == 0u,
          "advance exactly one atlas frame and clear accumulator");
    Check(runtime.corners == std::array<std::uint32_t, 4>{
              std::bit_cast<std::uint32_t>(0.5f),
              std::bit_cast<std::uint32_t>(0.5f),
              std::bit_cast<std::uint32_t>(0.75f),
              std::bit_cast<std::uint32_t>(1.0f)},
          "geometry receives updated UV corners");
    Check(runtime.update_elapsed == 0.2,
          "native state receives elapsed time");
    Check(runtime.events == std::vector<std::string>{
              "read-color", "ftol", "set-color", "geometry",
              "update-geometry", "system-resource", "update-state"},
          "native update call order");
}

void TestIntervalAndWrap() {
    Runtime runtime;
    auto state = ReadyState();
    state.words[2u] = 0u;
    pal4::SequenceAnimUpdater::Update(state, 0.05, runtime);
    Check(std::abs(GetFloat(state, 80u) - 0.05f) < 1e-6f &&
              state.words[16u] == 1u &&
              runtime.events == std::vector<std::string>{
                  "system-resource", "update-state"},
          "subinterval frame only updates native state");

    runtime.events.clear();
    PutFloat(state, 80u, 0.0f);
    PutFloat(state, 52u, 0.25f);
    state.words[14u] = 2u;
    state.words[15u] = 3u;
    state.words[16u] = 1u;
    state.words[17u] = 0u;
    pal4::SequenceAnimUpdater::Update(state, 1.0, runtime);
    Check(state.words[16u] == 0u && state.words[17u] == 2u &&
              GetFloat(state, 84u) == 0.0f &&
              GetFloat(state, 88u) == 1.0f &&
              GetFloat(state, 92u) == 0.25f &&
              GetFloat(state, 96u) == 1.5f &&
              state.words[20u] == 0u,
          "column and row wrap once despite large elapsed time");
}

void TestAlphaClampAndNaN() {
    Runtime runtime;
    auto state = ReadyState();
    state.words[2u] = 0x01000000u;
    runtime.color = 0x05010203u;
    PutFloat(state, 116u, 10.0f);
    pal4::SequenceAnimUpdater::Update(state, 1.0, runtime);
    Check(runtime.color == 0x00010203u,
          "negative faded alpha clamps to zero");

    runtime.events.clear();
    PutFloat(state, 80u, 0.0f);
    state.words[2u] = 0u;
    pal4::SequenceAnimUpdater::Update(
        state, std::numeric_limits<double>::quiet_NaN(), runtime);
    Check(std::isnan(GetFloat(state, 80u)) &&
              runtime.events == std::vector<std::string>{
                  "system-resource", "update-state"},
          "unordered accumulator skips atlas but still updates native state");
}

}  // namespace

int main() {
    TestGates();
    TestFadeAndAdvance();
    TestIntervalAndWrap();
    TestAlphaClampAndNaN();
}
