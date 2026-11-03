#include "pal4/player_manager_slot_inventory_state.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PlayerManagerSlotInventoryRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> items;
    std::unordered_map<std::uint32_t, std::uint8_t> conditions;
    std::vector<std::string> events;

    std::uint32_t ReadDword(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t InventoryManager() override {
        events.emplace_back("inventory");
        return 0x7000u;
    }
    std::uint32_t FindItem(std::uint32_t collection,
                           std::uint32_t key,
                           std::uint32_t category) override {
        Check(collection == 0x707cu && category == 7u,
              "inventory collection and item category");
        events.push_back("find:" + std::to_string(key));
        return items[key];
    }
    std::uint8_t CheckPlayerCondition(
        std::uint32_t item) override {
        events.push_back("condition:" + std::to_string(item));
        return conditions[item];
    }
};

void DisabledAndEmpty() {
    Runtime disabled;
    disabled.words[0x5920u] = 77u;
    Check(pal4::PlayerManagerSlotInventoryState::Update(
              0x5000u, 0u, disabled) == 0u &&
              disabled.words[0x5920u] == 77u &&
              disabled.events == std::vector<std::string>{"write:23304"},
          "disabled update only clears the enabled word");

    Runtime empty;
    Check(pal4::PlayerManagerSlotInventoryState::Update(
              0x5000u, 0x1234u, empty) == 0x34u &&
              empty.words[0x5b08u] == 0x1234u &&
              empty.events.size() == 7u &&
              empty.events.back() == "write:23304",
          "empty slots preserve low byte of enabled in return register");
}

void ValidateSixSlots() {
    Runtime runtime;
    runtime.words[0x5920u] = 10u;
    runtime.words[0x5928u] = 20u;
    runtime.words[0x5934u] = 30u;
    runtime.items[10u] = 0u;
    runtime.items[20u] = 0x8020u;
    runtime.items[30u] = 0x8030u;
    runtime.conditions[0x8020u] = 1u;
    runtime.conditions[0x8030u] = 0u;
    Check(pal4::PlayerManagerSlotInventoryState::Update(
              0x5000u, 1u, runtime) == 0u &&
              runtime.words[0x5920u] == 0u &&
              runtime.words[0x5928u] == 0u &&
              runtime.words[0x5934u] == 30u &&
              runtime.words[0x5b08u] == 1u,
          "missing and rejected items clear their slots, accepted item stays");
    Check(runtime.events == std::vector<std::string>{
              "read:22816", "inventory", "read:22816", "find:10",
              "write:22816", "read:22820", "read:22824",
              "inventory", "read:22824", "find:20",
              "condition:32800", "write:22824", "read:22828",
              "read:22832", "read:22836", "inventory",
              "read:22836", "find:30", "condition:32816",
              "write:23304"},
          "all six slots are visited and nonzero keys reread after manager lookup");
}

void LastConditionIsReturnByte() {
    Runtime runtime;
    runtime.words[0x5920u] = 42u;
    runtime.items[42u] = 0x8000u;
    runtime.conditions[0x8000u] = 0x80u;
    Check(pal4::PlayerManagerSlotInventoryState::Update(
              0x5000u, 0x1234u, runtime) == 0x80u &&
              runtime.words[0x5920u] == 0u &&
              runtime.words[0x5b08u] == 0x1234u,
          "last checked condition supplies low-byte return");
}

}  // namespace

int main() {
    DisabledAndEmpty();
    ValidateSixSlots();
    LastConditionIsReturnByte();
    std::cout << "3 player-manager inventory-state groups passed\n";
}
