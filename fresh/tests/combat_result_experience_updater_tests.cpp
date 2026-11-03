#include "pal4/combat_result_experience_updater.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kElement = 0x4000u;
constexpr std::uint32_t kRecord = 0x7084u;

struct Runtime final : pal4::ICombatResultExperienceRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kElement + 60u, 0x5000u}, {kElement + 64u, 0x7000u},
        {kElement + 68u, 0x6000u}, {kElement + 96u, 0x8000u},
        {0x5014u, 0x9000u}, {0x5018u, 0xa000u}};
    std::unordered_map<std::uint32_t, std::int32_t> integers{
        {0x6000u + 1192u, 2}, {kRecord + 28u, 100},
        {kRecord + 32u, 300}, {kElement + 80u, 10},
        {kElement + 88u, 0}};
    std::unordered_map<std::uint32_t, float> floats{
        {kElement + 72u, 120.0f}, {kElement + 76u, 2.0f},
        {kElement + 84u, 5.0f}};
    std::unordered_map<std::uint32_t, std::string> texts;
    std::vector<std::string> calls;
    std::int32_t metric = 20;
    std::int32_t current_metric = 225;
    std::int32_t next_experience = 250;
    std::uint32_t level_up_count = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.count(address) != 0u, "unexpected pointer read");
        return words.at(address);
    }
    std::int32_t ReadInt(std::uint32_t address) override {
        Check(integers.count(address) != 0u, "unexpected integer read");
        return integers.at(address);
    }
    float ReadFloat(std::uint32_t address) override {
        Check(floats.count(address) != 0u, "unexpected float read");
        return floats.at(address);
    }
    void WriteInt(std::uint32_t address, std::int32_t value) override {
        Check(address == kElement + 80u, "only level is written as int");
        integers[address] = value;
        calls.emplace_back("level");
    }
    void WriteFloat(std::uint32_t address, float value) override {
        Check(address == kElement + 72u ||
                  address == kElement + 84u,
              "only experience fields are written as floats");
        floats[address] = value;
        calls.emplace_back(address == kElement + 72u ?
                           "experience" : "remaining");
    }
    std::int32_t ActorMetric(std::uint32_t actor,
                             std::uint32_t key) override {
        Check(actor == 0x6000u && (key == 4u || key == 5u),
              "actor property slot");
        calls.emplace_back(key == 4u ? "metric" : "metric5");
        return key == 4u ? metric : current_metric;
    }
    std::uint32_t ActorId(std::uint32_t actor) override {
        Check(actor == 0x6000u, "actor identifier source");
        calls.emplace_back("actor-id");
        return 9u;
    }
    std::int32_t NextLevelExperience(std::uint32_t actor_id,
                                     std::int32_t level) override {
        Check(actor_id == 9u && level == 11,
              "database actor and next-level keys");
        calls.emplace_back("database");
        return next_experience;
    }
    std::int32_t FloatToInt(double value) override {
        Check(std::isfinite(value), "finite number conversion");
        return static_cast<std::int32_t>(std::trunc(value));
    }
    void SetWindowText(std::uint32_t window,
                       std::string_view text) override {
        Check(window == 0x9000u || window == 0xa000u,
              "result text window");
        texts[window] = text;
        calls.emplace_back(window == 0x9000u ?
                           "current-text" : "remaining-text");
    }
    void ApplyLevelUp(std::uint32_t row, std::int32_t level,
                      std::uint32_t actor, std::uint32_t hint,
                      std::uint32_t experience_record) override {
        Check(row == 0x5000u && level == 11 && actor == 0x6000u &&
                  hint == 0x8000u && experience_record == kRecord,
              "level-up call fields");
        ++level_up_count;
        calls.emplace_back("level-up");
    }
};

void OrdinaryIncrement() {
    Runtime runtime;
    Check(pal4::CombatResultExperienceUpdater::Update(
              kElement, 2.0f, runtime) == 1u,
          "ordinary update return");
    Check(runtime.floats.at(kElement + 72u) == 124.0f &&
              runtime.floats.at(kElement + 84u) == 1.0f &&
              runtime.texts.at(0x9000u) == "24" &&
              runtime.texts.at(0xa000u) == "1" &&
              runtime.level_up_count == 0u,
          "ordinary experience and text");
    Check(runtime.calls == std::vector<std::string>{
                               "experience", "current-text", "remaining",
                               "metric", "remaining-text"},
          "ordinary update order");
}

void ExperienceCap() {
    Runtime runtime;
    runtime.floats[kElement + 72u] = 295.0f;
    runtime.floats[kElement + 84u] = 50.0f;
    (void)pal4::CombatResultExperienceUpdater::Update(kElement, 5.0f,
                                                       runtime);
    Check(runtime.floats.at(kElement + 72u) == 300.0f &&
              runtime.floats.at(kElement + 84u) == 40.0f &&
              runtime.texts.at(0x9000u) == "200" &&
              runtime.calls.at(0) == "experience" &&
              runtime.calls.at(1) == "experience",
          "current experience caps before text update");
}

void ExperienceCapBeforeFloatRounding() {
    Runtime runtime;
    runtime.integers[kRecord + 32u] = 100;
    runtime.floats[kElement + 72u] =
        std::nextafter(100.0f, 0.0f);
    runtime.floats[kElement + 76u] = 0.00001f;
    runtime.floats[kElement + 84u] = 10.0f;
    (void)pal4::CombatResultExperienceUpdater::Update(kElement, 1.0f,
                                                       runtime);
    Check(runtime.floats.at(kElement + 72u) == 100.0f &&
              runtime.calls.at(0) == "experience" &&
              runtime.calls.at(1) == "experience",
          "x87 comparison clamps even when stored float rounded to cap");
}

void MinimumRemaining() {
    Runtime runtime;
    runtime.metric = 10;
    runtime.floats[kElement + 84u] = 1.0f;
    (void)pal4::CombatResultExperienceUpdater::Update(kElement, 2.0f,
                                                       runtime);
    Check(runtime.floats.at(kElement + 84u) == 0.0f &&
              runtime.integers.at(kElement + 80u) == 10 &&
              runtime.level_up_count == 0u &&
              runtime.texts.at(0xa000u) == "0" &&
              runtime.calls == std::vector<std::string>{
                                   "experience", "current-text", "remaining",
                                   "metric", "remaining", "remaining-text"},
          "minimum remaining clamps without leveling");
}

void LevelUp() {
    Runtime runtime;
    runtime.floats[kElement + 84u] = 3.0f;
    (void)pal4::CombatResultExperienceUpdater::Update(kElement, 2.0f,
                                                       runtime);
    Check(runtime.integers.at(kElement + 80u) == 11 &&
              runtime.floats.at(kElement + 84u) == 126.0f &&
              runtime.texts.at(0xa000u) == "126" &&
              runtime.level_up_count == 1u &&
              runtime.calls == std::vector<std::string>{
                                   "experience", "current-text", "remaining",
                                   "metric", "level", "actor-id", "database",
                                   "remaining", "level-up", "remaining-text"},
          "level-up lookup and callback precede remaining text");
}

void LevelLimit() {
    Runtime runtime;
    runtime.integers[kElement + 80u] = 99;
    runtime.metric = 100;
    runtime.floats[kElement + 84u] = 1.0f;
    (void)pal4::CombatResultExperienceUpdater::Update(kElement, 2.0f,
                                                       runtime);
    Check(runtime.integers.at(kElement + 80u) == 100 &&
              runtime.floats.at(kElement + 84u) == -3.0f &&
              runtime.texts.at(0xa000u) == "-3" &&
              runtime.level_up_count == 0u,
          "level over 99 skips lookup and callback");
}

void CompletionGates() {
    Runtime runtime;
    Check(pal4::CombatResultExperienceUpdater::Complete(
              kElement, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"metric"} &&
              runtime.texts.empty(),
          "level below actor metric returns without text");
    runtime.calls.clear();
    runtime.integers[kElement + 80u] = 20;
    Check(pal4::CombatResultExperienceUpdater::Complete(
              kElement, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"metric"} &&
              runtime.texts.empty(),
          "remaining above minimum returns without text");
    runtime.floats[kElement + 84u] =
        std::numeric_limits<float>::quiet_NaN();
    Check(pal4::CombatResultExperienceUpdater::Complete(
              kElement, runtime) == 0u && runtime.texts.empty(),
          "unordered remaining returns without text");
}

void CompletionText() {
    Runtime runtime;
    runtime.integers[kElement + 80u] = 20;
    runtime.integers[kElement + 88u] = 5;
    Check(pal4::CombatResultExperienceUpdater::Complete(
              kElement, runtime) == 1u &&
              runtime.texts.at(0x9000u) == "125" &&
              runtime.texts.at(0xa000u) == "5" &&
              runtime.calls == std::vector<std::string>{
                                   "metric", "metric5", "current-text",
                                   "remaining-text"},
          "equal minimum completes and updates both windows");
    runtime.calls.clear();
    runtime.texts.clear();
    runtime.floats[kElement + 84u] = 2.0f;
    Check(pal4::CombatResultExperienceUpdater::Complete(
              kElement, runtime) == 1u &&
              runtime.texts.at(0x9000u) == "125" &&
              runtime.texts.at(0xa000u) == "5",
          "remaining below minimum also completes");
}

}  // namespace

int main() {
    try {
        OrdinaryIncrement();
        ExperienceCap();
        ExperienceCapBeforeFloatRounding();
        MinimumRemaining();
        LevelUp();
        LevelLimit();
        CompletionGates();
        CompletionText();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
