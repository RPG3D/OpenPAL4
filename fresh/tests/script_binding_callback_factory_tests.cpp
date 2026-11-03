#include "pal4/script_binding_callback_factory.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptBindingCallbackRuntime {
    std::uint32_t allocation = 0x5000u;
    std::uint32_t destroy_result = 73u;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate " + std::to_string(size));
        return allocation;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    std::uint32_t DestroyAdapter(std::uint32_t address,
                                  std::uint32_t flags) override {
        calls.push_back("destroy " + std::to_string(address) + " " +
                        std::to_string(flags));
        return destroy_result;
    }
};

void Assignment() {
    Runtime runtime;
    pal4::ScriptOwnedAdapterSlot destination{0xabcdef01u, 0x2000u};
    pal4::ScriptOwnedAdapterSlot source{0x12345601u, 0x3000u};
    Check(&pal4::ScriptBindingCallbackFactory::Assign(
              destination, source, runtime) == &destination &&
              destination.flag_word == 0xabcdef01u &&
              destination.pointer == 0x3000u &&
              source.flag_word == 0x12345600u &&
              source.pointer == 0x3000u &&
              runtime.calls == std::vector<std::string>{"destroy 8192 1"},
          "different pointers release old ownership and transfer source flag");

    runtime.calls.clear();
    source = {0x77777701u, 0x3000u};
    Check(&pal4::ScriptBindingCallbackFactory::Assign(
              destination, source, runtime) == &destination &&
              destination.flag_word == 0xabcdef01u &&
              source.flag_word == 0x77777700u &&
              runtime.calls.empty(),
          "equal pointers transfer ownership without destroying the object");

    destination = {0xabcdef00u, 0u};
    source = {0x12345600u, 0u};
    static_cast<void>(pal4::ScriptBindingCallbackFactory::Assign(
        destination, source, runtime));
    Check(destination.flag_word == 0xabcdef00u &&
              source.flag_word == 0x12345600u,
          "equal null pointers preserve destination flag");
    Check(&pal4::ScriptBindingCallbackFactory::Assign(
              destination, destination, runtime) == &destination &&
              runtime.calls.empty(),
          "self-assignment has no side effects");
}

void FactoryEntries() {
    struct Entry {
        std::uint32_t address;
        std::uint32_t vtable;
        std::uint32_t size;
        std::uint32_t slot;
    };
    constexpr std::array<Entry, 15> entries{{
        {0x587470, 0x8482c0, 8, 1}, {0x587530, 0x8482c8, 8, 0},
        {0x5875e0, 0x8482d0, 8, 1}, {0x5876a0, 0x8482d8, 8, 0},
        {0x587750, 0x8482e0, 8, 1}, {0x587810, 0x8482e8, 8, 0},
        {0x5878c0, 0x8482f0, 8, 1}, {0x587980, 0x8482f8, 8, 0},
        {0x587a30, 0x848300, 8, 1}, {0x587af0, 0x848308, 8, 0},
        {0x587ba0, 0x848310, 8, 1}, {0x587c60, 0x848318, 8, 0},
        {0x587d10, 0x848320, 8, 1}, {0x587dd0, 0x848328, 12, 0},
        {0x587e90, 0x848330, 12, 1},
    }};
    for (const auto& entry : entries) {
        Runtime runtime;
        pal4::ScriptEnumAdapterPairState slots{
            {0xaaaaaa00u, 0u, 0xbbbbbb00u, 0u}};
        Check(pal4::ScriptBindingCallbackFactory::CreateAndAssign(
                  entry.address, slots, 0x6000u, 7u, runtime) == 0x5000u &&
                  slots.words[entry.slot * 2u] ==
                      (entry.slot == 0u ? 0xaaaaaa01u : 0xbbbbbb01u) &&
                  slots.words[entry.slot * 2u + 1u] == 0x5000u &&
                  slots.words[(1u - entry.slot) * 2u + 1u] == 0u,
              "every callback factory assigns the intended ownership slot");
        const auto expected = std::vector<std::string>{
            "allocate " + std::to_string(entry.size),
            "write 20480 " + std::to_string(entry.vtable),
            "write 20484 24576"};
        Check(runtime.calls.size() == (entry.size == 12u ? 4u : 3u) &&
                  std::equal(expected.begin(), expected.end(),
                             runtime.calls.begin()) &&
                  (entry.size != 12u ||
                   runtime.calls.back() == "write 20488 7"),
              "each allocation receives the exact vtable, callback and index");
    }

    Runtime runtime;
    runtime.allocation = 0u;
    pal4::ScriptEnumAdapterPairState slots{
        {0xabcdef01u, 0x3000u, 0u, 0u}};
    Check(pal4::ScriptBindingCallbackFactory::CreateAndAssign(
              0x587530u, slots, 0x6000u, 0u, runtime) == 73u &&
              slots.words[0] == 0xabcdef00u && slots.words[1] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "allocate 8", "destroy 12288 1"},
          "allocation failure releases old owned callback and forwards result");

    runtime.calls.clear();
    runtime.allocation = 0x3000u;
    slots.words = {0xabcdef00u, 0x3000u, 0u, 0u};
    Check(pal4::ScriptBindingCallbackFactory::CreateAndAssign(
              0x587530u, slots, 0x6000u, 0u, runtime) == 0x3000u &&
              slots.words[0] == 0xabcdef01u &&
              runtime.calls.size() == 3u,
          "equal callback pointers take ownership without deleting");

    bool rejected = false;
    try {
        static_cast<void>(pal4::ScriptBindingCallbackFactory::CreateAndAssign(
            0x587471u, slots, 0u, 0u, runtime));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    Check(rejected, "factory rejects a non-entry address");
}

void DirectFacets() {
    struct Entry {
        std::uint32_t address;
        std::uint32_t vtable;
        bool indexed;
    };
    constexpr std::array<Entry, 31> entries{{
        {0x587f50, 0x848338, false},
        {0x587f70, 0x848340, false},
        {0x587f90, 0x848348, false},
        {0x587fc0, 0x848350, false},
        {0x587fe0, 0x848358, true},
        {0x588010, 0x848360, true},
        {0x588030, 0x848368, true},
        {0x588050, 0x848370, true},
        {0x588070, 0x848378, true},
        {0x588090, 0x848380, true},
        {0x5880b0, 0x848388, true},
        {0x5880d0, 0x848390, true},
        {0x5880f0, 0x848398, true},
        {0x588110, 0x8483a0, true},
        {0x588130, 0x8483a8, true},
        {0x588150, 0x8483b0, true},
        {0x588190, 0x8483b8, true},
        {0x5881b0, 0x8483c0, true},
        {0x5881d0, 0x8483c8, true},
        {0x5881f0, 0x8483d0, true},
        {0x588210, 0x8483d8, true},
        {0x588230, 0x8483e0, true},
        {0x588250, 0x8483e8, true},
        {0x588270, 0x8483f0, true},
        {0x5882b0, 0x8483f8, false},
        {0x5882d0, 0x848400, false},
        {0x5882f0, 0x848408, false},
        {0x588310, 0x848410, false},
        {0x588330, 0x848418, false},
        {0x588350, 0x848420, false},
        {0x588370, 0x848428, false},
    }};
    for (const auto& entry : entries) {
        Runtime runtime;
        Check(pal4::ScriptBindingCallbackFactory::ConstructFacet(
                  entry.address, 0x7000u, 0x8000u, 9u,
                  runtime) == 0x7000u &&
                  runtime.calls.size() == (entry.indexed ? 3u : 2u) &&
                  runtime.calls[0] ==
                      "write 28672 " + std::to_string(entry.vtable) &&
                  runtime.calls[1] == "write 28676 32768" &&
                  (!entry.indexed ||
                   runtime.calls[2] == "write 28680 9"),
              "direct facet constructors write vtable, callback and optional index");
    }
}

}  // namespace

int main() {
    try {
        Assignment();
        FactoryEntries();
        DirectFacets();
        std::cout << "script binding callback factory tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
