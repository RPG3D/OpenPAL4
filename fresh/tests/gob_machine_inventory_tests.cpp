#include "pal4/gob_machine.h"
#include "pal4/gob_machine_inventory.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGobMachineInventoryRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::uint32_t machine = 0x9000u;
    std::vector<std::uint32_t> conditions;
    std::map<std::pair<std::uint32_t, std::string>, std::int32_t> attributes;
    std::map<std::int32_t, std::vector<std::uint32_t>> item_results;
    std::map<std::int32_t, std::size_t> item_calls;
    std::map<std::uint32_t, std::int32_t> item_counts;
    std::vector<std::string> calls;
    std::string shown_hint;
    bool hint_window = true;

    std::uint32_t MachineConfigNode(std::uint32_t address) override {
        Check(address == object, "machine object");
        calls.emplace_back("machine");
        return machine;
    }
    std::uint32_t FindCondition(std::uint32_t node,
                                 std::uint32_t previous) override {
        Check(node == machine, "condition parent");
        if (previous == 0u)
            return conditions.empty() ? 0u : conditions.front();
        const auto found = std::find(conditions.begin(), conditions.end(),
                                     previous);
        Check(found != conditions.end(), "previous condition");
        const auto next = found + 1;
        return next == conditions.end() ? 0u : *next;
    }
    std::int32_t ReadConditionInt(std::uint32_t node,
                                  std::string_view key,
                                  std::int32_t fallback) override {
        Check(key == "PAL4_GameObject-machine-propid" ||
              key == "PAL4_GameObject-machine-propnum", "condition key");
        calls.push_back(std::string(key));
        const auto found = attributes.find({node, std::string(key)});
        return found == attributes.end() ? fallback : found->second;
    }
    std::uint32_t FindInventoryItem(std::int32_t id,
                                     std::uint32_t type) override {
        Check(type == 4u, "inventory type");
        calls.push_back("find " + std::to_string(id));
        const auto found = item_results.find(id);
        if (found == item_results.end() || found->second.empty()) return 0u;
        const auto index = item_calls[id]++;
        return found->second[std::min(index, found->second.size() - 1u)];
    }
    std::int32_t ItemCount(std::uint32_t item) override {
        return item_counts.at(item);
    }
    bool CreateHintWindow() override {
        calls.emplace_back("create hint");
        return hint_window;
    }
    std::string PropertyName(std::int32_t id) override {
        calls.push_back("property " + std::to_string(id));
        return "property" + std::to_string(id);
    }
    std::string InventoryItemName(std::uint32_t item) override {
        calls.push_back("item name");
        return "item" + std::to_string(item);
    }
    std::string FormatGameHint(std::uint32_t text_id,
                                std::string_view name,
                                std::int32_t count) override {
        Check(text_id == 0xf7u || text_id == 0xf9u || text_id == 0xfau,
              "hint text id");
        calls.push_back("format " + std::to_string(text_id));
        return std::to_string(text_id) + ":" + std::string(name) +
               ":" + std::to_string(count) + ";";
    }
    void RemoveInventoryItem(std::int32_t id,
                             std::int32_t count) override {
        calls.push_back("remove " + std::to_string(id) + ":" +
                        std::to_string(count));
    }
    void ShowHint(std::string_view message, float x, float y,
                  float scale, std::uint32_t first,
                  std::uint32_t second) override {
        Check(x == 0.5f && y > 0.149f && y < 0.151f &&
              scale == 1.0f && first == 0u && second == 1u,
              "hint window placement");
        shown_hint = message;
        calls.emplace_back("show hint");
    }
};

void AddCondition(Runtime& runtime, std::uint32_t node,
                  std::int32_t id, std::int32_t count) {
    runtime.conditions.push_back(node);
    runtime.attributes[{node, "PAL4_GameObject-machine-propid"}] = id;
    runtime.attributes[{node, "PAL4_GameObject-machine-propnum"}] = count;
}

bool Called(const Runtime& runtime, const std::string& name) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), name) !=
           runtime.calls.end();
}

void CheckNoConditions() {
    Runtime no_machine;
    no_machine.machine = 0u;
    Check(pal4::GobMachine::CheckAndConsumeInventoryConditions(
              Runtime::object, no_machine) == 0u &&
          !Called(no_machine, "create hint"), "missing machine returns zero");
    Runtime empty;
    Check(pal4::GobMachine::CheckAndConsumeInventoryConditions(
              Runtime::object, empty) == 1u &&
          !Called(empty, "create hint"), "no condition succeeds silently");
    Runtime invalid;
    invalid.conditions = {0xa000u, 0xa100u};
    Check(pal4::GobMachine::CheckAndConsumeInventoryConditions(
              Runtime::object, invalid) == 1u &&
          !Called(invalid, "create hint") &&
          !Called(invalid, "find 0"),
          "missing propid defaults to minus one");
}

void CheckMissingItem() {
    Runtime missing;
    AddCondition(missing, 0xa000u, 5, 2);
    AddCondition(missing, 0xa100u, 6, 3);
    Check(pal4::GobMachine::CheckAndConsumeInventoryConditions(
              Runtime::object, missing) == 0u &&
          missing.shown_hint ==
              "247:property5:2;247:property6:3;" &&
          !Called(missing, "find 6") &&
          !Called(missing, "remove 5:2"),
          "first missing requirement shows every property without removal");
    Runtime short_count;
    AddCondition(short_count, 0xa000u, 5, 2);
    short_count.item_results[5] = {0xb000u};
    short_count.item_counts[0xb000u] = 1;
    Check(pal4::GobMachine::CheckAndConsumeInventoryConditions(
              Runtime::object, short_count) == 0u &&
          short_count.shown_hint == "247:property5:2;",
          "insufficient signed count blocks consumption");
    Runtime no_window;
    AddCondition(no_window, 0xa000u, 5, 2);
    no_window.hint_window = false;
    Check(pal4::GobMachine::CheckAndConsumeInventoryConditions(
              Runtime::object, no_window) == 0u &&
          no_window.shown_hint.empty() &&
          !Called(no_window, "remove 5:2"),
          "failed hint window returns zero");
}

void CheckSuccessfulConsumption() {
    Runtime runtime;
    AddCondition(runtime, 0xa000u, -1, 9);
    AddCondition(runtime, 0xa100u, 5, 2);
    AddCondition(runtime, 0xa200u, 6, 0);
    runtime.item_results[5] = {0xb000u};
    runtime.item_results[6] = {0xb100u};
    runtime.item_counts[0xb000u] = 2;
    runtime.item_counts[0xb100u] = 0;
    Check(pal4::GobMachine::CheckAndConsumeInventoryConditions(
              Runtime::object, runtime) == 1u &&
          runtime.shown_hint == "249:item45056:2;249:item45312:0;" &&
          Called(runtime, "remove 5:2") &&
          Called(runtime, "remove 6:0") &&
          runtime.calls.back() == "show hint",
          "all requirements consume and display acquired item names");
    Runtime vanished;
    AddCondition(vanished, 0xa000u, 5, 2);
    vanished.item_results[5] = {0xb000u, 0u};
    vanished.item_counts[0xb000u] = 2;
    Check(pal4::GobMachine::CheckAndConsumeInventoryConditions(
              Runtime::object, vanished) == 1u &&
          vanished.shown_hint == "250::2;" &&
          Called(vanished, "remove 5:2"),
          "second-pass missing item uses alternate hint and still removes");
}

}  // namespace

int main() {
    try {
        CheckNoConditions();
        CheckMissingItem();
        CheckSuccessfulConsumption();
        std::cout << "gob_machine_inventory_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
