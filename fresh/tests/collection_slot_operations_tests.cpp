#include "pal4/collection_slot_operations.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICollectionSlotRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t next_allocation{0x8000u};

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate:" + std::to_string(size));
        return next_allocation;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    void BeforeFreePairs(std::uint32_t begin, std::uint32_t end) override {
        calls.push_back("before:" + std::to_string(begin) + ":" +
                        std::to_string(end));
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
};

using Operations = pal4::CollectionSlotOperations;
using Slot = pal4::CollectionSlotState;

void TestPairCopy() {
    Runtime runtime;
    Slot slot{0x55u, 0x1000u, 0x1018u, 0x1020u};
    for (std::uint32_t index = 0u; index < 6u; ++index)
        runtime.words[0x1000u + index * 4u] = index + 1u;
    Check(Operations::CopyPairs(slot, 0x1000u, 0x1008u, runtime) == 0x1000u &&
              slot.end == 0x1010u && slot.reserved == 0x55u &&
              runtime.words[0x1000u] == 3u &&
              runtime.words[0x1004u] == 4u &&
              runtime.words[0x1008u] == 5u &&
              runtime.words[0x100cu] == 6u,
          "pair copy moves a suffix forward and updates end");
    runtime.calls.clear();
    Check(Operations::CopyPairs(slot, slot.begin, slot.end, runtime) ==
              slot.begin && slot.end == slot.begin && runtime.calls.empty(),
          "copy with source at end clears the collection without memory access");
}

void TestReserveWords() {
    Runtime runtime;
    Slot slot{0x77u, 0x1000u, 0x1008u, 0x1008u};
    runtime.words[0x1000u] = 11u;
    runtime.words[0x1004u] = 22u;
    Check(Operations::ReserveWords(slot, 4u, runtime) == 0x8008u &&
              slot.reserved == 0x77u && slot.begin == 0x8000u &&
              slot.end == 0x8008u && slot.capacity_end == 0x8010u &&
              runtime.words[0x8000u] == 11u &&
              runtime.words[0x8004u] == 22u &&
              runtime.calls.front() == "allocate:16" &&
              runtime.calls.back() == "free:4096",
          "word reserve copies old contents and sets exact requested capacity");
    runtime.calls.clear();
    Check(Operations::ReserveWords(slot, 3u, runtime) == 4u &&
              runtime.calls.empty(),
          "reserve within capacity returns current capacity without writes");
}

void TestReservePairs() {
    Runtime runtime;
    Slot slot{0u, 0x2000u, 0x2010u, 0x2010u};
    for (std::uint32_t index = 0u; index < 4u; ++index)
        runtime.words[0x2000u + index * 4u] = index + 1u;
    Check(Operations::ReservePairs(slot, 3u, runtime) == 0x8010u &&
              slot.begin == 0x8000u && slot.end == 0x8010u &&
              slot.capacity_end == 0x8018u &&
              runtime.words[0x8000u] == 1u &&
              runtime.words[0x800cu] == 4u &&
              runtime.calls.front() == "allocate:24" &&
              runtime.calls.back() == "free:8192",
          "pair reserve copies two DWORDs per element");
}

void TestResizeWords() {
    Runtime runtime;
    Slot slot{0u, 0x1000u, 0x1008u, 0x1010u};
    Check(Operations::ResizeWords(slot, 4u, 9u, runtime) == 8u &&
              slot.end == 0x1010u &&
              runtime.words[0x1008u] == 9u &&
              runtime.words[0x100cu] == 9u &&
              runtime.calls.size() == 2u,
          "word resize uses spare capacity and returns inserted byte count");
    Check(Operations::ResizeWords(slot, 4u, 0u, runtime) == 4u &&
              Operations::ResizeWords(slot, 1u, 0u, runtime) == 0x1010u &&
              slot.end == 0x1004u,
          "equal size returns count and shrink returns old end");

    runtime = {};
    slot = {0u, 0x1000u, 0x1008u, 0x1008u};
    runtime.words[0x1000u] = 1u;
    runtime.words[0x1004u] = 2u;
    Check(Operations::ResizeWords(slot, 3u, 7u, runtime) == 3u &&
              slot.begin == 0x8000u && slot.end == 0x800cu &&
              slot.capacity_end == 0x8010u &&
              runtime.words[0x8000u] == 1u &&
              runtime.words[0x8004u] == 2u &&
              runtime.words[0x8008u] == 7u &&
              runtime.calls.front() == "allocate:16" &&
              runtime.calls.back() == "free:4096",
          "word resize reallocates geometrically and fills added element");

    runtime = {};
    slot = {};
    Check(Operations::ResizeWords(slot, 2u, 5u, runtime) == 2u &&
              slot.begin == 0x8000u && slot.end == 0x8008u &&
              slot.capacity_end == 0x8008u &&
              runtime.words[0x8000u] == 5u &&
              runtime.words[0x8004u] == 5u,
          "empty word slot allocates exactly two elements");

    runtime = {};
    runtime.next_allocation = 0u;
    slot = {0u, 0x1000u, 0x1008u, 0x1008u};
    runtime.words[0x1000u] = 1u;
    runtime.words[0x1004u] = 2u;
    Check(Operations::ResizeWords(slot, 3u, 7u, runtime) == 3u &&
              slot.begin == 0u && slot.end == 12u &&
              slot.capacity_end == 16u &&
              runtime.words.count(0u) == 0u &&
              runtime.words[4u] == 2u && runtime.words[8u] == 7u,
          "null allocation skips address zero but advances copy/fill cursors");
}

void TestResizePairs() {
    Runtime runtime;
    Slot slot{0u, 0x2000u, 0x2008u, 0x2018u};
    Check(Operations::ResizePairs(slot, 3u, 5u, 6u, runtime) == 16u &&
              slot.end == 0x2018u &&
              runtime.words[0x2008u] == 5u &&
              runtime.words[0x200cu] == 6u &&
              runtime.words[0x2010u] == 5u &&
              runtime.words[0x2014u] == 6u,
          "pair resize fills both words in spare capacity");
    Check(Operations::ResizePairs(slot, 1u, 0u, 0u, runtime) == 0x2018u &&
              slot.end == 0x2008u,
          "pair shrink returns old end");

    runtime = {};
    slot = {0u, 0x2000u, 0x2010u, 0x2010u};
    for (std::uint32_t index = 0u; index < 4u; ++index)
        runtime.words[0x2000u + index * 4u] = index + 1u;
    Check(Operations::ResizePairs(slot, 3u, 9u, 10u, runtime) == 3u &&
              slot.begin == 0x8000u && slot.end == 0x8018u &&
              slot.capacity_end == 0x8020u &&
              runtime.words[0x8000u] == 1u &&
              runtime.words[0x800cu] == 4u &&
              runtime.words[0x8010u] == 9u &&
              runtime.words[0x8014u] == 10u &&
              runtime.calls[runtime.calls.size() - 2u] ==
                  "before:8192:8208" &&
              runtime.calls.back() == "free:8192",
          "pair reallocation copies, fills and calls range cleanup before free");

    runtime = {};
    runtime.next_allocation = 0u;
    slot = {0u, 0x2000u, 0x2008u, 0x2008u};
    runtime.words[0x2000u] = 1u;
    runtime.words[0x2004u] = 2u;
    Check(Operations::ResizePairs(slot, 2u, 9u, 10u, runtime) == 2u &&
              slot.begin == 0u && slot.end == 16u &&
              runtime.words.count(0u) == 0u &&
              runtime.words.count(4u) == 0u &&
              runtime.words[8u] == 9u && runtime.words[12u] == 10u,
          "null pair allocation skips both words at address zero");
}

void TestRecordCount() {
    Slot slot{};
    Check(pal4::CollectionRecordArray208::Count(slot) == 0u,
          "null record-array begin returns zero");
    slot.begin = 0x1000u;
    slot.end = 0x1000u + 3u * 208u;
    Check(pal4::CollectionRecordArray208::Count(slot) == 3u,
          "record-array length uses a 208-byte stride");
    slot.end = slot.begin - 208u;
    Check(pal4::CollectionRecordArray208::Count(slot) == 0xffffffffu,
          "negative difference is divided as a signed value");
}

}  // namespace

int main() {
    TestPairCopy();
    TestReserveWords();
    TestReservePairs();
    TestResizeWords();
    TestResizePairs();
    TestRecordCount();
    pal4::CollectionSlotRuntimeStub stub;
    Slot slot{};
    Check(Operations::ReserveWords(slot, 0u, stub) == 0u &&
              Operations::ReservePairs(slot, 0u, stub) == 0u,
          "default collection runtime stub links");
    std::cout << "7 collection slot operation cases passed\n";
}
