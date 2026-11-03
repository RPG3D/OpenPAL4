#include "pal4/game_data_property_table.h"

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

struct Runtime final : pal4::IGameDataPropertyTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::uint32_t missing_logs = 0;
    std::uint32_t action_calls = 0;
    int action_result = 1;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
    void LogMissingRecord() override { ++missing_logs; }
    int CanStartAction() override {
        ++action_calls;
        return action_result;
    }
};

Runtime IndexedTable() {
    Runtime runtime;
    runtime.words[0x1004] = 0x2000;
    runtime.words[0x2008] = 0x3000;
    runtime.words[0x200c] = 0x3018;
    runtime.words[0x2060] = 0x5000;
    runtime.words[0x3000] = static_cast<std::uint32_t>(-5);
    runtime.words[0x3004] = 2;
    runtime.words[0x3008] = 0;
    runtime.words[0x300c] = 0;
    runtime.words[0x3010] = 7;
    runtime.words[0x3014] = 1;
    return runtime;
}

void CheckRecordRules() {
    Runtime runtime;
    runtime.words[0x400c] = 5;
    runtime.words[0x4004] = 4;
    runtime.words[0x4008] = 6;
    Check(pal4::GameDataPropertyTable::IsValidCategory(0x4000, runtime) &&
              runtime.reads ==
                  std::vector<std::uint32_t>{0x400c, 0x4004, 0x4008},
          "three category bounds include 5, 4 and 6 in IDA order");
    runtime.words[0x400c] = 6;
    runtime.reads.clear();
    Check(!pal4::GameDataPropertyTable::IsValidCategory(0x4000, runtime) &&
              runtime.reads == std::vector<std::uint32_t>{0x400c},
          "first category failure short-circuits");
    runtime.words[0x400c] = 5;
    runtime.words[0x4004] = 5;
    runtime.reads.clear();
    Check(!pal4::GameDataPropertyTable::IsValidCategory(0x4000, runtime) &&
              runtime.reads ==
                  std::vector<std::uint32_t>{0x400c, 0x4004},
          "second category failure short-circuits");
    runtime.words[0x4004] = 4;
    runtime.words[0x4008] = 7;
    Check(!pal4::GameDataPropertyTable::IsValidCategory(0x4000, runtime),
          "third category rejects seven");
    for (const auto offset : {4u, 8u, 12u}) {
        runtime.words[0x4008] = 6;
        runtime.words[0x4000 + offset] = 0xffffffffu;
        Check(!pal4::GameDataPropertyTable::IsValidCategory(0x4000, runtime),
              "negative values fail nonnegative category checks");
        runtime.words[0x4000 + offset] = offset == 4 ? 4 :
                                             offset == 8 ? 6 : 5;
    }
    runtime.words[0x4008] = 5;
    Check(pal4::GameDataPropertyTable::IsAiCommandBelowSix(0x4000, runtime),
          "AI command five is below six");
    runtime.words[0x4008] = 6;
    Check(!pal4::GameDataPropertyTable::IsAiCommandBelowSix(0x4000, runtime),
          "AI command six is excluded");
    runtime.words[0x4008] = 0xffffffffu;
    Check(pal4::GameDataPropertyTable::IsAiCommandBelowSix(0x4000, runtime),
          "AI command comparison is signed");

    runtime.words[0x4004] = 0;
    Check(pal4::GameDataPropertyTable::CanUseRecord(0x4000, runtime) &&
              runtime.action_calls == 0,
          "zero flag allows use without inspecting ID or actor");
    runtime.words[0x4004] = 1;
    runtime.words[0x4000] = 42;
    Check(!pal4::GameDataPropertyTable::CanUseRecord(0x4000, runtime) &&
              runtime.action_calls == 0,
          "other IDs are rejected without querying actor state");
    for (std::uint32_t id : {3067u, 3182u}) {
        runtime.words[0x4000] = id;
        for (int action : {-1, 0, 1}) {
            runtime.action_result = action;
            const auto before = runtime.action_calls;
            Check(pal4::GameDataPropertyTable::CanUseRecord(0x4000, runtime) ==
                      (action > 0) &&
                      runtime.action_calls == before + 1,
                  "special IDs use positive actor action result");
        }
    }
}

void CheckLookup() {
    auto runtime = IndexedTable();
    Check(pal4::GameDataPropertyTable::FindOrLog(0x1000, -5, runtime) ==
              0x5000 + 2 * 168 &&
              pal4::GameDataPropertyTable::FindOrLog(0x1000, 0, runtime) ==
                  0x5000 &&
              pal4::GameDataPropertyTable::FindOrLog(0x1000, 7, runtime) ==
                  0x5000 + 168 &&
              runtime.missing_logs == 0,
          "signed keys resolve 168-byte indexed records");
    for (std::int32_t key : {-6, -4, 1, 8})
        Check(pal4::GameDataPropertyTable::FindOrLog(
                  0x1000, key, runtime) == 0,
              "missing lower, middle and upper keys return zero");
    Check(runtime.missing_logs == 4 &&
              pal4::GameDataPropertyTable::Contains(0x1000, -5, runtime) &&
              !pal4::GameDataPropertyTable::Contains(0x1000, 1, runtime) &&
              runtime.missing_logs == 4,
          "contains reuses lookup without logging missing keys");

    runtime = IndexedTable();
    runtime.words[0x200c] = 0x3000;
    Check(pal4::GameDataPropertyTable::FindOrLog(0x1000, 0, runtime) == 0 &&
              !pal4::GameDataPropertyTable::Contains(0x1000, 0, runtime) &&
              runtime.missing_logs == 1,
          "empty index logs only from FindOrLog");
    runtime = IndexedTable();
    runtime.words[0x2060] = 0;
    Check(pal4::GameDataPropertyTable::FindOrLog(0x1000, 0, runtime) == 0 &&
              runtime.missing_logs == 1 &&
              pal4::GameDataPropertyTable::Contains(0x1000, -5, runtime),
          "zero base with index zero logs while later indices retain arithmetic");
}

struct LoadRuntime final : pal4::IGameDataPropertyTableLoadRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t table = 0x3000;
    std::uint32_t property = 0x4000;
    std::uint8_t preload_result = 0x6a;
    std::string error;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address));
    }
    std::uint8_t PreloadTable(std::uint32_t database,
                              std::string_view name,
                              std::uint32_t factory) override {
        Check(database == 0x2000 && name == "Property" &&
                  factory == 0x4e57e0,
              "preload receives Property name and Effect factory");
        calls.push_back("preload");
        return preload_result;
    }
    std::uint32_t FindTable(std::uint32_t database,
                            std::string_view name) override {
        Check(database == 0x2000 && name == "Property",
              "verify locates Property table");
        calls.push_back("find table");
        return table;
    }
    std::uint32_t FindProperty(std::uint32_t database,
                               std::string_view name) override {
        Check(database == 0x2000 && name == "PropertyClass",
              "verify locates PropertyClass property");
        calls.push_back("find property");
        return property;
    }
    void LogError(std::string_view message) override {
        error = message;
        calls.push_back("log");
    }
};

void CheckLoad() {
    LoadRuntime runtime;
    runtime.words[0x1000] = 0x2000;
    Check(pal4::GameDataPropertyTable::Preload(0x1000, runtime) == 0x6a &&
              runtime.calls ==
                  std::vector<std::string>{"read", "preload"},
          "preload forwards database and byte return");
    runtime.calls.clear();
    Check(pal4::GameDataPropertyTable::Verify(0x1000, runtime) == 1 &&
              runtime.words[0x1004] == 0x3000 &&
              runtime.words[0x1008] == 0x4000 &&
              runtime.calls ==
                  std::vector<std::string>{"read", "find table",
                                           "write 4100", "find property",
                                           "write 4104"},
          "verify stores table then property in order");

    runtime.calls.clear();
    runtime.table = 0;
    try {
        (void)pal4::GameDataPropertyTable::Verify(0x1000, runtime);
        Check(false, "missing Property table throws");
    } catch (const std::runtime_error& exception) {
        Check(runtime.error == exception.what() &&
                  runtime.error ==
                      "ERROR [ palDBPropertyManager::verifyDataTable < "
                      "the table Property  doesn't exist>]" &&
                  runtime.words[0x1004] == 0 &&
                  runtime.calls ==
                      std::vector<std::string>{"read", "find table",
                                               "write 4100", "log"},
              "missing table stores zero, logs exact error and throws");
    }

    runtime.calls.clear();
    runtime.table = 0x3000;
    runtime.property = 0;
    try {
        (void)pal4::GameDataPropertyTable::Verify(0x1000, runtime);
        Check(false, "missing PropertyClass property throws");
    } catch (const std::runtime_error& exception) {
        Check(runtime.error == exception.what() &&
                  runtime.error ==
                      "ERROR [ palDBPropertyManager::verifyDataTable < "
                      "the table PropertyClass  doesn't exist>]" &&
                  runtime.words[0x1008] == 0 &&
                  runtime.calls ==
                      std::vector<std::string>{"read", "find table",
                                               "write 4100", "find property",
                                               "write 4104", "log"},
              "missing property stores zero, logs exact error and throws");
    }
}

}  // namespace

int main() {
    try {
        CheckRecordRules();
        CheckLookup();
        CheckLoad();
        std::cout << "game_data_property_table_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
