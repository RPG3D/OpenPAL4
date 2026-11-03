#include "pal4/game_data_monster_index.h"

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

struct Runtime final : pal4::IGameDataMonsterIndexRuntime {
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
        words[destination + 400] = words[source + 400];
    }
    void DestroyText(std::uint32_t text) override {
        destroyed_texts.push_back(text);
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004 && at == words[0x100c],
              "Monster key inserts at current index end");
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
        0x2000 + static_cast<std::uint32_t>(keys.size()) * 596;
    runtime.words[0x1008] = 0x3000;
    runtime.words[0x100c] = 0x3040;
    for (std::size_t i = 0; i < keys.size(); ++i)
        runtime.words[0x2000 + static_cast<std::uint32_t>(i) * 596 +
                      400] = static_cast<std::uint32_t>(keys[i]);
    return runtime;
}

void CheckCompaction() {
    auto runtime = WithKeys({3, 3, -2, 4, 4, 1, -2});
    pal4::GameDataMonsterIndex::Normalize(0x1000, runtime);
    Check(runtime.words[0x1064] == 0x2000 + 5 * 596 &&
              runtime.destroyed_texts.size() == 22 &&
              runtime.destroyed_texts[0] ==
                  0x2000 + 5 * 596 + 336 + 232 &&
              runtime.destroyed_texts[11] ==
                  0x2000 + 6 * 596 + 336 + 232 &&
              runtime.copies.front() ==
                  std::pair<std::uint32_t, std::uint32_t>{0x2000, 0x2000},
          "adjacent duplicate keys compact and destroy vacated records");
    const std::vector<std::int32_t> retained{3, -2, 4, 1, -2};
    for (std::size_t i = 0; i < retained.size(); ++i)
        Check(static_cast<std::int32_t>(runtime.words[
                  0x2000 + static_cast<std::uint32_t>(i) * 596 +
                  400]) == retained[i],
              "compaction preserves non-adjacent duplicate and record order");
    const std::vector<std::int32_t> sorted{-2, -2, 1, 3, 4};
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        const auto at = 0x3000 + static_cast<std::uint32_t>(i) * 8;
        Check(static_cast<std::int32_t>(runtime.words[at]) == sorted[i] &&
                  runtime.words[at + 4] < retained.size() &&
                  retained[runtime.words[at + 4]] == sorted[i],
              "signed sorted key retains matching record index");
    }
}

void CheckEmptyAndLong() {
    Runtime empty;
    empty.words[0x1060] = 0;
    empty.words[0x1064] = 0;
    empty.words[0x1008] = 0x3000;
    empty.words[0x100c] = 0x3040;
    pal4::GameDataMonsterIndex::Normalize(0x1000, empty);
    Check(empty.words[0x100c] == 0x3000 && empty.inserted.empty() &&
              empty.destroyed_texts.empty(),
          "empty records clear existing index without element operations");

    std::vector<std::int32_t> keys;
    for (std::int32_t key = 20; key > -20; --key) keys.push_back(key);
    auto many = WithKeys(keys);
    pal4::GameDataMonsterIndex::Normalize(0x1000, many);
    Check(many.words[0x1064] == 0x2000 + 40 * 596 &&
              many.copies.empty() && many.destroyed_texts.empty() &&
              many.words[0x100c] == 0x3000 + 40 * 8,
          "40 distinct records stay in place and rebuild key index");
    for (std::int32_t i = 0; i < 40; ++i) {
        const auto at = 0x3000 + static_cast<std::uint32_t>(i) * 8;
        Check(static_cast<std::int32_t>(many.words[at]) == i - 19 &&
                  many.words[at + 4] ==
                      static_cast<std::uint32_t>(39 - i),
              "large index sorts signed keys with original indices");
    }
}

void CheckRangeHelpers() {
    Runtime runtime;
    pal4::GameDataMonsterIndex::DestroyRange(
        0x2000, 0x2000 + 2 * 596, runtime);
    Check(runtime.destroyed_texts.size() == 22 &&
              runtime.destroyed_texts[0] == 0x2000 + 336 + 232 &&
              runtime.destroyed_texts[11] ==
                  0x2000 + 596 + 336 + 232,
          "range cleanup walks 596-byte records");
    Runtime empty;
    pal4::GameDataMonsterIndex::DestroyRange(0x2000, 0x2000, empty);
    Check(empty.destroyed_texts.empty(),
          "empty range performs no destruction");

    runtime.words[0x2000 + 400] = static_cast<std::uint32_t>(-3);
    runtime.words[0x3000 + 400] = static_cast<std::uint32_t>(-3);
    Check(pal4::GameDataMonsterIndex::EqualKeys(0x2000, 0x3000,
                                                 runtime),
          "record comparator tests exact +400 DWORD equality");
    runtime.words[0x3000 + 400] = 4;
    Check(!pal4::GameDataMonsterIndex::EqualKeys(0x2000, 0x3000,
                                                  runtime),
          "different signed keys compare unequal");

    auto compact = WithKeys({5, 5, 2, 5, 5, -1});
    Check(pal4::GameDataMonsterIndex::CompactFrom(
              0x2000, 0x2000 + 6 * 596, 0x5000, compact) ==
              0x5000 + 4 * 596 &&
              compact.copies.size() == 4 &&
              static_cast<std::int32_t>(compact.words[0x5000 + 400]) ==
                  5 &&
              static_cast<std::int32_t>(compact.words[0x5000 + 596 +
                                                      400]) == 2 &&
              static_cast<std::int32_t>(compact.words[0x5000 + 3 *
                                                      596 + 400]) == -1,
          "copy helper preserves only adjacent-unique source records");
}

}  // namespace

int main() {
    try {
        CheckCompaction();
        CheckEmptyAndLong();
        CheckRangeHelpers();
        std::cout << "game_data_monster_index_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
