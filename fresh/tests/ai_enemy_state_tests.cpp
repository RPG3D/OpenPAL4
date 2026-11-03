#include "pal4/ai_action_task.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kRecord = 0x5000u;
constexpr std::uint32_t kActor = 0x6000u;
constexpr std::uint32_t kEnemy1 = 0x7000u;
constexpr std::uint32_t kEnemy2 = 0x8000u;
constexpr std::uint32_t kIterator = 0x9000u;

struct Runtime final : pal4::IAiActorStateRuntime {
    std::map<std::uint32_t, float> fields{
        {kRecord + 32u, 0.5f}, {kRecord + 36u, 0.2f},
        {kRecord + 40u, 0.3f}, {kRecord + 44u, 0.6f}};
    std::map<std::uint32_t, std::int32_t> hp{
        {kEnemy1, 80}, {kEnemy2, 80}};
    std::vector<std::uint32_t> enemies{kEnemy1, kEnemy2};
    std::set<std::uint32_t> dead;
    std::size_t cursor = 0u;
    std::uint32_t reset_count = 0u;
    std::uint32_t release_count = 0u;
    std::uint32_t missing_count = 0u;
    bool throw_log = false;
    std::vector<pal4::AiActorStateLog> logs;
    std::vector<std::string> messages;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kTask + 4u, "enemy query reads task actor");
        calls.emplace_back("actor");
        return kActor;
    }
    float ReadFloat(std::uint32_t address) override {
        Check(fields.count(address) == 1u,
              "enemy query uses record threshold offset");
        calls.emplace_back("threshold");
        return fields.at(address);
    }
    std::int32_t ConvertToInt(float value) override {
        calls.emplace_back("convert");
        return static_cast<std::int32_t>(value);
    }
    std::int32_t TurnCount(std::uint32_t) override {
        throw std::runtime_error("enemy query must not read turn count");
    }
    std::int32_t CurrentStat(std::uint32_t actor,
                             std::uint32_t stat) override {
        Check(stat == 7u && hp.count(actor) == 1u,
              "enemy HP uses stat 7");
        calls.emplace_back("hp");
        return hp.at(actor);
    }
    std::int32_t MaxStat(std::uint32_t actor,
                         std::uint32_t stat) override {
        Check(stat == 0u && hp.count(actor) == 1u,
              "enemy maximum HP uses stat 0");
        calls.emplace_back("max-hp");
        return 100;
    }
    std::uint32_t CreateActorIterator(
        std::uint32_t actor, pal4::AiActorGroup group) override {
        Check(actor == kActor && group == pal4::AiActorGroup::Enemy,
              "enemy iterator uses actor virtual +472");
        cursor = 0u;
        calls.emplace_back("create-enemies");
        return kIterator;
    }
    std::uint32_t ActorCount(std::uint32_t iterator) override {
        Check(iterator == kIterator, "enemy iterator count");
        calls.emplace_back("count");
        return static_cast<std::uint32_t>(enemies.size());
    }
    bool ActorEnd(std::uint32_t iterator) override {
        Check(iterator == kIterator, "enemy iterator end");
        return cursor == enemies.size();
    }
    std::uint32_t ActorCurrent(std::uint32_t iterator) override {
        Check(iterator == kIterator && cursor < enemies.size(),
              "current enemy cursor");
        calls.emplace_back("current");
        return enemies[cursor];
    }
    void ActorNext(std::uint32_t iterator) override {
        Check(iterator == kIterator && cursor < enemies.size(),
              "enemy cursor advance");
        ++cursor;
        calls.emplace_back("next");
    }
    void ActorReset(std::uint32_t iterator) override {
        Check(iterator == kIterator, "enemy cursor reset");
        cursor = 0u;
        ++reset_count;
        calls.emplace_back("reset");
    }
    bool IsDead(std::uint32_t actor) override {
        Check(actor == kEnemy1 || actor == kEnemy2,
              "dead-state probe uses enemy actor");
        calls.emplace_back("dead");
        return dead.count(actor) != 0u;
    }
    void ReleaseActorIterator(std::uint32_t iterator) noexcept override {
        if (iterator == kIterator) ++release_count;
        calls.emplace_back("release");
    }
    void ReportMissingRecord() override {
        ++missing_count;
        calls.emplace_back("missing");
    }
    void LogState(std::uint32_t task,
                  std::string_view source,
                  std::string_view message,
                  const pal4::AiActorStateLog& details) override {
        Check(task == kTask && source == "cbActorAi::getEnemyState",
              "enemy-state log identity");
        Check(message.find("return EAES_") == 0u,
              "enemy-state log literal");
        logs.push_back(details);
        messages.emplace_back(message);
        calls.emplace_back("log");
        if (throw_log) throw std::runtime_error("log failure");
    }
};

pal4::AiEnemyState Classify(Runtime& runtime,
                             std::uint32_t record = kRecord) {
    return pal4::AiEnemyStateClassifier::Classify(
        kTask, record, runtime);
}

void AllStates() {
    Runtime runtime;
    Check(Classify(runtime, 0u) == pal4::AiEnemyState::MissingRecord &&
          runtime.missing_count == 1u &&
          runtime.calls == std::vector<std::string>{"missing"},
          "null AI record returns four before actor read");

    runtime = Runtime{};
    runtime.dead.insert(kEnemy1);
    Check(Classify(runtime) == pal4::AiEnemyState::Slaughtered &&
          runtime.logs[0].dead_count == 1u &&
          runtime.logs[0].team_count == 2u &&
          runtime.logs[0].dead_rate == 0.5 &&
          runtime.reset_count == 0u &&
          runtime.release_count == 1u,
          "dead fraction threshold equality returns zero");

    runtime = Runtime{};
    runtime.fields[kRecord + 32u] = 0.8f;
    runtime.hp[kEnemy1] = 20;
    Check(Classify(runtime) == pal4::AiEnemyState::InDanger &&
          runtime.logs[0].actor == kEnemy1 &&
          runtime.logs[0].hp_rate == 0.2 &&
          runtime.reset_count == 1u &&
          runtime.release_count == 1u,
          "first enemy danger threshold includes equality");

    runtime = Runtime{};
    runtime.fields[kRecord + 32u] = 0.8f;
    runtime.hp[kEnemy1] = 40;
    Check(Classify(runtime) == pal4::AiEnemyState::Wounded &&
          runtime.logs[0].actor == kEnemy1 &&
          runtime.logs[0].lower_threshold == 0.3f &&
          runtime.logs[0].upper_threshold == 0.6f &&
          runtime.reset_count == 2u &&
          runtime.release_count == 1u,
          "second scan finds enemy in wounded HP interval");

    runtime = Runtime{};
    runtime.fields[kRecord + 32u] = 0.8f;
    Check(Classify(runtime) == pal4::AiEnemyState::Normal &&
          runtime.messages[0] == "return EAES_NORMAL" &&
          runtime.reset_count == 2u &&
          runtime.release_count == 1u,
          "no enemy match returns three after two scans");
}

void BoundariesAndCleanup() {
    Runtime runtime;
    runtime.enemies.clear();
    Check(Classify(runtime) == pal4::AiEnemyState::Normal &&
          runtime.release_count == 1u,
          "empty enemy collection has unordered dead fraction");

    runtime = Runtime{};
    runtime.fields[kRecord + 32u] =
        std::numeric_limits<float>::quiet_NaN();
    Check(Classify(runtime) == pal4::AiEnemyState::Normal &&
          runtime.reset_count == 2u,
          "unordered dead threshold enters HP scans");

    runtime = Runtime{};
    runtime.fields[kRecord + 32u] = 0.8f;
    runtime.hp[kEnemy1] = 60;
    Check(Classify(runtime) == pal4::AiEnemyState::Wounded,
          "enemy wounded upper threshold includes equality");

    runtime = Runtime{};
    runtime.dead.insert(kEnemy1);
    runtime.throw_log = true;
    bool threw = false;
    try {
        (void)Classify(runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.release_count == 1u &&
          runtime.calls.back() == "release",
          "enemy iterator releases on logging exception");
}

}  // namespace

int main() {
    try {
        AllStates();
        BoundariesAndCleanup();
        std::cout << "ai_enemy_state_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
