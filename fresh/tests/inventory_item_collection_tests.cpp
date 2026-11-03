#include "pal4/inventory_item_collection.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IInventoryItemRuntime,
                       pal4::IInventoryItemStreamRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> game_data;
    std::vector<std::pair<std::int32_t, std::uint32_t>> invalid;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> observed;
    std::vector<std::uint32_t> appended;
    std::vector<std::uint32_t> incoming;
    std::vector<std::uint32_t> outgoing;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::size_t stream_read = 0u;
    std::uint32_t next_allocation = 0xb000u;
    bool fail_allocation = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void LogInvalidSlot(std::int32_t index,
                        std::uint32_t source_line) override {
        invalid.emplace_back(index, source_line);
    }
    std::uint32_t FindGameData(std::uint32_t id) override {
        calls.emplace_back("game data");
        return game_data[id];
    }
    void ObserveItem(std::uint32_t id) override {
        calls.emplace_back("observe");
        observed.push_back(id);
    }
    void AppendRecent(std::uint32_t recent,
                      std::uint32_t sequence) override {
        Check(recent == 0x506cu, "recent-array member address");
        calls.emplace_back("append");
        appended.push_back(sequence);
        const auto end = words[recent + 8u];
        words[end] = sequence;
        words[recent + 8u] = end + 4u;
    }
    std::uint32_t TemporaryRecordAddress() override {
        return 0xf000u;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        allocations.push_back(bytes);
        if (fail_allocation) return 0u;
        const auto address = next_allocation;
        next_allocation += 0x100u;
        return address;
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
    }
    std::uint32_t ReadStreamWord(std::uint32_t stream) override {
        Check(stream == 0x4000u && stream_read < incoming.size(),
              "stream read arguments");
        return incoming[stream_read++];
    }
    std::uint32_t WriteStreamWord(std::uint32_t stream,
                                  std::uint32_t value) override {
        Check(stream == 0x4000u, "stream write address");
        outgoing.push_back(value);
        return 0x80u;
    }
};

void SeedSlot(Runtime& runtime, std::uint32_t slot,
              std::uint32_t begin, std::uint32_t end) {
    runtime.words[slot + 4u] = begin;
    runtime.words[slot + 8u] = end;
}

void SeedItem(Runtime& runtime, std::uint32_t address,
              std::uint32_t data, std::uint32_t id,
              std::uint32_t sequence, std::uint32_t count) {
    runtime.words[data] = id;
    runtime.words[address] = data;
    runtime.words[address + 4u] = sequence;
    runtime.words[address + 8u] = count;
}

void CheckSlotHelpers() {
    Runtime runtime;
    Check(pal4::InventoryItemCollection::SlotAddress(
              0x5000u, 0, runtime) == 0x5008u &&
              pal4::InventoryItemCollection::SlotAddress(
                  0x5000u, 4, runtime) == 0x5048u &&
              pal4::InventoryItemCollection::SlotAddress(
                  0x5000u, -1, runtime) == 0u &&
              pal4::InventoryItemCollection::SlotAddress(
                  0x5000u, 5, runtime) == 0u &&
              runtime.invalid ==
                  std::vector<std::pair<std::int32_t, std::uint32_t>>{
                      {-1, 0x103u}, {5, 0x10au}},
          "five slots and distinct invalid-index source lines");
    SeedSlot(runtime, 0x5008u, 0x9000u, 0x9024u);
    SeedItem(runtime, 0x9000u, 0x8000u, 100u, 0x80000000u, 1u);
    SeedItem(runtime, 0x900cu, 0x8010u, 101u, 2u, 2u);
    SeedItem(runtime, 0x9018u, 0x8020u, 102u, 7u, 3u);
    Check(pal4::InventoryItemCollection::FindSequenceInSlot(
              0x5008u, static_cast<std::int32_t>(0x80000000u),
              runtime) == 0x9000u &&
              pal4::InventoryItemCollection::FindSequenceInSlot(
                  0x5008u, 2, runtime) == 0x900cu &&
              pal4::InventoryItemCollection::FindSequenceInSlot(
                  0x5008u, 3, runtime) == 0u &&
              pal4::InventoryItemCollection::FindSequenceInSlot(
                  0x5008u, 7, runtime) == 0x9018u &&
              pal4::InventoryItemCollection::FindIdInSlot(
                  0x5008u, 102u, runtime) == 0x9018u &&
              pal4::InventoryItemCollection::FindIdInSlot(
                  0x5008u, 103u, runtime) == 0u,
          "signed sequence lower bound and linear GameDB-ID lookup");
    Check(pal4::InventoryItemCollection::Count(
              0x5000u, 0, runtime) == 3 &&
              pal4::InventoryItemCollection::Count(
                  0x5000u, 1, runtime) == 0 &&
              pal4::InventoryItemCollection::AtReverse(
                  0x5000u, 0, 0u, runtime) == 0x9018u &&
              pal4::InventoryItemCollection::AtReverse(
                  0x5000u, 0, 2u, runtime) == 0x9000u &&
              pal4::InventoryItemCollection::AtReverse(
                  0x5000u, 0, 3u, runtime) == 0u,
          "slot count and reverse indexed value access");
    Check(pal4::InventoryItemCollection::FindBySequence(
              0x5000u, 7, 0u, runtime) == 0x9018u &&
              pal4::InventoryItemCollection::FindBySequence(
                  0x5000u, 7, 9u, runtime) == 0x9018u &&
              pal4::InventoryItemCollection::FindBySequence(
                  0x5000u, 9, 9u, runtime) == 0u,
          "specific slot and all-five-slot sequence search");
    Check(pal4::InventoryItemCollection::SetRecentKey(
              0x5000u, 0, 2u, runtime) == 2u &&
              runtime.words[0x5058u] == 2u &&
              pal4::InventoryItemCollection::FindRecent(
                  0x5000u, 0u, runtime) == 0x900cu,
          "recent key write and selected-slot lookup");
    runtime.game_data[102u] = 0x8020u;
    runtime.words[0x8024u] = 0u;
    Check(pal4::InventoryItemCollection::FindById(
              0x5000u, 102u, 9u, runtime) == 0x9018u &&
              pal4::InventoryItemCollection::FindById(
                  0x5000u, 999u, 9u, runtime) == 0u,
          "unknown category resolves through GameDB then selected slot");
    Check(!pal4::InventoryItemCollection::EraseSequence(
              0x5008u, 3, runtime) &&
              pal4::InventoryItemCollection::EraseSequence(
                  0x5008u, 2, runtime) &&
              runtime.words[0x5008u + 8u] == 0x9018u &&
              runtime.words[0x900cu] == 0x8020u &&
              runtime.words[0x9010u] == 7u &&
              runtime.words[0x9014u] == 3u,
          "sorted erase shifts a complete 12-byte record");
}

void CheckAddAndRemove() {
    Runtime runtime;
    runtime.game_data[100u] = 0x8000u;
    runtime.words[0x8000u] = 100u;
    runtime.words[0x8004u] = 1u;
    runtime.words[0x5004u] = 10u;
    SeedSlot(runtime, 0x5018u, 0x9000u, 0x9000u);
    runtime.words[0x5024u] = 0x9018u;
    runtime.words[0x5070u] = 0xa000u;
    runtime.words[0x5074u] = 0xa000u;
    Check(pal4::InventoryItemCollection::Add(
              0x5000u, 100u, 150, runtime) == 0x9000u &&
              runtime.words[0x5004u] == 11u &&
              runtime.words[0x9000u] == 0x8000u &&
              runtime.words[0x9004u] == 10u &&
              runtime.words[0x9008u] == 99u &&
              runtime.words[0x5074u] == 0xa004u &&
              runtime.words[0xa000u] == 10u &&
              runtime.calls == std::vector<std::string>{
                  "game data", "observe", "append"},
          "new item clamps to 99 and enters recent list");
    runtime.calls.clear();
    Check(pal4::InventoryItemCollection::Add(
              0x5000u, 100u, 1, runtime) == 0x9000u &&
              runtime.words[0x9008u] == 99u &&
              runtime.words[0x5074u] == 0xa004u &&
              runtime.words[0xa000u] == 10u &&
              runtime.words[0x5020u] == 0x900cu &&
              runtime.calls == std::vector<std::string>{
                  "game data", "observe", "append"},
          "existing item is capped and recent sequence is deduplicated");
    Check(pal4::InventoryItemCollection::Remove(
              0x5000u, 100u, 20, runtime) == 0x9000u &&
              runtime.words[0x9008u] == 79u &&
              runtime.words[0x5074u] == 0xa004u,
          "partial removal updates quantity and keeps recent sequence");
    Check(pal4::InventoryItemCollection::Remove(
              0x5000u, 100u, 79, runtime) == 0u &&
              runtime.words[0x5020u] == 0x9000u &&
              runtime.words[0x5074u] == 0xa000u,
          "complete removal erases slot record and recent sequence");
    Check(pal4::InventoryItemCollection::Add(
              0x5000u, 999u, 1, runtime) == 0u &&
              runtime.observed.size() == 2u,
          "missing GameDB record stops before observation");
    runtime.words[0x8004u] = 5u;
    Check(pal4::InventoryItemCollection::Add(
              0x5000u, 100u, 1, runtime) == 0u &&
              runtime.invalid.back() ==
                  std::pair<std::int32_t, std::uint32_t>{5, 0x10au},
          "invalid category stops before observation");
}

void CheckRecentAndNegativeDelegation() {
    Runtime runtime;
    runtime.game_data[100u] = 0x8000u;
    runtime.words[0x8000u] = 100u;
    runtime.words[0x8004u] = 0u;
    SeedSlot(runtime, 0x5008u, 0x9000u, 0x900cu);
    SeedItem(runtime, 0x9000u, 0x8000u, 100u, 9u, 5u);
    runtime.words[0x5070u] = 0xa000u;
    runtime.words[0x5074u] = 0xa018u;
    for (std::uint32_t i = 0u; i < 6u; ++i)
        runtime.words[0xa000u + i * 4u] = i + 1u;
    Check(pal4::InventoryItemCollection::Add(
              0x5000u, 100u, 1, runtime) == 0x9000u &&
              runtime.words[0xa000u] == 2u &&
              runtime.words[0xa014u] == 9u &&
              runtime.words[0x5074u] == 0xa018u,
          "full recent array discards oldest before appending sequence");
    Check(pal4::InventoryItemCollection::Add(
              0x5000u, 100u, -2, runtime) == 0x9000u &&
              runtime.words[0x9008u] == 4u &&
              pal4::InventoryItemCollection::Remove(
                  0x5000u, 100u, -3, runtime) == 0x9000u &&
              runtime.words[0x9008u] == 7u,
          "negative add/remove counts delegate in the opposite direction");
}

void CheckSlotStreams() {
    Runtime runtime;
    runtime.game_data[100u] = 0x8000u;
    runtime.game_data[101u] = 0x8010u;
    runtime.words[0x8000u] = 100u;
    runtime.words[0x8010u] = 101u;
    runtime.incoming = {3u, 11u, 2u, 100u,
                        22u, 3u, 999u, 33u, 4u, 101u};
    Check(pal4::InventoryItemCollection::DeserializeSlot(
              0x5008u, 0x4000u, runtime) == 1u &&
              runtime.stream_read == 10u &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{12u, 24u} &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0u, 0xb000u} &&
              runtime.words[0x500cu] == 0xb100u &&
              runtime.words[0x5010u] == 0xb118u &&
              runtime.words[0x5014u] == 0xb118u &&
              runtime.words[0xb100u] == 0x8000u &&
              runtime.words[0xb104u] == 11u &&
              runtime.words[0xb108u] == 2u &&
              runtime.words[0xb10cu] == 0x8010u &&
              runtime.words[0xb110u] == 33u &&
              runtime.words[0xb114u] == 4u,
          "reader skips missing GameDB record and grows one then two slots");
    Check(pal4::InventoryItemCollection::SerializeSlot(
              0x5008u, 0x4000u, runtime) == 1u &&
              runtime.outgoing ==
                  std::vector<std::uint32_t>{2u, 11u, 2u, 100u,
                                             33u, 4u, 101u},
          "writer emits count and each record's sequence, quantity and ID");

    Runtime reuse;
    reuse.game_data[100u] = 0x8000u;
    reuse.words[0x500cu] = 0x9000u;
    reuse.words[0x5010u] = 0x9024u;
    reuse.words[0x5014u] = 0x9018u;
    reuse.incoming = {1u, 5u, 6u, 100u};
    Check(pal4::InventoryItemCollection::DeserializeSlot(
              0x5008u, 0x4000u, reuse) == 1u &&
              reuse.allocations.empty() &&
              reuse.words[0x500cu] == 0x9000u &&
              reuse.words[0x5010u] == 0x900cu &&
              reuse.words[0x9000u] == 0x8000u &&
              reuse.words[0x9004u] == 5u &&
              reuse.words[0x9008u] == 6u,
          "reader resets end and reuses existing capacity");

    Runtime missing;
    missing.words[0x500cu] = 0x9000u;
    missing.words[0x5010u] = 0x900cu;
    missing.words[0x5014u] = 0x9018u;
    missing.incoming = {1u, 7u, 8u, 999u};
    Check(pal4::InventoryItemCollection::DeserializeSlot(
              0x5008u, 0x4000u, missing) == 0u &&
              missing.words[0x5010u] == 0x9000u &&
              missing.allocations.empty(),
          "last missing GameDB record returns false without insertion");

    Runtime negative;
    negative.words[0x500cu] = 0x9000u;
    negative.words[0x5010u] = 0x900cu;
    negative.incoming = {0xffffffffu};
    Check(pal4::InventoryItemCollection::DeserializeSlot(
              0x5008u, 0x4000u, negative) == 1u &&
              negative.stream_read == 1u &&
              negative.words[0x5010u] == 0x9000u,
          "negative count consumes only count and returns initial one");

    Runtime failed;
    failed.game_data[100u] = 0x8000u;
    failed.incoming = {1u, 1u, 2u, 100u};
    failed.fail_allocation = true;
    bool threw = false;
    try {
        (void)pal4::InventoryItemCollection::DeserializeSlot(
            0x5008u, 0x4000u, failed);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && failed.allocations ==
              std::vector<std::uint32_t>{12u} &&
              failed.freed.empty() &&
              failed.words[0x5010u] == 0u,
          "allocation failure retains reset slot and old storage");
}

void CheckInsertAt() {
    Runtime spare;
    SeedSlot(spare, 0x5008u, 0x9000u, 0x9018u);
    spare.words[0x5014u] = 0x9030u;
    SeedItem(spare, 0x9000u, 0x8000u, 100u, 1u, 10u);
    SeedItem(spare, 0x900cu, 0x8010u, 101u, 3u, 30u);
    Check(pal4::InventoryItemCollection::InsertAt(
              0x5008u, 0x900cu, 0x8020u, 2u, 20u,
              spare) == 0x900cu &&
              spare.words[0x5010u] == 0x9024u &&
              spare.words[0x900cu] == 0x8020u &&
              spare.words[0x9010u] == 2u &&
              spare.words[0x9014u] == 20u &&
              spare.words[0x9018u] == 0x8010u &&
              spare.words[0x901cu] == 3u &&
              spare.words[0x9020u] == 30u &&
              spare.allocations.empty(),
          "spare capacity shifts a complete record for middle insertion");

    Runtime grow;
    SeedSlot(grow, 0x5008u, 0x9000u, 0x9018u);
    grow.words[0x5014u] = 0x9018u;
    SeedItem(grow, 0x9000u, 0x8000u, 100u, 1u, 10u);
    SeedItem(grow, 0x900cu, 0x8010u, 101u, 3u, 30u);
    Check(pal4::InventoryItemCollection::InsertAt(
              0x5008u, 0x900cu, 0x8020u, 2u, 20u,
              grow) == 0xb00cu &&
              grow.allocations == std::vector<std::uint32_t>{48u} &&
              grow.freed == std::vector<std::uint32_t>{0x9000u} &&
              grow.words[0x500cu] == 0xb000u &&
              grow.words[0x5010u] == 0xb024u &&
              grow.words[0x5014u] == 0xb030u &&
              grow.words[0xb000u] == 0x8000u &&
              grow.words[0xb004u] == 1u &&
              grow.words[0xb00cu] == 0x8020u &&
              grow.words[0xb010u] == 2u &&
              grow.words[0xb018u] == 0x8010u &&
              grow.words[0xb01cu] == 3u,
          "full slot doubles capacity and preserves both sides of insertion");

    Runtime empty;
    Check(pal4::InventoryItemCollection::InsertAt(
              0x5008u, 0u, 0x8000u, 5u, 6u,
              empty) == 0xb000u &&
              empty.allocations == std::vector<std::uint32_t>{12u} &&
              empty.freed == std::vector<std::uint32_t>{0u} &&
              empty.words[0x500cu] == 0xb000u &&
              empty.words[0x5010u] == 0xb00cu &&
              empty.words[0x5014u] == 0xb00cu,
          "empty slot allocates exactly one record");

    Runtime failed;
    SeedSlot(failed, 0x5008u, 0x9000u, 0x900cu);
    failed.words[0x5014u] = 0x900cu;
    failed.fail_allocation = true;
    bool threw = false;
    try {
        (void)pal4::InventoryItemCollection::InsertAt(
            0x5008u, 0x900cu, 0x8000u, 5u, 6u, failed);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && failed.freed.empty() &&
              failed.words[0x500cu] == 0x9000u &&
              failed.words[0x5010u] == 0x900cu &&
              failed.words[0x5014u] == 0x900cu,
          "failed growth preserves previous pointers");
}

void CheckInvalidElementException() {
    bool threw = false;
    try {
        pal4::InventoryItemCollection::ThrowInvalidElement();
    } catch (const std::out_of_range& error) {
        threw = std::string(error.what()) ==
                "invalid vector<T> subscript";
    }
    Check(threw, "item bounds helper throws native vector message");
}

}  // namespace

int main() {
    try {
        CheckSlotHelpers();
        CheckAddAndRemove();
        CheckRecentAndNegativeDelegation();
        CheckSlotStreams();
        CheckInsertAt();
        CheckInvalidElementException();
        std::cout << "inventory_item_collection_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
