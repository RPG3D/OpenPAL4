#include "pal4/pal_actor_control_damage_feedback.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

std::uint32_t Bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IActorControlDamageFeedbackAnimationRuntime {
    pal4::DamageFeedbackRecord* record{};
    std::vector<std::string> calls;
    bool alter_rate{};

    void UpdateAnimationProgress(float delta) override {
        Check(record != nullptr && delta == 0.25f,
              "base animation receives delta first");
        calls.emplace_back("base");
        if (alter_rate) {
            record->words[18u] = Bits(4.0f);
        }
    }
    std::uint16_t FpuCompareStatus(long double left,
                                   long double right) override {
        calls.emplace_back("compare");
        return IActorControlDamageFeedbackAnimationRuntime::
            FpuCompareStatus(left, right);
    }
};

struct DeleteRuntime final : pal4::IActorControlDamageFeedbackDeleteRuntime {
    std::vector<std::string> calls;
    void Cleanup(std::uint32_t object) override {
        Check(object == 0x456700u, "feedback cleanup object");
        calls.emplace_back("cleanup");
    }
    void Free(std::uint32_t object) override {
        Check(object == 0x456700u, "feedback free object");
        calls.emplace_back("free");
    }
};

void Initialize(pal4::DamageFeedbackRecord& record) {
    record = {};
    record.words[9u] = Bits(-7.0f);
    record.words[10u] = Bits(10.0f);
    record.words[18u] = Bits(2.0f);
    record.words[19u] = Bits(3.0f);
    record.words[20u] = Bits(1.0f);
    record.words[21u] = Bits(0.5f);
}

}  // namespace

int main() {
    DeleteRuntime delete_runtime;
    Check(pal4::PalActorControlDamageFeedback::Delete(
              0x456700u, 2u, delete_runtime) == 0x456700u &&
              delete_runtime.calls == std::vector<std::string>{"cleanup"},
          "feedback destructor ignores non-low flags");
    delete_runtime.calls.clear();
    Check(pal4::PalActorControlDamageFeedback::Delete(
              0x456700u, 3u, delete_runtime) == 0x456700u &&
              delete_runtime.calls ==
                  std::vector<std::string>{"cleanup", "free"},
          "feedback destructor frees only after cleanup");

    pal4::DamageFeedbackRecord record{};
    Runtime runtime;
    runtime.record = &record;

    Initialize(record);
    runtime.alter_rate = true;
    Check(pal4::PalActorControlDamageFeedback::TickAnimation(
              record, 0.25f, runtime) == 0x100u &&
              record.words[22u] == Bits(0.25f) &&
              record.words[10u] == Bits(11.0f) &&
              record.words[9u] == record.words[10u] &&
              runtime.calls == std::vector<std::string>{
                  "base", "compare", "compare"},
          "positive timer uses base callback's updated rate");

    Initialize(record);
    runtime.calls.clear();
    runtime.alter_rate = false;
    record.words[22u] = Bits(0.75f);
    Check(pal4::PalActorControlDamageFeedback::TickAnimation(
              record, 0.25f, runtime) == 0x4000u &&
              record.words[22u] == Bits(1.0f) &&
              record.words[10u] == Bits(10.5f),
          "positive timer equality does not set terminal sentinel");

    Initialize(record);
    runtime.calls.clear();
    record.words[22u] = Bits(0.9f);
    Check(pal4::PalActorControlDamageFeedback::TickAnimation(
              record, 0.25f, runtime) == 0u &&
              record.words[22u] == 0xb8d1b717u &&
              record.words[10u] == Bits(10.5f) &&
              record.words[9u] == record.words[10u],
          "elapsed timer above threshold sets exact sentinel bits");

    Initialize(record);
    runtime.calls.clear();
    record.words[22u] = Bits(-0.25f);
    Check(pal4::PalActorControlDamageFeedback::TickAnimation(
              record, 0.25f, runtime) == 0x100u &&
              record.words[22u] == Bits(-0.5f) &&
              record.words[10u] == Bits(9.25f) &&
              record.words[9u] == record.words[10u],
          "negative timer below threshold counts down and fades value");

    for (const float timer : {-0.5f, -0.75f}) {
        Initialize(record);
        runtime.calls.clear();
        record.words[22u] = Bits(timer);
        Check(pal4::PalActorControlDamageFeedback::TickAnimation(
                  record, 0.25f, runtime) == Bits(10.0f) &&
                  record.words[22u] == Bits(timer) &&
                  record.words[9u] == Bits(10.0f) &&
                  runtime.calls == std::vector<std::string>{
                      "base", "compare", "compare"},
              "negative timer at or beyond threshold only copies value");
    }

    Initialize(record);
    runtime.calls.clear();
    record.words[22u] = Bits(NAN);
    Check(pal4::PalActorControlDamageFeedback::TickAnimation(
              record, 0.25f, runtime) == Bits(10.0f) &&
              std::isnan(std::bit_cast<float>(record.words[22u])) &&
              record.words[9u] == Bits(10.0f),
          "unordered comparison follows completion copy path");
}
