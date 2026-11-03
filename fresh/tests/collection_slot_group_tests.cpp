#include "pal4/collection_slot_group.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICollectionSlotGroupProcessRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::pair<char, std::uint32_t>> events;
    std::vector<std::pair<std::int32_t, std::uint32_t>> invalid_slots;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> appended;
    std::vector<std::uint32_t> inserted;
    std::uint32_t source = 0u;
    std::uint32_t slot = 0x1028u;
    std::uint32_t copied_source = 0u;
    std::uint32_t copied_destination = 0u;
    std::uint32_t looked_up_key = 0u;
    std::uint32_t allocated_bytes = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void DestroyEntry(std::uint32_t entry) override {
        events.emplace_back('D', entry);
    }
    void FreeMemory(std::uint32_t address) override {
        events.emplace_back('F', address);
    }
    void ReportInvalidSlot(std::int32_t index,
                           std::uint32_t line) override {
        invalid_slots.emplace_back(index, line);
    }
    std::uint32_t FindGameData(std::uint32_t key) override {
        looked_up_key = key;
        return source;
    }
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        allocated_bytes = size;
        if (allocations.empty()) return 0u;
        const auto result = allocations.front();
        allocations.erase(allocations.begin());
        return result;
    }
    void CopyItemSnapshot(std::uint32_t destination,
                          std::uint32_t from) override {
        copied_destination = destination;
        copied_source = from;
    }
    void AppendToSlot(std::uint32_t target,
                      std::uint32_t snapshot) override {
        Check(target == slot, "append slot");
        appended.push_back(snapshot);
    }
    void InsertGlobalData(std::uint32_t key,
                          std::uint32_t snapshot) override {
        Check(key == looked_up_key, "global key");
        inserted.push_back(snapshot);
    }
};

void CheckConstruction() {
    Runtime runtime;
    Check(pal4::CollectionSlotGroup::Construct(0x1000u, 0xa5u, 0x37u,
                                              runtime) == 0x1000u,
          "constructor returns object");
    Check(runtime.words[0x1000u] == pal4::CollectionSlotGroup::kVtable &&
              runtime.words[0x1004u] == 1u &&
              runtime.bytes[0x1078u] == 0xa5u &&
              runtime.words[0x107cu] == 0u &&
              runtime.words[0x1080u] == 0u &&
              runtime.words[0x1084u] == 0u,
          "constructor initializes header and tail");
    for (std::uint32_t index = 0; index < 7u; ++index) {
        const auto element = 0x1008u + index * 16u;
        Check(runtime.bytes[element] == 0x37u &&
                  runtime.words[element + 4u] == 0u &&
                  runtime.words[element + 8u] == 0u &&
                  runtime.words[element + 12u] == 0u,
              "constructor initializes all seven elements");
    }
    pal4::CollectionSlotGroup::InitializeElement(0x2000u, 0x56u, runtime);
    Check(runtime.bytes[0x2000u] == 0x56u &&
              runtime.words[0x2004u] == 0u &&
              runtime.words[0x2008u] == 0u &&
              runtime.words[0x200cu] == 0u,
          "single-element constructor preserves explicit stack seed");
}

void CheckClearAndDestroy() {
    Runtime runtime;
    (void)pal4::CollectionSlotGroup::Construct(0x1000u, 0u, 0u, runtime);
    runtime.words[0x100cu] = 0x2000u;
    runtime.words[0x1010u] = 0x200cu;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x2004u] = 0u;
    runtime.words[0x2008u] = 0x3100u;
    runtime.words[0x101cu] = 0x2100u;
    runtime.words[0x1020u] = 0x2104u;
    runtime.words[0x2100u] = 0x3200u;
    runtime.words[0x107cu] = 0x4000u;
    runtime.words[0x1080u] = 0x4008u;

    Check(pal4::CollectionSlotGroup::ClearEntries(0x1000u, runtime) == 0x4008u &&
              runtime.events ==
                  std::vector<std::pair<char, std::uint32_t>>{
                      {'D', 0x3000u}, {'F', 0x3000u},
                      {'D', 0x3100u}, {'F', 0x3100u},
                      {'D', 0x3200u}, {'F', 0x3200u}} &&
              runtime.words[0x1010u] == 0x2000u &&
              runtime.words[0x1020u] == 0x2100u &&
              runtime.words[0x1080u] == 0x4000u,
          "clear destroys nonnull entries in slot order and resets ends");
    runtime.events.clear();
    runtime.words[0x101cu] = 0x2100u;
    runtime.words[0x1020u] = 0x2104u;
    runtime.words[0x2100u] = 0x3300u;
    runtime.words[0x102cu] = 0x2200u;
    runtime.words[0x1030u] = 0x2200u;
    runtime.words[0x1034u] = 0x2204u;
    runtime.words[0x107cu] = 0x4000u;
    runtime.words[0x1080u] = 0x4004u;

    pal4::CollectionSlotGroup::Destroy(0x1000u, runtime);
    Check(runtime.events.size() == 10u &&
              runtime.events[0] == std::make_pair('D', 0x3300u) &&
              runtime.events[1] == std::make_pair('F', 0x3300u) &&
              runtime.events[2] == std::make_pair('F', 0x4000u) &&
              runtime.events[3] == std::make_pair('F', 0u) &&
              runtime.events[8] == std::make_pair('F', 0x2100u) &&
              runtime.events[9] == std::make_pair('F', 0x2000u),
          "destructor frees tail then seven element buffers in reverse order");
    Check(runtime.words[0x1000u] == pal4::CollectionSlotGroup::kBaseVtable &&
              runtime.words[0x1004u] == 1u &&
              runtime.words[0x107cu] == 0u &&
              runtime.words[0x1080u] == 0u &&
              runtime.words[0x1084u] == 0u,
          "destructor restores base state and clears tail");
    for (std::uint32_t index = 0; index < 7u; ++index) {
        const auto element = 0x1008u + index * 16u;
        Check(runtime.words[element + 4u] == 0u &&
                  runtime.words[element + 8u] == 0u &&
                  runtime.words[element + 12u] == 0u,
              "destructor clears each element buffer triplet");
    }
}

void CheckProcessGameData() {
    Runtime runtime;
    (void)pal4::CollectionSlotGroup::Construct(0x1000u, 0u, 0u, runtime);
    Check(pal4::CollectionSlotGroup::ProcessGameData(
              0x1000u, 0x55u, runtime) == 0u &&
              runtime.looked_up_key == 0x55u && runtime.appended.empty(),
          "missing game-data record returns null without insertion");
    runtime.source = 0x9000u;
    runtime.words[0x9000u + 148u] = 7u;
    Check(pal4::CollectionSlotGroup::ProcessGameData(
              0x1000u, 0x55u, runtime) == 0u &&
              runtime.invalid_slots.back() ==
                  std::make_pair(7, 0xd4u) && runtime.appended.empty(),
          "invalid target slot returns null without allocation");

    runtime.words[0x9000u + 148u] = 2u;
    runtime.allocations = {0xa000u, 0xb000u};
    Check(pal4::CollectionSlotGroup::ProcessGameData(
              0x1000u, 0x55u, runtime) == 0xa000u &&
              runtime.copied_source == 0x9000u &&
              runtime.copied_destination == 0xa000u &&
              runtime.words[0xa000u + 400u] == 1u &&
              runtime.words[0x1004u] == 2u &&
              runtime.appended == std::vector<std::uint32_t>{0xa000u} &&
              runtime.inserted == std::vector<std::uint32_t>{0xa000u} &&
              runtime.words[0x107cu] == 0xb000u &&
              runtime.words[0x1080u] == 0xb004u &&
              runtime.words[0x1084u] == 0xb004u &&
              runtime.words[0xb000u] == 1u &&
              runtime.allocated_bytes == 4u,
          "first snapshot grows sequence queue and inserts into both stores");

    runtime.allocations = {0xa100u, 0xc000u};
    Check(pal4::CollectionSlotGroup::ProcessGameData(
              0x1000u, 0x56u, runtime) == 0xa100u &&
              runtime.words[0x1004u] == 3u &&
              runtime.words[0xc000u] == 1u &&
              runtime.words[0xc004u] == 2u &&
              runtime.words[0x107cu] == 0xc000u &&
              runtime.words[0x1080u] == 0xc008u &&
              runtime.words[0x1084u] == 0xc008u &&
              runtime.events.back() == std::make_pair('F', 0xb000u),
          "full queue doubles storage and frees the old buffer");

    runtime.words[0x1084u] = 0xc00cu;
    runtime.allocations = {0xa200u};
    Check(pal4::CollectionSlotGroup::ProcessGameData(
              0x1000u, 0x57u, runtime) == 0xa200u &&
              runtime.words[0xc008u] == 3u &&
              runtime.words[0x1080u] == 0xc00cu &&
              runtime.events.back() == std::make_pair('F', 0xb000u),
          "available capacity appends without allocation or free");

    runtime.words[0x107cu] = 0xd000u;
    runtime.words[0x1080u] = 0xd018u;
    runtime.words[0x1084u] = 0xd018u;
    for (std::uint32_t index = 0; index < 6u; ++index)
        runtime.words[0xd000u + index * 4u] = 10u + index;
    runtime.allocations = {0xa300u};
    Check(pal4::CollectionSlotGroup::ProcessGameData(
              0x1000u, 0x58u, runtime) == 0xa300u &&
              runtime.words[0xd000u] == 11u &&
              runtime.words[0xd010u] == 15u &&
              runtime.words[0xd014u] == 4u &&
              runtime.words[0x1080u] == 0xd018u &&
              runtime.words[0x1004u] == 5u,
          "six-entry queue drops oldest before appending new sequence");

    runtime.allocations = {0u};
    bool threw = false;
    try {
        (void)pal4::CollectionSlotGroup::ProcessGameData(
            0x1000u, 0x59u, runtime);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && runtime.words[0x1004u] == 5u &&
              runtime.appended.back() == 0u &&
              runtime.inserted.back() == 0u,
          "null snapshot preserves pre-queue calls and stops before dereference");
}

void CheckRemoveGameData() {
    Runtime runtime;
    Check(!pal4::CollectionSlotGroup::RemoveGameData(
              0x1000u, 0u, runtime) && runtime.events.empty(),
          "null snapshot is rejected before any memory access");
    runtime.words[0xa000u + 148u] = 7u;
    runtime.words[0xa000u + 400u] = 2u;
    Check(!pal4::CollectionSlotGroup::RemoveGameData(
              0x1000u, 0xa000u, runtime) &&
              runtime.invalid_slots.back() == std::make_pair(7, 0xd4u),
          "invalid slot is rejected before queue mutation");

    runtime.words[0xa000u + 148u] = 2u;
    runtime.words[0x107cu] = 0x2000u;
    runtime.words[0x1080u] = 0x200cu;
    runtime.words[0x2000u] = 1u;
    runtime.words[0x2004u] = 2u;
    runtime.words[0x2008u] = 3u;
    runtime.words[0x102cu] = 0x7000u;
    runtime.words[0x1030u] = 0x700cu;
    runtime.words[0x7000u] = 0xb000u;
    runtime.words[0x7004u] = 0xb100u;
    runtime.words[0x7008u] = 0xb200u;
    runtime.words[0xb000u + 400u] = 1u;
    runtime.words[0xb100u + 400u] = 2u;
    runtime.words[0xb200u + 400u] = 3u;
    Check(pal4::CollectionSlotGroup::RemoveGameData(
              0x1000u, 0xa000u, runtime) &&
              runtime.words[0x1080u] == 0x2008u &&
              runtime.words[0x2000u] == 1u &&
              runtime.words[0x2004u] == 3u &&
              runtime.words[0x1030u] == 0x7008u &&
              runtime.words[0x7004u] == 0xb200u &&
              runtime.events ==
                  std::vector<std::pair<char, std::uint32_t>>{
                      {'D', 0xb100u}, {'F', 0xb100u}},
          "matching sequence removes queue entry and destroys slot snapshot");

    runtime.events.clear();
    runtime.words[0xa000u + 400u] = 4u;
    runtime.words[0x2004u] = 4u;
    Check(!pal4::CollectionSlotGroup::RemoveGameData(
              0x1000u, 0xa000u, runtime) &&
              runtime.words[0x1080u] == 0x2004u &&
              runtime.words[0x1030u] == 0x7008u &&
              runtime.events.empty(),
          "queue can change even when binary search misses the slot");

    runtime.words[0xa000u + 400u] = 0xffffffffu;
    runtime.words[0x7000u] = 0xb300u;
    runtime.words[0xb300u + 400u] = 0xffffffffu;
    runtime.words[0x7004u] = 0xb200u;
    Check(pal4::CollectionSlotGroup::RemoveGameData(
              0x1000u, 0xa000u, runtime) &&
              runtime.events ==
                  std::vector<std::pair<char, std::uint32_t>>{
                      {'D', 0xb300u}, {'F', 0xb300u}} &&
              runtime.words[0x7000u] == 0xb200u,
          "binary search compares signed sequence numbers");
}

void CheckQueries() {
    Runtime runtime;
    (void)pal4::CollectionSlotGroup::Construct(0x1000u, 0u, 0u, runtime);
    Check(pal4::CollectionSlotGroup::SlotAddress(0x1000u, 0,
                                                runtime) == 0x1008u &&
              pal4::CollectionSlotGroup::SlotAddress(0x1000u, 6,
                                                     runtime) == 0x1068u,
          "slot addresses use sixteen-byte stride");
    Check(pal4::CollectionSlotGroup::SlotAddress(0x1000u, -1,
                                                runtime) == 0u &&
              pal4::CollectionSlotGroup::SlotAddress(0x1000u, 7,
                                                     runtime) == 0u &&
              runtime.invalid_slots ==
                  std::vector<std::pair<std::int32_t, std::uint32_t>>{
                      {-1, 0xcdu}, {7, 0xd4u}},
          "negative and high index log their distinct source lines");

    runtime.words[0x100cu] = 0x2000u;
    runtime.words[0x1010u] = 0x200cu;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x2004u] = 0x3100u;
    runtime.words[0x2008u] = 0x3200u;
    runtime.words[0x3000u + 400u] = 0xffffffffu;
    runtime.words[0x3100u + 400u] = 2u;
    runtime.words[0x3200u + 400u] = 8u;
    runtime.words[0x101cu] = 0x2100u;
    runtime.words[0x1020u] = 0x2104u;
    runtime.words[0x2100u] = 0x3300u;
    runtime.words[0x3300u + 400u] = 9u;

    Check(pal4::CollectionSlotGroup::SearchSlot(0x1008u, -1,
                                                runtime) == 0x3000u &&
              pal4::CollectionSlotGroup::SearchSlot(0x1008u, 2,
                                                    runtime) == 0x3100u &&
              pal4::CollectionSlotGroup::SearchSlot(0x1008u, 8,
                                                    runtime) == 0x3200u &&
              pal4::CollectionSlotGroup::SearchSlot(0x1008u, 3,
                                                    runtime) == 0u &&
              pal4::CollectionSlotGroup::SearchSlot(0x1008u, 10,
                                                    runtime) == 0u,
          "slot binary search uses signed sequence and exact equality");
    Check(pal4::CollectionSlotGroup::CountInSlot(0x1000u, 0,
                                                 runtime) == 3u &&
              pal4::CollectionSlotGroup::CountInSlot(0x1000u, 2,
                                                     runtime) == 0u &&
              pal4::CollectionSlotGroup::GetEntry(0x1000u, 0, 0u,
                                                  runtime) == 0x3000u &&
              pal4::CollectionSlotGroup::GetEntry(0x1000u, 0, 2u,
                                                  runtime) == 0x3200u &&
              pal4::CollectionSlotGroup::GetEntry(0x1000u, 0, 3u,
                                                  runtime) == 0u &&
              pal4::CollectionSlotGroup::GetEntry(0x1000u, 2, 0u,
                                                  runtime) == 0u,
          "count and offset access respect empty and end boundaries");
    Check(pal4::CollectionSlotGroup::FindSequence(0x1000u, 9, 1,
                                                  runtime) == 0x3300u &&
              pal4::CollectionSlotGroup::FindSequence(0x1000u, 9, 0,
                                                      runtime) == 0u &&
              pal4::CollectionSlotGroup::FindSequence(0x1000u, 9, -1,
                                                      runtime) == 0x3300u &&
              pal4::CollectionSlotGroup::FindSequence(0x1000u, 9, 7,
                                                      runtime) == 0x3300u &&
              pal4::CollectionSlotGroup::FindSequence(0x1000u, 99, 7,
                                                      runtime) == 0u &&
              runtime.invalid_slots.size() == 2u,
          "invalid find index scans all slots without logging");
    Check(pal4::CollectionSlotGroup::CountInSlot(0x1000u, -1,
                                                 runtime) == 0u &&
              pal4::CollectionSlotGroup::GetEntry(0x1000u, 7, 0u,
                                                  runtime) == 0u &&
              runtime.invalid_slots.size() == 4u,
          "direct indexed queries report invalid slot indices");
}

void CheckLowerBoundSequence() {
    Runtime runtime;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x2004u] = 0x3100u;
    runtime.words[0x2008u] = 0x3200u;
    runtime.words[0x200cu] = 0x3300u;
    runtime.words[0x3000u + 400u] = 0xfffffffeu;
    runtime.words[0x3100u + 400u] = 1u;
    runtime.words[0x3200u + 400u] = 1u;
    runtime.words[0x3300u + 400u] = 7u;
    using Group = pal4::CollectionSlotGroup;
    Check(Group::LowerBoundSequence(0x2000u, 0x2010u, -3, runtime) ==
              0x2000u &&
              Group::LowerBoundSequence(0x2000u, 0x2010u, -2, runtime) ==
                  0x2000u &&
              Group::LowerBoundSequence(0x2000u, 0x2010u, 1, runtime) ==
                  0x2004u &&
              Group::LowerBoundSequence(0x2000u, 0x2010u, 2, runtime) ==
                  0x200cu &&
              Group::LowerBoundSequence(0x2000u, 0x2010u, 8, runtime) ==
                  0x2010u,
          "lower bound uses signed keys and returns first duplicate");
    Check(Group::LowerBoundSequence(0x2000u, 0x2000u, 1, runtime) ==
              0x2000u &&
              Group::LowerBoundSequence(0x2010u, 0x2000u, 1, runtime) ==
                  0x2010u,
          "empty and reversed spans return begin without reading entries");
}

}  // namespace

int main() {
    try {
        CheckConstruction();
        CheckClearAndDestroy();
        CheckProcessGameData();
        CheckRemoveGameData();
        CheckQueries();
        CheckLowerBoundSequence();
        std::cout << "collection_slot_group_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
