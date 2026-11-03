#include "pal4/inventory_slot_array.h"

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

struct Runtime final : pal4::IInventorySlotArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::string, std::uint32_t>> calls;
    std::uint32_t fail_at = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write", address);
    }
    void ConstructElement(std::uint32_t element) override {
        calls.emplace_back("construct", element);
        if (element == fail_at)
            throw std::runtime_error("element construction");
    }
    void DestroyElement(std::uint32_t element) override {
        calls.emplace_back("destroy", element);
    }
};

struct QueryRuntime final : pal4::IInventorySlotQueryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> records;
    std::vector<std::pair<std::int32_t, std::uint32_t>> invalid;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::uint32_t next_address = 0xa000u;
    bool fail_allocation = false;
    std::uint32_t db_calls = 0u;
    std::uint32_t find_calls = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        allocations.push_back(bytes);
        if (fail_allocation) return 0u;
        const auto result = next_address;
        next_address += 0x100u;
        return result;
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
    }
    std::uint32_t GameDbInstance() override {
        ++db_calls;
        return 0x9000u;
    }
    std::uint32_t FindGameData(std::uint32_t collection,
                               std::uint32_t id) override {
        Check(collection == 0x9080u, "GameDB lookup collection");
        ++find_calls;
        return records[id];
    }
    void LogInvalidIndex(std::int32_t index,
                         std::uint32_t line) override {
        invalid.emplace_back(index, line);
    }
};

struct StreamRuntime final : pal4::IInventorySlotStreamRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> written;
    std::vector<std::uint32_t> incoming;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::size_t read_index = 0u;
    std::uint32_t next_address = 0xa000u;
    bool fail_allocation = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        allocations.push_back(bytes);
        if (fail_allocation) return 0u;
        const auto result = next_address;
        next_address += 0x100u;
        return result;
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
    }
    void WriteStreamWord(std::uint32_t stream,
                         std::uint32_t value) override {
        Check(stream == 0x4000u, "stream writer address");
        written.push_back(value);
    }
    std::uint32_t ReadStreamWord(std::uint32_t stream) override {
        Check(stream == 0x4000u && read_index < incoming.size(),
              "stream reader bounds");
        return incoming[read_index++];
    }
};

void Prepare(Runtime& runtime) {
    runtime.words[0x5108u] = 0x6000u;
    runtime.words[0x510cu] = 0x6010u;
    runtime.words[0x5118u] = 0x7000u;
    runtime.words[0x511cu] = 0x7020u;
    runtime.words[0x5128u] = 0x8000u;
    runtime.words[0x512cu] = 0x8030u;
}

void CheckConstruction() {
    Runtime runtime;
    Check(pal4::InventorySlotArray::Construct(0x5104u, runtime) ==
              0x5104u &&
              runtime.calls ==
                  std::vector<std::pair<std::string, std::uint32_t>>{
                      {"construct", 0x5104u},
                      {"construct", 0x5114u},
                      {"construct", 0x5124u}},
          "constructor visits three 16-byte elements in order");
    runtime.calls.clear();
    runtime.fail_at = 0x5114u;
    bool threw = false;
    try {
        (void)pal4::InventorySlotArray::Construct(0x5104u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw &&
              runtime.calls ==
                  std::vector<std::pair<std::string, std::uint32_t>>{
                      {"construct", 0x5104u},
                      {"construct", 0x5114u},
                      {"destroy", 0x5104u}},
          "failed constructor unwinds completed elements in reverse");
}

void CheckResetAndCleanup() {
    Runtime runtime;
    Prepare(runtime);
    Check(pal4::InventorySlotArray::ResetEnds(0x5104u, runtime) ==
              0x8030u &&
              runtime.words[0x510cu] == 0x6000u &&
              runtime.words[0x511cu] == 0x7000u &&
              runtime.words[0x512cu] == 0x8000u &&
              runtime.calls ==
                  std::vector<std::pair<std::string, std::uint32_t>>{
                      {"write", 0x510cu},
                      {"write", 0x511cu},
                      {"write", 0x512cu}},
          "reset writes every end from begin and returns third old end");
    runtime.calls.clear();
    Prepare(runtime);
    pal4::InventorySlotArray::Cleanup(0x5104u, runtime);
    Check(runtime.calls ==
              std::vector<std::pair<std::string, std::uint32_t>>{
                  {"write", 0x510cu},
                  {"write", 0x511cu},
                  {"write", 0x512cu},
                  {"destroy", 0x5124u},
                  {"destroy", 0x5114u},
                  {"destroy", 0x5104u}},
          "cleanup resets ends before reverse-order destruction");
}

void CheckIndexAndInsertion() {
    QueryRuntime runtime;
    Check(pal4::InventorySlotArray::SlotAddress(
              0x5104u, 0, runtime) == 0x5104u &&
              pal4::InventorySlotArray::SlotAddress(
                  0x5104u, 2, runtime) == 0x5124u &&
              pal4::InventorySlotArray::SlotAddress(
                  0x5104u, -1, runtime) == 0u &&
              pal4::InventorySlotArray::SlotAddress(
                  0x5104u, 3, runtime) == 0u &&
              runtime.invalid ==
                  std::vector<std::pair<std::int32_t, std::uint32_t>>{
                      {-1, 0x82u}, {3, 0x89u}},
          "slot index uses sixteen-byte stride and distinct error lines");
    runtime.invalid.clear();
    Check(pal4::InventorySlotArray::InsertByIndex(
              0x5104u, 1, 8000u, runtime) == 0xa004u &&
              runtime.words[0x5118u] == 0xa000u &&
              runtime.words[0x511cu] == 0xa004u &&
              runtime.words[0xa000u] == 8000u,
          "indexed insert allocates and writes selected slot");
    Check(pal4::InventorySlotArray::InsertByIndex(
              0x5104u, 3, 8001u, runtime) == 0u &&
              runtime.allocations.size() == 1u &&
              runtime.invalid ==
                  std::vector<std::pair<std::int32_t, std::uint32_t>>{
                      {3, 0x89u}},
          "invalid insert does not call sorted-array insertion");
    runtime.records[8002u] = 0xa000u;
    runtime.words[0xa064u] = 2u;
    Check(pal4::InventorySlotArray::ProcessGameData(
              0x5104u, 8002u, runtime) == 0xa104u &&
              runtime.words[0x5128u] == 0xa100u &&
              runtime.words[0xa100u] == 8002u &&
              runtime.db_calls == 1u && runtime.find_calls == 1u,
          "GameDB record field +100 selects destination slot");
    Check(pal4::InventorySlotArray::ProcessGameData(
              0x5104u, 8003u, runtime) == 0u &&
              runtime.allocations.size() == 2u,
          "missing GameDB record skips indexed insertion");
}

void CheckSortedInsertion() {
    QueryRuntime runtime;
    runtime.words[0x5108u] = 0x6000u;
    runtime.words[0x510cu] = 0x600cu;
    runtime.words[0x5110u] = 0x6014u;
    runtime.words[0x6000u] = 0xfffffffeu;
    runtime.words[0x6004u] = 1u;
    runtime.words[0x6008u] = 9u;
    Check(pal4::InventorySlotArray::InsertSorted(
              0x5104u, 0u, runtime) == 0x6004u &&
              runtime.words[0x510cu] == 0x6010u &&
              runtime.words[0x6000u] == 0xfffffffeu &&
              runtime.words[0x6004u] == 0u &&
              runtime.words[0x6008u] == 1u &&
              runtime.words[0x600cu] == 9u,
          "in-place middle insert shifts entries and returns slot address");
    Check(pal4::InventorySlotArray::InsertSorted(
              0x5104u, 1u, runtime) == 1u &&
              runtime.words[0x510cu] == 0x6010u &&
              runtime.allocations.empty(),
          "duplicate ID returns the input without changing storage");
    Check(pal4::InventorySlotArray::InsertSorted(
              0x5104u, 12u, runtime) == 0x6010u &&
              runtime.words[0x510cu] == 0x6014u &&
              runtime.words[0x6010u] == 12u,
          "in-place append returns previous end address");
    Check(pal4::InventorySlotArray::InsertSorted(
              0x5104u, 0xfffffffdu, runtime) == 0xa018u &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{40u} &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0x6000u} &&
              runtime.words[0x5108u] == 0xa000u &&
              runtime.words[0x510cu] == 0xa018u &&
              runtime.words[0x5110u] == 0xa028u &&
              runtime.words[0xa000u] == 0xfffffffdu &&
              runtime.words[0xa004u] == 0xfffffffeu &&
              runtime.words[0xa008u] == 0u &&
              runtime.words[0xa00cu] == 1u &&
              runtime.words[0xa010u] == 9u &&
              runtime.words[0xa014u] == 12u,
          "full array doubles capacity, preserves order and returns new end");

    QueryRuntime failing;
    failing.fail_allocation = true;
    bool threw = false;
    try {
        (void)pal4::InventorySlotArray::InsertSorted(
            0x5104u, 3u, failing);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && failing.words[0x5108u] == 0u &&
              failing.words[0x510cu] == 0u &&
              failing.freed.empty() &&
              failing.allocations ==
                  std::vector<std::uint32_t>{4u},
          "allocation failure leaves empty slot unchanged");
}

void CheckSearchAndQueries() {
    QueryRuntime runtime;
    runtime.words[0x5108u] = 0x6000u;
    runtime.words[0x510cu] = 0x6010u;
    runtime.words[0x5118u] = 0u;
    runtime.words[0x511cu] = 0u;
    runtime.words[0x6000u] = 0xfffffffeu;
    runtime.words[0x6004u] = 1u;
    runtime.words[0x6008u] = 1u;
    runtime.words[0x600cu] = 9u;
    for (const auto id : {-2, 1, 2, 9})
        runtime.records[static_cast<std::uint32_t>(id)] = 0xa000u;
    runtime.words[0xa064u] = 0u;
    Check(pal4::InventorySlotArray::Contains(
              0x5104u, -2, runtime) &&
              pal4::InventorySlotArray::Contains(
                  0x5104u, 1, runtime) &&
              !pal4::InventorySlotArray::Contains(
                  0x5104u, 2, runtime) &&
              pal4::InventorySlotArray::Contains(
                  0x5104u, 9, runtime) &&
              !pal4::InventorySlotArray::Contains(
                  0x5104u, 10, runtime),
          "exact signed search handles negative, duplicate and gap keys");
    runtime.words[0xa064u] = 3u;
    Check(!pal4::InventorySlotArray::Contains(
              0x5104u, 1, runtime) &&
              runtime.invalid.back() ==
                  std::pair<std::int32_t, std::uint32_t>{3, 0x89u},
          "record with invalid slot index cannot match");
    Check(pal4::InventorySlotArray::Size(
              0x5104u, 0, runtime) == 4 &&
              pal4::InventorySlotArray::Size(
                  0x5104u, 1, runtime) == 0 &&
              pal4::InventorySlotArray::Size(
                  0x5104u, -1, runtime) == 0,
          "size reads signed four-byte count and handles missing slots");
    Check(pal4::InventorySlotArray::ValueAt(
              0x5104u, 0, 3u, runtime) == 9u &&
              pal4::InventorySlotArray::ValueAt(
                  0x5104u, 3, 0u, runtime) == 0u,
          "value access reads valid index and returns zero for bad slot");
    bool threw = false;
    try {
        (void)pal4::InventorySlotArray::ValueAt(
            0x5104u, 0, 4u, runtime);
    } catch (const std::out_of_range& error) {
        threw = std::string(error.what()) ==
                "invalid vector<T> subscript";
    }
    Check(threw,
          "offset at end invokes range exception on valid slot");
    threw = false;
    try {
        (void)pal4::InventorySlotArray::ValueAt(
            0x5104u, 1, 0u, runtime);
    } catch (const std::out_of_range& error) {
        threw = std::string(error.what()) ==
                "invalid vector<T> subscript";
    }
    Check(threw,
          "unallocated valid slot invokes range exception");
}

void CheckStreamSerialization() {
    StreamRuntime runtime;
    runtime.words[0x5108u] = 0x6000u;
    runtime.words[0x510cu] = 0x6008u;
    runtime.words[0x6000u] = 11u;
    runtime.words[0x6004u] = 22u;
    runtime.words[0x5118u] = 0u;
    runtime.words[0x511cu] = 0u;
    runtime.words[0x5128u] = 0x7000u;
    runtime.words[0x512cu] = 0x7004u;
    runtime.words[0x7000u] = 33u;
    Check(pal4::InventorySlotArray::SerializeAll(
              0x5104u, 0x4000u, runtime) == 1u &&
              runtime.written ==
                  std::vector<std::uint32_t>{2u, 11u, 22u,
                                             0u, 1u, 33u},
          "three slot writer emits each count before its values");
    runtime.written.clear();
    Check(pal4::InventorySlotArray::SerializeSlot(
              0x5114u, 0x4000u, runtime) == 1u &&
              runtime.written == std::vector<std::uint32_t>{0u},
          "empty slot writer emits only zero count");
}

void CheckStreamDeserialization() {
    StreamRuntime runtime;
    runtime.incoming = {2u, 11u, 22u, 1u, 33u, 0u};
    runtime.words[0x5108u] = 0x6000u;
    runtime.words[0x510cu] = 0x6008u;
    runtime.words[0x5110u] = 0x6008u;
    runtime.words[0x5128u] = 0x7000u;
    runtime.words[0x512cu] = 0x7004u;
    runtime.words[0x5130u] = 0x7004u;
    Check(pal4::InventorySlotArray::DeserializeAll(
              0x5104u, 0x4000u, runtime) == 1u &&
              runtime.read_index == 6u &&
              runtime.words[0x6000u] == 11u &&
              runtime.words[0x6004u] == 22u &&
              runtime.words[0x510cu] == 0x6008u &&
              runtime.words[0x5118u] == 0xa000u &&
              runtime.words[0x511cu] == 0xa004u &&
              runtime.words[0xa000u] == 33u &&
              runtime.words[0x512cu] == 0x7000u &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{4u} &&
              runtime.freed == std::vector<std::uint32_t>{0u},
          "reader reuses capacity, grows empty slot and clears zero-count slot");

    StreamRuntime growing;
    growing.incoming = {3u, 1u, 2u, 3u};
    Check(pal4::InventorySlotArray::DeserializeSlot(
              0x5104u, 0x4000u, growing) == 1u &&
              growing.words[0x5108u] == 0xa200u &&
              growing.words[0x510cu] == 0xa20cu &&
              growing.words[0x5110u] == 0xa210u &&
              growing.words[0xa200u] == 1u &&
              growing.words[0xa204u] == 2u &&
              growing.words[0xa208u] == 3u &&
              growing.allocations ==
                  std::vector<std::uint32_t>{4u, 8u, 16u} &&
              growing.freed ==
                  std::vector<std::uint32_t>{0u, 0xa000u, 0xa100u},
          "reader grows one, two and four slots while preserving values");

    StreamRuntime negative;
    negative.words[0x5108u] = 0x6000u;
    negative.words[0x510cu] = 0x6004u;
    negative.incoming = {0xffffffffu};
    Check(pal4::InventorySlotArray::DeserializeSlot(
              0x5104u, 0x4000u, negative) == 1u &&
              negative.read_index == 1u &&
              negative.words[0x510cu] == 0x6000u,
          "negative signed count only clears old entries");
    StreamRuntime failing;
    failing.incoming = {1u, 55u};
    failing.fail_allocation = true;
    bool threw = false;
    try {
        (void)pal4::InventorySlotArray::DeserializeSlot(
            0x5104u, 0x4000u, failing);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && failing.read_index == 2u &&
              failing.words[0x510cu] == 0u &&
              failing.allocations ==
                  std::vector<std::uint32_t>{4u},
          "read allocation failure leaves empty slot cleared");
}

}  // namespace

int main() {
    try {
        CheckConstruction();
        CheckResetAndCleanup();
        CheckIndexAndInsertion();
        CheckSortedInsertion();
        CheckSearchAndQueries();
        CheckStreamSerialization();
        CheckStreamDeserialization();
        std::cout << "inventory_slot_array_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
