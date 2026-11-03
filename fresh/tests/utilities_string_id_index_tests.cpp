#include "pal4/utilities_string_id_table.h"

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

struct Runtime final : pal4::IUtilitiesStringIdIndexRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> copies;
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
        words[destination] = words.at(source);
        words[destination + 4] = words.at(source + 4);
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004 && at == words.at(0x100c),
              "insertion uses current index-array end");
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
                                  static_cast<std::uint32_t>(keys.size()) * 8;
        words[0x1008] = 0x3000;
        words[0x100c] = 0x3008;
        for (std::size_t row = 0; row < keys.size(); ++row) {
            const auto at = 0x4000 + static_cast<std::uint32_t>(row) * 8;
            words[at] = static_cast<std::uint32_t>(keys[row]);
            words[at + 4] = static_cast<std::uint32_t>(row + 100);
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
    pal4::UtilitiesStringIdTable::DeduplicateAndRebuildIndex(0x1000,
                                                               empty);
    Check(empty.words[0x1064] == 0 && empty.words[0x100c] == 0x3000 &&
              empty.Index().empty() && empty.copies.empty(),
          "empty StringId table resets index without touching rows");

    Runtime unique;
    unique.SetRows({7, -2, 4});
    pal4::UtilitiesStringIdTable::DeduplicateAndRebuildIndex(0x1000,
                                                               unique);
    Check(unique.words[0x1064] == 0x4018 && unique.copies.empty() &&
              unique.words[0x4004] == 100 &&
              unique.words[0x400c] == 101 &&
              unique.Index() ==
                  std::vector<std::pair<std::int32_t, std::uint32_t>>{
                      {-2, 1}, {4, 2}, {7, 0}},
          "unique rows retain values and signed index uses physical positions");
}

void CheckDuplicateRuns() {
    Runtime runtime;
    runtime.SetRows({7, 5, 5, 3});
    pal4::UtilitiesStringIdTable::DeduplicateAndRebuildIndex(0x1000,
                                                               runtime);
    Check(runtime.words[0x1064] == 0x4018 &&
              runtime.words[0x4000] == 7 &&
              runtime.words[0x4008] == 5 &&
              runtime.words[0x4010] == 3 &&
              runtime.words[0x4014] == 103 &&
              runtime.copies ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x4008, 0x4008}, {0x4010, 0x4018}} &&
              runtime.Index() ==
                  std::vector<std::pair<std::int32_t, std::uint32_t>>{
                      {3, 2}, {5, 1}, {7, 0}},
          "adjacent duplicate IDs compact 8-byte value records");

    Runtime separated;
    separated.SetRows({5, 2, 5});
    pal4::UtilitiesStringIdTable::DeduplicateAndRebuildIndex(0x1000,
                                                               separated);
    Check(separated.words[0x1064] == 0x4018 &&
              separated.copies.empty() &&
              separated.Index()[0].first == 2 &&
              separated.Index()[1].first == 5 &&
              separated.Index()[2].first == 5,
          "separated equal IDs remain distinct rows");
}

void CheckLargeIndex() {
    Runtime large;
    std::vector<std::int32_t> keys;
    for (int key = 8; key >= -8; --key) keys.push_back(key);
    large.SetRows(keys);
    pal4::UtilitiesStringIdTable::DeduplicateAndRebuildIndex(0x1000,
                                                               large);
    const auto index = large.Index();
    Check(index.size() == 17 &&
              index.front() ==
                  std::pair<std::int32_t, std::uint32_t>{-8, 16} &&
              index.back() ==
                  std::pair<std::int32_t, std::uint32_t>{8, 0} &&
              std::is_sorted(index.begin(), index.end()),
          "large index follows signed sorting path");
}

}  // namespace

int main() {
    try {
        CheckEmptyAndUnique();
        CheckDuplicateRuns();
        CheckLargeIndex();
        std::cout << "utilities_string_id_index_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
