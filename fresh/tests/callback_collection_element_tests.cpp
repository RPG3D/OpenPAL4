#include "pal4/callback_collection_element.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class FakeRuntime final : public pal4::ICallbackCollectionRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(const std::uint32_t size) override {
        events.emplace_back("allocate:" + std::to_string(size));
        return allocation_result;
    }

    void Free(const std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
    }

    [[nodiscard]] std::uint32_t ReadDword(
        const std::uint32_t address) override {
        return memory[address];
    }

    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        memory[address] = value;
        events.emplace_back("write:" + std::to_string(address) + ":" +
                            std::to_string(value));
    }

    [[nodiscard]] std::uint32_t InvokeFactory(
        const std::uint32_t table_address) override {
        events.emplace_back("factory:" + std::to_string(table_address));
        if (throw_from_factory) throw std::runtime_error("factory");
        return payload;
    }

    [[nodiscard]] std::uint32_t InvokeInsert(
        const std::uint32_t callback_address, const std::uint32_t ecx,
        const std::uint32_t argument1,
        const std::uint32_t argument2) override {
        events.emplace_back("insert:" + std::to_string(callback_address) +
                            ":" + std::to_string(ecx) + ":" +
                            std::to_string(argument1) + ":" +
                            std::to_string(argument2));
        return ecx_after_insert;
    }

    std::uint32_t allocation_result = 0x6000;
    std::uint32_t payload = 0x7000;
    std::uint32_t ecx_after_insert = 0x5000;
    bool throw_from_factory = false;
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> events;
};

void AllocationFailureSetsErrorAndStops() {
    pal4::CallbackCollectionState collection;
    collection.Write(0x0c, 88);
    collection.Write(0x14, 3);
    FakeRuntime runtime;
    runtime.allocation_result = 0;
    Check(!pal4::CallbackCollectionElementInitializer::AllocateAndInitialize(
              collection, 0x4000, runtime),
          "allocation failure returns false");
    Check(collection.Read(0x0c) == 1 && collection.Read(0x14) == 3,
          "failure sets error and preserves count");
    Check(runtime.events == std::vector<std::string>{"allocate:16"},
          "failure does not run factory or insertion callbacks");
}

void SuccessInitializesLinksCallsHookAndUpdatesCounters() {
    pal4::CallbackCollectionState collection;
    collection.Write(0x08, 0x9000);
    collection.Write(0x14, 4);
    collection.Write(0x18, 7);
    collection.Write(0x20, 0x5000);
    FakeRuntime runtime;
    runtime.memory[0x5004] = 0x5100;
    runtime.memory[0x5100] = 0x5000;

    Check(pal4::CallbackCollectionElementInitializer::AllocateAndInitialize(
              collection, 0x4000, runtime),
          "successful allocation returns true");
    Check(runtime.memory[0x6000] == 0x5000 &&
              runtime.memory[0x6004] == 0x5100 &&
              runtime.memory[0x6008] == 0x7000 &&
              runtime.memory[0x600c] == 0x4000,
          "new record has observed link, payload, and factory-table fields");
    Check(runtime.memory[0x5100] == 0x6000 &&
              runtime.memory[0x5004] == 0x6000,
          "inserts the record at the front of the sentinel chain");
    Check(collection.Read(0x0c) == 0 && collection.Read(0x10) == 0 &&
              collection.Read(0x14) == 5 && collection.Read(0x18) == 8 &&
              collection.Read(0x20) == 0x6000,
          "success clears status, increments counters, and stores new element");
    Check(std::find(runtime.events.begin(), runtime.events.end(),
                    "insert:36864:20480:28672:20480") != runtime.events.end(),
          "insertion callback receives address, ECX, payload, and sentinel");
}

void FactoryExceptionFreesTemporaryAllocation() {
    pal4::CallbackCollectionState collection;
    FakeRuntime runtime;
    runtime.throw_from_factory = true;
    bool threw = false;
    try {
        static_cast<void>(
            pal4::CallbackCollectionElementInitializer::AllocateAndInitialize(
                collection, 0x4000, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw, "factory exception propagates");
    Check(runtime.events == std::vector<std::string>{
                                "allocate:16", "write:24576:0",
                                "write:24580:0", "write:24588:16384",
                                "factory:16384", "free:24576"},
          "SEH cleanup frees the allocation if constructor factory throws");
}

}  // namespace

int main() {
    AllocationFailureSetsErrorAndStops();
    SuccessInitializesLinksCallsHookAndUpdatesCounters();
    FactoryExceptionFreesTemporaryAllocation();
    std::cout << "3 callback-collection element tests passed\n";
}
