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
    std::string name;
    std::uint32_t first;
    std::uint32_t second = 0;
    bool operator==(const Call&) const = default;
};

struct IndexRuntime final : pal4::IGameQuestionDataIndexRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    bool fail_copy = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write", address, value});
    }
    std::uint32_t CopyUnique(std::uint32_t begin, std::uint32_t end,
                             std::uint32_t destination) override {
        calls.push_back({"unique", begin, end});
        Check(begin == destination, "compaction must start in place");
        if (fail_copy) throw std::runtime_error("copy failed");
        if (begin == end) return destination;
        auto output = destination;
        auto previous = begin;
        words[output] = words[begin];
        output += 76;
        for (auto current = begin + 76; current != end; current += 76) {
            if (words[previous] != words[current]) {
                words[output] = words[current];
                previous = current;
                output += 76;
            }
        }
        return output;
    }
    std::uint32_t DestroyRecord(std::uint32_t record) override {
        calls.push_back({"destroy", record});
        return record + 1;
    }
    void InsertKey(std::uint32_t array, std::uint32_t end,
                   std::uint32_t key, std::uint32_t index) override {
        calls.push_back({"insert", key, index});
        Check(array == 0x7004, "index array offset");
        words[end] = key;
        words[end + 4] = index;
        words[array + 8] = end + 8;
    }
};

void Seed(IndexRuntime& runtime, const std::vector<std::int32_t>& keys) {
    runtime.words[0x7060] = keys.empty() ? 0 : 0x9000;
    runtime.words[0x7064] = keys.empty()
        ? 0 : 0x9000 + 76 * static_cast<std::uint32_t>(keys.size());
    runtime.words[0x7008] = 0xa000;
    runtime.words[0x700c] = 0xa000;
    for (std::size_t index = 0; index < keys.size(); ++index)
        runtime.words[0x9000 + 76 * static_cast<std::uint32_t>(index)] =
            static_cast<std::uint32_t>(keys[index]);
}

void CheckIndex() {
    IndexRuntime compact;
    Seed(compact, {3, 3, -2, 3});
    pal4::GameQuestionDataTable::RebuildIndex(0x7000, compact);
    Check(compact.calls[0] == Call{"unique", 0x9000, 0x9130} &&
              compact.calls[1] == Call{"destroy", 0x90e4} &&
              compact.words[0x7064] == 0x90e4 &&
              compact.words[0x904c] == static_cast<std::uint32_t>(-2) &&
              compact.words[0x9098] == 3 &&
              compact.words[0x700c] == 0xa018,
          "first adjacent duplicate compacts only suffix with 76-byte rows");
    Check(compact.words[0xa000] == static_cast<std::uint32_t>(-2) &&
              compact.words[0xa004] == 1 &&
              compact.words[0xa008] == 3 &&
              compact.words[0xa00c] == 0 &&
              compact.words[0xa010] == 3 &&
              compact.words[0xa014] == 2,
          "index sorts signed IDs and retains physical row positions");

    IndexRuntime distinct;
    Seed(distinct, {-1, 2, 1});
    pal4::GameQuestionDataTable::RebuildIndex(0x7000, distinct);
    Check(distinct.calls[0] == Call{"unique", 0x90e4, 0x90e4} &&
              distinct.words[0xa000] == static_cast<std::uint32_t>(-1) &&
              distinct.words[0xa008] == 1 &&
              distinct.words[0xa010] == 2,
          "distinct rows remain intact and use signed ordering");

    IndexRuntime empty;
    Seed(empty, {});
    pal4::GameQuestionDataTable::RebuildIndex(0x7000, empty);
    Check(empty.calls[0] == Call{"unique", 0, 0} &&
              empty.words[0x700c] == 0xa000,
          "empty table resets index end");

    IndexRuntime failed;
    Seed(failed, {4, 4});
    failed.fail_copy = true;
    try {
        pal4::GameQuestionDataTable::RebuildIndex(0x7000, failed);
        Check(false, "copy failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "copy failed" &&
                  failed.calls.size() == 1 &&
                  failed.words[0x7064] == 0x9098,
              "failed compaction stops before table mutation");
    }

    IndexRuntime range;
    Check(pal4::GameQuestionDataTable::DestroyRange(
              0x9000, 0x9098, range) == 0x904d &&
              range.calls == std::vector<Call>{{"destroy", 0x9000},
                                               {"destroy", 0x904c}} &&
              pal4::GameQuestionDataTable::DestroyRange(
                  0x9000, 0x9000, range) == 0,
          "half-open range uses 76-byte stride and returns last result");
}

struct CapacityRuntime final : pal4::IGameQuestionDataCapacityRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::vector<std::uint32_t> allocations;
    std::size_t next_allocation = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write", address, value});
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back({"allocate", bytes});
        Check(next_allocation < allocations.size(), "unexpected allocation");
        return allocations[next_allocation++];
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.push_back({"copy-record", destination, source});
    }
    void DestroyRecord(std::uint32_t record) override {
        calls.push_back({"destroy-record", record});
    }
    void CopyIndex(std::uint32_t destination,
                   std::uint32_t source) override {
        calls.push_back({"copy-index", destination, source});
    }
    void Free(std::uint32_t address) override {
        calls.push_back({"free", address});
    }
};

void CheckCapacity() {
    constexpr auto table = 0x7000u;
    CapacityRuntime both;
    both.allocations = {0x3000, 0x4000};
    both.words[table + 96] = 0x1000;
    both.words[table + 100] = 0x1098;
    both.words[table + 104] = 0x10e4;
    both.words[table + 8] = 0x2000;
    both.words[table + 12] = 0x2010;
    both.words[table + 16] = 0x2018;
    Check(pal4::GameQuestionDataTable::Reserve(table, 5, both) == 0x4010 &&
              both.calls == std::vector<Call>{
                  {"allocate", 380},
                  {"copy-record", 0x3000, 0x1000},
                  {"copy-record", 0x304c, 0x104c},
                  {"destroy-record", 0x1000},
                  {"destroy-record", 0x104c},
                  {"free", 0x1000},
                  {"write", table + 104, 0x317c},
                  {"write", table + 96, 0x3000},
                  {"write", table + 100, 0x3098},
                  {"allocate", 40},
                  {"copy-index", 0x4000, 0x2000},
                  {"copy-index", 0x4008, 0x2008},
                  {"free", 0x2000},
                  {"write", table + 16, 0x4028},
                  {"write", table + 8, 0x4000},
                  {"write", table + 12, 0x4010}},
          "both arrays grow independently, preserving active records");

    CapacityRuntime enough;
    enough.words[table + 96] = 0x1000;
    enough.words[table + 100] = 0x104c;
    enough.words[table + 104] = 0x11c8;
    enough.words[table + 8] = 0x2000;
    enough.words[table + 12] = 0x2008;
    enough.words[table + 16] = 0x2030;
    Check(pal4::GameQuestionDataTable::Reserve(table, 4, enough) == 6 &&
              enough.calls.empty(), "sufficient capacities cause no mutation");
    enough.words[table + 16] = 0x2008;
    enough.allocations = {0x4000};
    Check(pal4::GameQuestionDataTable::Reserve(table, 4, enough) == 0x4008 &&
              enough.calls[0] == Call{"allocate", 32} &&
              enough.words[table + 100] == 0x104c,
          "index may grow without moving records");

    CapacityRuntime rows;
    rows.allocations = {0x3000};
    rows.words[table + 96] = 0x1000;
    rows.words[table + 100] = 0x104c;
    rows.words[table + 104] = 0x104c;
    rows.words[table + 8] = 0x2000;
    rows.words[table + 12] = 0x2008;
    rows.words[table + 16] = 0x2028;
    Check(pal4::GameQuestionDataTable::Reserve(table, 3, rows) == 5 &&
              rows.words[table + 100] == 0x304c &&
              rows.words[table + 16] == 0x2028,
          "records may grow without moving index");

    CapacityRuntime empty;
    empty.allocations = {0x3000, 0x4000};
    Check(pal4::GameQuestionDataTable::Reserve(table, 2, empty) == 0x4000 &&
              empty.words[table + 100] == 0x3000 &&
              empty.words[table + 104] == 0x3098 &&
              empty.words[table + 16] == 0x4010,
          "empty arrays gain capacity without active entries");

    CapacityRuntime negative;
    negative.allocations = {0x3000, 0x4000};
    Check(pal4::GameQuestionDataTable::Reserve(table, -1, negative) ==
              0x4000 &&
              negative.calls[0] == Call{"allocate", 0} &&
              negative.calls[5] == Call{"allocate", 0} &&
              negative.words[table + 104] == 0x3000 - 76 &&
              negative.words[table + 16] == 0x4000 - 8,
          "negative request follows unsigned growth and zero-byte allocation");

    CapacityRuntime null_index;
    null_index.allocations = {0};
    null_index.words[table + 96] = 0x1000;
    null_index.words[table + 100] = 0x1000;
    null_index.words[table + 104] = 0x1130;
    null_index.words[table + 8] = 0x2000;
    null_index.words[table + 12] = 0x2008;
    null_index.words[table + 16] = 0x2008;
    Check(pal4::GameQuestionDataTable::Reserve(table, 2, null_index) == 8 &&
              null_index.calls == std::vector<Call>{
                  {"allocate", 16}, {"free", 0x2000},
                  {"write", table + 16, 16},
                  {"write", table + 8, 0},
                  {"write", table + 12, 8}},
          "null index allocation skips copying and preserves pointer math");
}

}  // namespace

int main() {
    try {
        CheckIndex();
        CheckCapacity();
        std::cout << "game_question_data_table_index_capacity_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
