#include "pal4/object_transform_basis.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

using pal4::BehaviorVector3;
using pal4::ObjectMovementState;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void SetAxis(ObjectMovementState& state, std::size_t first,
             const BehaviorVector3& value) {
    for (std::size_t i = 0u; i != 3u; ++i)
        state.words[first + i] = std::bit_cast<std::uint32_t>(value[i]);
}

BehaviorVector3 GetAxis(const ObjectMovementState& state, std::size_t first) {
    return {std::bit_cast<float>(state.words[first]),
            std::bit_cast<float>(state.words[first + 1u]),
            std::bit_cast<float>(state.words[first + 2u])};
}

struct Runtime final : pal4::IObjectTransformRuntime {
    std::uint32_t flag = 0u;
    std::vector<std::string> calls;
    BehaviorVector3 received{};
    BehaviorVector3 difference_input{};
    BehaviorVector3 transform_input{};
    BehaviorVector3 final_input{};

    std::uint32_t OrientationFlag(ObjectMovementState&) override {
        calls.emplace_back("orientation");
        return flag;
    }
    void PrepareAxis(ObjectMovementState&) override {
        calls.emplace_back("prepare");
    }
    std::uint32_t ProcessNormalized(
        ObjectMovementState&, const BehaviorVector3& vector) override {
        calls.emplace_back("process_normalized");
        received = vector;
        return 0x8765u;
    }
    void MakeDifference(ObjectMovementState&, BehaviorVector3& result,
                        const BehaviorVector3& input) override {
        calls.emplace_back("difference");
        difference_input = input;
        result = {2.0f, 3.0f, 4.0f};
    }
    void TransformDifference(ObjectMovementState&, BehaviorVector3& result,
                             const BehaviorVector3& input) override {
        calls.emplace_back("transform");
        transform_input = input;
        result = {5.0f, 6.0f, 7.0f};
    }
    void ProcessTransformed(ObjectMovementState&, BehaviorVector3& output,
                            const BehaviorVector3& input) override {
        calls.emplace_back("final");
        final_input = input;
        output = {8.0f, 9.0f, 10.0f};
    }
};

void TestMultiply() {
    ObjectMovementState state;
    SetAxis(state, 1u, {1.0f, 2.0f, 3.0f});
    SetAxis(state, 4u, {4.0f, 5.0f, 6.0f});
    SetAxis(state, 7u, {7.0f, 8.0f, 9.0f});
    BehaviorVector3 output{};
    Check(&pal4::ObjectTransformBasis::Multiply(
              state, output, {2.0f, 3.0f, 4.0f}) == &output &&
          output == BehaviorVector3{42.0f, 51.0f, 60.0f},
          "matrix column multiplication");
}

void TestNormalizeFirstAxis() {
    ObjectMovementState state;
    Runtime runtime;
    SetAxis(state, 4u, {2.0f, 0.0f, 0.0f});
    SetAxis(state, 7u, {0.0f, 3.0f, 0.0f});
    Check(pal4::ObjectTransformBasis::NormalizeFirstAxis(state, runtime) == 0u &&
          GetAxis(state, 1u) == BehaviorVector3{0.0f, 0.0f, 1.0f},
          "forward cross and normalization");
    runtime.flag = 1u;
    Check(pal4::ObjectTransformBasis::NormalizeFirstAxis(state, runtime) == 0u &&
          GetAxis(state, 1u) == BehaviorVector3{0.0f, 0.0f, -1.0f},
          "reversed cross");
    SetAxis(state, 4u, {});
    Check(pal4::ObjectTransformBasis::NormalizeFirstAxis(state, runtime) ==
              state.words[3] &&
          GetAxis(state, 1u) == BehaviorVector3{},
          "zero length copies calculated vector");
}

void TestSetThirdAxis() {
    ObjectMovementState state;
    Runtime runtime;
    SetAxis(state, 1u, {1.0f, 0.0f, 0.0f});
    runtime.flag = 0x100u;
    Check(pal4::ObjectTransformBasis::SetThirdAxis(
              state, {0.0f, 0.0f, 1.0f}, runtime) == 0x100u &&
          GetAxis(state, 4u) == BehaviorVector3{0.0f, 1.0f, 0.0f} &&
          runtime.calls == std::vector<std::string>{"prepare", "orientation"},
          "low byte controls cross and raw virtual result returns");
    runtime.flag = 1u;
    Check(pal4::ObjectTransformBasis::SetThirdAxis(
              state, {0.0f, 0.0f, 1.0f}, runtime) == 1u &&
          GetAxis(state, 4u) == BehaviorVector3{0.0f, -1.0f, 0.0f},
          "set third axis reversed cross");
}

void TestVirtualVectorPaths() {
    ObjectMovementState state;
    Runtime runtime;
    Check(pal4::ObjectTransformBasis::NormalizeAndProcess(
              state, {3.0f, 4.0f, 0.0f}, runtime) == 0x8765u &&
          std::abs(runtime.received[0] - 0.6f) < 0.00001f &&
          std::abs(runtime.received[1] - 0.8f) < 0.00001f,
          "normalized vector reaches slot +60");
    Check(pal4::ObjectTransformBasis::NormalizeAndProcess(
              state, {}, runtime) == 0x8765u &&
          runtime.received == BehaviorVector3{},
          "zero vector remains zero");

    BehaviorVector3 output{};
    Check(&pal4::ObjectTransformBasis::ReorderAndNegate(
              state, output, {1.0f, 2.0f, 3.0f}, runtime) == &output &&
          output == BehaviorVector3{3.0f, 2.0f, 1.0f},
          "forward vector reorder");
    runtime.flag = 1u;
    pal4::ObjectTransformBasis::ReorderAndNegate(
        state, output, {1.0f, 2.0f, 3.0f}, runtime);
    Check(output == BehaviorVector3{-3.0f, 2.0f, 1.0f},
          "reverse vector reorder");

    runtime.calls.clear();
    Check(&pal4::ObjectTransformBasis::ProcessTransformations(
              state, output, {11.0f, 12.0f, 13.0f}, runtime) == &output &&
          runtime.calls == std::vector<std::string>{"difference", "transform", "final"} &&
          runtime.difference_input == BehaviorVector3{11.0f, 12.0f, 13.0f} &&
          runtime.transform_input == BehaviorVector3{2.0f, 3.0f, 4.0f} &&
          runtime.final_input == BehaviorVector3{5.0f, 6.0f, 7.0f} &&
          output == BehaviorVector3{8.0f, 9.0f, 10.0f},
          "three virtual transformations in order");

    pal4::ObjectTransformRuntimeStub stub;
    Check(pal4::ObjectTransformBasis::NormalizeAndProcess(
              state, {}, stub) == 0u, "linkable transform stub");
}

}  // namespace

int main() {
    TestMultiply();
    TestNormalizeFirstAxis();
    TestSetThirdAxis();
    TestVirtualVectorPaths();
}
