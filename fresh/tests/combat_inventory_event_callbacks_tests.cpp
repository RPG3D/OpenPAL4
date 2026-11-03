#include "pal4/combat_inventory_event_callbacks.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatInventoryEventRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> count_reads;
    std::vector<std::uint32_t> item_reads;
    std::uint32_t event = 0x1000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        if (address == event && !item_reads.empty()) {
            const auto value = item_reads.front();
            item_reads.erase(item_reads.begin());
            return value;
        }
        if (address == event + 4u && !count_reads.empty()) {
            const auto value = count_reads.front();
            count_reads.erase(count_reads.begin());
            return value;
        }
        return words[address];
    }
    std::uint32_t InventoryInstance() override {
        calls.emplace_back("inventory");
        return 0x2000u;
    }
    std::uint32_t UpdateAndClampCounter(std::uint32_t inventory,
                                        std::uint32_t value) override {
        Check(inventory == 0x2000u && value == 17u, "counter arguments");
        calls.emplace_back("counter");
        return 0x55u;
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        return 0x3000u;
    }
    void FindProperty(std::uint32_t database,
                      std::uint32_t item) override {
        Check(database == 0x3000u && item == 7u, "property lookup");
        calls.emplace_back("find");
    }
    std::uint32_t ProcessItem(std::uint32_t inventory,
                              std::uint32_t item,
                              std::uint32_t count) override {
        Check(inventory == 0x2000u && item == 8u && count == 3u,
              "inventory processing arguments");
        calls.emplace_back("process");
        return 0x66u;
    }
    std::uint32_t ActorManager() override {
        calls.emplace_back("actor-manager");
        return 0x4000u;
    }
    void CreateMonster(std::uint32_t manager,
                       std::uint32_t monster,
                       std::uint32_t mode,
                       std::uint32_t flags) override {
        Check(manager == 0x4000u && monster == 0x99u &&
                  mode == 6u && flags == 0u,
              "monster creation arguments");
        calls.emplace_back("create");
    }
};

void CounterAndItem() {
    Runtime runtime;
    runtime.words[runtime.event] = 17u;
    Check(pal4::CombatInventoryEventCallbacks::UpdateCounter(
              runtime.event, runtime) == 0x55u &&
              runtime.calls == std::vector<std::string>{
                  "inventory", "read:4096", "counter"},
          "counter obtains inventory before reading event");

    runtime.calls.clear();
    runtime.item_reads = {7u, 8u};
    runtime.words[runtime.event + 4u] = 3u;
    Check(pal4::CombatInventoryEventCallbacks::ProcessItem(
              runtime.event, runtime) == 0x66u &&
              runtime.calls == std::vector<std::string>{
                  "database", "read:4096", "find", "inventory",
                  "read:4100", "read:4096", "process"},
          "item callback rereads item after inventory initialization");
}

void Monsters() {
    Runtime runtime;
    runtime.words[runtime.event] = 0x99u;
    runtime.count_reads = {3u, 1u};
    Check(pal4::CombatInventoryEventCallbacks::CreateMonsters(
              runtime.event, runtime) == 1 &&
              runtime.calls == std::vector<std::string>{
                  "read:4100", "actor-manager", "read:4096", "create",
                  "read:4100"},
          "count change stops creation after one iteration");

    runtime.calls.clear();
    runtime.count_reads = {2u, 2u, 2u};
    Check(pal4::CombatInventoryEventCallbacks::CreateMonsters(
              runtime.event, runtime) == 2 &&
              runtime.calls == std::vector<std::string>{
                  "read:4100", "actor-manager", "read:4096", "create",
                  "read:4100", "actor-manager", "read:4096", "create",
                  "read:4100"},
          "each creation reacquires manager and rereads count");

    runtime.calls.clear();
    runtime.count_reads = {0xffffffffu};
    Check(pal4::CombatInventoryEventCallbacks::CreateMonsters(
              runtime.event, runtime) == -1 &&
              runtime.calls == std::vector<std::string>{"read:4100"},
          "signed negative count creates no monsters");
}

}  // namespace

int main() {
    try {
        CounterAndItem();
        Monsters();
        std::cout << "combat inventory event callbacks tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
