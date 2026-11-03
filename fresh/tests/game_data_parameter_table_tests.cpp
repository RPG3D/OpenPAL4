#include "pal4/game_data_parameter_table.h"

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

struct Runtime final : pal4::IGameDataParameterTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    unsigned missing_logs = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
    void LogMissingRecord() override { ++missing_logs; }
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

void CheckRecordLookup() {
    auto runtime = IndexedTable();
    Check(pal4::GameDataParameterTable::FindOrLog(
              0x1000, -5, runtime) == 0x5000 + 2 * 328 &&
              pal4::GameDataParameterTable::FindOrLog(
                  0x1000, 0, runtime) == 0x5000 &&
              pal4::GameDataParameterTable::FindOrLog(
                  0x1000, 7, runtime) == 0x5000 + 328 &&
              runtime.missing_logs == 0,
          "signed keys map to their indexed 328-byte records");
    Check(pal4::GameDataParameterTable::Contains(0x1000, -5, runtime) &&
              pal4::GameDataParameterTable::Contains(0x1000, 7, runtime) &&
              !pal4::GameDataParameterTable::Contains(0x1000, 1, runtime) &&
              runtime.missing_logs == 0,
          "contains shares lookup without missing-record logging");

    for (std::int32_t key : {-6, -4, 1, 8})
        Check(pal4::GameDataParameterTable::FindOrLog(
                  0x1000, key, runtime) == 0,
              "lower, middle and upper missing keys return zero");
    Check(runtime.missing_logs == 4,
          "each missing FindOrLog call writes one diagnostic");
}

void CheckEmptyAndZeroAddress() {
    auto runtime = IndexedTable();
    runtime.words[0x200c] = 0x3000;
    Check(pal4::GameDataParameterTable::FindOrLog(
              0x1000, 0, runtime) == 0 &&
              !pal4::GameDataParameterTable::Contains(0x1000, 0, runtime) &&
              runtime.missing_logs == 1,
          "empty index returns zero and logs only for FindOrLog");
    runtime = IndexedTable();
    runtime.words[0x2060] = 0;
    Check(pal4::GameDataParameterTable::FindOrLog(
              0x1000, 0, runtime) == 0 &&
              runtime.missing_logs == 1 &&
              pal4::GameDataParameterTable::Contains(
                  0x1000, -5, runtime),
          "zero base at index zero logs; arithmetic for other indices follows IDA");
}

void CheckCategory() {
    Runtime runtime;
    for (std::uint32_t value : {0u, 1u, 2u}) {
        runtime.words[0x4000 + 100] = value;
        Check(pal4::GameDataParameterTable::IsValidCategory(
                  0x4000, runtime),
              "categories zero through two are valid");
    }
    for (std::uint32_t value : {3u, 0xffffffffu}) {
        runtime.words[0x4000 + 100] = value;
        Check(!pal4::GameDataParameterTable::IsValidCategory(
                  0x4000, runtime),
              "category comparison is unsigned and bounded by two");
    }
    Check(runtime.reads.back() == 0x4000 + 100,
          "category reads record word at +100");
}

struct LoadRuntime final : pal4::IGameDataParameterTableLoadRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t table = 0x3000;
    std::uint32_t property = 0x4000;
    std::uint8_t preload_result = 0x6a;
    std::string error;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address));
    }
    std::uint8_t PreloadTable(std::uint32_t database,
                              std::string_view name,
                              std::uint32_t factory) override {
        Check(database == 0x2000 && name == "Prescription" &&
                  factory == 0x4e2ad0,
              "preload receives IDA table name, database and factory");
        calls.emplace_back("preload");
        return preload_result;
    }
    std::uint32_t FindTable(std::uint32_t database,
                            std::string_view name) override {
        Check(database == 0x2000 && name == "Prescription",
              "verify finds Prescription in database");
        calls.emplace_back("find table");
        return table;
    }
    std::uint32_t FindProperty(std::uint32_t database,
                               std::string_view name) override {
        Check(database == 0x2000 && name == "PrescriptionType",
              "verify finds PrescriptionType in database");
        calls.emplace_back("find property");
        return property;
    }
    void LogError(std::string_view message) override {
        error = message;
        calls.emplace_back("log");
    }
};

void CheckLoadAndVerify() {
    LoadRuntime runtime;
    runtime.words[0x1000] = 0x2000;
    Check(pal4::GameDataParameterTable::Preload(
              0x1000, runtime) == 0x6a &&
              runtime.calls == std::vector<std::string>({"read 4096", "preload"}),
          "preload forwards source database and byte result");
    runtime.calls.clear();
    Check(pal4::GameDataParameterTable::Verify(
              0x1000, runtime) == 1 &&
              runtime.words[0x1004] == 0x3000 &&
              runtime.words[0x1008] == 0x4000 &&
              runtime.calls == std::vector<std::string>({
                  "read 4096", "find table", "write 4100",
                  "find property", "write 4104"}),
          "verify stores both table and property in order");

    runtime.calls.clear();
    runtime.table = 0;
    try {
        (void)pal4::GameDataParameterTable::Verify(0x1000, runtime);
        Check(false, "missing Prescription table should throw");
    } catch (const std::runtime_error& error) {
        Check(runtime.error == error.what() &&
                  runtime.error.find("the table Prescription  doesn't exist") !=
                      std::string::npos &&
                  runtime.words[0x1004] == 0 &&
                  runtime.calls.back() == "log" &&
                  runtime.calls.size() == 4,
              "missing table stores zero, logs exact name and throws");
    }

    runtime.calls.clear();
    runtime.table = 0x3000;
    runtime.property = 0;
    try {
        (void)pal4::GameDataParameterTable::Verify(0x1000, runtime);
        Check(false, "missing PrescriptionType property should throw");
    } catch (const std::runtime_error& error) {
        Check(runtime.error == error.what() &&
                  runtime.error.find("the table PrescriptionType  doesn't exist") !=
                      std::string::npos &&
                  runtime.words[0x1008] == 0 &&
                  runtime.calls.back() == "log" &&
                  runtime.calls.size() == 6,
              "missing property stores zero, logs exact name and throws");
    }
}

}  // namespace

int main() {
    try {
        CheckRecordLookup();
        CheckEmptyAndZeroAddress();
        CheckCategory();
        CheckLoadAndVerify();
        std::cout << "game_data_parameter_table_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
