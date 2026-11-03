#include "pal4/combat_system_update.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kObject = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatSystemUpdateRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t random = 499u;
    std::uint32_t candidate = 0x3000u;
    std::int32_t count = 0;
    bool throw_action = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    std::uint32_t RandomNumber() override {
        calls.emplace_back("random");
        return random;
    }
    std::uint32_t GetActorManager() override {
        calls.emplace_back("manager");
        return 0x2000u;
    }
    std::uint32_t SelectQuickEndCandidate(
        std::uint32_t collection, std::uint32_t bucket) override {
        Check(collection == 0x2020u && bucket == random % 1000u,
              "quick-end selection collection and bucket");
        calls.emplace_back("select");
        return candidate;
    }
    std::int32_t GetCandidateCount(std::uint32_t selected) override {
        Check(selected == candidate, "candidate count target");
        calls.emplace_back("count");
        return count;
    }
    std::uint32_t GetCandidateActor(std::uint32_t selected) override {
        Check(selected == candidate, "candidate actor target");
        calls.emplace_back("actor");
        return 0x4000u;
    }
    void RunCandidateAction(std::uint32_t actor) override {
        Check(actor == 0x4000u, "candidate action actor");
        calls.emplace_back("action");
        if (throw_action) throw std::runtime_error("action failed");
    }
    void ReleaseCandidate(std::uint32_t selected,
                          std::uint32_t flag) override {
        Check(selected == candidate && flag == 1u,
              "selected candidate deleting flag");
        calls.emplace_back("release");
    }
    std::uint32_t DispatchScriptEvent(std::string_view name,
                                      std::uint32_t payload) override {
        calls.emplace_back("event:" + std::string(name) + ":" +
                           std::to_string(payload));
        return 0x9abcu;
    }
    void UpdateTimedEffects(float delta) override {
        Check(delta == 1.5f, "timed effect delta");
        calls.emplace_back("effects");
    }
    void UpdateActorData(std::uint32_t manager,
                         std::uint32_t delta_bits) override {
        Check(manager == 0x2000u &&
                  delta_bits == std::bit_cast<std::uint32_t>(1.5f),
              "actor data manager and float bits");
        calls.emplace_back("actor-data");
    }
    void DispatchSystemUpdate(std::uint32_t manager,
                              std::uint32_t delta_bits) override {
        Check(manager == 0x5000u &&
                  delta_bits == std::bit_cast<std::uint32_t>(1.5f),
              "system update manager and float bits");
        calls.emplace_back("system");
    }
    std::uint32_t UpdateConsoleAnimation(float delta) override {
        Check(delta == 1.5f, "animation delta");
        calls.emplace_back("animation");
        return 0x1234u;
    }
    void UpdateWorldAndInput(float delta) override {
        Check(delta == 1.5f, "world update delta");
        calls.emplace_back("world");
    }
    std::uint32_t UpdateGameObject(float delta) override {
        Check(delta == 1.5f, "game object update delta");
        calls.emplace_back("game-object");
        return 0x5678u;
    }
    void CloseCombatUi(std::uint32_t object) override {
        Check(object == kObject, "close UI combat object");
        calls.emplace_back("close-ui");
    }
    void EndCombat(std::uint32_t object) override {
        Check(object == kObject, "end combat object");
        calls.emplace_back("end-combat");
    }
};

void StateUpdates() {
    for (std::uint32_t state = 1u; state <= 5u; ++state) {
        Runtime runtime;
        runtime.words[kObject + 508u] = state;
        runtime.words[kObject + 512u] = std::bit_cast<std::uint32_t>(2.0f);
        runtime.words[kObject + 396u] = 0x5000u;
        Check(pal4::CombatSystemUpdate::Update(
                  kObject, 1.5f, 0x55u, runtime) == 0x5678u,
              "states 1..5 return game-object update result");
        const auto expected_time = state == 1u ? 2.0f : 3.5f;
        Check(std::bit_cast<float>(runtime.words[kObject + 512u]) ==
                  expected_time, "only active states accumulate delta");
        const std::vector<std::string> expected = state == 1u ?
            std::vector<std::string>{"effects", "manager", "actor-data",
                                     "system", "animation", "world",
                                     "game-object"} :
            std::vector<std::string>{"write:4608", "effects", "manager",
                                     "actor-data", "system", "animation",
                                     "world", "game-object"};
        Check(runtime.calls == expected, "state 1..5 update order");

        runtime.calls.clear();
        runtime.bytes[kObject + 516u] = 9u;
        runtime.words[kObject + 512u] = std::bit_cast<std::uint32_t>(1.0f);
        Check(pal4::CombatSystemUpdate::Update(
                  kObject, 1.5f, 0u, runtime) == 9u &&
                  runtime.calls.back() == "animation",
              "inactive states 1..5 return flag and skip world calls");
    }
}

void StateSixAndSeven() {
    Runtime runtime;
    runtime.words[kObject + 508u] = 6u;
    Check(pal4::CombatSystemUpdate::Update(
              kObject, 1.5f, 0u, runtime) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "effects", "world", "game-object", "manager",
                  "actor-data", "animation"},
          "state 6 updates world before actor data");
    runtime.calls.clear();
    runtime.bytes[kObject + 516u] = 2u;
    Check(pal4::CombatSystemUpdate::Update(
              kObject, 1.5f, 0u, runtime) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "effects", "manager", "actor-data", "animation"},
          "inactive state 6 skips world and game object");

    runtime.calls.clear();
    runtime.bytes[kObject + 516u] = 0u;
    runtime.words[kObject + 508u] = 7u;
    runtime.words[kObject + 504u] = 0x1357u;
    Check(pal4::CombatSystemUpdate::Update(
              kObject, 1.5f, 0u, runtime) == 0x9abcu &&
              runtime.calls == std::vector<std::string>{
                  "close-ui", "end-combat", "event:close_system:4951"},
          "state 7 closes UI, ends combat and dispatches payload");

    runtime.calls.clear();
    runtime.words[kObject + 508u] = 0u;
    Check(pal4::CombatSystemUpdate::Update(
              kObject, 1.5f, 0u, runtime) == 0xffffffffu &&
              runtime.calls.empty(),
          "unknown state returns wrapped state minus one");
}

void QuickEnd() {
    Runtime direct;
    direct.words[kObject + 508u] = 0u;
    direct.words[kObject + 512u] = std::bit_cast<std::uint32_t>(20.0f);
    direct.bytes[kObject + 516u] = 1u;
    Check(pal4::CombatSystemUpdate::Update(
              kObject, 1.5f, 0x55u, direct) == 0xffffffffu &&
              direct.calls == std::vector<std::string>{
                  "random", "event:quick_end:85"},
          "bucket 499 dispatches quick_end directly at inclusive threshold");

    Runtime below;
    below.words[kObject + 512u] = std::bit_cast<std::uint32_t>(19.99f);
    below.bytes[kObject + 516u] = 1u;
    Check(pal4::CombatSystemUpdate::Update(
              kObject, 1.5f, 0u, below) == 0xffffffffu &&
              below.calls.empty(), "below 20 seconds skips quick_end");

    Runtime fallback;
    fallback.random = 1500u;
    fallback.words[kObject + 512u] =
        std::bit_cast<std::uint32_t>(20.0f);
    fallback.bytes[kObject + 516u] = 1u;
    Check(pal4::CombatSystemUpdate::Update(
              kObject, 1.5f, 0x44u, fallback) == 0xffffffffu &&
              fallback.calls == std::vector<std::string>{
                  "random", "manager", "select", "count",
                  "event:quick_end:68", "release"},
          "candidate count zero falls back to event then releases");

    Runtime action;
    action.random = 999u;
    action.count = 1;
    action.words[kObject + 512u] =
        std::bit_cast<std::uint32_t>(20.0f);
    action.bytes[kObject + 516u] = 1u;
    Check(pal4::CombatSystemUpdate::Update(
              kObject, 1.5f, 0u, action) == 0xffffffffu &&
              action.calls == std::vector<std::string>{
                  "random", "manager", "select", "count", "actor",
                  "action", "release"},
          "positive candidate count runs action then releases");
    action.calls.clear();
    action.throw_action = true;
    bool raised = false;
    try {
        static_cast<void>(pal4::CombatSystemUpdate::Update(
            kObject, 1.5f, 0u, action));
    } catch (const std::runtime_error&) {
        raised = true;
    }
    Check(raised && action.calls.back() == "release",
          "candidate releases when action throws");
}

}  // namespace

int main() {
    try {
        StateUpdates();
        StateSixAndSeven();
        QuickEnd();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
