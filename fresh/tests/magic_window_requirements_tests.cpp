#include "pal4/magic_window_requirements.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MagicWindowRequirementsRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> looked_up_items;
    std::map<std::uint32_t, pal4::MagicItemDefinition> definitions;
    std::map<std::uint32_t, std::int32_t> inventory;
    std::uint32_t player{0x9000u};
    bool condition_contains{};
    bool sorted_contains{true};
    bool has_condition{true};
    std::int32_t player_value{5};

    std::uint32_t GetPlayer(std::int32_t index) override {
        Check(index == 3, "selected player index");
        calls.push_back("player");
        return player;
    }
    bool ConditionContains(std::uint32_t selected_player,
                           std::uint32_t key) override {
        Check(selected_player == player && key == 71u, "condition array target");
        calls.push_back("condition-array");
        return condition_contains;
    }
    bool SortedContains(std::uint32_t selected_player,
                        std::uint32_t key) override {
        Check(selected_player == player && key == 71u, "sorted array target");
        calls.push_back("sorted-array");
        return sorted_contains;
    }
    std::int32_t PlayerValue(std::uint32_t selected_player,
                             std::uint32_t index) override {
        Check(selected_player == player && index == 8u, "player virtual stat slot");
        calls.push_back("player-value");
        return player_value;
    }
    bool HasCondition(std::uint32_t selected_player,
                      std::uint32_t key) override {
        Check(selected_player == player && key == 17u, "condition key");
        calls.push_back("has-condition");
        return has_condition;
    }
    std::optional<pal4::MagicItemDefinition> LookupItem(
        std::uint32_t key) override {
        looked_up_items.push_back(key);
        calls.push_back("item:" + std::to_string(key));
        const auto found = definitions.find(key);
        if (found == definitions.end()) return std::nullopt;
        return found->second;
    }
    std::optional<std::int32_t> InventoryCount(
        const pal4::MagicItemDefinition& item) override {
        calls.push_back("inventory:" + std::to_string(item.item_id));
        const auto found = inventory.find(item.item_id);
        if (found == inventory.end()) return std::nullopt;
        return found->second;
    }
};

pal4::MagicWindowState State() {
    pal4::MagicWindowState state;
    state.selected_character_index = 3;
    return state;
}

pal4::MagicDbStuntRecord Record() {
    pal4::MagicDbStuntRecord record;
    record.key = 71u;
    record.kind = 2u;
    record.required_stat = 5;
    return record;
}

void TestEarlyGates() {
    auto record = Record();
    auto runtime = Runtime{};
    runtime.player = 0;
    Check(!pal4::MagicWindowRequirements::Check(State(), record, runtime) &&
              runtime.calls == std::vector<std::string>{"player"},
          "missing player exits first");
    runtime = Runtime{};
    record.kind = 1u;
    Check(!pal4::MagicWindowRequirements::Check(State(), record, runtime) &&
              runtime.calls == std::vector<std::string>{"player"},
          "kind other than two exits before collection queries");
    record.kind = 2u;
    runtime = Runtime{};
    runtime.condition_contains = true;
    Check(!pal4::MagicWindowRequirements::Check(State(), record, runtime) &&
              runtime.calls.back() == "condition-array", "existing condition blocks use");
    runtime = Runtime{};
    runtime.sorted_contains = false;
    Check(!pal4::MagicWindowRequirements::Check(State(), record, runtime) &&
              runtime.calls.back() == "sorted-array", "missing sorted entry blocks use");
    runtime = Runtime{};
    runtime.player_value = 4;
    Check(!pal4::MagicWindowRequirements::Check(State(), record, runtime) &&
              runtime.calls.back() == "player-value", "signed stat threshold");
    runtime = Runtime{};
    Check(pal4::MagicWindowRequirements::Check(State(), record, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "player", "condition-array", "sorted-array", "player-value"},
          "inclusive stat threshold and empty optional requirements");
    runtime = Runtime{};
    record.required_stat = -1;
    runtime.player_value = -2;
    Check(!pal4::MagicWindowRequirements::Check(State(), record, runtime),
          "negative stat comparison stays signed");
}

void TestConditionAndSpecialItem() {
    auto record = Record();
    record.condition_key = 17u;
    Runtime runtime;
    runtime.has_condition = false;
    Check(!pal4::MagicWindowRequirements::Check(State(), record, runtime) &&
              runtime.calls.back() == "has-condition", "required condition failure");
    runtime = Runtime{};
    record.special_item_id = 80u;
    Check(pal4::MagicWindowRequirements::Check(State(), record, runtime) &&
              runtime.looked_up_items == std::vector<std::uint32_t>{80u},
          "missing GameDB definition bypasses special item gate");
    runtime = Runtime{};
    runtime.definitions[80u] = {800u, 2u};
    Check(!pal4::MagicWindowRequirements::Check(State(), record, runtime) &&
              runtime.calls.back() == "inventory:800",
          "defined special item requires inventory entry");
    runtime.inventory[800u] = 0;
    Check(pal4::MagicWindowRequirements::Check(State(), record, runtime),
          "special item checks presence rather than positive quantity");
}

void TestFiveItemPairs() {
    auto record = Record();
    record.item_ids = {10u, 0u, 20u, 30u, 40u};
    record.item_counts = {2, 99, 0, -1, 4};
    Runtime runtime;
    for (const auto id : {10u, 20u, 30u, 40u})
        runtime.definitions[id] = {id + 100u, 1u};
    runtime.inventory[110u] = 2;
    runtime.inventory[120u] = 0;
    runtime.inventory[130u] = 0;
    runtime.inventory[140u] = 3;
    Check(!pal4::MagicWindowRequirements::Check(State(), record, runtime) &&
              runtime.looked_up_items == std::vector<std::uint32_t>{10u, 20u, 30u, 40u},
          "five pairs skip zero ID and fail on final insufficient count");
    runtime.inventory[140u] = 4;
    runtime.looked_up_items.clear();
    Check(pal4::MagicWindowRequirements::Check(State(), record, runtime) &&
              runtime.looked_up_items.size() == 4u,
          "inclusive count permits all five pairs");
    runtime.inventory.erase(120u);
    Check(pal4::MagicWindowRequirements::Check(State(), record, runtime),
          "missing inventory count treated as zero for zero requirement");
    record.item_counts[2] = 1;
    Check(!pal4::MagicWindowRequirements::Check(State(), record, runtime),
          "missing inventory entry fails positive requirement");
    runtime.definitions.erase(20u);
    Check(pal4::MagicWindowRequirements::Check(State(), record, runtime),
          "missing GameDB definition skips that pair as in IDA");
}

}  // namespace

int main() {
    TestEarlyGates();
    TestConditionAndSpecialItem();
    TestFiveItemPairs();
}
