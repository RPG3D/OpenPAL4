#include "pal4/object_collision_processor.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

using pal4::BehaviorVector3;
using pal4::ObjectCollisionSweep;
using pal4::ObjectMovementState;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IObjectCollisionRuntime {
    std::vector<std::string> calls;
    float scale = 2.0f;
    float first_measure = 1.0f;
    float second_measure = 2.0f;
    float predicted_measure = 0.0f;
    float motion_delta = 0.0f;
    float adjusted_time = 0.0f;
    int measure_calls = 0;
    int position_calls = 0;
    bool adjusted_alias = false;
    std::uint32_t target_seen = 0u;
    ObjectCollisionSweep sweep{{7.0f, 8.0f, 9.0f}, -1.0f};
    BehaviorVector3 motion{4.0f, 5.0f, 6.0f};
    BehaviorVector3 current{1.0f, 2.0f, 3.0f};
    BehaviorVector3 predicted{10.0f, 20.0f, 30.0f};
    BehaviorVector3 offset{1.0f, 1.0f, 1.0f};
    BehaviorVector3 global_zero{9.0f, 9.0f, 9.0f};

    float MovementScale(ObjectMovementState&) override {
        calls.emplace_back("scale");
        return scale;
    }
    BehaviorVector3 Motion(ObjectMovementState&, float delta) override {
        calls.emplace_back("motion");
        motion_delta = delta;
        return motion;
    }
    BehaviorVector3 Position(ObjectMovementState&) override {
        calls.emplace_back("position");
        ++position_calls;
        return current;
    }
    float Measure(std::uint32_t target,
                  const BehaviorVector3& vector) override {
        calls.emplace_back("measure");
        Check(target == 0x7000u, "measure target");
        ++measure_calls;
        Check(vector == (measure_calls == 1 ? current : motion),
              "measure vector order");
        return measure_calls == 1 ? first_measure : second_measure;
    }
    ObjectCollisionSweep Sweep(
        std::uint32_t target, const BehaviorVector3& input) override {
        calls.emplace_back("sweep");
        Check(input == motion, "sweep motion");
        target_seen = target;
        return sweep;
    }
    BehaviorVector3 Predict(
        std::uint32_t target, const BehaviorVector3& position,
        float measure) override {
        calls.emplace_back("predict");
        Check(target == 0x7000u && position == current,
              "prediction target and position");
        predicted_measure = measure;
        return predicted;
    }
    void Adjust(ObjectMovementState&, const BehaviorVector3& motion_input,
                BehaviorVector3& hit_position, BehaviorVector3& predicted_input,
                float hit_time) override {
        calls.emplace_back("adjust");
        Check(motion_input == motion && hit_position == sweep.position,
              "adjust inputs");
        adjusted_alias = &hit_position == &predicted_input;
        adjusted_time = hit_time;
        predicted_input[0] += 1.0f;
        predicted_input[1] += 2.0f;
        predicted_input[2] += 3.0f;
    }
    BehaviorVector3 Offset(ObjectMovementState&) override {
        calls.emplace_back("offset");
        return offset;
    }
    BehaviorVector3 GlobalZeroVector() override {
        calls.emplace_back("global_zero");
        return global_zero;
    }
};

void TestClampedNegativeHitGate() {
    ObjectMovementState state;
    Runtime runtime;
    BehaviorVector3 output{};
    Check(&pal4::ObjectCollisionProcessor::ProcessClamped(
              state, output, 3, 0.5f, 0x7000u, runtime) == &output &&
          output == runtime.global_zero &&
          runtime.calls == std::vector<std::string>{
              "scale", "motion", "position", "measure", "measure",
              "sweep", "global_zero"},
          "positive range and increasing measure reject negative hit");

    Runtime inverse;
    inverse.first_measure = 2.0f;
    inverse.second_measure = 1.0f;
    pal4::ObjectCollisionProcessor::ProcessClamped(
        state, output, -3, 0.5f, 0x7000u, inverse);
    Check(output == inverse.global_zero,
          "negative range reverses measure gate");
}

void TestClampedCompleteResponse() {
    ObjectMovementState state;
    Runtime runtime;
    runtime.sweep.time = 0.25f;
    BehaviorVector3 output{};
    pal4::ObjectCollisionProcessor::ProcessClamped(
        state, output, 3, 0.5f, 0x7000u, runtime);
    Check(runtime.target_seen == 0x7000u && runtime.motion_delta == 0.5f &&
          runtime.predicted_measure == 4.0f &&
          runtime.adjusted_time == 0.25f && !runtime.adjusted_alias,
          "clamped prediction and distinct hit buffers");
    Check(output == BehaviorVector3{9.0f, 19.0f, 29.0f},
          "predicted minus current and offset");
    Check(runtime.calls == std::vector<std::string>{
              "scale", "motion", "position", "measure", "measure",
              "sweep", "predict", "adjust", "position", "offset"},
          "clamped call sequence");

    Runtime opposite;
    opposite.first_measure = 2.0f;
    opposite.second_measure = 1.0f;
    pal4::ObjectCollisionProcessor::ProcessClamped(
        state, output, 3, 0.5f, 0x7000u, opposite);
    Check(opposite.calls.back() == "offset",
          "negative hit with opposite measure direction continues");
}

void TestUnclampedPaths() {
    ObjectMovementState state;
    Runtime runtime;
    BehaviorVector3 output{};
    Check(&pal4::ObjectCollisionProcessor::ProcessUnclamped(
              state, output, 2.5f, 0x7000u, runtime) == &output &&
          output == runtime.global_zero &&
          runtime.calls == std::vector<std::string>{
              "motion", "sweep", "global_zero"},
          "unclamped negative hit returns global vector");

    Runtime valid;
    valid.sweep.time = 0.0f;
    pal4::ObjectCollisionProcessor::ProcessUnclamped(
        state, output, 2.5f, 0x7000u, valid);
    Check(valid.adjusted_alias && valid.adjusted_time == 0.0f &&
          output == BehaviorVector3{6.0f, 7.0f, 8.0f} &&
          valid.calls == std::vector<std::string>{
              "motion", "sweep", "adjust", "position", "offset"},
          "unclamped hit aliases both adjustment vectors");

    pal4::ObjectCollisionRuntimeStub stub;
    pal4::ObjectCollisionProcessor::ProcessUnclamped(
        state, output, 1.0f, 0u, stub);
    Check(output == BehaviorVector3{}, "linkable collision stub");
}

}  // namespace

int main() {
    TestClampedNegativeHitGate();
    TestClampedCompleteResponse();
    TestUnclampedPaths();
}
