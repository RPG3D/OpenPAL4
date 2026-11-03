#include "pal4/object_movement_vectors.h"

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

struct Runtime final : pal4::IObjectMovementVectorRuntime {
    pal4::BehaviorVector3 difference{};
    std::uint32_t input = 0u;
    int difference_calls = 0;
    int transform_calls = 0;

    void ProcessDifference(pal4::ObjectMovementState&,
                           pal4::BehaviorVector3& output,
                           const pal4::BehaviorVector3& value) override {
        ++difference_calls;
        difference = value;
        output = {9.0f, 8.0f, 7.0f};
    }
    pal4::BehaviorVector3 TransformVector(
        pal4::ObjectMovementState&, std::uint32_t value) override {
        ++transform_calls;
        input = value;
        return {2.0f, 3.0f, 4.0f};
    }
};

void TestCopyAndSet() {
    pal4::ObjectMovementState state;
    pal4::BehaviorVector3 output{};
    state.words[4] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[5] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[6] = std::bit_cast<std::uint32_t>(3.0f);
    state.words[7] = std::bit_cast<std::uint32_t>(4.0f);
    state.words[8] = std::bit_cast<std::uint32_t>(5.0f);
    state.words[9] = std::bit_cast<std::uint32_t>(6.0f);
    state.words[10] = std::bit_cast<std::uint32_t>(7.0f);
    state.words[11] = std::bit_cast<std::uint32_t>(8.0f);
    state.words[12] = std::bit_cast<std::uint32_t>(9.0f);
    Check(&pal4::ObjectMovementVectors::CopyAt16(state, output) == &output &&
          output == pal4::BehaviorVector3{1.0f, 2.0f, 3.0f}, "copy +16");
    Check(&pal4::ObjectMovementVectors::CopyAt28(state, output) == &output &&
          output == pal4::BehaviorVector3{4.0f, 5.0f, 6.0f}, "copy +28");
    Check(&pal4::ObjectMovementVectors::CopyAt40(state, output) == &output &&
          output == pal4::BehaviorVector3{7.0f, 8.0f, 9.0f}, "copy +40");

    const pal4::BehaviorVector3 first{10.0f, 11.0f, 12.0f};
    Check(&pal4::ObjectMovementVectors::SetAt4(state, output, first) == &output &&
          output == first, "set +4 returns output");
    Check(std::bit_cast<float>(state.words[1]) == first[0] &&
          std::bit_cast<float>(state.words[2]) == first[1] &&
          std::bit_cast<float>(state.words[3]) == first[2],
          "set +4 persists");
    const pal4::BehaviorVector3 last{-1.0f, -2.0f, -3.0f};
    Check(&pal4::ObjectMovementVectors::SetAt40(state, output, last) == &output &&
          output == last, "set +40 returns output");
    pal4::ObjectMovementVectors::CopyAt40(state, output);
    Check(output == last, "set +40 persists");
}

void TestVectorDispatch() {
    pal4::ObjectMovementState state;
    pal4::BehaviorVector3 output{};
    Runtime runtime;
    pal4::ObjectMovementVectors::SetAt40(
        state, output, {1.0f, 2.0f, 3.0f});
    Check(&pal4::ObjectMovementVectors::DifferenceAndProcess(
              state, output, {5.0f, 7.0f, 9.0f}, runtime) == &output &&
          runtime.difference == pal4::BehaviorVector3{4.0f, 5.0f, 6.0f} &&
          runtime.difference_calls == 1 &&
          output == pal4::BehaviorVector3{9.0f, 8.0f, 7.0f},
          "slot +40 receives target minus origin");
    Check(&pal4::ObjectMovementVectors::TransformAndAdd(
              state, output, 0x4321u, runtime) == &output &&
          runtime.transform_calls == 1 && runtime.input == 0x4321u &&
          output == pal4::BehaviorVector3{3.0f, 5.0f, 7.0f},
          "slot +52 result plus origin");
    pal4::ObjectMovementVectors::IgnoreFourArguments(1u, 2u, 3u, 4u);
    pal4::ObjectMovementVectorRuntimeStub stub;
    pal4::ObjectMovementVectors::DifferenceAndProcess(
        state, output, {1.0f, 2.0f, 3.0f}, stub);
    Check(output == pal4::BehaviorVector3{3.0f, 5.0f, 7.0f},
          "empty callback leaves output intact");
}

}  // namespace

int main() {
    TestCopyAndSet();
    TestVectorDispatch();
}
