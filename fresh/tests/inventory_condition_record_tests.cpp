#include "pal4/inventory_condition_record.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IInventoryConditionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::array<std::uint32_t, 4> players{};
    std::map<std::uint32_t, bool> contains;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> visited;
    std::uint32_t field = 0;
    std::uint32_t stream = 0;
    std::uint32_t size = 0;
    std::uint32_t checked_slots = 0;
    std::uint32_t checked_item = 0;
    std::int32_t result = -17;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words[address];
    }
    void WriteStreamWord(std::uint32_t s, std::uint32_t address,
                         std::uint32_t bytes) override {
        calls.emplace_back("write field");
        stream = s; field = address; size = bytes;
    }
    void ReadStreamWord(std::uint32_t s, std::uint32_t address,
                        std::uint32_t bytes) override {
        calls.emplace_back("read field");
        stream = s; field = address; size = bytes;
    }
    std::int32_t SerializeRecord(std::uint32_t record,
                                 std::uint32_t s) override {
        calls.emplace_back("serialize");
        Check(record == 0x1000u && s == 0x2000u, "serialize arguments");
        return result;
    }
    std::int32_t DeserializeRecord(std::uint32_t record,
                                   std::uint32_t s) override {
        calls.emplace_back("deserialize");
        Check(record == 0x1000u && s == 0x2000u, "deserialize arguments");
        return result;
    }
    std::uint32_t InitializePlayerManager() override {
        calls.emplace_back("manager");
        return 0x3000u;
    }
    std::uint32_t GetPlayerByIndex(std::uint32_t manager,
                                   std::uint32_t index) override {
        Check(manager == 0x3000u && index < players.size(),
              "player lookup arguments");
        calls.emplace_back("player");
        visited.push_back(index);
        return players[index];
    }
    bool InventoryContains(std::uint32_t slots,
                           std::uint32_t item) override {
        calls.emplace_back("contains");
        checked_slots = slots;
        checked_item = item;
        return contains[slots];
    }
};

void CheckStreams() {
    Runtime runtime;
    Check(pal4::InventoryConditionRecord::Serialize(0x1000u, 0x2000u,
                                                       runtime) == -17 &&
              runtime.calls == std::vector<std::string>{"write field", "serialize"} &&
              runtime.stream == 0x2000u && runtime.field == 0x1190u &&
              runtime.size == 4u,
          "serialize writes item field first and forwards the result");
    runtime.calls.clear();
    runtime.result = 29;
    Check(pal4::InventoryConditionRecord::Deserialize(0x1000u, 0x2000u,
                                                         runtime) == 29 &&
              runtime.calls == std::vector<std::string>{"read field", "deserialize"} &&
              runtime.field == 0x1190u && runtime.size == 4u,
          "deserialize reads item field first and forwards the result");
}

void CheckPlayerConditions() {
    Runtime runtime;
    runtime.words[0x1190u] = 0x77u;
    runtime.players = {0u, 0x4000u, 0x5000u, 0x6000u};
    runtime.words[0x4000u + 0xb08u] = 0u;
    runtime.words[0x5000u + 0xb08u] = 1u;
    runtime.words[0x6000u + 0xb08u] = 2u;
    runtime.contains[0x4000u + 0x920u] = true;
    runtime.contains[0x5000u + 0x920u] = false;
    runtime.contains[0x6000u + 0x920u] = true;

    Check(pal4::InventoryConditionRecord::AnyActivePlayerHasItem(
              0x1000u, runtime) &&
              runtime.visited == std::vector<std::uint32_t>{0u, 1u, 2u, 3u} &&
              runtime.checked_slots == 0x6920u &&
              runtime.checked_item == 0x77u,
          "active search skips absent and inactive players and stops on match");
    runtime.visited.clear();
    Check(!pal4::InventoryConditionRecord::NoInactivePlayerHasItem(
              0x1000u, runtime) &&
              runtime.visited == std::vector<std::uint32_t>{0u, 1u} &&
              runtime.checked_slots == 0x4920u,
          "inactive search stops on first matching player");

    runtime.contains.clear();
    runtime.visited.clear();
    Check(!pal4::InventoryConditionRecord::AnyActivePlayerHasItem(
              0x1000u, runtime) && runtime.visited.size() == 4u,
          "active search returns false after all four players");
    runtime.visited.clear();
    Check(pal4::InventoryConditionRecord::NoInactivePlayerHasItem(
              0x1000u, runtime) && runtime.visited.size() == 4u,
          "inactive search returns true after all four players");

    runtime.players = {};
    runtime.visited.clear();
    runtime.calls.clear();
    Check(!pal4::InventoryConditionRecord::AnyActivePlayerHasItem(
              0x1000u, runtime) &&
              pal4::InventoryConditionRecord::NoInactivePlayerHasItem(
                  0x1000u, runtime) &&
              runtime.visited.size() == 8u &&
              runtime.calls.size() == 16u,
          "missing players still initialize and query the manager each iteration");
}

}  // namespace

int main() {
    try {
        CheckStreams();
        CheckPlayerConditions();
        std::cout << "inventory_condition_record_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
