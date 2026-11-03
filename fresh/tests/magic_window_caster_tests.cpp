#include "pal4/magic_window_caster.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MagicWindowCastRuntimeStub {
    std::vector<std::string> calls;
    std::map<std::int32_t, std::uint32_t> players{
        {0, 100u}, {1, 101u}, {2, 102u}, {3, 103u}};
    std::map<std::uint32_t, bool> active{
        {100u, true}, {101u, false}, {102u, true}, {103u, true}};
    std::int32_t value{5};
    std::int32_t type{};

    std::uint32_t GetPlayer(std::int32_t index) override {
        calls.push_back("player:" + std::to_string(index));
        const auto found = players.find(index);
        return found == players.end() ? 0u : found->second;
    }
    std::int32_t PlayerValue(std::uint32_t player,
                             std::uint32_t stat_index) override {
        Check(player == 101u && stat_index == 9u, "caster stat lookup");
        calls.push_back("value");
        return value;
    }
    void AdjustPlayerValue(std::uint32_t player, std::uint32_t stat_index,
                           std::int32_t delta) override {
        Check(player == 101u && stat_index == 9u, "caster stat adjustment");
        calls.push_back("adjust:" + std::to_string(delta));
    }
    std::int32_t SkillType(const pal4::MagicDbSkillRecord& skill) override {
        Check(skill.effect_id == 72u, "type lookup uses skill record");
        calls.push_back("type");
        return type;
    }
    bool IsActive(std::uint32_t player) override {
        calls.push_back("active:" + std::to_string(player));
        return active[player];
    }
    void CastEffect(std::uint32_t caster, std::uint32_t effect_id,
                    std::uint32_t target) override {
        Check(caster == 101u && effect_id == 72u, "effect arguments");
        calls.push_back("cast:" + std::to_string(target));
    }
    void PlaySound(std::string_view name) override {
        calls.push_back("sound:" + std::string(name));
    }
    std::string_view TriggerText(std::uint32_t trigger_id) override {
        Check(trigger_id == 0x6bu, "insufficient trigger ID");
        calls.push_back("text");
        return "Insufficient resource";
    }
    void ShowInsufficientMessage(std::string_view text, float x, float y,
                                 std::uint32_t style,
                                 std::uint32_t flags) override {
        Check(text == "Insufficient resource" && x == 0.35f && y == 0.4f &&
                  style == 4u && flags == 0u,
              "insufficient dialog payload");
        calls.push_back("dialog");
    }
};

pal4::MagicWindowState State() {
    pal4::MagicWindowState state;
    state.selected_character_index = 1;
    return state;
}

pal4::MagicDbSkillRecord Skill() {
    pal4::MagicDbSkillRecord skill;
    skill.effect_id = 72u;
    skill.cast_cost = 5;
    return skill;
}

void TestMissingPlayers() {
    Runtime runtime;
    runtime.players.erase(2);
    Check(pal4::MagicWindowCaster::Cast(State(), 2, Skill(), runtime),
          "missing target returns true");
    Check(runtime.calls == std::vector<std::string>{"player:2"},
          "missing target exits before caster lookup");
    runtime = Runtime{};
    runtime.players.erase(1);
    Check(pal4::MagicWindowCaster::Cast(State(), 2, Skill(), runtime),
          "missing caster returns true");
    Check(runtime.calls == std::vector<std::string>{"player:2", "player:1"},
          "missing caster has no sound or stat calls");
}

void TestInsufficientResource() {
    Runtime runtime;
    runtime.value = 4;
    Check(pal4::MagicWindowCaster::Cast(State(), 2, Skill(), runtime),
          "insufficient resource returns true");
    Check(runtime.calls == std::vector<std::string>{
                               "player:2", "player:1", "value",
                               "sound:WA05.WAV", "text", "dialog"},
          "insufficient resource branch and order");
}

void TestSingleTargetAndInclusiveCost() {
    Runtime runtime;
    Check(pal4::MagicWindowCaster::Cast(State(), 2, Skill(), runtime),
          "equal resource casts successfully");
    Check(runtime.calls == std::vector<std::string>{
                               "player:2", "player:1", "value",
                               "sound:WD001.WAV", "adjust:-5", "type",
                               "cast:102"},
          "single-target cast order and cost");
}

void TestPartyCastFiltersInactiveAndMissing() {
    Runtime runtime;
    runtime.type = 3;
    runtime.players.erase(3);
    Check(pal4::MagicWindowCaster::Cast(State(), 2, Skill(), runtime),
          "party cast returns true");
    Check(runtime.calls == std::vector<std::string>{
                               "player:2", "player:1", "value",
                               "sound:WD001.WAV", "adjust:-5", "type",
                               "player:0", "active:100", "cast:100",
                               "player:1", "active:101", "player:2",
                               "active:102", "cast:102", "player:3"},
          "party cast scans four slots and filters inactive or missing players");
}

void TestSignedCostComparison() {
    Runtime runtime;
    auto skill = Skill();
    skill.cast_cost = -1;
    runtime.value = -2;
    Check(pal4::MagicWindowCaster::Cast(State(), 2, skill, runtime),
          "negative costs still return true");
    Check(runtime.calls.back() == "dialog", "cost comparison remains signed");
}

}  // namespace

int main() {
    TestMissingPlayers();
    TestInsufficientResource();
    TestSingleTargetAndInclusiveCost();
    TestPartyCastFiltersInactiveAndMissing();
    TestSignedCostComparison();
}
