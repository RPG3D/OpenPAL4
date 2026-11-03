#include "pal4/object_movement_reset.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IObjectMovementResetRuntime {
    std::vector<std::string> calls;
    std::vector<std::array<std::uint32_t, 2>> setters;
    std::uint32_t captured_address = 0;
    std::uint32_t dispatched_address = 0;
    std::uint32_t dispatched_vtable = 0;
    std::uint32_t dispatched_token = 0;

    void CallSlot24(pal4::ObjectMovementState& state) override {
        calls.emplace_back("24");
        Check(state.words[13] == 0xccccccccu &&
                  state.words[14] == 0xccccccccu,
              "slot 24 precedes state clearing");
    }
    void CallScalarSetter(pal4::ObjectMovementState& state,
                          std::uint32_t slot,
                          std::uint32_t bits) override {
        calls.emplace_back("setter");
        setters.push_back({slot, bits});
        Check(state.words[13] == 0u && state.words[14] == 0u &&
                  state.words[21] == 0xcccccc00u,
              "state clearing precedes scalar setters");
    }
    std::array<std::uint32_t, 3> GlobalVector() override {
        calls.emplace_back("global");
        return {11u, 22u, 33u};
    }
    std::uint32_t MemberVtable(std::uint32_t address) override {
        calls.emplace_back("member_vtable");
        captured_address = address;
        return 0x4321u;
    }
    std::uint32_t CallSlot18(
        pal4::ObjectMovementState& state,
        std::array<std::uint32_t, 3>& output) override {
        calls.emplace_back("18");
        for (std::uint32_t start : {29u, 32u, 35u, 39u}) {
            Check(state.words[start] == 11u &&
                      state.words[start + 1u] == 22u &&
                      state.words[start + 2u] == 33u,
                  "four vector fields copied before slot 18");
        }
        Check(state.words[28] == 0u && state.words[38] == 0u,
              "scalar fields zero before slot 18");
        state.words[42] = 0x2000u;
        output = {7u, 8u, 9u};
        return 0x9876u;
    }
    std::uint32_t CallMemberSlot4(std::uint32_t vtable,
                                  std::uint32_t address,
                                  std::uint32_t token) override {
        calls.emplace_back("member_4");
        dispatched_vtable = vtable;
        dispatched_address = address;
        dispatched_token = token;
        return 0xabcdef01u;
    }
};

void TestResetOrderAndValues() {
    pal4::ObjectMovementState state;
    state.words.fill(0xccccccccu);
    state.words[42] = 0x1000u;
    Runtime runtime;
    Check(pal4::ObjectMovementReset::Reset(state, runtime) == 0xabcdef01u,
          "reset returns member slot 4 result");
    Check(runtime.setters ==
              std::vector<std::array<std::uint32_t, 2>>{
                  {0x54u, 0x3f800000u}, {0x68u, 0u},
                  {0x5cu, 0x3f000000u}, {0x74u, 0x3dcccccdu},
                  {0x7cu, 0x3f800000u}},
          "scalar setter slots and float bit patterns");
    Check(runtime.calls ==
              std::vector<std::string>{"24", "setter", "setter", "setter",
                                       "setter", "setter", "global",
                                       "member_vtable", "18", "member_4"},
          "virtual operations remain in assembly order");
    Check(runtime.captured_address == 0x1000u &&
              runtime.dispatched_address == 0x2000u &&
              runtime.dispatched_vtable == 0x4321u &&
              runtime.dispatched_token == 0x9876u,
          "captured member vtable and current address are distinct");
    Check(state.words[20] == 0xccccccccu &&
              state.words[21] == 0xcccccc00u &&
              state.words[43] == 0xccccccccu,
          "adjacent memory is preserved");
}

void TestDefaultRuntime() {
    pal4::ObjectMovementState state;
    state.words.fill(0xffffffffu);
    pal4::ObjectMovementResetRuntimeStub runtime;
    Check(pal4::ObjectMovementReset::Reset(state, runtime) == 0u &&
              state.words[29] == 0u &&
              state.words[41] == 0u,
          "linkable default copies the CRT-initialized zero vector");
}

}  // namespace

int main() {
    TestResetOrderAndValues();
    TestDefaultRuntime();
    std::cout << "2 object movement reset cases passed\n";
}
