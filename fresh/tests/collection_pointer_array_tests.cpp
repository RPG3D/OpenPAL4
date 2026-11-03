#include "pal4/collection_pointer_array.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <new>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICollectionPointerArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t next_allocation{0x5000u};
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return next_allocation;
    }
    void FreeMemory(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
};

void TestEmptyAndSpareCapacity() {
    Runtime runtime;
    pal4::DwordCollectionSpanState array{0x1234u, 0u, 0u, 0u};
    Check(pal4::CollectionPointerArray::PushBack(
              array, 0x1111u, runtime) == 0x5004u &&
              array.unknown_00 == 0x1234u &&
              array.begin == 0x5000u && array.end == 0x5004u &&
              array.capacity == 0x5004u &&
              runtime.words.at(0x5000u) == 0x1111u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:4", "write:20480:4369", "free:0"},
          "empty append allocates one slot, frees old null pointer and returns end");

    array.capacity = 0x5010u;
    runtime.calls.clear();
    Check(pal4::CollectionPointerArray::PushBack(
              array, 0x2222u, runtime) == 0x5008u &&
              array.begin == 0x5000u && array.capacity == 0x5010u &&
              runtime.words.at(0x5004u) == 0x2222u &&
              runtime.calls == std::vector<std::string>{
                  "write:20484:8738"},
          "spare capacity writes at end without allocating");
}

void TestGrowth() {
    Runtime runtime;
    runtime.words[0x3000u] = 0xaaaau;
    pal4::DwordCollectionSpanState array{0x1234u, 0x3000u,
                                         0x3004u, 0x3004u};
    Check(pal4::CollectionPointerArray::PushBack(
              array, 0xbbbbu, runtime) == 0x5008u &&
              array.begin == 0x5000u && array.capacity == 0x5008u &&
              runtime.words.at(0x5000u) == 0xaaaau &&
              runtime.words.at(0x5004u) == 0xbbbbu &&
              runtime.calls == std::vector<std::string>{
                  "allocate:8", "read:12288", "write:20480:43690",
                  "write:20484:48059", "free:12288"},
          "one-slot growth doubles capacity and preserves item order");

    runtime.calls.clear();
    runtime.next_allocation = 0x6000u;
    Check(pal4::CollectionPointerArray::PushBack(
              array, 0xccccu, runtime) == 0x600cu &&
              array.begin == 0x6000u && array.capacity == 0x6010u &&
              runtime.words.at(0x6000u) == 0xaaaau &&
              runtime.words.at(0x6004u) == 0xbbbbu &&
              runtime.words.at(0x6008u) == 0xccccu &&
              runtime.calls.front() == "allocate:16" &&
              runtime.calls.back() == "free:20480",
          "two-slot growth doubles capacity to four and appends third item");
}

void TestAllocationFailure() {
    Runtime runtime;
    runtime.next_allocation = 0u;
    pal4::DwordCollectionSpanState array{0x1234u, 0x3000u,
                                         0x3004u, 0x3004u};
    bool caught = false;
    try {
        (void)pal4::CollectionPointerArray::PushBack(
            array, 0x1111u, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && array.begin == 0x3000u &&
              array.end == 0x3004u && array.capacity == 0x3004u &&
              runtime.calls == std::vector<std::string>{"allocate:8"},
          "failed allocation leaves array metadata unchanged");

    pal4::CollectionPointerArrayRuntimeStub stub;
    Check(stub.ReadWord(1u) == 0u && stub.AllocateMemory(4u) == 0u,
          "native allocator boundary has a linkable stub");
}

}  // namespace

int main() {
    TestEmptyAndSpareCapacity();
    TestGrowth();
    TestAllocationFailure();
}
