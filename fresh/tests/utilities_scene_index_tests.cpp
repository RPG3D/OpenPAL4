#include "pal4/utilities_scene_table.h"

#include <algorithm>
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

struct Runtime final : pal4::IUtilitiesSceneIndexRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> copies;
    std::vector<std::uint32_t> destroyed;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> inserted;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        copies.emplace_back(destination, source);
        for (std::uint32_t i = 0; i < 60; i += 4)
            words[destination + i] = words.at(source + i);
    }
    std::uint32_t DestroyRecord(std::uint32_t record) override {
        destroyed.push_back(record);
        return record + 4;
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004 && at == words.at(0x100c),
              "key insertion uses current index end");
        inserted.emplace_back(key, index);
        words[at] = key;
        words[at + 4] = index;
        words[0x100c] = at + 8;
    }
    void SetRows(const std::vector<std::int32_t>& keys) {
        words[0x1060] = keys.empty() ? 0 : 0x4000;
        words[0x1064] = keys.empty()
                            ? 0
                            : 0x4000 +
                                  static_cast<std::uint32_t>(keys.size()) * 60;
        words[0x1008] = 0x3000;
        words[0x100c] = 0x3008;
        for (std::size_t row = 0; row < keys.size(); ++row) {
            const auto at = 0x4000 + static_cast<std::uint32_t>(row) * 60;
            words[at] = static_cast<std::uint32_t>(keys[row]);
            for (std::uint32_t i = 4; i < 60; i += 4)
                words[at + i] = static_cast<std::uint32_t>(row * 100 + i);
        }
    }
    std::vector<std::pair<std::int32_t, std::uint32_t>> Index() const {
        std::vector<std::pair<std::int32_t, std::uint32_t>> result;
        for (auto at = words.at(0x1008); at != words.at(0x100c); at += 8)
            result.emplace_back(static_cast<std::int32_t>(words.at(at)),
                                words.at(at + 4));
        return result;
    }
};

void CheckEmptyAndUnique() {
    Runtime empty;
    empty.SetRows({});
    pal4::UtilitiesSceneTable::DeduplicateAndRebuildIndex(0x1000, empty);
    Check(empty.words[0x1064] == 0 && empty.words[0x100c] == 0x3000 &&
              empty.Index().empty() && empty.copies.empty() &&
              empty.destroyed.empty(),
          "empty Scene table resets index without touching rows");

    Runtime unique;
    unique.SetRows({7, -2, 4});
    pal4::UtilitiesSceneTable::DeduplicateAndRebuildIndex(0x1000, unique);
    Check(unique.words[0x1064] == 0x40b4 && unique.copies.empty() &&
              unique.destroyed.empty() &&
              unique.Index() ==
                  std::vector<std::pair<std::int32_t, std::uint32_t>>{
                      {-2, 1}, {4, 2}, {7, 0}},
          "unique rows retain physical order with signed key index");
}

void CheckDuplicateRuns() {
    Runtime runtime;
    runtime.SetRows({7, 5, 5, 3});
    pal4::UtilitiesSceneTable::DeduplicateAndRebuildIndex(0x1000, runtime);
    Check(runtime.words[0x1064] == 0x40b4 &&
              runtime.words[0x4000] == 7 &&
              runtime.words[0x403c] == 5 &&
              runtime.words[0x4078] == 3 &&
              runtime.copies ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x403c, 0x403c}, {0x4078, 0x40b4}} &&
              runtime.destroyed == std::vector<std::uint32_t>{0x40b4} &&
              runtime.Index() ==
                  std::vector<std::pair<std::int32_t, std::uint32_t>>{
                      {3, 2}, {5, 1}, {7, 0}},
          "adjacent duplicate run compacts 60-byte rows and rebuilds keys");

    Runtime separated;
    separated.SetRows({5, 2, 5});
    pal4::UtilitiesSceneTable::DeduplicateAndRebuildIndex(0x1000, separated);
    Check(separated.words[0x1064] == 0x40b4 &&
              separated.copies.empty() && separated.destroyed.empty() &&
              separated.Index()[0].first == 2 &&
              separated.Index()[1].first == 5 &&
              separated.Index()[2].first == 5,
          "separated equal keys remain distinct rows");
}

void CheckLargeIndexAndRange() {
    Runtime large;
    std::vector<std::int32_t> keys;
    for (int key = 8; key >= -8; --key) keys.push_back(key);
    large.SetRows(keys);
    pal4::UtilitiesSceneTable::DeduplicateAndRebuildIndex(0x1000, large);
    const auto index = large.Index();
    Check(index.size() == 17 &&
              index.front() ==
                  std::pair<std::int32_t, std::uint32_t>{-8, 16} &&
              index.back() ==
                  std::pair<std::int32_t, std::uint32_t>{8, 0} &&
              std::is_sorted(index.begin(), index.end()),
          "large index follows signed sorting path");

    Runtime range;
    Check(pal4::UtilitiesSceneArray::DestroyRange(
              0x4000, 0x40b4, range) == 0x407c &&
              range.destroyed ==
                  std::vector<std::uint32_t>{0x4000, 0x403c, 0x4078},
          "half-open 60-byte range returns final cleanup result");
    range.destroyed.clear();
    Check(pal4::UtilitiesSceneArray::DestroyRange(
              0x4000, 0x4000, range) == 0 &&
              range.destroyed.empty(),
          "empty range does not clean a record");
}

}  // namespace

int main() {
    try {
        CheckEmptyAndUnique();
        CheckDuplicateRuns();
        CheckLargeIndexAndRange();
        std::cout << "utilities_scene_index_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
