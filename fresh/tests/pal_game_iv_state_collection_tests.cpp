#include "pal4/pal_game_iv_state_collection.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalGameIVStateCollectionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::string> strings;
    std::unordered_map<std::uint32_t, std::string> by_data;
    std::vector<std::uint32_t> allocation_sizes;
    std::vector<std::uint32_t> freed;
    std::vector<std::string> visited;
    std::uint32_t next_block = 0x5000u;
    std::uint32_t next_data = 0x9000u;
    std::uint32_t temporary_cleanups = 0u;
    std::uint32_t cleanup_count = 0u;
    std::uint32_t empty_result_reads = 0u;
    std::uint32_t callback_count = 0u;
    bool fail_allocation = false;
    bool append_in_callback = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocation_sizes.push_back(bytes);
        if (fail_allocation) return 0u;
        const auto block = next_block;
        next_block += 0x1000u;
        return block;
    }
    void Free(std::uint32_t address) override { freed.push_back(address); }
    bool EqualsString(std::uint32_t record,
                      std::string_view text) override {
        return strings.at(record) == text;
    }
    void ConstructString(std::uint32_t record,
                         std::string_view text) override {
        const auto data = next_data++;
        strings[record] = text;
        by_data[data] = text;
        words[record] = data;
    }
    void CopyString(std::uint32_t destination,
                    std::uint32_t source) override {
        ConstructString(destination, strings.at(source));
    }
    std::uint32_t CleanupString(std::uint32_t record) override {
        ++cleanup_count;
        by_data.erase(words[record]);
        strings.erase(record);
        words[record] = 0u;
        return record;
    }
    std::uint32_t CleanupInputTemporary(std::string_view) override {
        ++temporary_cleanups;
        return 0xabcu;
    }
    std::uint32_t EmptyCleanupResult() override {
        ++empty_result_reads;
        return 0xdefu;
    }
    void InvokeCallback(std::uint32_t callback,
                        std::uint32_t data,
                        std::uint32_t argument) override {
        Check(callback == 0x1234u && argument == 0x5678u,
              "callback arguments");
        visited.push_back(by_data.at(data));
        ++callback_count;
        if (append_in_callback && callback_count == 1u)
            (void)pal4::PalGameIVStateCollection::AddOrUpdate(
                0x1000u, "appended", *this);
    }
};

void GrowthDedupAndIteration() {
    Runtime runtime;
    for (auto text : {"logo", "scene", "combat", "trade", "loading", "bink"}) {
        Check(pal4::PalGameIVStateCollection::AddOrUpdate(
                  0x1000u, text, runtime) == 0xabcu, "append return");
    }
    Check(runtime.allocation_sizes ==
              std::vector<std::uint32_t>({12u, 24u, 48u, 96u}),
          "geometric growth");
    Check(runtime.freed ==
              std::vector<std::uint32_t>({0u, 0x5000u, 0x6000u, 0x7000u}),
          "old block release");
    Check(runtime.ReadWord(0x11a0u) == 0x8000u &&
              runtime.ReadWord(0x11a4u) == 0x8000u + 72u &&
              runtime.ReadWord(0x11a8u) == 0x8000u + 96u,
          "native vector fields");
    const auto alloc_count = runtime.allocation_sizes.size();
    Check(pal4::PalGameIVStateCollection::AddOrUpdate(
              0x1000u, "scene", runtime) == 0xabcu &&
              runtime.allocation_sizes.size() == alloc_count &&
              runtime.ReadWord(0x11a4u) == 0x8000u + 72u,
          "duplicate leaves vector unchanged");
    Check(runtime.temporary_cleanups == 7u, "input temporary cleanup");
    const auto end = runtime.ReadWord(0x11a4u);
    Check(pal4::PalGameIVStateCollection::ForEach(
              0x1000u, 0u, 0x5678u, runtime) == end &&
              runtime.callback_count == 0u, "null callback");
    Check(pal4::PalGameIVStateCollection::ForEach(
              0x1000u, 0x1234u, 0x5678u, runtime) == end,
          "callback iteration return");
    Check(runtime.visited == std::vector<std::string>({
              "logo", "scene", "combat", "trade", "loading", "bink"}),
          "callback strings in order");
    Check(pal4::PalGameIVStateCollection::Clear(0x1000u, runtime) ==
              0x8000u + 60u, "last cleanup return");
    Check(runtime.ReadWord(0x11a0u) == runtime.ReadWord(0x11a4u) &&
              runtime.ReadWord(0x11a8u) == 0x8000u + 96u &&
              runtime.cleanup_count == 13u, "clear keeps capacity");
    Check(pal4::PalGameIVStateCollection::Clear(0x1000u, runtime) ==
              0xdefu, "empty cleanup result boundary");
}

void CallbackGrowthAndFailure() {
    Runtime runtime;
    for (auto text : {"first", "second"})
        (void)pal4::PalGameIVStateCollection::AddOrUpdate(
            0x1000u, text, runtime);
    runtime.fail_allocation = true;
    try {
        (void)pal4::PalGameIVStateCollection::AddOrUpdate(
            0x1000u, "third", runtime);
        Check(false, "allocation failure must throw");
    } catch (const std::bad_alloc&) {
        Check(runtime.ReadWord(0x11a4u) == 0x6000u + 24u &&
                  runtime.strings.size() == 2u,
              "allocation failure preserves entries");
    }
    runtime.fail_allocation = false;
    for (auto text : {"third", "fourth"})
        (void)pal4::PalGameIVStateCollection::AddOrUpdate(
            0x1000u, text, runtime);
    (void)pal4::PalGameIVStateCollection::Clear(0x1000u, runtime);
    for (auto text : {"first", "second"})
        (void)pal4::PalGameIVStateCollection::AddOrUpdate(
            0x1000u, text, runtime);
    runtime.append_in_callback = true;
    (void)pal4::PalGameIVStateCollection::ForEach(
        0x1000u, 0x1234u, 0x5678u, runtime);
    Check(runtime.visited ==
              std::vector<std::string>({"first", "second", "appended"}),
          "iteration observes updated end");
}

}  // namespace

int main() {
    try {
        GrowthDedupAndIteration();
        CallbackGrowthAndFailure();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
