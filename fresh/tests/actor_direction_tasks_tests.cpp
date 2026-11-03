#include "pal4/actor_direction_tasks.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x5000u;
constexpr std::uint32_t kActor = 0x7000u;

struct Runtime final : pal4::IActorDirectionTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::vector<pal4::ActorDirectionVector> positions;
    std::uint32_t position_actor = 0u;
    std::uint32_t position_source = 0u;
    std::uint32_t position_mode = 0u;
    std::uint32_t facing_actor = 0u;
    std::uint32_t facing_mode = 0u;
    pal4::ActorDirectionVector facing;
    float cosine = 0.0f;
    float normalize_scale = 1.0f;
    bool allow_nan = false;
    double angle_result = 1.0;
    std::uint32_t prepare_actor = 0u;
    std::uint32_t prepare_state = 0u;

    void ConstructBase(std::uint32_t task) override {
        Check(task == kTask, "base receives task");
        calls.emplace_back("base");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read_word");
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write_word");
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("read_float");
        return floats[address];
    }
    pal4::ActorDirectionVector ChildPosition(std::uint32_t actor) override {
        Check(actor == kActor, "child position uses task actor");
        calls.emplace_back("child_position");
        const auto index = static_cast<std::size_t>(
            std::count(calls.begin(), calls.end(), "child_position") - 1);
        return positions[index];
    }
    void Normalize(pal4::ActorDirectionVector& vector) override {
        calls.emplace_back("normalize");
        if (allow_nan && std::isnan(vector.x)) return;
        const auto magnitude = std::sqrt(vector.x * vector.x +
                                         vector.y * vector.y +
                                         vector.z * vector.z);
        Check(magnitude > 0.0f, "normalizes nonzero vector");
        vector.x = vector.x / magnitude * normalize_scale;
        vector.y = vector.y / magnitude * normalize_scale;
        vector.z = vector.z / magnitude * normalize_scale;
    }
    double AngleRadians(float value) override {
        calls.emplace_back("acos");
        cosine = value;
        return angle_result;
    }
    void SetPosition(std::uint32_t actor, std::uint32_t source,
                     std::uint32_t mode) override {
        calls.emplace_back("set_position");
        position_actor = actor;
        position_source = source;
        position_mode = mode;
    }
    void SetFacing(std::uint32_t actor,
                   const pal4::ActorDirectionVector& rotation,
                   std::uint32_t mode) override {
        calls.emplace_back("set_facing");
        facing_actor = actor;
        facing = rotation;
        facing_mode = mode;
    }
    void SetPrepareState(std::uint32_t actor,
                         std::uint32_t state) override {
        calls.emplace_back("set_prepare");
        prepare_actor = actor;
        prepare_state = state;
    }
};

void TranslateActor() {
    Runtime runtime;
    Check(pal4::TranslateActorTask::Process(kTask, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{"read_word"},
          "absent actor returns one without forwarding");
    runtime.words[kTask + 72u] = kActor;
    runtime.calls.clear();
    Check(pal4::TranslateActorTask::Process(kTask, runtime) == 1u &&
          runtime.position_actor == kActor &&
          runtime.position_source == kTask + 60u &&
          runtime.position_mode == 0u &&
          runtime.calls == std::vector<std::string>{"read_word", "set_position"},
          "translate forwards embedded target and mode zero");
}

void FaceConstructionAndMissingActor() {
    Runtime runtime;
    Check(pal4::FaceTask::Construct(kTask, runtime) == kTask &&
          runtime.words[kTask + 72u] == 0u &&
          runtime.words[kTask] == pal4::FaceTask::kVtable &&
          runtime.calls == std::vector<std::string>{"base", "write_word",
                                                     "write_word"},
          "face construction sets base, actor and vtable in order");
    runtime.calls.clear();
    Check(pal4::FaceTask::Process(kTask, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"read_word"},
          "missing face actor returns zero without touching duration");
}

void FaceDeadzone() {
    Runtime runtime;
    runtime.words[kTask + 72u] = kActor;
    runtime.words[kTask + 20u] = 0xffffffffu;
    runtime.floats[kTask + 60u] = 0.00001f;
    runtime.positions.assign(3, {});
    Check(pal4::FaceTask::Process(kTask, runtime) == 1u &&
          runtime.words[kTask + 20u] == 0u &&
          std::count(runtime.calls.begin(), runtime.calls.end(),
                     "child_position") == 3 &&
          std::count(runtime.calls.begin(), runtime.calls.end(),
                     "normalize") == 0 && runtime.facing_actor == 0u,
          "small delta resets duration and skips rotation");
}

void FaceSignedAngleAndClamp() {
    for (const float target_x : {1.0f, -1.0f}) {
        Runtime runtime;
        runtime.words[kTask + 72u] = kActor;
        runtime.floats[kTask + 60u] = target_x;
        runtime.positions.assign(3, {});
        runtime.normalize_scale = 2.0f;
        Check(pal4::FaceTask::Process(kTask, runtime) == 1u &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "child_position") == 3 &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "normalize") == 2 &&
              runtime.cosine == 0.0f &&
              runtime.facing_actor == kActor &&
              runtime.facing_mode == 0u &&
              runtime.facing.x == 0.0f && runtime.facing.z == 0.0f &&
              std::fabs(runtime.facing.y -
                        (target_x > 0.0f ? 57.29578f : -57.29578f)) < 0.001f,
              "face clamps normalized vectors and chooses signed degrees");
        Check(runtime.calls.back() == "set_facing" &&
              runtime.calls[runtime.calls.size() - 2] == "read_word",
              "face reads actor again before dispatch");
    }
}

void FaceOriginalAcosPrecision() {
    Runtime runtime;
    runtime.words[kTask + 72u]=kActor;
    runtime.floats[kTask + 60u]=1.0f;
    runtime.positions.assign(3,{});
    // Original pair98 Monster2528: cosine BF599E3C, stored yaw43143828.
    // Keep the original double result until the actual degree-store boundary.
    runtime.angle_result=2.586915874473175;
    (void)pal4::FaceTask::Process(kTask,runtime);
    Check(std::bit_cast<std::uint32_t>(runtime.facing.y)==0x43143828u,
        "Face preserves original acos double result through degree conversion");
}

void FaceUnorderedDelta() {
    Runtime runtime;
    runtime.words[kTask + 72u] = kActor;
    runtime.floats[kTask + 60u] =
        std::numeric_limits<float>::quiet_NaN();
    runtime.positions.assign(3, {});
    runtime.allow_nan = true;
    Check(pal4::FaceTask::Process(kTask, runtime) == 1u &&
          std::count(runtime.calls.begin(), runtime.calls.end(),
                     "normalize") == 2 &&
          runtime.facing_actor == kActor,
          "unordered delta follows IDA normalization branch");
}

void ChangeToPrepare() {
    Runtime runtime;
    Check(pal4::ChangeToPrepareActionTask::Process(kTask, runtime) == 1u &&
          runtime.prepare_actor == 0u,
          "missing prepare actor returns one");
    runtime.words[kTask + 60u] = kActor;
    Check(pal4::ChangeToPrepareActionTask::Process(kTask, runtime) == 1u &&
          runtime.prepare_actor == kActor && runtime.prepare_state == 1u &&
          runtime.calls.back() == "set_prepare",
          "prepare task sends state one to actor virtual slot");
}

}  // namespace

int main() {
    try {
        TranslateActor();
        FaceConstructionAndMissingActor();
        FaceDeadzone();
        FaceSignedAngleAndClamp();
        FaceOriginalAcosPrecision();
        FaceUnorderedDelta();
        ChangeToPrepare();
        std::cout << "actor_direction_tasks_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
