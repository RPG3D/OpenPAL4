#include "pal4/behavior_path_collision_state.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void StoreEndpoint(pal4::BehaviorPathEndpointState& state,
                   std::uint32_t first_word,
                   const pal4::BehaviorVector3& value) {
    for (std::uint32_t i = 0u; i != 3u; ++i)
        state.words[first_word + i] = std::bit_cast<std::uint32_t>(value[i]);
}

struct Runtime final : pal4::IBehaviorPathCollisionStateRuntime {
    pal4::BehaviorPathEndpointState* state = nullptr;
    std::vector<std::string> calls;
    std::vector<pal4::BehaviorVector3> positions;
    std::vector<std::uint32_t> radius_paths;
    std::size_t position_index = 0u;
    float radius = 1.0f;
    bool change_path_after_first_radius = false;

    pal4::BehaviorVector3 ReadPosition(
        std::uint32_t object,
        pal4::BehaviorContext3& scratch) override {
        calls.emplace_back("position");
        Check(object == 0x7000u && position_index < positions.size(),
              "virtual slot +0x18 reads supplied object twice");
        scratch = {1u, 2u, 3u};
        return positions[position_index++];
    }
    float ReadPathRadius(std::uint32_t path_address) override {
        calls.emplace_back("radius");
        radius_paths.push_back(path_address);
        if (change_path_after_first_radius && radius_paths.size() == 1u)
            state->words[47] = 0x5000u;
        return radius;
    }
};

void TestDisabled() {
    pal4::BehaviorPathEndpointState state;
    Runtime runtime;
    runtime.state = &state;
    state.words[47] = 0x4000u;
    state.words[48] = 0x123u;
    state.words[49] = 0xaabb00ddu;
    pal4::BehaviorPathCollisionState::UpdateFromObject(
        state, 0u, 0x99u, runtime);
    pal4::BehaviorPathCollisionState::UpdateFromVector(
        state, 1u, 2u, 3u, nullptr, runtime);
    pal4::BehaviorPathCollisionState::UpdateFromRecord(
        state, nullptr, 4u, runtime);
    Check(runtime.calls.empty() && state.words[48] == 0x123u &&
              state.words[49] == 0xaabb00ddu,
          "disabled flag skips all position and radius dereferences");
}

void TestVirtualPositions() {
    pal4::BehaviorPathEndpointState state;
    Runtime runtime;
    runtime.state = &state;
    state.words[47] = 0x4000u;
    state.words[48] = 0x123u;
    state.words[49] = 0x100u;
    StoreEndpoint(state, 50u, {0.0f, 0.0f, 0.0f});
    StoreEndpoint(state, 53u, {10.0f, 0.0f, 0.0f});
    runtime.positions = {{0.5f, 0.0f, 0.0f}, {9.5f, 0.0f, 0.0f}};
    runtime.change_path_after_first_radius = true;
    pal4::BehaviorPathCollisionState::UpdateFromObject(
        state, 0x7000u, 0xdeadbeefu, runtime);
    Check(runtime.position_index == 2u &&
              runtime.calls == std::vector<std::string>{
                  "position", "radius", "position", "radius"} &&
              runtime.radius_paths == std::vector<std::uint32_t>{
                  0x4000u, 0x5000u} &&
              state.words[48] == 0xffffffffu,
          "virtual-position entry rereads path and last endpoint wins");
}

void TestDirectVector() {
    pal4::BehaviorPathEndpointState state;
    Runtime runtime;
    runtime.state = &state;
    state.words[47] = 0x4000u;
    state.words[48] = 0x123u;
    state.words[49] = 0x100u;
    StoreEndpoint(state, 50u, {0.0f, 0.0f, 0.0f});
    StoreEndpoint(state, 53u, {10.0f, 0.0f, 0.0f});
    const pal4::BehaviorVector3 boundary{1.0f, 0.0f, 0.0f};
    runtime.change_path_after_first_radius = true;
    pal4::BehaviorPathCollisionState::UpdateFromVector(
        state, 1u, 2u, 3u, &boundary, runtime);
    Check(state.words[48] == 0x123u &&
              runtime.radius_paths == std::vector<std::uint32_t>{
                  0x4000u, 0x4000u} &&
              runtime.calls == std::vector<std::string>{"radius", "radius"},
          "direct-vector entry captures path once and uses strict radius");

    runtime.calls.clear();
    runtime.radius_paths.clear();
    state.words[47] = 0x4000u;
    const pal4::BehaviorVector3 near_end{9.5f, 0.0f, 0.0f};
    pal4::BehaviorPathCollisionState::UpdateFromVector(
        state, 4u, 5u, 6u, &near_end, runtime);
    Check(state.words[48] == 0xffffffffu,
          "last endpoint can flip direction without first endpoint match");
}

void TestRecordOffsets() {
    pal4::BehaviorPathEndpointState state;
    Runtime runtime;
    runtime.state = &state;
    state.words[47] = 0x4000u;
    state.words[48] = 0x123u;
    state.words[49] = 0x100u;
    StoreEndpoint(state, 50u, {0.0f, 0.0f, 0.0f});
    StoreEndpoint(state, 53u, {10.0f, 0.0f, 0.0f});
    const std::array<float, 5> record{100.0f, 200.0f,
                                       0.5f, 0.0f, 0.0f};
    runtime.change_path_after_first_radius = true;
    pal4::BehaviorPathCollisionState::UpdateFromRecord(
        state, &record, 0xfeedu, runtime);
    Check(state.words[48] == 1u &&
              runtime.radius_paths == std::vector<std::uint32_t>{
                  0x4000u, 0x4000u} &&
              runtime.calls == std::vector<std::string>{"radius", "radius"},
          "record entry reads floats at +8/+12/+16 and captures path once");
    pal4::BehaviorPathCollisionStateRuntimeStub stub;
    pal4::BehaviorPathCollisionState::UpdateFromRecord(
        state, &record, 0u, stub);
}

}  // namespace

int main() {
    TestDisabled();
    TestVirtualPositions();
    TestDirectVector();
    TestRecordOffsets();
}
