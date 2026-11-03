#include "pal4/actor_manager_string_method_property.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
constexpr std::uint32_t object = 0x1000u;

struct Runtime final : pal4::IActorManagerStringMethodRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        trace.emplace_back("byte:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        trace.emplace_back("word:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    void DeleteObject(std::uint32_t address,
                      std::uint32_t flags) override {
        Check(flags == 1u, "owned object deleting flag");
        trace.emplace_back("delete:" + std::to_string(address));
    }
    void Free(std::uint32_t address) override {
        trace.emplace_back("free:" + std::to_string(address));
    }
};

void OwnedAndAllocatedText() {
    Runtime runtime;
    runtime.bytes[object + 4u] = 1u;
    runtime.words[object + 8u] = 0x2000u;
    runtime.bytes[object + 12u] = 1u;
    runtime.words[object + 16u] = 0x3000u;
    runtime.words[object + 24u] = 0x4001u;
    runtime.words[object + 28u] = 12u;
    runtime.words[object + 32u] = 16u;
    runtime.bytes[0x4000u] = 0u;
    pal4::ActorManagerStringMethodProperty::Destroy(object, runtime);
    Check(runtime.trace == std::vector<std::string>{
              "word:4096:8671076", "delete:8192", "byte:4100:0",
              "word:4104:0", "delete:12288", "byte:4108:0",
              "word:4112:0", "free:16384", "word:4120:0",
              "word:4124:0", "word:4128:0", "word:4104:0",
              "word:4112:0", "word:4096:8661972"},
          "destroy owned slots, release text, then restore base vtable");
    Check(runtime.words[object] ==
              pal4::ActorManagerStringMethodProperty::kBaseVtable &&
              runtime.words[object + 8u] == 0u &&
              runtime.words[object + 16u] == 0u &&
              runtime.words[object + 24u] == 0u &&
              runtime.words[object + 28u] == 0u &&
              runtime.words[object + 32u] == 0u,
          "all pointer and text fields end cleared");
}

void SharedTextAndBorrowedSlots() {
    Runtime runtime;
    runtime.bytes[object + 4u] = 0u;
    runtime.words[object + 8u] = 0x2000u;
    runtime.bytes[object + 12u] = 1u;
    runtime.words[object + 16u] = 0u;
    runtime.words[object + 24u] = 0x4001u;
    runtime.bytes[0x4000u] = 3u;
    pal4::ActorManagerStringMethodProperty::Destroy(object, runtime);
    Check(runtime.bytes[0x4000u] == 2u &&
              runtime.bytes[object + 12u] == 1u &&
              runtime.words[object + 16u] == 0u,
          "shared text decrements and null owned slot retains flag");
    for (const auto& call : runtime.trace)
        Check(call.find("delete:") != 0u && call.find("free:") != 0u,
              "borrowed and null owned slots never delete");
}

void EmptyTextAndSentinel() {
    Runtime empty;
    pal4::ActorManagerStringMethodProperty::Destroy(object, empty);
    Check(empty.trace.front() == "word:4096:8671076" &&
              empty.trace.back() == "word:4096:8661972",
          "empty object still changes both vtables");

    Runtime sentinel;
    sentinel.words[object + 24u] = 0x4001u;
    sentinel.bytes[0x4000u] = 0xffu;
    pal4::ActorManagerStringMethodProperty::Destroy(object, sentinel);
    Check(std::find(sentinel.trace.begin(), sentinel.trace.end(),
                    "free:16384") != sentinel.trace.end(),
          "text sentinel reference byte frees allocation");
}

void OwnedPairCleanup() {
    Runtime runtime;
    runtime.bytes[object] = 1u;
    runtime.words[object + 4u] = 0x2000u;
    runtime.bytes[object + 8u] = 1u;
    runtime.words[object + 12u] = 0x3000u;
    pal4::ActorManagerStringMethodProperty::CleanupOwnedPair(object, runtime);
    Check(runtime.trace == std::vector<std::string>{
              "delete:8192", "byte:4096:0", "word:4100:0",
              "delete:12288", "byte:4104:0", "word:4108:0"},
          "pair cleanup releases first then second exactly once");
}

void OwnedSlotRelease() {
    Runtime runtime;
    runtime.words[object + 4u] = 0x2000u;
    pal4::ActorManagerStringMethodProperty::ReleaseOwnedSlot(object,
                                                               runtime);
    Check(runtime.trace.empty(), "borrowed slot does not release object");
    runtime.bytes[object] = 1u;
    runtime.words[object + 4u] = 0u;
    pal4::ActorManagerStringMethodProperty::ReleaseOwnedSlot(object,
                                                               runtime);
    Check(runtime.trace.empty(), "null owned slot does not release object");
    runtime.words[object + 4u] = 0x2000u;
    pal4::ActorManagerStringMethodProperty::ReleaseOwnedSlot(object,
                                                               runtime);
    Check(runtime.trace == std::vector<std::string>{"delete:8192"} &&
              runtime.bytes[object] == 1u &&
              runtime.words[object + 4u] == 0x2000u,
          "owned-slot thunk invokes virtual slot four without clearing");
}

}  // namespace

int main() {
    try {
        OwnedAndAllocatedText();
        SharedTextAndBorrowedSlots();
        EmptyTextAndSentinel();
        OwnedPairCleanup();
        OwnedSlotRelease();
        std::cout << "actor_manager_string_method_property_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
