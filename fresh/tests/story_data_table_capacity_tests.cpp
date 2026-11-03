#include "pal4/story_data_table.h"
#include "pal4/story_desc_data_table.h"

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

struct Runtime final : pal4::IStoryDataCapacityRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::vector<std::uint32_t> allocations;
    std::size_t next_allocation = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write", address, value});
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back({"allocate", bytes});
        if (next_allocation == allocations.size())
            throw std::runtime_error("unexpected allocation");
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

constexpr std::uint32_t kTable = 0x9000;

void CheckBothGrow() {
    Runtime runtime;
    runtime.allocations = {0x3000, 0x4000};
    runtime.words[kTable + 96] = 0x1000;
    runtime.words[kTable + 100] = 0x1000 + 2 * 36;
    runtime.words[kTable + 104] = 0x1000 + 3 * 36;
    runtime.words[kTable + 8] = 0x2000;
    runtime.words[kTable + 12] = 0x2010;
    runtime.words[kTable + 16] = 0x2018;
    Check(pal4::StoryDataTable::Reserve(kTable, 5, runtime) == 0x4010 &&
              runtime.calls == std::vector<Call>{
                  {"allocate", 180},
                  {"copy-record", 0x3000, 0x1000},
                  {"copy-record", 0x3024, 0x1024},
                  {"destroy-record", 0x1000},
                  {"destroy-record", 0x1024},
                  {"free", 0x1000},
                  {"write", kTable + 104, 0x30b4},
                  {"write", kTable + 96, 0x3000},
                  {"write", kTable + 100, 0x3048},
                  {"allocate", 40},
                  {"copy-index", 0x4000, 0x2000},
                  {"copy-index", 0x4008, 0x2008},
                  {"free", 0x2000},
                  {"write", kTable + 16, 0x4028},
                  {"write", kTable + 8, 0x4000},
                  {"write", kTable + 12, 0x4010}},
          "two arrays grow independently, preserving active lengths and order");
}

void CheckIndependentGrowth() {
    Runtime enough;
    enough.words[kTable + 96] = 0x1000;
    enough.words[kTable + 100] = 0x1024;
    enough.words[kTable + 104] = 0x1000 + 6 * 36;
    enough.words[kTable + 8] = 0x2000;
    enough.words[kTable + 12] = 0x2008;
    enough.words[kTable + 16] = 0x2030;
    Check(pal4::StoryDataTable::Reserve(kTable, 4, enough) == 6 &&
              enough.calls.empty(),
          "sufficient capacity returns index capacity without mutation");

    enough.words[kTable + 16] = 0x2008;
    enough.allocations = {0x4000};
    Check(pal4::StoryDataTable::Reserve(kTable, 4, enough) == 0x4008 &&
              enough.calls.front() == Call{"allocate", 32} &&
              enough.calls[1] == Call{"copy-index", 0x4000, 0x2000} &&
              enough.words[kTable + 100] == 0x1024,
          "index grows without touching sufficient row capacity");

    Runtime rows_only;
    rows_only.allocations = {0x3000};
    rows_only.words[kTable + 96] = 0x1000;
    rows_only.words[kTable + 100] = 0x1024;
    rows_only.words[kTable + 104] = 0x1024;
    rows_only.words[kTable + 8] = 0x2000;
    rows_only.words[kTable + 12] = 0x2008;
    rows_only.words[kTable + 16] = 0x2028;
    Check(pal4::StoryDataTable::Reserve(kTable, 3, rows_only) == 5 &&
              rows_only.words[kTable + 100] == 0x3024 &&
              rows_only.words[kTable + 16] == 0x2028,
          "row growth preserves sufficient index allocation");
}

void CheckEmptyNegativeAndNullIndex() {
    Runtime empty;
    empty.allocations = {0x3000, 0x4000};
    Check(pal4::StoryDataTable::Reserve(kTable, 2, empty) == 0x4000 &&
              empty.words[kTable + 100] == 0x3000 &&
              empty.words[kTable + 104] == 0x3048 &&
              empty.words[kTable + 12] == 0x4000 &&
              empty.words[kTable + 16] == 0x4010,
          "empty arrays gain capacity with zero active elements");

    Runtime negative;
    negative.allocations = {0x3000, 0x4000};
    Check(pal4::StoryDataTable::Reserve(kTable, -1, negative) == 0x4000 &&
              negative.calls[0] == Call{"allocate", 0} &&
              negative.calls[5] == Call{"allocate", 0} &&
              negative.words[kTable + 104] == 0x3000 - 36 &&
              negative.words[kTable + 16] == 0x4000 - 8,
          "negative request takes unsigned growth branch but clamps allocation");

    Runtime null_index;
    null_index.allocations = {0};
    null_index.words[kTable + 96] = 0x1000;
    null_index.words[kTable + 100] = 0x1000;
    null_index.words[kTable + 104] = 0x1000 + 3 * 36;
    null_index.words[kTable + 8] = 0x2000;
    null_index.words[kTable + 12] = 0x2008;
    null_index.words[kTable + 16] = 0x2008;
    Check(pal4::StoryDataTable::Reserve(kTable, 2, null_index) == 8 &&
              null_index.calls == std::vector<Call>{
                  {"allocate", 16}, {"free", 0x2000},
                  {"write", kTable + 16, 16},
                  {"write", kTable + 8, 0},
                  {"write", kTable + 12, 8}},
          "null index allocation skips copy and retains native pointer math");
}

void CheckStoryDescCapacity() {
    Runtime runtime;
    runtime.allocations = {0x3000, 0x4000};
    Check(pal4::StoryDescDataTable::Reserve(kTable, 2, runtime) ==
              0x4000 &&
              runtime.calls[0] == Call{"allocate", 72} &&
              runtime.words[kTable + 104] == 0x3048 &&
              runtime.words[kTable + 16] == 0x4010,
          "StoryDesc reserves the same 36-byte rows and eight-byte keys");
}

}  // namespace

int main() {
    try {
        CheckBothGrow();
        CheckIndependentGrowth();
        CheckEmptyNegativeAndNullIndex();
        CheckStoryDescCapacity();
        std::cout << "story_data_table_capacity_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
