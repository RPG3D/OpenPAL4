#include "pal4/game_data_role_database.h"

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

struct Runtime final : pal4::IGameDataRoleDatabaseRuntime {
    std::vector<std::string> calls;
    std::map<std::string, std::uint32_t> tables{
        {"Role", 0x1000}, {"CombatPrize", 0x2000},
        {"UpgradeData", 0x3000}, {"Race", 0x4000}};
    std::uint8_t RegisterSchema(std::string_view name,
                                std::uint32_t constructor) override {
        calls.push_back("register:" + std::string(name) + ":" +
                        std::to_string(constructor));
        return 0x5a;
    }
    std::uint32_t FindTable(std::string_view name) override {
        calls.push_back("table:" + std::string(name));
        return tables[std::string(name)];
    }
    std::uint32_t FindProperty(std::string_view name) override {
        calls.push_back("property:" + std::string(name));
        return tables[std::string(name)];
    }
    void ReportMissing(std::string_view name) override {
        calls.push_back("missing:" + std::string(name));
    }
};

void CheckDatabase() {
    const std::array<pal4::GameDataRoleIndexEntry, 4> index{{
        {1001, 3}, {1003, 0}, {1010, 7}, {1011, 2}}};
    using Database = pal4::GameDataRoleDatabase;
    Check(Database::FindRoleRecord(index, 0x5000, 0) == 0x5000 + 960,
          "first key resolves to record index");
    Check(Database::FindRoleRecord(index, 0x5000, 2) == 0x5000,
          "middle key resolves to first record");
    Check(Database::FindRoleRecord(index, 0x5000, 9) == 0x5000 + 2240,
          "last key resolves to record index");
    for (std::int32_t id : {-1, 1, 3, 11})
        Check(Database::FindRoleRecord(index, 0x5000, id) == 0,
              "missing keys return null");
    Check(Database::FindRoleRecord({}, 0x5000, 0) == 0,
          "empty index returns null");

    Runtime runtime;
    Database database;
    Check(database.RegisterSchemas(runtime) == 0x5a &&
              runtime.calls == std::vector<std::string>{
                  "register:Role:5152704",
                  "register:UpgradeData:5155888",
                  "register:CombatPrize:5158512"},
          "schema registration names, callbacks and order");
    runtime.calls.clear();
    Check(database.Load(runtime) == 1 &&
              runtime.calls == std::vector<std::string>{
                  "table:Role", "table:CombatPrize",
                  "table:UpgradeData", "property:Race"} &&
              database.role() == 0x1000 &&
              database.combat_prize() == 0x2000 &&
              database.upgrade_data() == 0x3000 &&
              database.race() == 0x4000,
          "database loads in native order into four slots");
    for (const auto* missing : {"Role", "CombatPrize", "UpgradeData", "Race"}) {
        Runtime failure;
        failure.tables[missing] = 0;
        Database failed;
        bool threw = false;
        try {
            static_cast<void>(failed.Load(failure));
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Check(threw && failure.calls.back() ==
                           "missing:" + std::string(missing),
              "first missing table reports then throws");
        const auto expected_count = std::string_view(missing) == "Role" ? 2u :
            std::string_view(missing) == "CombatPrize" ? 3u :
            std::string_view(missing) == "UpgradeData" ? 4u : 5u;
        Check(failure.calls.size() == expected_count,
              "missing table stops later lookups");
    }
}

void CheckUpgradeSelection() {
    using Selector = pal4::GameDataUpgradeSelector;
    const std::array<pal4::GameDataUpgradeRecordView, 7> records{{
        {0x1000, 8, 20, 40}, {0x10a0, 7, 1, 100},
        {0x1140, 8, 15, 40}, {0x11e0, 8, 12, 41},
        {0x1280, 8, 99, 101}, {0x1320, 8, 100, 1000},
        {0x13c0, 8, 12, 99}}};
    Check(Selector::MatchesRole(records[0], 8) &&
              !Selector::MatchesRole(records[1], 8),
          "role predicate reads record +148");
    Check(Selector::FindNextRecord(records, 8, 40) == 0x11e0,
          "lowest qualifying level wins with first tie retained");
    Check(Selector::FindNextRecord(records, 8, 101) == 0,
          "level 100 excluded and value threshold strict");
    Check(Selector::FindNextRecord(records, 9, 0) == 0,
          "missing role returns null record");
    Check(Selector::PreviousValue(records, 8, 12) == 40,
          "first exact level returns record value minus one");
    Check(Selector::PreviousValue(records, 8, 20) == 39 &&
              Selector::PreviousValue(records, 8, 2) == 0,
          "previous value exact and missing cases");
    Check(Selector::NextValue(records, 8, 13) == 41 &&
              Selector::NextValue(records, 8, 21) == 40 &&
              Selector::NextValue(records, 8, 0) == 0 &&
              Selector::NextValue(records, 8, -1) == 0 &&
              Selector::NextValue(records, 8, 3) == 0,
          "next value uses previous level with nonpositive guard");
    Check(Selector::FindNextRecord({}, 8, 0) == 0 &&
              Selector::PreviousValue({}, 8, 0) == 0 &&
              Selector::NextValue({}, 8, 1) == 0,
          "empty record array");
}

}  // namespace

int main() {
    try {
        CheckDatabase();
        CheckUpgradeSelection();
        std::cout << "game_data_role_database_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
