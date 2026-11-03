#include "pal4/game_data_ai_config.h"

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

struct Runtime final : pal4::IGameDataAiConfigRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{0x1000, 0x2000},
                                                  {0x1008, 0x3456}};
    std::vector<std::string> calls;
    std::uint32_t property = 0x3000;
    std::uint32_t final_registration = 0x1234;
    std::uint32_t config_table = 0x3000;
    std::uint32_t property_table = 0x4000;
    std::string logged;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t GetPropertyString(std::uint32_t owner,
                                    std::string_view name) override {
        Check(owner == 0x2000 && name == "AiCommandType",
              "property query receives owner and exact key");
        calls.emplace_back("property");
        return property;
    }
    std::uint32_t FindEntry(std::uint32_t target,
                            std::uint32_t value) override {
        Check(target == 0x3000 && value == 0x9876,
              "entry search receives property and requested value");
        calls.emplace_back("find");
        return 0x4567;
    }
    std::uint32_t RegisterFactory(std::uint32_t owner,
                                  std::string_view name,
                                  std::uint32_t factory_address) override {
        Check(owner == 0x2000, "factory registration receives owner");
        if (name == "AiConfig") {
            Check(factory_address == 0x4cc040,
                  "AiConfig uses the first IDA factory");
            calls.emplace_back("config");
            return 0x99;
        }
        Check(name == "AiProperty" && factory_address == 0x4ccb60,
              "AiProperty uses the second IDA factory");
        calls.emplace_back("property factory");
        return final_registration;
    }
    std::uint32_t LookupTable(std::uint32_t owner,
                              std::string_view name) override {
        Check(owner == 0x2000, "table lookup receives database owner");
        calls.push_back("lookup " + std::string(name));
        if (name == "AiConfig") return config_table;
        Check(name == "AiProperty", "only the second required table follows");
        return property_table;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address));
    }
    void LogError(std::string_view message) override {
        logged = message;
        calls.emplace_back("log");
    }
};

void CheckLookup() {
    Runtime runtime;
    Check(pal4::GameDataAiConfig::FindCommandType(
              0x1000, 0x9876, runtime) == 0x4567 &&
              runtime.calls == std::vector<std::string>{"read 4096",
                                                         "property", "find"},
          "found property delegates entry search in order");
    runtime = Runtime{};
    runtime.property = 0;
    Check(pal4::GameDataAiConfig::FindCommandType(
              0x1000, 0x9876, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"read 4096",
                                                         "property"},
          "missing property returns without entry search");
    runtime = Runtime{};
    Check(pal4::GameDataAiConfig::MemberAt8(0x1000, runtime) == 0x3456 &&
              runtime.calls == std::vector<std::string>{"read 4104"},
          "third DWORD accessor reads only object plus eight");
}

void CheckRegistration() {
    Runtime runtime;
    Check(pal4::GameDataAiConfig::RegisterFactories(
              0x1000, runtime) == 0x34 &&
              runtime.calls == std::vector<std::string>{
                  "read 4096", "config", "property factory"},
          "both factories register in order and return low byte of second");
    runtime = Runtime{};
    runtime.final_registration = 0;
    Check(pal4::GameDataAiConfig::RegisterFactories(
              0x1000, runtime) == 0,
          "zero registration result is forwarded");
}

void CheckDatabaseInitialization() {
    Runtime runtime;
    Check(pal4::GameDataAiConfig::InitializeFromDatabase(
              0x1000, runtime) == 1 &&
              runtime.words[0x1004] == 0x3000 &&
              runtime.words[0x1008] == 0x4000 &&
              runtime.calls == std::vector<std::string>{
                  "read 4096", "lookup AiConfig", "write 4100",
                  "lookup AiProperty", "write 4104"},
          "both required tables resolve and write in order");

    runtime = Runtime{};
    runtime.config_table = 0;
    try {
        (void)pal4::GameDataAiConfig::InitializeFromDatabase(0x1000, runtime);
        Check(false, "missing AiConfig should throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR [ palDBAiManager::verifyDataTable < the table "
                  "AiConfig  doesn't exist>]" &&
                  runtime.logged == error.what() &&
                  runtime.words[0x1004] == 0 &&
                  runtime.calls == std::vector<std::string>{
                      "read 4096", "lookup AiConfig", "write 4100", "log"},
              "first missing table logs exact text after writing null");
    }

    runtime = Runtime{};
    runtime.property_table = 0;
    try {
        (void)pal4::GameDataAiConfig::InitializeFromDatabase(0x1000, runtime);
        Check(false, "missing AiProperty should throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR [ palDBAiManager::verifyDataTable < the table "
                  "AiProperty  doesn't exist>]" &&
                  runtime.logged == error.what() &&
                  runtime.words[0x1004] == 0x3000 &&
                  runtime.words[0x1008] == 0 &&
                  runtime.calls == std::vector<std::string>{
                      "read 4096", "lookup AiConfig", "write 4100",
                      "lookup AiProperty", "write 4104", "log"},
              "second missing table preserves first and logs exact text");
    }
}

}  // namespace

int main() {
    try {
        CheckLookup();
        CheckRegistration();
        CheckDatabaseInitialization();
        std::cout << "game_data_ai_config_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
