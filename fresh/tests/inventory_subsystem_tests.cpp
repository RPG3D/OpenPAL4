#include "pal4/inventory_subsystem.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IInventorySubsystemRuntime {
    std::vector<std::pair<std::string, std::uint32_t>> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::set<std::uint32_t> first_ids;
    std::set<std::uint32_t> second_ids;
    std::set<std::uint32_t> third_ids;
    std::vector<std::uint32_t> first_processed;
    std::vector<std::uint32_t> second_calculated;
    std::vector<std::uint32_t> items_processed;
    std::vector<std::uint32_t> third_processed;
    std::uint32_t game_db_calls = 0u;
    std::uint32_t first_checks = 0u;
    std::uint32_t second_checks = 0u;
    std::uint32_t third_checks = 0u;
    std::uint32_t allocation = 0x5000u;
    std::uint32_t registration_result = 0x3000u;
    std::uint32_t name = 0u;
    std::uint32_t singleton_id = 0u;
    std::uint32_t factory = 0u;
    std::uint32_t cleanup = 0u;
    bool throw_on_array = false;

    std::uint32_t RegisterSingleton(
        std::uint32_t name_address, std::uint32_t id,
        std::uint32_t factory_address,
        std::uint32_t cleanup_address) override {
        name = name_address;
        singleton_id = id;
        factory = factory_address;
        cleanup = cleanup_address;
        calls.emplace_back("register", id);
        return registration_result;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        calls.emplace_back("allocate", bytes);
        return allocation;
    }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        calls.emplace_back("free", address);
        return 0xbeefu;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void InitializeBase(std::uint32_t object) override {
        calls.emplace_back("init base", object);
    }
    void InitializeSlots(std::uint32_t slots) override {
        calls.emplace_back("init slots", slots);
    }
    void InitializeArray(std::uint32_t array) override {
        calls.emplace_back("init array", array);
        if (throw_on_array) throw std::runtime_error("array construction");
    }
    void CleanupArray(std::uint32_t array) override {
        calls.emplace_back("cleanup array", array);
    }
    void CleanupSlots(std::uint32_t slots) override {
        calls.emplace_back("cleanup slots", slots);
    }
    void CleanupBase(std::uint32_t object) override {
        calls.emplace_back("cleanup base", object);
    }
    std::uint32_t GameDbInstance() override {
        ++game_db_calls;
        return 0x9000u;
    }
    bool HasFirst(std::uint32_t collection,
                  std::uint32_t id) override {
        Check(collection == 0x9080u && id >= 8000u && id < 8500u,
              "first GameDB range and collection");
        ++first_checks;
        return first_ids.contains(id);
    }
    void ProcessFirst(std::uint32_t collection,
                      std::uint32_t id) override {
        Check(collection == 0x5104u, "first inventory member offset");
        first_processed.push_back(id);
    }
    bool HasSecond(std::uint32_t database,
                   std::uint32_t id) override {
        Check(database == 0x9000u && id >= 3000u && id < 3500u,
              "second GameDB range and base");
        ++second_checks;
        return second_ids.contains(id);
    }
    void CalculateSecond(std::uint32_t database,
                         std::uint32_t id) override {
        Check(database == 0x9000u, "second calculation GameDB base");
        second_calculated.push_back(id);
    }
    void ProcessItem(std::uint32_t object, std::uint32_t id,
                     std::uint32_t count) override {
        Check(object == 0x5000u && count == 99u,
              "second item count and owner");
        items_processed.push_back(id);
    }
    std::uint8_t HasThird(std::uint32_t collection,
                          std::uint32_t id) override {
        Check(collection == 0x9058u && id >= 3500u && id < 4000u,
              "third GameDB range and collection");
        ++third_checks;
        return third_ids.contains(id) ? 1u : 0u;
    }
    std::uint32_t ProcessThird(std::uint32_t collection,
                               std::uint32_t id) override {
        Check(collection == 0x507cu, "third inventory member offset");
        third_processed.push_back(id);
        return id == 3999u ? 0x1234abu : 0x123456u;
    }
};

struct ResetRuntime final : pal4::IInventoryResetRuntime {
    std::vector<std::pair<std::string, std::uint32_t>> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write", address);
    }
    void ResetSlots(std::uint32_t slots) override {
        calls.emplace_back("slots", slots);
    }
    void ResetArray(std::uint32_t array) override {
        calls.emplace_back("array", array);
    }
    std::uint32_t ResetBase(std::uint32_t object) override {
        calls.emplace_back("base", object);
        return 0xbeefu;
    }
};

void CheckSingleton() {
    Runtime runtime;
    std::uint8_t guard = 0x80u;
    std::uint32_t cached = 0u;
    Check(pal4::InventorySubsystem::InitializeSingleton(
              guard, cached, runtime) == 0x3000u &&
              guard == 0x81u && cached == 0x3000u &&
              runtime.name == pal4::InventorySubsystem::kNameAddress &&
              runtime.singleton_id == 1024u &&
              runtime.factory == 0x4c5ec0u &&
              runtime.cleanup == 0x4c5f20u &&
              runtime.calls ==
                  std::vector<std::pair<std::string, std::uint32_t>>{
                      {"register", 1024u}},
          "first singleton registration forwards name, id and callbacks");
    runtime.registration_result = 0x6000u;
    Check(pal4::InventorySubsystem::InitializeSingleton(
              guard, cached, runtime) == 0x3000u &&
              runtime.calls.size() == 1u,
          "set guard returns cached singleton without registration");
}

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::InventorySubsystem::Create(runtime) == 0x5000u &&
              runtime.calls ==
                  std::vector<std::pair<std::string, std::uint32_t>>{
                      {"allocate", 0x138u},
                      {"init base", 0x5000u},
                      {"init slots", 0x507cu},
                      {"init array", 0x5104u}},
          "create allocates 312 bytes and constructs three members in order");
    runtime.calls.clear();
    Check(pal4::InventorySubsystem::Destroy(0x5000u, runtime) ==
              0xbeefu &&
              runtime.calls ==
                  std::vector<std::pair<std::string, std::uint32_t>>{
                      {"cleanup array", 0x5104u},
                      {"cleanup slots", 0x507cu},
                      {"cleanup base", 0x5000u},
                      {"free", 0x5000u}},
          "destroy cleans members in reverse order and returns free result");
    runtime.calls.clear();
    Check(pal4::InventorySubsystem::Destroy(0u, runtime) == 0u &&
              runtime.calls.empty(),
          "null destroy does not touch runtime");
    pal4::InventorySubsystem::Cleanup(0u, runtime);
    Check(runtime.calls ==
              std::vector<std::pair<std::string, std::uint32_t>>{
                  {"cleanup array", 0u},
                  {"cleanup slots", 0u},
                  {"cleanup base", 0u}},
          "direct cleanup forwards null to all three child destructors");
}

void CheckCreationFailure() {
    Runtime runtime;
    runtime.allocation = 0u;
    Check(pal4::InventorySubsystem::Create(runtime) == 0u &&
              runtime.calls ==
                  std::vector<std::pair<std::string, std::uint32_t>>{
                      {"allocate", 0x138u}},
          "null allocation skips member construction");
    runtime.allocation = 0x5000u;
    runtime.throw_on_array = true;
    runtime.calls.clear();
    bool threw = false;
    try {
        (void)pal4::InventorySubsystem::Create(runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw &&
              runtime.calls ==
                  std::vector<std::pair<std::string, std::uint32_t>>{
                      {"allocate", 0x138u},
                      {"init base", 0x5000u},
                      {"init slots", 0x507cu},
                      {"init array", 0x5104u},
                      {"free", 0x5000u}},
          "construction exception releases the allocated instance");
}

void CheckCounter() {
    Runtime runtime;
    runtime.words[0x5134u] = 5u;
    Check(pal4::InventorySubsystem::UpdateCounter(
              0x5000u, -3, runtime) == 2 &&
              runtime.words[0x5134u] == 2u,
          "counter adds signed delta at object offset 0x134");
    Check(pal4::InventorySubsystem::UpdateCounter(
              0x5000u, -10, runtime) == 0 &&
              runtime.words[0x5134u] == 0u,
          "negative counter result clamps to zero");
    Check(pal4::InventorySubsystem::UpdateCounter(
              0x5000u, 10000000, runtime) == 9999999 &&
              runtime.words[0x5134u] == 9999999u,
          "positive counter result clamps to 9999999");
    runtime.words[0x5134u] = 0x7fffffffu;
    Check(pal4::InventorySubsystem::UpdateCounter(
              0x5000u, 1, runtime) == 0,
          "32-bit signed overflow wraps before clamp");
    runtime.words[0x5134u] = 0x80000000u;
    Check(pal4::InventorySubsystem::UpdateCounter(
              0x5000u, -1, runtime) == 9999999,
          "negative overflow wraps positive before upper clamp");
}

void CheckGameDataInitialization() {
    Runtime runtime;
    runtime.first_ids = {8000u, 8499u};
    runtime.second_ids = {3000u, 3499u};
    runtime.third_ids = {3500u, 3999u};
    Check(pal4::InventorySubsystem::InitializeGameData(
              0x5000u, runtime) == 0xabu &&
              runtime.game_db_calls == 1502u &&
              runtime.first_checks == 500u &&
              runtime.second_checks == 500u &&
              runtime.third_checks == 500u &&
              runtime.first_processed ==
                  std::vector<std::uint32_t>{8000u, 8499u} &&
              runtime.second_calculated ==
                  std::vector<std::uint32_t>{3000u, 3499u} &&
              runtime.items_processed ==
                  std::vector<std::uint32_t>{3000u, 3499u} &&
              runtime.third_processed ==
                  std::vector<std::uint32_t>{3500u, 3999u},
          "all three 500-id ranges, selected actions and final low byte");
    Runtime empty;
    Check(pal4::InventorySubsystem::InitializeGameData(
              0x5000u, empty) == 0u &&
              empty.game_db_calls == 1500u &&
              empty.first_processed.empty() &&
              empty.items_processed.empty() &&
              empty.third_processed.empty(),
          "no matches still checks every ID and returns false");
}

void CheckReset() {
    ResetRuntime runtime;
    Check(pal4::InventorySubsystem::Reset(0x5000u, runtime) ==
              0xbeefu &&
              runtime.words[0x5134u] == 0u &&
              runtime.words[0x5080u] == 1u &&
              runtime.words[0x5004u] == 1u &&
              runtime.calls ==
                  std::vector<std::pair<std::string, std::uint32_t>>{
                      {"write", 0x5134u}, {"slots", 0x507cu},
                      {"write", 0x5080u}, {"array", 0x5104u},
                      {"base", 0x5000u}, {"write", 0x5004u}},
          "reset clears counter, resets members and preserves base result");
}

}  // namespace

int main() {
    try {
        CheckSingleton();
        CheckLifecycle();
        CheckCreationFailure();
        CheckCounter();
        CheckGameDataInitialization();
        CheckReset();
        std::cout << "inventory_subsystem_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
