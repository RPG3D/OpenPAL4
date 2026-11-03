#include "pal4/ai_action_task.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kActor = 0x5000u;

struct Runtime final : pal4::IAiActionAnimationRuntime {
    std::uint32_t actor = kActor;
    std::uint32_t animation_id = 42u;
    std::uint32_t record = 0x6000u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kTask + 4u, "animation reads task actor");
        calls.emplace_back("read-actor");
        return actor;
    }
    std::uint32_t ActorAnimationId(std::uint32_t value) override {
        Check(value == kActor, "animation ID uses task actor");
        calls.emplace_back("animation-id");
        return animation_id;
    }
    std::uint32_t FindAnimationRecord(std::uint32_t id) override {
        Check(id == animation_id, "lookup receives animation ID");
        calls.emplace_back("find");
        return record;
    }
    void ReportMissingActor() override {
        calls.emplace_back("missing-actor");
    }
    void BeginMissingAnimation() override {
        calls.emplace_back("begin-missing-animation");
    }
    void ReportMissingAnimation(std::uint32_t id) override {
        Check(id == animation_id, "diagnostic receives repeated ID");
        calls.emplace_back("missing-animation");
    }
};

void AnimationIdPaths() {
    Runtime runtime;
    Check(pal4::AiActionAnimation::GetAnimationId(
              kTask, runtime) == 42u &&
          runtime.calls == std::vector<std::string>{
              "read-actor", "animation-id"},
          "actor virtual slot returns animation ID");
    runtime = Runtime{};
    runtime.actor = 0u;
    Check(pal4::AiActionAnimation::GetAnimationId(
              kTask, runtime) == 0xffffffffu &&
          runtime.calls == std::vector<std::string>{
              "read-actor", "missing-actor"},
          "null actor reports and returns minus one");
}

void RecordPaths() {
    Runtime runtime;
    Check(pal4::AiActionAnimation::FindRecord(
              kTask, runtime) == 0x6000u &&
          runtime.calls == std::vector<std::string>{
              "read-actor", "read-actor", "animation-id", "find"},
          "successful lookup reads actor then uses ID helper");
    runtime = Runtime{};
    runtime.record = 0u;
    Check(pal4::AiActionAnimation::FindRecord(
              kTask, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{
              "read-actor", "read-actor", "animation-id", "find",
              "begin-missing-animation", "read-actor", "animation-id",
              "missing-animation"},
          "failed lookup repeats ID query before diagnostic");
    runtime = Runtime{};
    runtime.actor = 0u;
    Check(pal4::AiActionAnimation::FindRecord(
              kTask, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{
              "read-actor", "missing-actor"},
          "null actor stops before database lookup");
}

}  // namespace

int main() {
    try {
        AnimationIdPaths();
        RecordPaths();
        std::cout << "ai_action_animation_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
