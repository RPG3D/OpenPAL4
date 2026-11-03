#include "pal4/object_movement_scalars.h"

#include <bit>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IObjectMovementScalarRuntime {
    int calls = 0;
    float scale_during_call = 0.0f;
    pal4::BehaviorVector3 ReadSlot10Vector(
        pal4::ObjectMovementState& state) override {
        ++calls;
        scale_during_call = std::bit_cast<float>(state.words[25]);
        state.words[25] = std::bit_cast<std::uint32_t>(99.0f);
        return {2.0f, -3.0f, 4.0f};
    }
};

void TestAccessors() {
    pal4::ObjectMovementState state;
    Check(pal4::ObjectMovementScalars::Set23(state, 1.25f) == 1.25f &&
          pal4::ObjectMovementScalars::Get23(state) == 1.25f &&
          state.words[23] == std::bit_cast<std::uint32_t>(1.25f),
          "field 23 round trip");
    Check(pal4::ObjectMovementScalars::Set24(state, -2.5f) == -2.5f &&
          pal4::ObjectMovementScalars::Get24(state) == -2.5f &&
          state.words[24] == std::bit_cast<std::uint32_t>(-2.5f),
          "field 24 round trip");
    Check(pal4::ObjectMovementScalars::Set25(state, 3.0f) == 3.0f &&
          pal4::ObjectMovementScalars::Get25(state) == 3.0f &&
          state.words[25] == std::bit_cast<std::uint32_t>(3.0f),
          "field 25 round trip");
    Check(pal4::ObjectMovementScalars::Set26(state, 4.0f) == 4.0f &&
          pal4::ObjectMovementScalars::Get26(state) == 4.0f &&
          state.words[26] == std::bit_cast<std::uint32_t>(4.0f),
          "field 26 round trip");
    Check(pal4::ObjectMovementScalars::Set27(state, 5.0f) == 5.0f &&
          pal4::ObjectMovementScalars::Get27(state) == 5.0f &&
          state.words[27] == std::bit_cast<std::uint32_t>(5.0f),
          "field 27 round trip");
    Check(state.words[22] == 0u && state.words[28] == 0u,
          "neighboring words untouched");
}

void TestScaledVirtualVector() {
    pal4::ObjectMovementState state;
    Check(pal4::ObjectMovementScalars::Set25(state, 2.0f) == 2.0f,
          "scale setter return");
    pal4::BehaviorVector3 output{};
    Runtime runtime;
    Check(&pal4::ObjectMovementScalars::ScaledSlot10Vector(
              state, output, runtime) == &output &&
          output == pal4::BehaviorVector3{4.0f, -6.0f, 8.0f} &&
          runtime.calls == 1 && runtime.scale_during_call == 2.0f &&
          pal4::ObjectMovementScalars::Get25(state) == 99.0f,
          "scale captured before virtual call");
    pal4::ObjectMovementScalarRuntimeStub stub;
    pal4::ObjectMovementScalars::ScaledSlot10Vector(state, output, stub);
    Check(output == pal4::BehaviorVector3{}, "linkable vector stub");
}

}  // namespace

int main() {
    TestAccessors();
    TestScaledVirtualVector();
}
