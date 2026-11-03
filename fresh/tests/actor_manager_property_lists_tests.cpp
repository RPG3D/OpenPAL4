#include "pal4/actor_manager_property_lists.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IActorManagerPropertyListsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::uint32_t allocations = 0u;
    std::uint32_t fail_at = 0u;

    std::uint32_t Allocate(std::uint32_t size) override {
        trace.emplace_back("alloc:" + std::to_string(size));
        ++allocations;
        if (allocations == fail_at) return 0u;
        return allocations == 1u ? 0x4000u : 0x5000u;
    }
    void Free(std::uint32_t address) override {
        trace.emplace_back("free:" + std::to_string(address));
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
};

void InitializedLayout() {
    Runtime runtime;
    constexpr std::uint32_t object = 0x1000u;
    Check(pal4::ActorManagerPropertyLists::Initialize(object, runtime) ==
              object, "initializer returns object");
    Check(runtime.bytes[object] == 0u &&
              runtime.bytes[object + 16u] == 0u,
          "indeterminate legacy flags use deterministic zero");
    Check(runtime.words[object + 4u] == 0x5000u &&
              runtime.words[object + 8u] == 0x5000u &&
              runtime.words[object + 12u] == 0x5020u,
          "primary list reserves four 8-byte slots");
    Check(runtime.words[object + 20u] == 0x4000u &&
              runtime.words[object + 24u] == 0x4000u &&
              runtime.words[object + 28u] == 0x4060u,
          "secondary list reserves twelve 8-byte slots");
    Check(runtime.trace == std::vector<std::string>{
              "alloc:96", "free:0", "alloc:32", "free:0"},
          "allocation and empty-buffer cleanup order");
}

void AllocationFailures() {
    constexpr std::uint32_t object = 0x1000u;
    for (const auto fail_at : {1u, 2u}) {
        Runtime runtime;
        runtime.fail_at = fail_at;
        bool caught = false;
        try {
            (void)pal4::ActorManagerPropertyLists::Initialize(
                object, runtime);
        } catch (const std::bad_alloc&) {
            caught = true;
        }
        Check(caught && runtime.words[object + 4u] == 0u &&
                  runtime.words[object + 20u] == 0u &&
                  runtime.words[object + 28u] == 0u,
              "failed allocation leaves both lists empty");
        if (fail_at == 2u)
            Check(runtime.trace.back() == "free:16384",
                  "second failure releases first allocation");
    }
}

}  // namespace

int main() {
    try {
        InitializedLayout();
        AllocationFailures();
        std::cout << "actor_manager_property_lists_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
