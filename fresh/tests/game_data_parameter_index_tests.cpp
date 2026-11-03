#include "pal4/game_data_parameter_index.h"

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

struct Runtime final : pal4::IGameDataParameterIndexRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> copies;
    std::vector<std::uint32_t> destroyed_texts;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> inserted;

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
    void DestroyTextRecord(std::uint32_t address) override {
        destroyed_texts.push_back(address);
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004 && at == words[0x100c],
              "key inserts at current index end");
        words[at] = key;
        words[at + 4] = index;
        words[0x100c] = at + 8;
        inserted.emplace_back(key, index);
    }
};

Runtime WithKeys(const std::vector<std::int32_t>& keys) {
    Runtime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] =
        0x2000 + static_cast<std::uint32_t>(keys.size()) * 328;
    runtime.words[0x1008] = 0x3000;
    runtime.words[0x100c] = 0x3400;
    for (std::size_t i = 0; i < keys.size(); ++i)
        runtime.words[0x2000 + static_cast<std::uint32_t>(i) * 328] =
            static_cast<std::uint32_t>(keys[i]);
    return runtime;
}

void CheckCompaction() {
    auto runtime = WithKeys({3, 3, -2, 4, 4, 1, -2});
    pal4::GameDataParameterIndex::Normalize(0x1000, runtime);
    Check(runtime.words[0x1064] == 0x2000 + 5 * 328 &&
              runtime.destroyed_texts.size() == 12 &&
              runtime.destroyed_texts[0] == 0x2000 + 5 * 328 + 84 &&
              runtime.destroyed_texts[6] == 0x2000 + 6 * 328 + 84 &&
              runtime.copies.front() ==
                  std::pair<std::uint32_t, std::uint32_t>{0x2000, 0x2000},
          "adjacent duplicates compact and vacated records are destroyed");
    const std::vector<std::int32_t> retained{3, -2, 4, 1, -2};
    for (std::size_t i = 0; i < retained.size(); ++i)
        Check(static_cast<std::int32_t>(runtime.words[
                  0x2000 + static_cast<std::uint32_t>(i) * 328]) == retained[i],
              "nonadjacent duplicates and source order remain");
    const std::vector<std::int32_t> sorted{-2, -2, 1, 3, 4};
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        const auto at = 0x3000 + static_cast<std::uint32_t>(i) * 8;
        Check(static_cast<std::int32_t>(runtime.words[at]) == sorted[i] &&
                  runtime.words[at + 4] < retained.size() &&
                  retained[runtime.words[at + 4]] == sorted[i],
              "sorted signed key retains its record index");
    }
}

void CheckEmptyAndLarge() {
    Runtime empty;
    empty.words[0x1060] = 0;
    empty.words[0x1064] = 0;
    empty.words[0x1008] = 0x3000;
    empty.words[0x100c] = 0x3400;
    pal4::GameDataParameterIndex::Normalize(0x1000, empty);
    Check(empty.words[0x100c] == 0x3000 &&
              empty.inserted.empty() && empty.destroyed_texts.empty(),
          "empty records reset index without element operations");

    std::vector<std::int32_t> keys;
    for (std::int32_t key = 20; key > -20; --key) keys.push_back(key);
    auto many = WithKeys(keys);
    pal4::GameDataParameterIndex::Normalize(0x1000, many);
    Check(many.words[0x1064] == 0x2000 + 40 * 328 &&
              many.copies.empty() && many.destroyed_texts.empty() &&
              many.words[0x100c] == 0x3000 + 40 * 8,
          "forty unique records need no compaction and rebuild index");
    for (std::int32_t i = 0; i < 40; ++i) {
        const auto at = 0x3000 + static_cast<std::uint32_t>(i) * 8;
        Check(static_cast<std::int32_t>(many.words[at]) == i - 19 &&
                  many.words[at + 4] ==
                      static_cast<std::uint32_t>(39 - i),
              "forty keys sort with matching source indices");
    }
}

void CheckDestroyRange() {
    Runtime runtime;
    pal4::GameDataParameterIndex::DestroyRange(
        0x2000, 0x2000 + 2 * 328, runtime);
    Check(runtime.destroyed_texts.size() == 12 &&
              runtime.destroyed_texts[0] == 0x2000 + 84 &&
              runtime.destroyed_texts[5] == 0x2000 + 4 &&
              runtime.destroyed_texts[6] == 0x2000 + 328 + 84,
          "range destroy walks 328-byte records with reverse six texts");
    Runtime empty;
    pal4::GameDataParameterIndex::DestroyRange(0x2000, 0x2000, empty);
    Check(empty.destroyed_texts.empty(),
          "empty range performs no text destruction");
}

}  // namespace

int main() {
    try {
        CheckCompaction();
        CheckEmptyAndLarge();
        CheckDestroyRange();
        std::cout << "game_data_parameter_index_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
