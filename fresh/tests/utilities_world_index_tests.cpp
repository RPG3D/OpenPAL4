#include "pal4/utilities_world_table.h"

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

struct Runtime final : pal4::IUtilitiesWorldIndexRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> destroyed;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> copied;

    explicit Runtime(const std::vector<std::int32_t>& keys) {
        words[0x1060] = keys.empty() ? 0 : 0x4000;
        words[0x1064] = keys.empty()
                             ? 0
                             : 0x4000 + static_cast<std::uint32_t>(keys.size()) * 40;
        words[0x1008] = 0x3000;
        words[0x100c] = 0x3000;
        for (std::size_t i = 0; i < keys.size(); ++i)
            words[0x4000 + static_cast<std::uint32_t>(i) * 40] =
                static_cast<std::uint32_t>(keys[i]);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void CopyRecord(std::uint32_t to, std::uint32_t from) override {
        copied.emplace_back(to, from);
        words[to] = words.at(from);
    }
    std::uint32_t DestroyRecord(std::uint32_t address) override {
        destroyed.push_back(address);
        return address;
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004 && at == words.at(0x100c),
              "index append location");
        words[at] = key;
        words[at + 4] = index;
        words[0x100c] = at + 8;
    }
};

void CheckDeduplication() {
    Runtime runtime({5, 5, 3, -2});
    pal4::UtilitiesWorldTable::DeduplicateAndRebuildIndex(
        0x1000, runtime);
    Check(runtime.words.at(0x1064) == 0x4078 &&
              runtime.words.at(0x4000) == 5 &&
              runtime.words.at(0x4028) == 3 &&
              static_cast<std::int32_t>(runtime.words.at(0x4050)) == -2,
          "adjacent duplicate compacted while preserving physical order");
    Check(runtime.destroyed == std::vector<std::uint32_t>{0x4078} &&
              runtime.copied ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x4000, 0x4000}, {0x4028, 0x4050},
                      {0x4050, 0x4078}},
          "moved records and trailing destructor addresses");
    Check(runtime.words.at(0x100c) == 0x3018 &&
              static_cast<std::int32_t>(runtime.words.at(0x3000)) == -2 &&
              runtime.words.at(0x3004) == 2 &&
              runtime.words.at(0x3008) == 3 &&
              runtime.words.at(0x300c) == 1 &&
              runtime.words.at(0x3010) == 5 &&
              runtime.words.at(0x3014) == 0,
          "signed sorted index retains physical positions");
}

void CheckEmptyAndLarge() {
    Runtime empty({});
    pal4::UtilitiesWorldTable::DeduplicateAndRebuildIndex(
        0x1000, empty);
    Check(empty.words.at(0x100c) == 0x3000 &&
              empty.destroyed.empty(), "empty arrays remain empty");

    std::vector<std::int32_t> keys;
    for (std::int32_t key = 17; key >= 1; --key)
        keys.push_back(key);
    Runtime large(keys);
    pal4::UtilitiesWorldTable::DeduplicateAndRebuildIndex(
        0x1000, large);
    Check(large.destroyed.empty() && large.copied.empty() &&
              large.words.at(0x1064) == 0x42a8 &&
              large.words.at(0x100c) == 0x3088,
          "17 unique records retained");
    for (std::uint32_t i = 0; i < 17; ++i) {
        Check(large.words.at(0x3000 + i * 8) == i + 1 &&
                  large.words.at(0x3004 + i * 8) == 16 - i,
              "large signed index sorted with physical positions");
    }
}

void CheckDestroyRange() {
    Runtime runtime({});
    Check(pal4::UtilitiesWorldArray::DestroyRange(
              0x4000, 0x4078, runtime) == 0x4050 &&
              runtime.destroyed == std::vector<std::uint32_t>{
                  0x4000, 0x4028, 0x4050},
          "half-open 40-byte range and last destructor result");
    Runtime empty({});
    Check(pal4::UtilitiesWorldArray::DestroyRange(
              0x4000, 0x4000, empty) == 0 &&
              empty.destroyed.empty(), "empty range does nothing");
}

}  // namespace

int main() {
    try {
        CheckDeduplication();
        CheckEmptyAndLarge();
        CheckDestroyRange();
        std::cout << "utilities_world_index_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
