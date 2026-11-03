#include "pal4/skill_data_table.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISkillDataIndexRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> assignments;
    std::vector<std::uint32_t> destroyed;
    bool fail_assign = false;

    explicit Runtime(const std::vector<std::uint32_t>& keys) {
        words[0x1000 + 96] = 0x2000;
        words[0x1000 + 100] = 0x2000 +
            static_cast<std::uint32_t>(keys.size()) * 132;
        words[0x1000 + 8] = 0x3000;
        words[0x1000 + 12] = 0x3008;
        for (std::size_t index = 0; index < keys.size(); ++index) {
            const auto record = 0x2000 +
                static_cast<std::uint32_t>(index) * 132;
            words[record] = keys[index];
            words[record + 4] = 100 + static_cast<std::uint32_t>(index);
            words[record + 128] = 200 + static_cast<std::uint32_t>(index);
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
        for (std::uint32_t offset = 0; offset < 132; offset += 4)
            words[destination + offset] = words[source + offset];
    }
    std::uint32_t DestroyRecord(std::uint32_t record) override {
        destroyed.push_back(record);
        return record + 1;
    }
    void InsertKey(std::uint32_t array, std::uint32_t end,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004 && end == words[0x100c],
              "index insertion uses current table end");
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
    pal4::SkillDataTable::RebuildIndex(0x1000, runtime);
    Check(runtime.words[0x1064] == 0x2000 + 4 * 132 &&
              runtime.words[0x2004] == 100 &&
              runtime.words[0x2000 + 132 + 4] == 102 &&
              runtime.words[0x2000 + 2 * 132 + 4] == 104 &&
              runtime.words[0x2000 + 3 * 132 + 128] == 205,
          "adjacent duplicate runs retain first whole records");
    Check(runtime.assignments ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x2000, 0x2000},
                  {0x2000 + 132, 0x2000 + 2 * 132},
                  {0x2000 + 2 * 132, 0x2000 + 4 * 132},
                  {0x2000 + 3 * 132, 0x2000 + 5 * 132}} &&
              runtime.destroyed ==
                  std::vector<std::uint32_t>{
                      0x2000 + 4 * 132, 0x2000 + 5 * 132},
          "only the compacted tail is destroyed");
    Check(runtime.words[0x100c] == 0x3000 + 4 * 8 &&
              runtime.KeyAt(0) == std::pair{0xffffffffu, 3u} &&
              runtime.KeyAt(1) == std::pair{1u, 1u} &&
              runtime.KeyAt(2).first == 2 &&
              runtime.KeyAt(3).first == 2 &&
              runtime.KeyAt(2).second != runtime.KeyAt(3).second,
          "signed index sort retains physical record positions");
}

void CheckEdgesAndFailure() {
    Runtime empty({});
    pal4::SkillDataTable::RebuildIndex(0x1000, empty);
    Check(empty.words[0x100c] == 0x3000 &&
              empty.destroyed.empty() && empty.assignments.empty(),
          "empty array clears stale index end");

    Runtime unique({3, 1, 3});
    pal4::SkillDataTable::RebuildIndex(0x1000, unique);
    Check(unique.words[0x1064] == 0x2000 + 3 * 132 &&
              unique.assignments.empty() && unique.destroyed.empty() &&
              unique.KeyAt(0) == std::pair{1u, 1u},
          "nonadjacent equal keys remain distinct and require no copies");

    Runtime interrupted({5, 5, 6});
    interrupted.fail_assign = true;
    try {
        pal4::SkillDataTable::RebuildIndex(0x1000, interrupted);
        Check(false, "copy failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "assign failed" &&
                  interrupted.words[0x1064] == 0x2000 + 3 * 132 &&
                  interrupted.destroyed.empty(),
              "failure stops before tail deletion and index reset");
    }
}

void CheckDestroyRange() {
    Runtime runtime({1, 2, 3});
    Check(pal4::SkillDataTable::DestroyRange(
              0x2000, 0x2000 + 3 * 132, runtime) ==
              0x2000 + 2 * 132 + 1 &&
              runtime.destroyed == std::vector<std::uint32_t>{
                  0x2000, 0x2000 + 132, 0x2000 + 2 * 132},
          "half-open Skill range uses 132-byte stride and last result");
    runtime.destroyed.clear();
    Check(pal4::SkillDataTable::DestroyRange(
              0x2000, 0x2000, runtime) == 0 &&
              runtime.destroyed.empty(),
          "empty range has no destructor calls");
}

}  // namespace

int main() {
    try {
        CheckCompactionAndIndex();
        CheckEdgesAndFailure();
        CheckDestroyRange();
        std::cout << "skill_data_table_index_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
