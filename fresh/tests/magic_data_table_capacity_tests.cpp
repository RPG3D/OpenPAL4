#include "pal4/magic_data_table.h"

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
    std::string operation;
    std::uint32_t first;
    std::uint32_t second = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IMagicDataCapacityRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::vector<std::uint32_t> allocations;
    std::size_t next_allocation = 0;
    bool fail_copy = false;

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
        if (fail_copy) throw std::runtime_error("copy failed");
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
    runtime.words[kTable + 100] = 0x1000 + 2 * 152;
    runtime.words[kTable + 104] = 0x1000 + 3 * 152;
    runtime.words[kTable + 8] = 0x2000;
    runtime.words[kTable + 12] = 0x2010;
    runtime.words[kTable + 16] = 0x2018;
    Check(pal4::MagicDataTable::Reserve(kTable, 5, runtime) == 0x4010 &&
              runtime.calls == std::vector<Call>{
                  {"allocate", 5 * 152},
                  {"copy-record", 0x3000, 0x1000},
                  {"copy-record", 0x3000 + 152, 0x1000 + 152},
                  {"destroy-record", 0x1000},
                  {"destroy-record", 0x1000 + 152},
                  {"free", 0x1000},
                  {"write", kTable + 104, 0x3000 + 5 * 152},
                  {"write", kTable + 96, 0x3000},
                  {"write", kTable + 100, 0x3000 + 2 * 152},
                  {"allocate", 40},
                  {"copy-index", 0x4000, 0x2000},
                  {"copy-index", 0x4008, 0x2008},
                  {"free", 0x2000},
                  {"write", kTable + 16, 0x4028},
                  {"write", kTable + 8, 0x4000},
                  {"write", kTable + 12, 0x4010}},
          "both arrays preserve active lengths and relocation order");
}

void CheckIndependentGrowth() {
    Runtime sufficient;
    sufficient.words[kTable + 96] = 0x1000;
    sufficient.words[kTable + 100] = 0x1000 + 152;
    sufficient.words[kTable + 104] = 0x1000 + 6 * 152;
    sufficient.words[kTable + 8] = 0x2000;
    sufficient.words[kTable + 12] = 0x2008;
    sufficient.words[kTable + 16] = 0x2030;
    Check(pal4::MagicDataTable::Reserve(kTable, 4, sufficient) == 6 &&
              sufficient.calls.empty(),
          "sufficient index capacity returns element count");

    sufficient.words[kTable + 16] = 0x2008;
    sufficient.allocations = {0x4000};
    Check(pal4::MagicDataTable::Reserve(kTable, 4, sufficient) == 0x4008 &&
              sufficient.calls.front() == Call{"allocate", 32} &&
              sufficient.calls[1] ==
                  Call{"copy-index", 0x4000, 0x2000} &&
              sufficient.words[kTable + 100] == 0x1000 + 152,
          "index can grow without moving records");

    Runtime records_only;
    records_only.allocations = {0x3000};
    records_only.words[kTable + 96] = 0x1000;
    records_only.words[kTable + 100] = 0x1000 + 152;
    records_only.words[kTable + 104] = 0x1000 + 152;
    records_only.words[kTable + 8] = 0x2000;
    records_only.words[kTable + 12] = 0x2008;
    records_only.words[kTable + 16] = 0x2028;
    Check(pal4::MagicDataTable::Reserve(kTable, 3, records_only) == 5 &&
              records_only.words[kTable + 100] == 0x3000 + 152 &&
              records_only.words[kTable + 16] == 0x2028 &&
              records_only.next_allocation == 1,
          "record growth leaves sufficient index storage intact");
}

void CheckEmptyNegativeAndNull() {
    Runtime empty;
    empty.allocations = {0x3000, 0x4000};
    Check(pal4::MagicDataTable::Reserve(kTable, 2, empty) == 0x4000 &&
              empty.words[kTable + 100] == 0x3000 &&
              empty.words[kTable + 104] == 0x3000 + 304 &&
              empty.words[kTable + 12] == 0x4000 &&
              empty.words[kTable + 16] == 0x4010,
          "empty arrays reserve without active elements");

    Runtime negative;
    negative.allocations = {0x3000, 0x4000};
    Check(pal4::MagicDataTable::Reserve(kTable, -1, negative) ==
              0x4000 &&
              negative.calls[0] == Call{"allocate", 0} &&
              negative.calls[5] == Call{"allocate", 0} &&
              negative.words[kTable + 104] == 0x3000 - 152 &&
              negative.words[kTable + 16] == 0x4000 - 8,
          "negative request uses unsigned growth and zero allocation");

    Runtime null_index;
    null_index.allocations = {0};
    null_index.words[kTable + 96] = 0x1000;
    null_index.words[kTable + 100] = 0x1000;
    null_index.words[kTable + 104] = 0x1000 + 3 * 152;
    null_index.words[kTable + 8] = 0x2000;
    null_index.words[kTable + 12] = 0x2008;
    null_index.words[kTable + 16] = 0x2008;
    Check(pal4::MagicDataTable::Reserve(kTable, 2, null_index) == 8 &&
              null_index.calls == std::vector<Call>{
                  {"allocate", 16}, {"free", 0x2000},
                  {"write", kTable + 16, 16},
                  {"write", kTable + 8, 0},
                  {"write", kTable + 12, 8}},
          "null index allocation skips copy but preserves pointer math");
}

void CheckCopyFailure() {
    Runtime runtime;
    runtime.allocations = {0x3000};
    runtime.fail_copy = true;
    runtime.words[kTable + 96] = 0x1000;
    runtime.words[kTable + 100] = 0x1000 + 152;
    runtime.words[kTable + 104] = 0x1000 + 152;
    try {
        static_cast<void>(pal4::MagicDataTable::Reserve(kTable, 2, runtime));
        Check(false, "copy failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "copy failed" &&
                  runtime.calls == std::vector<Call>{
                      {"allocate", 304},
                      {"copy-record", 0x3000, 0x1000}} &&
                  runtime.words[kTable + 96] == 0x1000,
              "copy failure leaves old pointers and records untouched");
    }
}

}  // namespace

int main() {
    try {
        CheckBothGrow();
        CheckIndependentGrowth();
        CheckEmptyNegativeAndNull();
        CheckCopyFailure();
        std::cout << "magic_data_table_capacity_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
