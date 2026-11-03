#include "pal4/game_data_mission_selection.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataMissionSelectionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5004, 0x7000}, {0x7060, 0x9000}, {0x7064, 0x9000}};
    std::vector<std::size_t> allocations;
    std::vector<std::uint32_t> freed;
    std::uint32_t next_address = 0xa000;
    bool fail_allocation = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.contains(address), "Mission selector reads known addresses");
        return words.at(address);
    }
    std::uint32_t AllocatePointerStorage(std::size_t capacity) override {
        allocations.push_back(capacity);
        if (fail_allocation) return 0;
        const auto result = next_address;
        next_address += 0x1000;
        return result;
    }
    void FreePointerStorage(std::uint32_t address) override {
        freed.push_back(address);
    }
    void SetRecords(const std::vector<std::int32_t>& keys) {
        words[0x7060] = 0x9000;
        words[0x7064] = 0x9000 +
                        static_cast<std::uint32_t>(keys.size() * 68);
        for (std::size_t i = 0; i < keys.size(); ++i)
            words[0x9008 + static_cast<std::uint32_t>(i * 68)] =
                static_cast<std::uint32_t>(keys[i]);
    }
};

void CheckSorted(const std::vector<std::int32_t>& keys,
                 const pal4::GameDataMissionPointerArray& output,
                 const Runtime& runtime) {
    Check(output.records.size() == keys.size(),
          "every 68-byte Mission record was collected");
    auto addresses = output.records;
    std::sort(addresses.begin(), addresses.end());
    for (std::size_t i = 0; i < addresses.size(); ++i)
        Check(addresses[i] == 0x9000 + i * 68,
              "pointer list contains every original record exactly once");
    for (std::size_t i = 1; i < output.records.size(); ++i) {
        const auto before = static_cast<std::int32_t>(
            runtime.words.at(output.records[i - 1] + 8));
        const auto after = static_cast<std::int32_t>(
            runtime.words.at(output.records[i] + 8));
        Check(before >= after, "signed member +8 sorts descending");
    }
}

void CheckEmptyAndSingle() {
    Runtime runtime;
    pal4::GameDataMissionPointerArray output{0xb000, 4, {0xdead, 0xbeef}};
    Check(pal4::GameDataMissionSelection::CollectAndSort(
              0x5000, output, runtime) == 0 &&
              output.records.empty() && runtime.allocations.empty(),
          "empty table clears output without allocating");
    runtime.SetRecords({-9});
    Check(pal4::GameDataMissionSelection::CollectAndSort(
              0x5000, output, runtime) == 0xb004 &&
              output.records == std::vector<std::uint32_t>{0x9000} &&
              runtime.allocations.empty(),
          "single record reuses existing pointer storage");
    runtime.words[0x7060] = 0;
    Check(pal4::GameDataMissionSelection::CollectAndSort(
              0x5000, output, runtime) == 0 && output.records.empty(),
          "null record begin returns zero without reading its end");
}

void CheckThresholdsAndGrowth() {
    for (std::size_t count : {2u, 16u, 17u, 40u}) {
        Runtime runtime;
        std::vector<std::int32_t> keys;
        for (std::size_t i = 0; i < count; ++i)
            keys.push_back(static_cast<std::int32_t>(
                (i * 37) % (count + 3)) - 8);
        runtime.SetRecords(keys);
        pal4::GameDataMissionPointerArray output;
        const auto result = pal4::GameDataMissionSelection::CollectAndSort(
            0x5000, output, runtime);
        CheckSorted(keys, output, runtime);
        Check(output.capacity >= count &&
                  result == output.begin_address + count * 4,
              "output end points past all sorted pointers");
        Check(runtime.allocations.front() == 1 &&
                  runtime.allocations.back() == output.capacity &&
                  runtime.freed.size() + 1 == runtime.allocations.size(),
              "pointer capacity grows by doubling and frees old buffers");
        for (std::size_t i = 1; i < runtime.allocations.size(); ++i)
            Check(runtime.allocations[i] == runtime.allocations[i - 1] * 2,
                  "capacity doubles at each growth boundary");
    }
}

void CheckDuplicatesAndFailure() {
    Runtime runtime;
    std::vector<std::int32_t> keys(25, 4);
    runtime.SetRecords(keys);
    pal4::GameDataMissionPointerArray output{0xd000, 32, {0xdead}};
    Check(pal4::GameDataMissionSelection::CollectAndSort(
              0x5000, output, runtime) == 0xd000 + 25 * 4 &&
              runtime.allocations.empty(),
          "equal keys terminate partition and reuse reserved storage");
    CheckSorted(keys, output, runtime);

    Runtime failing;
    failing.SetRecords({1});
    failing.fail_allocation = true;
    pal4::GameDataMissionPointerArray empty;
    bool threw = false;
    try {
        (void)pal4::GameDataMissionSelection::CollectAndSort(
            0x5000, empty, failing);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && empty.begin_address == 0 &&
              empty.capacity == 0 && empty.records.empty() &&
              failing.freed.empty(),
          "allocation failure leaves the empty output unmodified");
}

}  // namespace

int main() {
    try {
        CheckEmptyAndSingle();
        CheckThresholdsAndGrowth();
        CheckDuplicatesAndFailure();
        std::cout << "game_data_mission_selection_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
