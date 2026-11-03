#include "pal4/game_question_data_table.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string action;
    std::uint32_t first;
    std::uint32_t second = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameQuestionDataStorageRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::uint32_t fail_at = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write", address, value});
    }
    std::uint32_t ClearEffectData(std::uint32_t table) override {
        calls.push_back({"clear", table});
        return 0x55;
    }
    std::uint32_t DestroyQuestionRecord(
        std::uint32_t record) override {
        calls.push_back({"destroy", record});
        if (record == fail_at) throw std::runtime_error("destroy failed");
        return record + 1;
    }
    void Free(std::uint32_t address) override {
        calls.push_back({"free", address});
    }
};

void CheckReleaseArray() {
    Runtime runtime;
    constexpr std::uint32_t array = 0x5000;
    runtime.words[array + 4] = 0x9000;
    runtime.words[array + 8] = 0x9000 + 2 * 76;
    runtime.words[array + 12] = 0x9000 + 3 * 76;
    Check(pal4::GameQuestionDataTable::ReleaseRecordArray(array, runtime) ==
              0 &&
              runtime.calls == std::vector<Call>{
                  {"destroy", 0x9000}, {"destroy", 0x904c},
                  {"free", 0x9000},
                  {"write", array + 4, 0},
                  {"write", array + 8, 0},
                  {"write", array + 12, 0}},
          "standalone array destroys 76-byte rows before free and reset");

    Runtime empty;
    Check(pal4::GameQuestionDataTable::ReleaseRecordArray(array, empty) ==
              0 &&
              empty.calls.front() == Call{"free", 0} &&
              empty.calls.size() == 4,
          "empty standalone array still frees null and clears triplet");

    Runtime failed;
    failed.words[array + 4] = 0x9000;
    failed.words[array + 8] = 0x904c;
    failed.fail_at = 0x9000;
    try {
        static_cast<void>(pal4::GameQuestionDataTable::ReleaseRecordArray(
            array, failed));
        Check(false, "destroy failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "destroy failed" &&
                  failed.calls ==
                      std::vector<Call>{{"destroy", 0x9000}},
              "failure stops before storage free and pointer resets");
    }
}

void CheckClearRows() {
    Runtime runtime;
    constexpr std::uint32_t table = 0x7000;
    runtime.words[table + 96] = 0x9000;
    runtime.words[table + 100] = 0x9000 + 2 * 76;
    runtime.words[table + 104] = 0x9000 + 4 * 76;
    Check(pal4::GameQuestionDataTable::ClearRows(table, runtime) ==
              0x904d &&
              runtime.calls == std::vector<Call>{
                  {"clear", table}, {"destroy", 0x9000},
                  {"destroy", 0x904c},
                  {"write", table + 100, 0x9000}} &&
              runtime.words[table + 104] == 0x9000 + 4 * 76,
          "clear keeps capacity and forwards last row destructor result");

    Runtime empty;
    empty.words[table + 96] = 0x9000;
    empty.words[table + 100] = 0x9000;
    Check(pal4::GameQuestionDataTable::ClearRows(table, empty) == 0x55 &&
              empty.calls == std::vector<Call>{
                  {"clear", table}, {"write", table + 100, 0x9000}},
          "empty row range forwards Effect clear result");
}

}  // namespace

int main() {
    try {
        CheckReleaseArray();
        CheckClearRows();
        std::cout << "game_question_data_table_storage_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
