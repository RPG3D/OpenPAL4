#include "pal4/behavior_path_endpoint_helper.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.002f;
}

struct Runtime final : pal4::IBehaviorPathInitializationRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> rand_values;
    std::size_t rand_index = 0u;
    std::vector<std::uint32_t> scalar_slots;
    std::vector<std::uint32_t> scalar_bits;
    pal4::BehaviorPathEndpointState* state = nullptr;
    bool rejected_once = false;

    pal4::BehaviorVector3 ReadGlobalVector(
        std::uint32_t address) override {
        calls.emplace_back("global");
        if (address == 0x93cf6cu) return {1.0f, 2.0f, 3.0f};
        Check(address == 0x93cf60u, "second vector uses input global");
        return {-1.0f, 0.0f, 1.0f};
    }
    void CallVoidSlot(pal4::BehaviorPathEndpointState& target,
                      std::uint32_t slot) override {
        calls.emplace_back("void");
        Check(&target == state && slot == 0x24u,
              "first virtual call uses slot +0x24");
        target.words[13] = 0x1234u;
        target.words[14] = 0x5678u;
        target.words[21] = 0xaaaaaaffu;
    }
    void CallScalarSlot(pal4::BehaviorPathEndpointState& target,
                        std::uint32_t slot,
                        std::uint32_t value_bits) override {
        calls.emplace_back("scalar");
        Check(&target == state && target.words[13] == 0u &&
                  target.words[14] == 0u &&
                  target.words[21] == 0xaaaaaa00u,
              "state fields clear after first virtual call");
        scalar_slots.push_back(slot);
        scalar_bits.push_back(value_bits);
    }
    void CallVectorSlot0C(pal4::BehaviorPathEndpointState& target,
                           pal4::BehaviorVector3& output,
                           const pal4::BehaviorVector3& input) override {
        calls.emplace_back("slot0c");
        Check(&target == state && input ==
                  pal4::BehaviorVector3{-1.0f, 0.0f, 1.0f} &&
                  output == pal4::BehaviorVector3{1.0f, 2.0f, 3.0f},
              "slot +0x0c receives input global and default output");
        output = {9.0f, 9.0f, 9.0f};
    }
    void CallVectorSlot14(pal4::BehaviorPathEndpointState& target,
                           pal4::BehaviorContext3& output,
                           const pal4::BehaviorVector3& input) override {
        calls.emplace_back("slot14");
        Check(&target == state && Near(input[0], 0.7071f) &&
                  input[1] == 0.0f && Near(input[2], 0.7071f),
              "slot +0x14 receives normalized horizontal random vector");
        output = {0x11u, 0x22u, 0x33u};
    }
    std::uint32_t CallContextSlot10(
        pal4::BehaviorPathEndpointState& target,
        pal4::BehaviorContext3& context) override {
        calls.emplace_back("slot10");
        Check(&target == state && context ==
                  pal4::BehaviorContext3{0x11u, 0x22u, 0x33u},
              "slot +0x10 receives context from slot +0x14");
        return 0x77u;
    }
    pal4::BehaviorVector3 CallTokenSlot48(
        pal4::BehaviorPathEndpointState& target,
        pal4::BehaviorVector3& output,
        std::uint32_t token) override {
        calls.emplace_back("slot48");
        Check(&target == state && token == 0x77u,
              "slot +0x48 receives token from slot +0x10");
        output = {4.0f, 5.0f, 6.0f};
        return {7.0f, 8.0f, 9.0f};
    }
    void CallVectorSlot04(pal4::BehaviorPathEndpointState& target,
                           pal4::BehaviorVector3& output,
                           const pal4::BehaviorVector3& input) override {
        calls.emplace_back("slot04");
        Check(&target == state &&
                  output == pal4::BehaviorVector3{9.0f, 9.0f, 9.0f} &&
                  input == pal4::BehaviorVector3{7.0f, 8.0f, 9.0f},
              "slot +4 receives resolved vector by value");
        output = {10.0f, 11.0f, 12.0f};
    }
    std::uint32_t Rand() override {
        calls.emplace_back("rand");
        Check(rand_index < rand_values.size(),
              "random stream contains every required draw");
        return rand_values[rand_index++];
    }
    std::uint32_t Read32(std::uint32_t address) override {
        calls.emplace_back("read_target");
        Check(address == 0x9000u,
              "target vtable read occurs before event callback");
        return 0xa000u;
    }
    std::uint32_t CallContextSlot18(
        pal4::BehaviorPathEndpointState& target,
        pal4::BehaviorContext3& context) override {
        calls.emplace_back("slot18");
        Check(&target == state && context ==
                  pal4::BehaviorContext3{0x11u, 0x22u, 0x33u} &&
                  target.words[48] != 0u,
              "slot +0x18 follows direction write and receives context");
        target.words[42] = 0x9004u;
        return 0x88u;
    }
    std::uint32_t CallTargetSlot04(
        std::uint32_t captured_vtable, std::uint32_t target,
        std::uint32_t value) override {
        calls.emplace_back("target04");
        Check(captured_vtable == 0xa000u && target == 0x9004u &&
                  value == 0x88u,
              "final call keeps captured vtable but rereads target pointer");
        return 0xbeefu;
    }
};

void TestInitialization(std::vector<std::uint32_t> random_values,
                        std::uint32_t expected_direction,
                        std::size_t expected_random_calls) {
    pal4::BehaviorPathEndpointState state;
    Runtime runtime;
    runtime.state = &state;
    runtime.rand_values = std::move(random_values);
    state.words.fill(0xaaaaaaaau);
    state.words[42] = 0x9000u;
    Check(pal4::BehaviorPathEndpointHelper::InitializeMovement(
              state, runtime) == 0xbeefu &&
              runtime.rand_index == expected_random_calls &&
              state.words[13] == 0u && state.words[14] == 0u &&
              state.words[21] == 0xaaaaaa00u &&
              state.words[28] == 0u && state.words[38] == 0u &&
              state.words[48] == expected_direction &&
              state.words[42] == 0x9004u,
          "initialization writes confirmed state and returns target result");
    Check(runtime.scalar_slots == std::vector<std::uint32_t>{
              0x54u, 0x68u, 0x5cu, 0x74u, 0x7cu} &&
              runtime.scalar_bits == std::vector<std::uint32_t>{
                  0x3f800000u, 0u, 0x3f000000u,
                  0x3dcccccdu, 0x3f800000u},
          "five scalar virtual calls use exact slots and raw float bits");
    for (const std::uint32_t first : {29u, 32u, 35u, 39u}) {
        Check(state.words[first] == std::bit_cast<std::uint32_t>(1.0f) &&
                  state.words[first + 1u] ==
                      std::bit_cast<std::uint32_t>(2.0f) &&
                  state.words[first + 2u] ==
                      std::bit_cast<std::uint32_t>(3.0f),
              "default global vector is copied to four observed locations");
    }
    Check(runtime.calls.front() == "void" &&
              runtime.calls[6] == "global" &&
              runtime.calls[7] == "global" &&
              runtime.calls[8] == "slot0c" &&
              runtime.calls.back() == "target04",
          "major virtual and global stages remain ordered");
    const auto read_target = std::find(runtime.calls.begin(),
                                       runtime.calls.end(), "read_target");
    Check(read_target != runtime.calls.end() &&
              *(read_target + 1) == "slot18" &&
              *(read_target + 2) == "target04",
          "target vtable capture precedes context encoding and dispatch");
}

void TestStub() {
    pal4::BehaviorPathEndpointState state;
    pal4::BehaviorPathInitializationRuntimeStub runtime;
    Check(pal4::BehaviorPathEndpointHelper::InitializeMovement(
              state, runtime) == 0u && state.words[48] == 0xffffffffu,
          "initialization links without original globals, CRT or virtuals");
}

}  // namespace

int main() {
    TestInitialization({32767u, 32767u, 32767u,
                        16384u, 16384u, 16384u, 0u},
                       1u, 7u);
    TestInitialization({16384u, 16384u, 16384u, 32767u},
                       0xffffffffu, 4u);
    TestStub();
}
