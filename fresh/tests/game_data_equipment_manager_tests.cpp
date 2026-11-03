#include "pal4/game_data_equipment_manager.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataEquipmentManagerRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{0x1000, 0x7000}};
    std::map<std::string, std::uint32_t> found{
        {"Equip", 0x2000}, {"EquipRelation", 0x3000},
        {"EquipClass", 0x4000}, {"EquipType", 0x5000}};
    std::vector<std::string> calls;
    std::string logged;
    std::uint32_t factory_result = 0x1234;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read " + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write " + std::to_string(address));
    }
    std::uint32_t RegisterFactory(std::uint32_t database,
                                  std::string_view name,
                                  std::uint32_t factory_address) override {
        Check(database == 0x7000, "factory database pointer");
        Check((name == "Equip" && factory_address == 0x4d6040) ||
                  (name == "EquipRelation" &&
                   factory_address == 0x4d5640),
              "factory names and code addresses");
        calls.emplace_back("factory " + std::string(name));
        return factory_result;
    }
    std::uint32_t LookupTable(std::uint32_t database,
                              std::string_view name) override {
        Check(database == 0x7000, "lookup database pointer");
        calls.emplace_back("table " + std::string(name));
        return found.at(std::string(name));
    }
    std::uint32_t FindProperty(std::uint32_t database,
                               std::string_view name) override {
        Check(database == 0x7000, "property database pointer");
        calls.emplace_back("property " + std::string(name));
        return found.at(std::string(name));
    }
    void LogError(std::string_view message) override {
        logged = message;
        calls.emplace_back("log");
    }
};

void CheckFactoryAndSuccess() {
    Runtime runtime;
    Check(pal4::GameDataEquipmentManager::RegisterFactories(
              0x1000, runtime) == 0x34 &&
              runtime.calls == std::vector<std::string>{
                                   "read 4096", "factory Equip",
                                   "factory EquipRelation"},
          "both factories register in order and return low byte");
    runtime.calls.clear();
    Check(pal4::GameDataEquipmentManager::InitializeFromDatabase(
              0x1000, runtime) == 1 &&
              runtime.words[0x1008] == 0x2000 &&
              runtime.words[0x1004] == 0x3000 &&
              runtime.words[0x1010] == 0x4000 &&
              runtime.words[0x100c] == 0x5000 &&
              runtime.logged.empty(),
          "four required resources write their own member offsets");
    Check(runtime.calls == std::vector<std::string>{
                               "read 4096", "table Equip", "write 4104",
                               "table EquipRelation", "write 4100",
                               "property EquipClass", "write 4112",
                               "property EquipType", "write 4108"},
          "tables and properties resolve in IDA order");
}

void CheckMissingResources() {
    constexpr std::array<std::string_view, 4> names{
        "Equip", "EquipRelation", "EquipClass", "EquipType"};
    constexpr std::array<std::uint32_t, 4> offsets{8, 4, 16, 12};
    for (std::uint32_t missing = 0; missing < names.size(); ++missing) {
        Runtime runtime;
        runtime.found[std::string(names[missing])] = 0;
        bool thrown = false;
        const auto expected =
            "ERROR [ palDBEquipmentManager::verifyDataTable < the table " +
            std::string(names[missing]) + "  doesn't exist>]";
        try {
            (void)pal4::GameDataEquipmentManager::InitializeFromDatabase(
                0x1000, runtime);
        } catch (const std::runtime_error& error) {
            thrown = expected == error.what();
        }
        Check(thrown && runtime.logged == expected &&
                  runtime.words[0x1000 + offsets[missing]] == 0 &&
                  runtime.calls.back() == "log" &&
                  runtime.calls.size() == 4 + missing * 2,
              "each missing resource is written, logged and thrown before next lookup");
    }
}

}  // namespace

int main() {
    try {
        CheckFactoryAndSuccess();
        CheckMissingResources();
        std::cout << "game_data_equipment_manager_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
