#include "pal4/game_data_mission_table.h"

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

struct Runtime final : pal4::IGameDataMissionTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5000, 0x6000}, {0x5004, 0x7000},
        {0x7008, 0x8000}, {0x700c, 0x8018}, {0x7060, 0x9000},
        {0x8000, static_cast<std::uint32_t>(-3)}, {0x8004, 2},
        {0x8008, 2}, {0x800c, 0},
        {0x8010, 9}, {0x8014, 1},
        {0xa008, static_cast<std::uint32_t>(-2)}, {0xb008, 1}};
    std::vector<std::string> calls;
    std::uint32_t found_table = 0x7000;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        Check(words.contains(address), "only observed Mission addresses read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write");
        Check(address == 0x5004, "Mission table stored at manager +4");
        words[address] = value;
    }
    std::uint8_t PreloadTable(std::uint32_t database,
                              std::string_view name,
                              std::uint32_t factory) override {
        calls.emplace_back("preload");
        Check(database == 0x6000 && name == "Mission" &&
                  factory == 0x4db740,
              "Mission preloader receives database, name and factory");
        return 0xd3;
    }
    std::uint32_t FindTable(std::uint32_t database,
                            std::string_view name) override {
        calls.emplace_back("find");
        Check(database == 0x6000 && name == "Mission",
              "Mission lookup uses database and literal table name");
        return found_table;
    }
    void LogError(std::string_view message) override {
        calls.emplace_back("log");
        Check(message ==
                  "ERROR [ palDBMissionManager::verifyDataTable < the table "
                  "Mission  doesn't exist>]",
              "missing Mission table logs the observed format");
    }
};

void CheckLookup() {
    Runtime runtime;
    Check(pal4::GameDataMissionTable::FindRecord(0x5000, -3, runtime) ==
              0x9000 + 2 * 68,
          "negative key uses signed ordering and record index");
    Check(pal4::GameDataMissionTable::FindRecord(0x5000, 2, runtime) ==
              0x9000,
          "middle key resolves to first record");
    Check(pal4::GameDataMissionTable::FindRecord(0x5000, 9, runtime) ==
              0x9000 + 68,
          "last key resolves to second record");
    for (auto key : {-4, 0, 10})
        Check(pal4::GameDataMissionTable::FindRecord(0x5000, key,
                                                     runtime) == 0,
              "missing key returns null record");
    runtime.words[0x700c] = 0x8000;
    Check(pal4::GameDataMissionTable::FindRecord(0x5000, 2, runtime) == 0,
          "empty key array returns null without dereferencing it");
}

void CheckLoading() {
    Runtime runtime;
    Check(pal4::GameDataMissionTable::Preload(0x5000, runtime) == 0xd3 &&
              runtime.calls == std::vector<std::string>{
                                   "read 20480", "preload"},
          "preload forwards signed byte and constructor factory");
    runtime.calls.clear();
    Check(pal4::GameDataMissionTable::Verify(0x5000, runtime) == 1 &&
              runtime.calls == std::vector<std::string>{
                                   "read 20480", "find", "write"} &&
              runtime.words[0x5004] == 0x7000,
          "successful verification stores the table before returning");
    runtime.found_table = 0;
    runtime.calls.clear();
    bool thrown = false;
    try {
        (void)pal4::GameDataMissionTable::Verify(0x5000, runtime);
    } catch (const std::runtime_error& error) {
        thrown = std::string_view(error.what()) ==
                 "ERROR [ palDBMissionManager::verifyDataTable < the table "
                 "Mission  doesn't exist>]";
    }
    Check(thrown && runtime.words[0x5004] == 0 &&
              runtime.calls == std::vector<std::string>{
                                   "read 20480", "find", "write", "log"},
          "missing table is stored, logged and thrown in IDA order");
}

void CheckComparison() {
    Runtime runtime;
    Check(!pal4::GameDataMissionTable::CompareMemberAt8(
              0xa000, 0xb000, runtime),
          "negative member sorts below positive member");
    Check(pal4::GameDataMissionTable::CompareMemberAt8(
              0xb000, 0xa000, runtime),
          "positive member sorts above negative member");
    runtime.words[0xb008] = static_cast<std::uint32_t>(-2);
    Check(!pal4::GameDataMissionTable::CompareMemberAt8(
              0xb000, 0xa000, runtime),
          "equal members are not greater");
}

}  // namespace

int main() {
    try {
        CheckLookup();
        CheckLoading();
        CheckComparison();
        std::cout << "game_data_mission_table_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
