#include "pal4/utilities_text_record_table.h"

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

struct Runtime final : pal4::IUtilitiesTextIndexRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> copies;
    std::vector<std::uint32_t> destroyed;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> inserted;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        copies.emplace_back(destination, source);
        for (std::uint32_t i = 0; i < 20; i += 4)
            words[destination + i] = words.at(source + i);
    }
    std::uint32_t DestroyRecord(std::uint32_t record) override {
        destroyed.push_back(record);
        return record + 4;
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004 && at == words.at(0x100c),
              "index insertion uses current end pointer");
        inserted.emplace_back(key, index);
        words[at] = key;
        words[at + 4] = index;
        words[0x100c] = at + 8;
    }

    void SetRows(const std::vector<std::int32_t>& keys) {
        words[0x1060] = keys.empty() ? 0 : 0x4000;
        words[0x1064] = keys.empty()
                            ? 0
                            : 0x4000 + static_cast<std::uint32_t>(keys.size()) * 20;
        words[0x1008] = 0x3000;
        words[0x100c] = 0x3008;
        for (std::size_t row = 0; row < keys.size(); ++row) {
            const auto at = 0x4000 + static_cast<std::uint32_t>(row) * 20;
            words[at] = static_cast<std::uint32_t>(keys[row]);
            for (std::uint32_t i = 4; i < 20; i += 4)
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
    pal4::UtilitiesTextRecordTable::DeduplicateAndRebuildIndex(0x1000, empty);
    Check(empty.words[0x1064] == 0 && empty.words[0x100c] == 0x3000 &&
              empty.Index().empty() && empty.copies.empty() &&
              empty.destroyed.empty(),
          "empty table clears index without accessing row storage");

    Runtime unique;
    unique.SetRows({7, -2, 4});
    pal4::UtilitiesTextRecordTable::DeduplicateAndRebuildIndex(0x1000, unique);
    Check(unique.words[0x1064] == 0x403c && unique.copies.empty() &&
              unique.destroyed.empty() &&
              unique.inserted ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {7, 0}, {static_cast<std::uint32_t>(-2), 1}, {4, 2}} &&
              unique.Index() ==
                  std::vector<std::pair<std::int32_t, std::uint32_t>>{
                      {-2, 1}, {4, 2}, {7, 0}},
          "unique records retain physical order and index sorts signed keys");
}

void CheckAdjacentDuplicates() {
    Runtime runtime;
    runtime.SetRows({5, 5, 2, 2, 8});
    pal4::UtilitiesTextRecordTable::DeduplicateAndRebuildIndex(0x1000,
                                                                 runtime);
    Check(runtime.words[0x1064] == 0x403c &&
              runtime.words[0x4000] == 5 &&
              runtime.words[0x4014] == 2 &&
              runtime.words[0x4028] == 8 &&
              runtime.copies ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x4000, 0x4000}, {0x4014, 0x4028},
                      {0x4028, 0x4050}} &&
              runtime.destroyed == std::vector<std::uint32_t>{0x403c,
                                                               0x4050} &&
              runtime.Index() ==
                  std::vector<std::pair<std::int32_t, std::uint32_t>>{
                      {2, 1}, {5, 0}, {8, 2}},
          "adjacent duplicate runs compact and tail records are cleaned");

    Runtime prefixed;
    prefixed.SetRows({7, 5, 5, 3});
    pal4::UtilitiesTextRecordTable::DeduplicateAndRebuildIndex(0x1000,
                                                                 prefixed);
    Check(prefixed.words[0x1064] == 0x403c &&
              prefixed.words[0x4000] == 7 &&
              prefixed.words[0x4014] == 5 &&
              prefixed.words[0x4028] == 3 &&
              prefixed.copies ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x4014, 0x4014}, {0x4028, 0x403c}} &&
              prefixed.destroyed == std::vector<std::uint32_t>{0x403c} &&
              prefixed.Index() ==
                  std::vector<std::pair<std::int32_t, std::uint32_t>>{
                      {3, 2}, {5, 1}, {7, 0}},
          "unique prefix stays in place before later duplicate run");
}

void CheckSeparatedDuplicatesAndLargeIndex() {
    Runtime separated;
    separated.SetRows({5, 2, 5});
    pal4::UtilitiesTextRecordTable::DeduplicateAndRebuildIndex(0x1000,
                                                                 separated);
    Check(separated.words[0x1064] == 0x403c &&
              separated.destroyed.empty() && separated.copies.empty() &&
              separated.Index()[0].first == 2 &&
              separated.Index()[1].first == 5 &&
              separated.Index()[2].first == 5,
          "nonadjacent duplicate keys remain distinct records");

    Runtime large;
    std::vector<std::int32_t> descending;
    for (int key = 8; key >= -8; --key) descending.push_back(key);
    large.SetRows(descending);
    pal4::UtilitiesTextRecordTable::DeduplicateAndRebuildIndex(0x1000,
                                                                 large);
    const auto index = large.Index();
    Check(index.size() == 17 && index.front() ==
              std::pair<std::int32_t, std::uint32_t>{-8, 16} &&
              index.back() ==
                  std::pair<std::int32_t, std::uint32_t>{8, 0} &&
              std::is_sorted(index.begin(), index.end()),
          "index larger than 16 entries sorts by signed key");
}

void CheckDestroyRange() {
    Runtime runtime;
    Check(pal4::UtilitiesTextRecordArray::DestroyRange(
              0x4000, 0x403c, runtime) == 0x402c &&
              runtime.destroyed ==
                  std::vector<std::uint32_t>{0x4000, 0x4014, 0x4028},
          "half-open 20-byte range returns last cleanup result");
    runtime.destroyed.clear();
    Check(pal4::UtilitiesTextRecordArray::DestroyRange(
              0x4000, 0x4000, runtime) == 0 &&
              runtime.destroyed.empty(),
          "empty cleanup range is deterministic and makes no calls");
}

}  // namespace

int main() {
    try {
        CheckEmptyAndUnique();
        CheckAdjacentDuplicates();
        CheckSeparatedDuplicatesAndLargeIndex();
        CheckDestroyRange();
        std::cout << "utilities_text_record_index_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
