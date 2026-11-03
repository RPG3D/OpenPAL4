#include "pal4/game_data_mission_index.h"

#include <algorithm>
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

struct Runtime final : pal4::IGameDataMissionIndexRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> copies;
    std::vector<std::uint32_t> destroyed;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        copies.emplace_back(destination, source);
        words[destination] = words[source];
    }
    std::uint32_t DestroyRecord(std::uint32_t record) override {
        destroyed.push_back(record);
        return record + 1;
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004, "index insertion uses table +4");
        Check(at == words[0x100c], "index inserts at current end");
        words[at] = key;
        words[at + 4] = index;
        words[0x100c] = at + 8;
        calls.push_back("insert " + std::to_string(key) + " " +
                        std::to_string(index));
    }
};

Runtime WithKeys(const std::vector<std::int32_t>& keys) {
    Runtime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] =
        0x2000 + static_cast<std::uint32_t>(keys.size()) * 68;
    runtime.words[0x1008] = 0x3000;
    runtime.words[0x100c] = 0x3040;
    for (std::size_t i = 0; i < keys.size(); ++i)
        runtime.words[0x2000 + static_cast<std::uint32_t>(i) * 68] =
            static_cast<std::uint32_t>(keys[i]);
    return runtime;
}

void CheckCompaction() {
    auto runtime = WithKeys({3, 3, -2, 4, 4, 1, -2});
    pal4::GameDataMissionIndex::Normalize(0x1000, runtime);
    Check(runtime.words[0x1064] == 0x2000 + 5 * 68 &&
              runtime.destroyed ==
                  std::vector<std::uint32_t>{0x2000 + 5 * 68,
                                             0x2000 + 6 * 68},
          "adjacent duplicates compact and trailing text records are destroyed");
    const std::vector<std::int32_t> retained{3, -2, 4, 1, -2};
    for (std::size_t i = 0; i < retained.size(); ++i)
        Check(static_cast<std::int32_t>(runtime.words[
                  0x2000 + static_cast<std::uint32_t>(i) * 68]) ==
                  retained[i],
              "record compaction retains first occurrence in original order");
    Check(!runtime.copies.empty() &&
              runtime.copies.front() ==
                  std::pair<std::uint32_t, std::uint32_t>{0x2000, 0x2000},
          "first duplicate invokes the original self-copy path");
    Check(runtime.words[0x100c] == 0x3000 + 5 * 8 &&
              runtime.calls.size() == 5,
          "key index is cleared then rebuilt for retained record count");
    const std::vector<std::int32_t> sorted{-2, -2, 1, 3, 4};
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        const auto address = 0x3000 + static_cast<std::uint32_t>(i) * 8;
        Check(static_cast<std::int32_t>(runtime.words[address]) == sorted[i],
              "key index sorts by signed first word");
        const auto record_index = runtime.words[address + 4];
        Check(record_index < retained.size() &&
                  retained[record_index] == sorted[i],
              "sorted key keeps its record index");
    }
}

void CheckEmptyAndLong() {
    Runtime empty;
    empty.words[0x1060] = 0;
    empty.words[0x1064] = 0;
    empty.words[0x1008] = 0x3000;
    empty.words[0x100c] = 0x3040;
    pal4::GameDataMissionIndex::Normalize(0x1000, empty);
    Check(empty.words[0x100c] == 0x3000 && empty.calls.empty() &&
              empty.destroyed.empty(),
          "empty records clear existing key index without element operations");

    std::vector<std::int32_t> keys;
    for (std::int32_t key = 20; key > -20; --key) keys.push_back(key);
    auto long_array = WithKeys(keys);
    pal4::GameDataMissionIndex::Normalize(0x1000, long_array);
    Check(long_array.words[0x1064] == 0x2000 + 40 * 68 &&
              long_array.copies.empty() && long_array.destroyed.empty() &&
              long_array.words[0x100c] == 0x3000 + 40 * 8,
          "unique records stay in place and rebuild a 40-entry index");
    for (std::int32_t i = 0; i < 40; ++i) {
        const auto address = 0x3000 + static_cast<std::uint32_t>(i) * 8;
        Check(static_cast<std::int32_t>(long_array.words[address]) ==
                  i - 19 &&
                  long_array.words[address + 4] ==
                      static_cast<std::uint32_t>(39 - i),
              "long key index uses signed ascending order and original indices");
    }
}

void CheckDestroyRange() {
    Runtime runtime;
    Check(pal4::GameDataMissionIndex::DestroyRange(0x2000, 0x2088,
                                                    runtime) == 0x2045 &&
              runtime.destroyed ==
                  std::vector<std::uint32_t>{0x2000, 0x2044},
          "range cleanup follows 68-byte stride and returns last result");
    Runtime empty;
    Check(pal4::GameDataMissionIndex::DestroyRange(0x2000, 0x2000,
                                                    empty) == 0 &&
              empty.destroyed.empty(),
          "empty range performs no destructor calls");
}

void CheckCompactFrom() {
    auto runtime = WithKeys({5, 5, 2, 5, 5, -1});
    Check(pal4::GameDataMissionIndex::CompactFrom(
              0x2000, 0x2000 + 6 * 68, 0x5000, runtime) ==
              0x5000 + 4 * 68 &&
              runtime.words[0x5000] == 5 &&
              runtime.words[0x5044] == 2 &&
              runtime.words[0x5088] == 5 &&
              static_cast<std::int32_t>(runtime.words[0x50cc]) == -1 &&
              runtime.copies.size() == 4,
          "compaction removes only adjacent equal keys into a separate array");
    Runtime empty;
    Check(pal4::GameDataMissionIndex::CompactFrom(
              0x2000, 0x2000, 0x5000, empty) == 0x5000 &&
              empty.copies.empty(),
          "empty compaction returns original destination");
}

}  // namespace

int main() {
    try {
        CheckCompaction();
        CheckEmptyAndLong();
        CheckDestroyRange();
        CheckCompactFrom();
        std::cout << "game_data_mission_index_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
