#include "pal4/magic_window_stunt_caster.h"

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

struct CastRuntime final : pal4::MagicWindowCastRuntimeStub {
    explicit CastRuntime(std::vector<std::string>& calls) : calls(calls) {}
    std::vector<std::string>& calls;
    std::map<std::int32_t, std::uint32_t> players{
        {0, 100u}, {1, 101u}, {2, 102u}, {3, 103u}};
    std::int32_t value{5};
    std::int32_t type{};

    std::uint32_t GetPlayer(std::int32_t index) override {
        calls.push_back("player:" + std::to_string(index));
        const auto found = players.find(index);
        return found == players.end() ? 0u : found->second;
    }
    std::int32_t PlayerValue(std::uint32_t player,
                             std::uint32_t stat) override {
        Check(player == 101u && stat == 8u, "stunt stat lookup");
        calls.push_back("value");
        return value;
    }
    void AdjustPlayerValue(std::uint32_t player, std::uint32_t stat,
                           std::int32_t delta) override {
        Check(player == 101u && stat == 8u, "stunt stat adjustment");
        calls.push_back("adjust:" + std::to_string(delta));
    }
    std::int32_t StuntType(const pal4::MagicDbStuntRecord& stunt) override {
        Check(stunt.effect_id == 72u, "stunt type source");
        calls.push_back("type");
        return type;
    }
    bool IsActive(std::uint32_t player) override {
        calls.push_back("active:" + std::to_string(player));
        return player == 100u || player == 102u;
    }
    void CastEffect(std::uint32_t caster, std::uint32_t effect,
                    std::uint32_t target) override {
        Check(caster == 101u && effect == 72u, "stunt effect arguments");
        calls.push_back("cast:" + std::to_string(target));
    }
    void PlaySound(std::string_view name) override {
        calls.push_back("sound:" + std::string(name));
    }
    std::string_view TriggerText(std::uint32_t id) override {
        calls.push_back("text:" + std::to_string(id));
        return "Failure";
    }
    void ShowInsufficientMessage(std::string_view text, float x, float y,
                                 std::uint32_t style,
                                 std::uint32_t flags) override {
        Check(text == "Failure" && x == 0.5f && y == 0.4f &&
                  style == 4u && flags == 0u,
              "stunt failure dialog arguments");
        calls.push_back("dialog");
    }
};

struct RequirementsRuntime final : pal4::MagicWindowRequirementsRuntimeStub {
    explicit RequirementsRuntime(std::vector<std::string>& calls) : calls(calls) {}
    std::vector<std::string>& calls;
    bool allowed{true};

    std::uint32_t GetPlayer(std::int32_t index) override {
        Check(index == 1, "requirements selected player");
        calls.push_back("requirements-player");
        return 101u;
    }
    bool ConditionContains(std::uint32_t player,
                           std::uint32_t key) override {
        Check(player == 101u && key == 71u, "condition lookup");
        calls.push_back("condition");
        return !allowed;
    }
    bool SortedContains(std::uint32_t player, std::uint32_t key) override {
        Check(player == 101u && key == 71u, "sorted lookup");
        calls.push_back("sorted");
        return true;
    }
    std::int32_t PlayerValue(std::uint32_t player,
                             std::uint32_t stat) override {
        Check(player == 101u && stat == 8u, "requirements stat");
        calls.push_back("requirements-value");
        return 5;
    }
};

struct CostRuntime final : pal4::MagicWindowCostRuntimeStub {
    explicit CostRuntime(std::vector<std::string>& calls) : calls(calls) {}
    std::vector<std::string>& calls;
    void AdjustCounter(std::int32_t delta) override {
        calls.push_back("counter:" + std::to_string(delta));
    }
    void RemoveItem(std::uint32_t id, std::int32_t count) override {
        calls.push_back("item:" + std::to_string(id) + ":" +
                        std::to_string(count));
    }
};

struct Fixture {
    std::vector<std::string> calls;
    CastRuntime cast{calls};
    RequirementsRuntime requirements{calls};
    CostRuntime cost{calls};
    pal4::MagicWindowState state;
    pal4::MagicDbStuntRecord stunt;

    Fixture() {
        state.selected_character_index = 1;
        stunt.key = 71u;
        stunt.kind = 2u;
        stunt.effect_id = 72u;
        stunt.required_stat = 5;
        stunt.counter_cost = 3;
        stunt.item_ids[0] = 20u;
        stunt.item_counts[0] = 2;
    }
    bool Run() {
        return pal4::MagicWindowStuntCaster::Cast(
            state, 2, stunt, cast, requirements, cost);
    }
};

void TestMissingPlayers() {
    Fixture fixture;
    fixture.cast.players.erase(2);
    Check(fixture.Run() &&
              fixture.calls == std::vector<std::string>{"player:2"},
          "missing target exits before caster");
    Fixture missing_caster;
    missing_caster.cast.players.erase(1);
    Check(missing_caster.Run() &&
              missing_caster.calls == std::vector<std::string>{"player:2", "player:1"},
          "missing caster exits before stat and costs");
}

void TestResourceFailures() {
    Fixture insufficient_stat;
    insufficient_stat.cast.value = 4;
    Check(insufficient_stat.Run(), "stat failure returns true");
    Check(insufficient_stat.calls == std::vector<std::string>{
                                         "player:2", "player:1", "value",
                                         "sound:WA05.WAV", "text:108", "dialog"},
          "stat failure uses trigger 0x6c before requirements");

    Fixture unmet_requirements;
    unmet_requirements.requirements.allowed = false;
    Check(unmet_requirements.Run(), "requirements failure returns true");
    Check(unmet_requirements.calls == std::vector<std::string>{
                                            "player:2", "player:1", "value",
                                            "requirements-player", "condition",
                                            "sound:WA05.WAV", "text:59", "dialog"},
          "requirements failure uses trigger 0x3b without costs");
}

void TestSingleTargetAndCostOrder() {
    Fixture fixture;
    Check(fixture.Run(), "single-target cast returns true");
    Check(fixture.calls == std::vector<std::string>{
                               "player:2", "player:1", "value",
                               "requirements-player", "condition", "sorted",
                               "requirements-value", "sound:WD001.WAV",
                               "adjust:-5", "counter:-3", "item:20:2",
                               "type", "cast:102"},
          "successful cast deducts stat and items before effect");
}

void TestPartyTargets() {
    Fixture fixture;
    fixture.cast.type = 3;
    fixture.cast.players.erase(3);
    Check(fixture.Run(), "party cast returns true");
    Check(fixture.calls == std::vector<std::string>{
                               "player:2", "player:1", "value",
                               "requirements-player", "condition", "sorted",
                               "requirements-value", "sound:WD001.WAV",
                               "adjust:-5", "counter:-3", "item:20:2", "type",
                               "player:0", "active:100", "cast:100",
                               "player:1", "active:101", "player:2",
                               "active:102", "cast:102", "player:3"},
          "party cast scans four slots and skips inactive or missing players");
}

}  // namespace

int main() {
    TestMissingPlayers();
    TestResourceFailures();
    TestSingleTargetAndCostOrder();
    TestPartyTargets();
}
