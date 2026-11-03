#include "pal4/game_data_collection.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataCollectionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::set<std::uint32_t> ids;
    std::vector<std::uint32_t> inserted;
    std::uint32_t count = 0;
    std::uint32_t count_calls = 0;
    std::uint32_t allocation_bytes = 0;
    std::uint32_t allocation_result = 0x8000u;

    std::uint32_t GameDataOwner() override { return 0x4000u; }
    std::uint32_t RecordCount(std::uint32_t owner) override {
        Check(owner == 0x4000u, "record owner");
        ++count_calls;
        return count;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    bool ContainsId(std::uint32_t set, std::uint32_t id) override {
        Check(set == 0x2000u, "fixed sorted set address");
        return ids.contains(id);
    }
    void InsertUniqueId(std::uint32_t set, std::uint32_t id) override {
        Check(set == 0x2000u, "fixed sorted insert address");
        inserted.push_back(id);
        ids.insert(id);
    }
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        allocation_bytes = size;
        return allocation_result;
    }
};

void CheckRange() {
    Runtime runtime;
    runtime.words[0x5000u + 60u] = 1u;
    Check(pal4::GameDataCollection::CheckValueRange({1, 3}, 0x5000u,
                                                    runtime),
          "positive indexed value is accepted");
    runtime.words[0x5000u + 60u] = 3u;
    Check(pal4::GameDataCollection::CheckValueRange({1, 3}, 0x5000u,
                                                    runtime),
          "inclusive upper bound");
    runtime.words[0x5000u + 60u] = 0u;
    Check(!pal4::GameDataCollection::CheckValueRange({1, 3}, 0x5000u,
                                                     runtime),
          "zero is rejected");
    runtime.words[0x5000u + 60u] = 0xffffffffu;
    Check(!pal4::GameDataCollection::CheckValueRange({1, 3}, 0x5000u,
                                                     runtime),
          "negative signed value is rejected");
    runtime.words[0x5000u + 60u] = 4u;
    Check(!pal4::GameDataCollection::CheckValueRange({1, 3}, 0x5000u,
                                                     runtime),
          "value above the limit is rejected");
}

void CheckCollection() {
    Runtime empty;
    Check(pal4::GameDataCollection::BuildNewMatches(
              0x2000u, 0x3000u, {0, 3}, 0x12abu, empty) == 0x3000u &&
              empty.bytes[0x3000u] == 0xabu &&
              empty.allocation_bytes == 0u &&
              empty.words[0x3004u] == 0x8000u &&
              empty.words[0x3008u] == 0x8000u &&
              empty.words[0x300cu] == 0x8000u,
          "empty collection preserves the output header and zero allocation");

    Runtime runtime;
    runtime.count = 4u;
    runtime.words[0x4060u] = 0x5000u;
    const auto r0 = 0x5000u;
    const auto r1 = r0 + 208u;
    const auto r2 = r1 + 208u;
    const auto r3 = r2 + 208u;
    runtime.words[r0] = 10u;
    runtime.words[r1] = 20u;
    runtime.words[r2] = 20u;
    runtime.words[r3] = 30u;
    runtime.words[r0 + 56u] = 1u;
    runtime.words[r1 + 56u] = 2u;
    runtime.words[r2 + 56u] = 3u;
    runtime.words[r3 + 56u] = 4u;
    runtime.ids.insert(10u);
    Check(pal4::GameDataCollection::BuildNewMatches(
              0x2000u, 0x3000u, {0, 3}, 0x55u, runtime) == 0x3000u &&
              runtime.count_calls == 5u &&
              runtime.allocation_bytes == 8u &&
              runtime.bytes[0x3000u] == 0x55u &&
              runtime.words[0x3004u] == 0x8000u &&
              runtime.words[0x3008u] == 0x8008u &&
              runtime.words[0x300cu] == 0x8008u &&
              runtime.words[0x8000u] == r1 &&
              runtime.words[0x8004u] == r2 &&
              runtime.inserted == std::vector<std::uint32_t>{10u, 20u, 20u} &&
              runtime.ids == std::set<std::uint32_t>{10u, 20u},
          "all matching IDs insert after both duplicate records enter output");

    Runtime allocation_failure;
    allocation_failure.count = 1u;
    allocation_failure.words[0x4060u] = 0x5000u;
    allocation_failure.words[0x5000u] = 7u;
    allocation_failure.words[0x5038u] = 1u;
    allocation_failure.allocation_result = 0u;
    Check(pal4::GameDataCollection::BuildNewMatches(
              0x2000u, 0x3000u, {0, 1}, 0u,
              allocation_failure) == 0x3000u &&
              allocation_failure.allocation_bytes == 4u &&
              allocation_failure.words[0x3004u] == 0u &&
              allocation_failure.words[0x3008u] == 4u,
          "null allocation preserves the observed pointer arithmetic");
}

}  // namespace

int main() {
    try {
        CheckRange();
        CheckCollection();
        std::cout << "game_data_collection_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
