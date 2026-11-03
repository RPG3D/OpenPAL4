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
constexpr std::uint32_t kMate1 = 0x7000u;
constexpr std::uint32_t kMate2 = 0x8000u;
constexpr std::uint32_t kIterator = 0x9000u;

struct Runtime final : pal4::IAiActorStateRuntime {
    std::map<std::uint32_t, float> fields{
        {kRecord + 4u, 0.2f}, {kRecord + 8u, 0.3f},
        {kRecord + 12u, 0.6f}, {kRecord + 16u, 0.2f},
        {kRecord + 20u, 0.5f}, {kRecord + 24u, 0.3f},
        {kRecord + 28u, 0.6f}, {kRecord + 48u, 99.0f},
        {kRecord + 52u, 0.0f}};
    std::map<std::uint32_t, std::int32_t> hp{
        {kActor, 80}, {kMate1, 80}, {kMate2, 80}};
    std::map<std::uint32_t, std::int32_t> max_hp{
        {kActor, 100}, {kMate1, 100}, {kMate2, 100}};
    std::vector<std::uint32_t> team{kMate1, kMate2};
    std::set<std::uint32_t> dead;
    std::int32_t turn = 1;
    std::size_t cursor = 0u;
    std::uint32_t release_count = 0u;
    std::uint32_t reset_count = 0u;
    std::uint32_t diagnostic_count = 0u;
    bool throw_log = false;
    std::vector<pal4::AiActorStateLog> logs;
    std::vector<std::string> messages;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kTask + 4u, "task actor offset");
        calls.emplace_back("actor");
        return kActor;
    }
    float ReadFloat(std::uint32_t address) override {
        Check(fields.count(address) == 1u, "known AI state field offset");
        calls.emplace_back("field");
        return fields.at(address);
    }
    std::int32_t ConvertToInt(float value) override {
        calls.emplace_back("convert");
        return static_cast<std::int32_t>(value);
    }
    std::int32_t TurnCount(std::uint32_t actor) override {
        Check(actor == kActor, "turn count actor");
        calls.emplace_back("turn");
        return turn;
    }
    std::int32_t CurrentStat(std::uint32_t actor,
                             std::uint32_t stat) override {
        Check(stat == 7u && hp.count(actor) == 1u,
              "current HP virtual slot and stat ID");
        calls.emplace_back("hp");
        return hp.at(actor);
    }
    std::int32_t MaxStat(std::uint32_t actor,
                         std::uint32_t stat) override {
        Check(stat == 0u && max_hp.count(actor) == 1u,
              "maximum HP virtual slot and stat ID");
        calls.emplace_back("max-hp");
        return max_hp.at(actor);
    }
    std::uint32_t CreateActorIterator(
        std::uint32_t actor, pal4::AiActorGroup group) override {
        Check(actor == kActor && group == pal4::AiActorGroup::Team,
              "team iterator uses task actor");
        cursor = 0u;
        calls.emplace_back("create-team");
        return kIterator;
    }
    std::uint32_t ActorCount(std::uint32_t iterator) override {
        Check(iterator == kIterator, "team count iterator");
        calls.emplace_back("count");
        return static_cast<std::uint32_t>(team.size());
    }
    bool ActorEnd(std::uint32_t iterator) override {
        Check(iterator == kIterator, "team end iterator");
        return cursor == team.size();
    }
    std::uint32_t ActorCurrent(std::uint32_t iterator) override {
        Check(iterator == kIterator && cursor < team.size(),
              "current team member");
        calls.emplace_back("member");
        return team[cursor];
    }
    void ActorNext(std::uint32_t iterator) override {
        Check(iterator == kIterator && cursor < team.size(),
              "team cursor advance");
        ++cursor;
        calls.emplace_back("next");
    }
    void ActorReset(std::uint32_t iterator) override {
        Check(iterator == kIterator, "team cursor reset");
        cursor = 0u;
        ++reset_count;
        calls.emplace_back("reset");
    }
    bool IsDead(std::uint32_t actor) override {
        Check(actor == kMate1 || actor == kMate2,
              "death probe uses team actor");
        calls.emplace_back("dead");
        return dead.count(actor) != 0u;
    }
    void ReleaseActorIterator(std::uint32_t iterator) noexcept override {
        if (iterator == kIterator) ++release_count;
        calls.emplace_back("release");
    }
    void ReportMissingRecord() override {
        ++diagnostic_count;
        calls.emplace_back("missing");
    }
    void LogState(std::uint32_t task, std::string_view source,
                  std::string_view message,
                  const pal4::AiActorStateLog& details) override {
        Check(task == kTask && source == "cbActorAi::getTeamState",
              "team-state log identity");
        Check(message.find("return EATS_") == 0u,
              "team-state log literal");
        logs.push_back(details);
        messages.emplace_back(message);
        calls.emplace_back("log");
        if (throw_log) throw std::runtime_error("log failure");
    }
};

pal4::AiTeamState Classify(Runtime& runtime,
                            std::uint32_t record = kRecord) {
    return pal4::AiTeamStateClassifier::Classify(
        kTask, record, runtime);
}

void EarlyReturns() {
    Runtime runtime;
    Check(Classify(runtime, 0u) ==
              pal4::AiTeamState::MissingRecord &&
          runtime.diagnostic_count == 1u &&
          runtime.calls == std::vector<std::string>{"missing"},
          "null record emits diagnostic and returns eight");

    runtime = Runtime{};
    runtime.fields[kRecord + 48u] = 1.0f;
    Check(Classify(runtime) == pal4::AiTeamState::TurnMatch &&
          runtime.release_count == 0u && runtime.logs.empty(),
          "matching turn returns six before HP and iterator");

    runtime = Runtime{};
    runtime.turn = 4;
    runtime.fields[kRecord + 52u] = 2.0f;
    Check(Classify(runtime) == pal4::AiTeamState::TurnInterval &&
          runtime.release_count == 0u && runtime.logs.empty(),
          "positive divisor and zero remainder return seven");
    runtime.fields[kRecord + 52u] = -2.0f;
    Check(Classify(runtime) == pal4::AiTeamState::Normal,
          "nonpositive divisor proceeds to HP checks");
}

void SelfStates() {
    Runtime runtime;
    runtime.hp[kActor] = 20;
    Check(Classify(runtime) == pal4::AiTeamState::SelfInDanger &&
          runtime.logs.size() == 1u &&
          runtime.logs[0].hp_rate == 0.2f &&
          runtime.logs[0].hp == 20 &&
          runtime.messages[0].find("EATS_SELF_IN_DANGER") !=
              std::string::npos &&
          runtime.release_count == 0u,
          "self danger includes threshold equality and HP log");

    runtime = Runtime{};
    runtime.hp[kActor] = 30;
    Check(Classify(runtime) == pal4::AiTeamState::SelfWounded &&
          runtime.logs[0].lower_threshold == 0.3f &&
          runtime.logs[0].upper_threshold == 0.6f &&
          runtime.release_count == 0u,
          "self wounded includes lower bound");

    runtime = Runtime{};
    runtime.hp[kActor] = 60;
    Check(Classify(runtime) == pal4::AiTeamState::SelfWounded,
          "self wounded includes upper bound");

    runtime = Runtime{};
    runtime.hp[kActor] = 0;
    runtime.max_hp[kActor] = 0;
    Check(Classify(runtime) == pal4::AiTeamState::SelfInDanger,
          "unordered self HP comparison selects danger");
}

void TeamStates() {
    Runtime runtime;
    runtime.dead.insert(kMate1);
    Check(Classify(runtime) ==
              pal4::AiTeamState::TeammateSlaughtered &&
          runtime.logs[0].dead_count == 1u &&
          runtime.logs[0].team_count == 2u &&
          runtime.logs[0].dead_rate == 0.5 &&
          runtime.reset_count == 0u && runtime.release_count == 1u,
          "dead threshold equality returns teammate slaughtered");

    runtime = Runtime{};
    runtime.fields[kRecord + 20u] = 0.8f;
    runtime.hp[kMate1] = 20;
    Check(Classify(runtime) ==
              pal4::AiTeamState::TeammateInDanger &&
          runtime.logs[0].actor == kMate1 &&
          runtime.logs[0].hp_rate == 0.2 &&
          runtime.reset_count == 1u && runtime.release_count == 1u,
          "first teammate danger threshold includes equality");

    runtime = Runtime{};
    runtime.fields[kRecord + 20u] = 0.8f;
    runtime.hp[kMate1] = 40;
    Check(Classify(runtime) ==
              pal4::AiTeamState::TeammateWounded &&
          runtime.logs[0].actor == kMate1 &&
          runtime.reset_count == 2u && runtime.release_count == 1u,
          "second team scan finds wounded member in interval");

    runtime = Runtime{};
    runtime.fields[kRecord + 20u] = 0.8f;
    Check(Classify(runtime) == pal4::AiTeamState::Normal &&
          runtime.reset_count == 2u &&
          runtime.release_count == 1u &&
          runtime.messages[0] == "return EATS_NORMAL",
          "no teammate match returns normal after two scans");

    runtime = Runtime{};
    runtime.team.clear();
    Check(Classify(runtime) == pal4::AiTeamState::Normal &&
          runtime.release_count == 1u,
          "empty team produces unordered dead rate then normal");

    runtime = Runtime{};
    runtime.fields[kRecord + 20u] =
        std::numeric_limits<float>::quiet_NaN();
    Check(Classify(runtime) == pal4::AiTeamState::Normal &&
          runtime.reset_count == 2u,
          "unordered dead threshold enters teammate scans");
}

void ExceptionCleanup() {
    Runtime runtime;
    runtime.dead.insert(kMate1);
    runtime.throw_log = true;
    bool threw = false;
    try {
        (void)Classify(runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.release_count == 1u &&
          runtime.calls.back() == "release",
          "iterator is released if team-state logging fails");
}

}  // namespace

int main() {
    try {
        EarlyReturns();
        SelfStates();
        TeamStates();
        ExceptionCleanup();
        std::cout << "ai_team_state_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
