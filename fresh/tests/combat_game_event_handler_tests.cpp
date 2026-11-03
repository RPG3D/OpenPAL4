#include "pal4/combat_game_event_handler.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kDatabase = 0x2000u;
constexpr std::uint32_t kCollection = 0x3000u;
constexpr std::uint32_t kIndex = 0x4000u;
constexpr std::uint32_t kRecords = 0x5000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatGameEventHandlerRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::uint32_t database_calls = 0u;
    std::uint32_t router_calls = 0u;
    std::uint32_t register_calls = 0u;
    std::uint32_t remove_calls = 0u;
    std::uint32_t combat_calls = 0u;
    bool inactive = false;

    Runtime() {
        words[kDatabase + 0x30u] = kCollection;
        words[kCollection + 8u] = kIndex;
        words[kCollection + 12u] = kIndex + 32u;
        words[kCollection + 0x60u] = kRecords;
        words[kIndex] = 0xffffffffu;
        words[kIndex + 4u] = 2u;
        words[kIndex + 8u] = 0u;
        words[kIndex + 12u] = 0u;
        words[kIndex + 16u] = 1u;
        words[kIndex + 20u] = 1u;
        words[kIndex + 24u] = 2u;
        words[kIndex + 28u] = 3u;
        for (std::uint32_t i = 0u; i < 26u; ++i) {
            words[kRecords + 0x18u + i * 4u] = 100u + i;
            words[kRecords + 0x80u + 0x18u + i * 4u] = 200u + i;
        }
        constexpr std::array<std::uint32_t, 5> thresholds{
            9u, 70u, 50u, 30u, 10u};
        for (std::uint32_t i = 0u; i < thresholds.size(); ++i)
            words[kRecords + 4u + i * 4u] = thresholds[i];
    }

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        writes.emplace_back(address, value);
        words[address] = value;
    }
    std::uint32_t GetGameDatabase() override {
        ++database_calls;
        for (std::uint32_t i = 0u; i < 34u; ++i)
            Check(words[kObject + i * 4u] == 0u,
                  "constructor clears all handler slots before stats lookup");
        return kDatabase;
    }
    std::uint32_t GetCombatSystem() override {
        ++combat_calls;
        return 0x6000u;
    }
    bool IsCombatSystemInactive(std::uint32_t combat_system) override {
        Check(combat_system == 0x6000u,
              "rank tests the requested combat system");
        return inactive;
    }
    void RemoveAll(std::uint32_t owner) override {
        Check(owner == kObject && database_calls == 2u &&
                  words[kObject + 0x258u] == 9u,
              "handler registration follows statistics initialization");
        ++remove_calls;
    }
    std::uint32_t GetRouter() override {
        ++router_calls;
        return 0x7000u;
    }
    std::uint32_t Register(
        std::uint32_t router, std::uint32_t mode,
        std::uint32_t descriptor, std::uint32_t owner,
        std::uint32_t callback) override {
        Check(router == 0x7000u && owner == kObject &&
                  mode >= 1u && descriptor != 0u && callback != 0u,
              "constructor forwards each handler registration");
        return 0x8000u + ++register_calls;
    }
};

void ConstructorAndRecords() {
    Runtime runtime;
    for (std::uint32_t i = 0u; i < 155u; ++i)
        runtime.words[kObject + i * 4u] = 0xdeadbeefu;
    Check(pal4::CombatGameEventHandler::Construct(kObject, runtime) ==
              kObject && runtime.database_calls == 2u &&
              runtime.remove_calls == 1u &&
              runtime.router_calls == 17u &&
              runtime.register_calls == 17u,
          "constructor initializes stats and all event subscriptions");
    for (std::uint32_t i = 0u; i < 34u; ++i)
        Check(runtime.writes[i] ==
                  std::pair{kObject + i * 4u, 0u},
              "constructor clears 17 handle/descriptor pairs first");
    for (std::uint32_t i = 0u; i < 26u; ++i) {
        Check(runtime.words[kObject + 0x88u + i * 4u] == 0u &&
                  runtime.words[kObject + 0xf0u + i * 4u] == 0u &&
                  runtime.words[kObject + 0x158u + i * 4u] == 200u + i &&
                  runtime.words[kObject + 0x1c0u + i * 4u] == 100u + i,
              "stats clear delta arrays and copy level-one/zero records");
    }
    for (std::uint32_t i = 0u; i < 12u; ++i)
        Check(runtime.words[kObject + 0x228u + i * 4u] == 0u,
              "stats clear all twelve trailing words");
    constexpr std::array<std::uint32_t, 5> tail{
        9u, 70u, 50u, 30u, 10u};
    for (std::uint32_t i = 0u; i < tail.size(); ++i)
        Check(runtime.words[kObject + 0x258u + i * 4u] == tail[i],
              "stats copy five score and rank words from the first record");
    Check(runtime.words[kObject] == 0x8001u &&
              runtime.words[kObject + 0x80u] == 0x8011u,
          "constructor's handler layout matches the registration result");
}

void MissingDatabaseRecords() {
    Runtime runtime;
    runtime.words[kCollection + 12u] = kIndex + 16u;
    runtime.words[kObject + 0x158u] = 0x1111u;
    runtime.words[kObject + 0x1c0u] = 0x2222u;
    runtime.words[kObject + 0x258u] = 0x3333u;
    pal4::CombatGameStats::Initialize(kObject, runtime);
    Check(runtime.database_calls == 2u &&
              runtime.words[kObject + 0x88u] == 0u &&
              runtime.words[kObject + 0xf0u] == 0u &&
              runtime.words[kObject + 0x228u] == 0u &&
              runtime.words[kObject + 0x158u] == 0x1111u &&
              runtime.words[kObject + 0x1c0u] == 0x2222u &&
              runtime.words[kObject + 0x258u] == 0x3333u,
          "missing level-one record leaves source-dependent fields untouched");
}

void TotalAndRankBoundaries() {
    Runtime runtime;
    pal4::CombatGameStats::Initialize(kObject, runtime);
    Check(pal4::CombatGameStats::CalculateTotal(kObject, runtime) == 9u,
          "initial total is copied base value");
    runtime.words[kObject + 0x88u] = 0xffffffffu;
    runtime.words[kObject + 0x8cu] = 5u;
    Check(pal4::CombatGameStats::CalculateTotal(kObject, runtime) == 13u,
          "26-word total uses native 32-bit wraparound");
    constexpr std::array<std::pair<std::int32_t, std::uint32_t>, 9> cases{{
        {71, 0u}, {70, 1u}, {51, 1u}, {50, 2u}, {31, 2u},
        {30, 3u}, {11, 3u}, {10, 4u}, {-1, 4u}}};
    for (const auto& [score, expected] : cases)
        Check(pal4::CombatGameStats::GetBattleRank(
                  kObject, score, runtime) == expected,
              "rank uses signed ordered threshold boundaries");
    runtime.inactive = true;
    Check(pal4::CombatGameStats::GetBattleRank(
              kObject, 1000, runtime) == 4u &&
              runtime.combat_calls == cases.size() + 1u,
          "inactive combat returns fourth rank before reading thresholds");
}

void StatUpdateAndEvent() {
    struct UpdateRuntime final : pal4::ICombatGameStatUpdateRuntime {
        std::unordered_map<std::uint32_t, std::uint32_t> words;
        std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
        std::uint32_t router_calls = 0u;
        std::uint32_t ReadWord(std::uint32_t address) override {
            return words[address];
        }
        void WriteWord(std::uint32_t address,
                       std::uint32_t value) override {
            writes.emplace_back(address, value);
            words[address] = value;
        }
        std::uint32_t GetScriptEventRouter() override {
            ++router_calls;
            return 0x7000u;
        }
        std::uint32_t ProcessScoreChanged(
            std::uint32_t router, std::uint32_t descriptor,
            std::uint32_t delta, std::uint32_t mode) override {
            Check(router == 0x7000u && descriptor == 0x8b8b50u &&
                      mode == 1u && !writes.empty(),
                  "score_changed is dispatched after stat writes");
            last_delta = delta;
            ++event_calls;
            return 0x9000u + event_calls;
        }
        std::uint32_t last_delta = 0u;
        std::uint32_t event_calls = 0u;
    } runtime;

    constexpr auto score = kObject + 0x88u + 3u * 4u;
    constexpr auto count = kObject + 0xf0u + 3u * 4u;
    constexpr auto limit = kObject + 0x158u + 3u * 4u;
    constexpr auto factor = kObject + 0x1c0u + 3u * 4u;
    runtime.words[score] = 3u;
    runtime.words[count] = 5u;
    runtime.words[limit] = 10u;
    runtime.words[factor] = 4u;
    Check(pal4::CombatGameStats::Update(
              kObject, 3u, 2u, runtime) == 0x9001u &&
              runtime.words[score] == 10u &&
              runtime.words[count] == 6u &&
              runtime.last_delta == 8u &&
              runtime.writes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {score, 11u}, {count, 6u}, {score, 10u}},
          "positive score clamps at limit after counter increment");

    runtime.writes.clear();
    runtime.words[score] = 0xfffffffbu;
    runtime.words[limit] = 0xfffffff9u;
    runtime.words[factor] = 0xfffffffcu;
    Check(pal4::CombatGameStats::Update(
              kObject, 3u, 1u, runtime) == 0x9002u &&
              runtime.words[score] == 0xfffffff9u &&
              runtime.words[count] == 7u &&
              runtime.last_delta == 0xfffffffcu &&
              runtime.writes.back() ==
                  std::pair{score, 0xfffffff9u},
          "negative score clamps to its signed lower limit");

    runtime.writes.clear();
    runtime.words[score] = 1u;
    runtime.words[limit] = 20u;
    runtime.words[factor] = 2u;
    Check(pal4::CombatGameStats::Update(
              kObject, 3u, 1u, runtime) == 0x9003u &&
              runtime.words[score] == 3u &&
              runtime.writes.size() == 2u &&
              runtime.event_calls == 3u &&
              runtime.router_calls == 3u,
          "in-range score is kept and still emits score_changed");
}

}  // namespace

int main() {
    try {
        ConstructorAndRecords();
        MissingDatabaseRecords();
        TotalAndRankBoundaries();
        StatUpdateAndEvent();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
