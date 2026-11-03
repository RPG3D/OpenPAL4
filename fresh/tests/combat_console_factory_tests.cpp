#include "pal4/combat_console_factory.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatConsoleFactoryRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {pal4::CombatConsoleFactory::kFactorySource, 0x7000u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x8000u;
    bool transfer_event = false;
    std::uint32_t inserted_callback = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        Check(words.count(address) != 0u, "factory word exists");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 8u || bytes == 0x13cu, "allocation size");
        calls.emplace_back(bytes == 8u ? "allocate-callback" : "allocate-console");
        return allocation;
    }
    std::uint32_t ConstructConsole(std::uint32_t storage,
                                    std::string_view name) override {
        Check(storage == allocation && name == "combat-console",
              "constructor arguments");
        calls.emplace_back("construct");
        return storage + 4u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x9000u;
    }
    std::uint32_t InsertFactory(std::uint32_t tree,
                                std::uint32_t create_callback) override {
        Check(tree == 0x7000u &&
                  create_callback == pal4::CombatConsoleFactory::kCreateCallback,
              "factory registration arguments");
        calls.emplace_back("insert-factory");
        return 0xa000u;
    }
    void InsertEvent(std::uint32_t event_tree, std::uint32_t event_key,
                     std::uint32_t& callback_record) override {
        Check(event_tree == 0x9000u + 260u &&
                  event_key == pal4::CombatConsoleFactory::kEventKey,
              "event insertion target");
        inserted_callback = callback_record;
        if (transfer_event) callback_record = 0u;
        calls.emplace_back("insert-event");
    }
    void DeleteCallback(std::uint32_t callback_record,
                         std::uint8_t flags) override {
        Check(callback_record == allocation && flags == 1u,
              "temporary callback deletion");
        calls.emplace_back("delete");
    }
};

void InstanceAndCreation() {
    Runtime runtime;
    Check(pal4::CombatConsoleFactory::GetInstanceThunk(runtime) == 0x7000u &&
              runtime.words.at(pal4::CombatConsoleFactory::kFactoryCache) ==
                  0x7000u &&
              runtime.calls == std::vector<std::string>{"read", "write"},
          "instance thunk caches factory source");
    runtime.words[pal4::CombatConsoleFactory::kFactorySource] = 0u;
    Check(pal4::CombatConsoleFactory::GetInstance(runtime) == 0u &&
              runtime.words.at(pal4::CombatConsoleFactory::kFactoryCache) == 0u,
          "null instance is cached too");

    Runtime creation;
    Check(pal4::CombatConsoleFactory::Create("combat-console", creation) ==
              0x8004u &&
              creation.calls == std::vector<std::string>{
                  "allocate-console", "construct"},
          "creation allocates and constructs console");
    creation.calls.clear();
    creation.allocation = 0u;
    Check(pal4::CombatConsoleFactory::Create("combat-console", creation) ==
              0u &&
              creation.calls == std::vector<std::string>{"allocate-console"},
          "allocation failure skips constructor");
}

void FactoryAndEventRegistration() {
    Runtime runtime;
    runtime.words[pal4::CombatConsoleFactory::kFactoryCache] = 0x7000u;
    Check(pal4::CombatConsoleFactory::RegisterThunk(runtime) == 0xa000u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "read", "insert-factory"},
          "factory thunk preserves manager call and callback identity");

    for (const bool transferred : {false, true}) {
        Runtime event;
        event.transfer_event = transferred;
        Check(pal4::CombatConsoleFactory::RegisterEventThunk(event) ==
                  pal4::CombatConsoleFactory::kEventSentinel &&
                  event.inserted_callback == 0x8000u &&
                  event.words.at(0x8000u) ==
                      pal4::CombatConsoleFactory::kEventCallbackVtable &&
                  event.words.at(0x8004u) == 0x7000u,
              "event record fields and return");
        Check(event.calls ==
                  (transferred
                       ? std::vector<std::string>{
                             "read", "allocate-callback", "write", "write",
                             "manager", "insert-event"}
                       : std::vector<std::string>{
                             "read", "allocate-callback", "write", "write",
                             "manager", "insert-event", "delete"}),
              "callback transfer controls temporary deletion");
    }

    Runtime no_allocation;
    no_allocation.allocation = 0u;
    Check(pal4::CombatConsoleFactory::RegisterEvent(no_allocation) ==
              pal4::CombatConsoleFactory::kEventSentinel &&
              no_allocation.inserted_callback == 0u &&
              no_allocation.calls == std::vector<std::string>{
                  "read", "allocate-callback", "manager", "insert-event"},
          "null allocation still inserts empty event record");
}

}  // namespace

int main() {
    try {
        InstanceAndCreation();
        FactoryAndEventRegistration();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
