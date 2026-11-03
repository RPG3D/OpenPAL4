#include "pal4/magic_data_table.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IMagicDataIndexRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> assignments;
    std::vector<std::uint32_t> destroyed;
    bool fail_assign = false;

    explicit Runtime(const std::vector<std::uint32_t>& keys) {
        words[0x1060] = 0x2000;
        words[0x1064] = 0x2000 +
            static_cast<std::uint32_t>(keys.size()) * 152;
        words[0x1008] = 0x3000;
        words[0x100c] = 0x3008;
        for (std::size_t index = 0; index < keys.size(); ++index) {
            const auto record = 0x2000 +
                static_cast<std::uint32_t>(index) * 152;
            words[record] = keys[index];
            words[record + 4] = 100 + static_cast<std::uint32_t>(index);
            words[record + 148] = 200 + static_cast<std::uint32_t>(index);
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void AssignRecord(std::uint32_t destination,
                      std::uint32_t source) override {
        assignments.emplace_back(destination, source);
        if (fail_assign) throw std::runtime_error("assign failed");
        for (std::uint32_t offset = 0; offset < 152; offset += 4)
            words[destination + offset] = words[source + offset];
    }
    std::uint32_t DestroyRecord(std::uint32_t record) override {
        destroyed.push_back(record);
        return record + 1;
    }
    void InsertKey(std::uint32_t array, std::uint32_t end,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004 && end == words[0x100c],
              "index insertion uses current end");
        words[end] = key;
        words[end + 4] = index;
        words[0x100c] = end + 8;
    }
    [[nodiscard]] std::pair<std::uint32_t, std::uint32_t> KeyAt(
        std::uint32_t index) {
        return {words[0x3000 + 8 * index],
                words[0x3004 + 8 * index]};
    }
};

void CheckCompactionAndIndex() {
    Runtime runtime({2, 2, 1, 1, 2, 0xffffffff});
    pal4::MagicDataTable::RebuildIndex(0x1000, runtime);
    Check(runtime.words[0x1064] == 0x2000 + 4 * 152 &&
              runtime.words[0x2004] == 100 &&
              runtime.words[0x2000 + 152 + 4] == 102 &&
              runtime.words[0x2000 + 2 * 152 + 4] == 104 &&
              runtime.words[0x2000 + 3 * 152 + 148] == 205,
          "compaction retains first complete 152-byte record per run");
    Check(runtime.assignments ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x2000, 0x2000},
                  {0x2000 + 152, 0x2000 + 2 * 152},
                  {0x2000 + 2 * 152, 0x2000 + 4 * 152},
                  {0x2000 + 3 * 152, 0x2000 + 5 * 152}} &&
              runtime.destroyed == std::vector<std::uint32_t>{
                  0x2000 + 4 * 152, 0x2000 + 5 * 152},
          "copied records precede destruction of vacated tail");
    Check(runtime.words[0x100c] == 0x3020 &&
              runtime.KeyAt(0) == std::pair{0xffffffffu, 3u} &&
              runtime.KeyAt(1) == std::pair{1u, 1u} &&
              runtime.KeyAt(2).first == 2 &&
              runtime.KeyAt(3).first == 2 &&
              runtime.KeyAt(2).second != runtime.KeyAt(3).second,
          "signed sort preserves distinct physical positions");
}

void CheckEdgesAndRange() {
    Runtime empty({});
    pal4::MagicDataTable::RebuildIndex(0x1000, empty);
    Check(empty.words[0x100c] == 0x3000 &&
              empty.assignments.empty() && empty.destroyed.empty(),
          "empty source clears stale index end");

    Runtime nonadjacent({3, 1, 3});
    pal4::MagicDataTable::RebuildIndex(0x1000, nonadjacent);
    Check(nonadjacent.words[0x1064] == 0x2000 + 3 * 152 &&
              nonadjacent.assignments.empty() &&
              nonadjacent.destroyed.empty() &&
              nonadjacent.KeyAt(0) == std::pair{1u, 1u},
          "nonadjacent duplicate IDs remain separate");

    Runtime range({1, 2, 3});
    Check(pal4::MagicDataTable::DestroyRange(
              0x2000, 0x2000 + 3 * 152, range) ==
              0x2000 + 2 * 152 + 1 &&
              range.destroyed == std::vector<std::uint32_t>{
                  0x2000, 0x2000 + 152, 0x2000 + 2 * 152},
          "range destruction uses 152-byte stride and last return");
    range.destroyed.clear();
    Check(pal4::MagicDataTable::DestroyRange(
              0x2000, 0x2000, range) == 0 && range.destroyed.empty(),
          "empty range has no destructor calls");
}

void CheckFailure() {
    Runtime runtime({5, 5, 6});
    runtime.fail_assign = true;
    try {
        pal4::MagicDataTable::RebuildIndex(0x1000, runtime);
        Check(false, "copy failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "assign failed" &&
                  runtime.words[0x1064] == 0x2000 + 3 * 152 &&
                  runtime.destroyed.empty(),
              "failure stops before tail destruction and index reset");
    }
}

}  // namespace

int main() {
    try {
        CheckCompactionAndIndex();
        CheckEdgesAndRange();
        CheckFailure();
        std::cout << "magic_data_table_index_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
