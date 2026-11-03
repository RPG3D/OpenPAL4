#include "pal4/move_to_target_task.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

float Field(const pal4::MoveToTargetTaskState& state, std::size_t index) {
    return std::bit_cast<float>(state.words[index]);
}

void Put(pal4::MoveToTargetTaskState& state, std::size_t index, float value) {
    state.words[index] = std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IMoveToTargetTrajectoryRuntime {
    std::vector<std::string> calls;
    std::array<float, 3u> last_displacement{};

    std::uint32_t ResolveMoveFacet(std::uint32_t object,
                                   std::string_view name) override {
        Check(object == 0x5000u && name == "MoveToTargetTask",
              "facet lookup uses owner and native class");
        calls.emplace_back("resolve");
        return 0x6000u;
    }
    std::array<float, 3u> ReadFacetPosition(std::uint32_t facet) override {
        Check(facet == 0x6000u, "read resolved move facet");
        calls.emplace_back("position");
        return {1.0f, 2.0f, 3.0f};
    }
    std::array<float, 3u> ReadTargetOffset(
        std::uint32_t owner,
        const pal4::MoveToTargetTaskState& state) override {
        Check(owner == 0x5100u && state.words[28u] == 0x7000u,
              "target property reads string at +108 from owner +68");
        calls.emplace_back("offset");
        return {1.0f, 2.0f, 3.0f};
    }
    std::uint32_t GetObjectTransform(std::uint32_t object) override {
        Check(object == 0x5000u, "get object transform from owner");
        calls.emplace_back("transform");
        return 0x8000u;
    }
    void SetupTransformMatrix(
        std::array<std::uint32_t, 16u>& matrix,
        std::uint32_t transform) override {
        Check(transform == 0x8000u, "setup matrix transform argument");
        calls.emplace_back("setup");
        matrix[0u] = 0x1234u;
    }
    void MultiplyOwnerMatrix(
        std::array<std::uint32_t, 16u>& matrix,
        std::uint32_t object) override {
        Check(object == 0x5000u && matrix[0u] == 0x1234u,
              "multiply initialized matrix by owner frame");
        calls.emplace_back("multiply");
        matrix[0u] = 0x5678u;
    }
    void TransformPosition(
        std::array<float, 3u>& position,
        const std::array<std::uint32_t, 16u>& matrix) override {
        Check(matrix[0u] == 0x5678u &&
                  position == (std::array<float, 3u>{3.0f, 6.0f, 9.0f}),
              "transform local target after adding named offset");
        calls.emplace_back("calculate");
        position[0u] += 10.0f;
        position[1u] -= 5.0f;
        position[2u] += 1.0f;
    }
    float VectorLength(const std::array<float, 3u>& value) override {
        calls.emplace_back("length");
        last_displacement = value;
        return std::sqrt(value[0u] * value[0u] +
                         value[1u] * value[1u] +
                         value[2u] * value[2u]);
    }
};

void Initialize(pal4::MoveToTargetTaskState& state) {
    state.words[16u] = 0x5000u;
    state.words[17u] = 0x5100u;
    state.words[28u] = 0x7000u;
    Put(state, 21u, 2.0f);
    Put(state, 22u, 4.0f);
    Put(state, 23u, 6.0f);
    Put(state, 5u, 4.0f);
    Put(state, 35u, 2.0f);
}

}  // namespace

int main() {
    pal4::MoveToTargetTaskState state{};
    Initialize(state);
    Runtime runtime;
    pal4::MoveToTargetTask::InitializeTrajectory(state, runtime);
    Check(state.words[31u] == 0x6000u &&
              Field(state, 21u) == 13.0f && Field(state, 22u) == 1.0f &&
              Field(state, 23u) == 10.0f && Field(state, 5u) == 4.0f &&
              Field(state, 18u) == 3.0f && Field(state, 19u) == -0.25f &&
              Field(state, 20u) == 1.75f,
          "ordinary duration derives velocity from transformed target");
    Check(runtime.calls == std::vector<std::string>{
                               "resolve", "position", "offset", "transform",
                               "setup", "multiply", "calculate"},
          "matrix calls occur in native order without length for timed mode");

    pal4::MoveToTargetTaskState fixed{};
    Initialize(fixed);
    fixed.words[34u] = 1u;
    runtime.calls.clear();
    pal4::MoveToTargetTask::InitializeTrajectory(fixed, runtime);
    const auto expected_duration = std::sqrt(194.0f) / 2.0f;
    Check(std::fabs(Field(fixed, 5u) - expected_duration) < 0.001f &&
              runtime.last_displacement ==
                  (std::array<float, 3u>{12.0f, -1.0f, 7.0f}) &&
              std::fabs(Field(fixed, 18u) -
                        12.0f / expected_duration) < 0.001f &&
              runtime.calls.back() == "length",
          "fixed speed replaces duration with distance divided by speed");
}
