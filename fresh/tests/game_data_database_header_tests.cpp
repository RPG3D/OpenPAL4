#include "pal4/game_data_database_header.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataDatabaseInitializationRuntime {
    bool table_present = true;
    std::uint32_t missing_row = 0;
    int warning_result = -17;
    std::vector<pal4::GameDataAiKeyEntry> keys;
    std::vector<float> values;
    std::vector<std::string> calls;
    std::vector<std::string> errors;
    std::vector<std::uint32_t> warning_lines;
    std::vector<std::string> warning_details;
    std::uint32_t upgrade_factory = 0x9000;
    std::uint32_t drop_factory = 0xa000;

    Runtime() : values(33, 0.0f) {
        for (std::uint32_t key = 1; key <= 32; ++key) {
            keys.push_back({key, key});
            values[key] = static_cast<float>(key);
        }
    }
    bool OpenTable(std::string_view name) override {
        Check(name == "CombatFormulaConfig", "database table name");
        calls.emplace_back("open");
        return table_present;
    }
    std::span<const pal4::GameDataAiKeyEntry> Keys() override {
        calls.emplace_back("keys");
        return keys;
    }
    bool HasRow(std::uint32_t row_index) override {
        calls.emplace_back("row " + std::to_string(row_index));
        return row_index != missing_row;
    }
    float ReadFloatProperty(std::uint32_t row_index,
                            std::uint32_t property_index) override {
        Check(property_index == 1, "formula reads float field one");
        calls.emplace_back("value " + std::to_string(row_index));
        return values.at(row_index);
    }
    void LogError(std::string_view message) override {
        errors.emplace_back(message);
        calls.emplace_back("error");
    }
    int LogRangeWarning(std::uint32_t source_line,
                        std::string_view detail) override {
        warning_lines.push_back(source_line);
        warning_details.emplace_back(detail);
        calls.emplace_back("warning");
        return warning_result;
    }
    std::uint32_t FindFactory(std::uint32_t registry,
                              std::string_view name) override {
        Check(registry == 0x7000, "factory lookup uses member +136");
        calls.emplace_back("find " + std::string(name));
        if (name == "UpgradeParam") return upgrade_factory;
        if (name == "DropProperty") return drop_factory;
        throw std::runtime_error("unknown factory name");
    }
};

struct FactoryRuntime final : pal4::IGameDataDatabaseFactoryRuntime {
    struct Call {
        std::uint32_t registry;
        std::string name;
        std::uint32_t constructor;
    };
    std::vector<Call> calls;

    std::uint32_t RegisterFactory(std::uint32_t registry,
                                  std::string_view name,
                                  std::uint32_t constructor) override {
        calls.push_back({registry, std::string(name), constructor});
        return calls.size() == 1 ? 0x1111u : 0x12abu;
    }
};

void CheckFormulaSuccessAndRanges() {
    Runtime runtime;
    pal4::GameDataDatabaseHeader header;
    header.Data().fill(0xdeadbeefu);
    Check(header.LoadCombatFormulas(runtime) == 0x100 &&
              runtime.calls.front() == "open" &&
              runtime.warning_lines.empty() &&
              runtime.calls.size() == 97,
          "32 sorted keys load and increasing ranges return x87 less mask");
    for (std::uint32_t key = 1; key <= 32; ++key)
        Check(std::bit_cast<float>(header.Data()[key - 1]) ==
                  static_cast<float>(key),
              "formula value lands in its matching float slot");
    Check(header.Data()[32] == 0xdeadbeefu &&
              header.Data()[33] == 0xdeadbeefu &&
              header.Data()[34] == 0xdeadbeefu,
          "formula load leaves the final three words untouched");

    runtime.values[8] = runtime.values[7];
    Check(header.LoadCombatFormulas(runtime) == 0x4000,
          "equal magic endpoints return the x87 equal mask");
    runtime.values[8] = std::nanf("");
    Check(header.LoadCombatFormulas(runtime) == 0x4100,
          "unordered magic endpoints return the x87 unordered mask");

    runtime.values[3] = 0.5f;
    runtime.values[8] = 8.0f;
    Check(header.LoadCombatFormulas(runtime) == 0x100 &&
              runtime.warning_lines.back() == 0x73 &&
              runtime.warning_details.back() ==
                  "physical attack param2 data abnormal!!",
          "inverted physical range logs and continues");
    runtime.values[8] = 1.0f;
    Check(header.LoadCombatFormulas(runtime) == -17 &&
              runtime.warning_lines.back() == 0x7f &&
              runtime.warning_details.back() ==
                  "magic attack param2 data abnormal!!",
          "inverted magic range returns the log result");
}

void CheckFormulaFailures() {
    pal4::GameDataDatabaseHeader header;
    header.Data().fill(0xaaaaaaaau);
    Runtime no_table;
    no_table.table_present = false;
    bool threw = false;
    try {
        (void)header.LoadCombatFormulas(no_table);
    } catch (const std::runtime_error& error) {
        threw = true;
        Check(std::string(error.what()) == no_table.errors.at(0) &&
                  no_table.errors.at(0).find("combatFormulaConfig Table") !=
                      std::string::npos &&
                  header.Data()[0] == 0xaaaaaaaau,
              "missing table logs and throws before writing values");
    }
    Check(threw && no_table.calls == std::vector<std::string>{"open", "error"},
          "missing table bypasses row lookups");

    Runtime no_row;
    no_row.missing_row = 3;
    threw = false;
    try {
        (void)header.LoadCombatFormulas(no_row);
    } catch (const std::runtime_error& error) {
        threw = true;
        Check(std::string(error.what()) == no_row.errors.at(0) &&
                  no_row.errors.at(0).find("key:3") != std::string::npos &&
                  std::bit_cast<float>(header.Data()[0]) == 1.0f &&
                  std::bit_cast<float>(header.Data()[1]) == 2.0f &&
                  header.Data()[2] == 0xaaaaaaaau,
              "missing row reports key and preserves prior partial writes");
    }
    Check(threw, "missing row throws");

    Runtime no_key;
    no_key.keys.erase(no_key.keys.begin() + 4);
    threw = false;
    try {
        (void)header.LoadCombatFormulas(no_key);
    } catch (const std::runtime_error& error) {
        threw = true;
        Check(std::string(error.what()).find("key:5") != std::string::npos,
              "missing sorted key follows the missing-row path");
    }
    Check(threw, "missing sorted key throws");
}

void CheckAccessAndFactories() {
    pal4::GameDataDatabaseHeader header;
    header.Data()[0] = std::bit_cast<std::uint32_t>(-1.5f);
    header.Data()[31] = std::bit_cast<std::uint32_t>(42.25f);
    Check(header.FloatAt(0) == -1.5 && header.FloatAt(31) == 42.25 &&
              header.FloatAt(-1) == 0.0 && header.FloatAt(32) == 0.0 &&
              header.FloatAt(0x7fffffff) == 0.0,
          "float accessor uses signed bounds and zero fallback");
    header.Data()[32] = 0xdeadbeefu;
    header.Data()[33] = 0xa5a5a5a5u;
    Check(header.SetFactoryRegistry(0x7000) == 0x7000 &&
              header.Data()[34] == 0x7000 &&
              header.Data()[32] == 0xdeadbeefu &&
              header.Data()[33] == 0xa5a5a5a5u,
          "registry setter only writes +136 and returns the input");
    FactoryRuntime runtime;
    Check(header.RegisterFactories(runtime) == 0xabu &&
              runtime.calls.size() == 2 &&
              runtime.calls[0].registry == 0x7000 &&
              runtime.calls[0].name == "UpgradeParam" &&
              runtime.calls[0].constructor == 0x4cfee0 &&
              runtime.calls[1].registry == 0x7000 &&
              runtime.calls[1].name == "DropProperty" &&
              runtime.calls[1].constructor == 0x4d0910,
          "two factories register in order and return second low byte");
}

void CheckDatabaseInitialization() {
    pal4::GameDataDatabaseHeader header;
    (void)header.SetFactoryRegistry(0x7000);
    Runtime runtime;
    Check(header.InitializeFromDatabase(runtime) == 1 &&
              header.Data()[32] == 0x9000 &&
              header.Data()[33] == 0xa000 &&
              runtime.calls[runtime.calls.size() - 2] ==
                  "find UpgradeParam" &&
              runtime.calls.back() == "find DropProperty" &&
              std::bit_cast<float>(header.Data()[31]) == 32.0f,
          "database initializes formulas before both named tables");

    Runtime no_upgrade;
    no_upgrade.upgrade_factory = 0;
    header.Data()[32] = 0xdeadbeefu;
    header.Data()[33] = 0xdeadbeefu;
    bool threw = false;
    try {
        (void)header.InitializeFromDatabase(no_upgrade);
    } catch (const std::runtime_error& error) {
        threw = true;
        Check(std::string(error.what()) == no_upgrade.errors.at(0) &&
                  no_upgrade.errors.at(0).find("UpgradeParam") !=
                      std::string::npos &&
                  header.Data()[32] == 0 &&
                  header.Data()[33] == 0xdeadbeefu &&
                  no_upgrade.calls.back() == "error",
              "missing first table writes null, logs and stops");
    }
    Check(threw, "missing first table throws");

    Runtime no_drop;
    no_drop.drop_factory = 0;
    threw = false;
    try {
        (void)header.InitializeFromDatabase(no_drop);
    } catch (const std::runtime_error& error) {
        threw = true;
        Check(std::string(error.what()) == no_drop.errors.at(0) &&
                  no_drop.errors.at(0).find("DropProperty") !=
                      std::string::npos &&
                  header.Data()[32] == 0x9000 &&
                  header.Data()[33] == 0,
              "missing second table preserves first result and writes null");
    }
    Check(threw, "missing second table throws");
}

}  // namespace

int main() {
    try {
        pal4::GameDataDatabaseHeader header;
        header.Data().fill(0xdeadbeefu);
        Check(&header.Reset() == &header,
              "database constructor returns original object");
        for (std::size_t index = 0; index < 32; ++index)
            Check(header.Data()[index] == 0,
                  "database constructor clears first 128 bytes");
        Check(header.Data()[32] == 0xdeadbeefu &&
                  header.Data()[33] == 0xdeadbeefu &&
                  header.Data()[34] == 0,
              "constructor preserves two middle words and clears +136");
        CheckFormulaSuccessAndRanges();
        CheckFormulaFailures();
        CheckAccessAndFactories();
        CheckDatabaseInitialization();
        std::cout << "game_data_database_header_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
