#include "pal4/effect_system_object_f_collection.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemObjectFCollectionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> generated;
    std::vector<std::string> events;
    std::uint32_t next_memory = 0x7000u;
    std::uint32_t calls = 0u;

    std::uint32_t HandleEvent(pal4::EffectSystemObjectFState&,
                              std::uint32_t offset, std::uint32_t context,
                              std::uint32_t event) override {
        Check(offset == 56u && context == 0x1111u && event == 0x2222u,
              "event arguments use owner XML and context fields");
        events.push_back("event");
        return calls < generated.size() ? generated[calls++] : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("alloc:" + std::to_string(bytes));
        const auto address = next_memory;
        next_memory += 0x100u;
        return address;
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
    void Release(std::uint32_t object) override {
        events.push_back("release:" + std::to_string(object));
    }
};

void TestGatesAndClear() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[3u] = 0x1111u;
    state.words[4u] = 0x2222u;
    Check(!pal4::EffectSystemObjectFCollection::Update(state, 2u, runtime) &&
              pal4::EffectSystemObjectFCollection::Clear(state, runtime) == 0u &&
              pal4::EffectSystemObjectFCollection::Get(state, 0u, runtime) == 0u &&
              runtime.events.empty(),
          "inactive collection skips all work");
    state.words[1u] = 1u;
    Check(pal4::EffectSystemObjectFCollection::Update(state, 0u, runtime) &&
              runtime.events.empty(),
          "zero request succeeds without creating an element");
    state.words[39u] = 0x6010u;
    state.words[40u] = 0x6018u;
    state.words[41u] = 0x6020u;
    runtime.words[0x6010u] = 0x111u;
    runtime.words[0x6014u] = 0u;
    Check(pal4::EffectSystemObjectFCollection::Update(state, 4u, runtime) &&
              runtime.events.empty() &&
              pal4::EffectSystemObjectFCollection::Get(state, 0u, runtime) == 0x111u &&
              pal4::EffectSystemObjectFCollection::Get(state, 1u, runtime) == 0u &&
              pal4::EffectSystemObjectFCollection::Get(state, 2u, runtime) == 0u,
          "existing entries satisfy request and indexed lookup is bounded");
    runtime.events.clear();
    Check(pal4::EffectSystemObjectFCollection::Clear(state, runtime) == 0x18u &&
              state.words[39u] == 0x6010u && state.words[40u] == 0x6010u &&
              state.words[41u] == 0x6020u &&
              runtime.words[0x6010u] == 0u &&
              runtime.events == std::vector<std::string>{
                  "read:24592", "release:273", "write:24592", "read:24596"},
          "clear releases nonzero entries and retains allocated capacity");
}

void TestGenerationAndFailureCleanup() {
    Runtime runtime;
    runtime.generated = {0x101u, 0x202u};
    pal4::EffectSystemObjectFState state{};
    state.words[1u] = 1u;
    state.words[3u] = 0x1111u;
    state.words[4u] = 0x2222u;
    Check(pal4::EffectSystemObjectFCollection::Update(state, 2u, runtime) &&
              state.words[39u] == 0x7100u &&
              state.words[40u] == 0x7108u &&
              state.words[41u] == 0x7108u &&
              runtime.words[0x7100u] == 0x101u &&
              runtime.words[0x7104u] == 0x202u &&
              runtime.events == std::vector<std::string>{
                  "event", "alloc:4", "write:28672", "free:0",
                  "event", "alloc:8", "read:28672", "write:28928",
                  "write:28932", "free:28672"},
          "generation doubles full capacity and copies existing pointer");
    runtime.events.clear();
    Check(pal4::EffectSystemObjectFCollection::Get(state, 1u, runtime) == 0x202u &&
              pal4::EffectSystemObjectFCollection::Get(state, 2u, runtime) == 0u,
          "generated entries are indexed by native four-byte slots");
    state.words[40u] = state.words[39u];
    runtime.events.clear();
    Check(!pal4::EffectSystemObjectFCollection::Update(state, 1u, runtime) &&
              state.words[40u] == state.words[39u] &&
              runtime.events == std::vector<std::string>{"event"},
          "failed generation clears an empty vector");
}

void TestAvailableCapacityAndPartialFailure() {
    Runtime runtime;
    runtime.generated = {0x303u};
    pal4::EffectSystemObjectFState state{};
    state.words[1u] = 1u;
    state.words[3u] = 0x1111u;
    state.words[4u] = 0x2222u;
    state.words[39u] = 0x6000u;
    state.words[40u] = 0x6000u;
    state.words[41u] = 0x6008u;
    Check(!pal4::EffectSystemObjectFCollection::Update(state, 2u, runtime) &&
              state.words[39u] == 0x6000u &&
              state.words[40u] == 0x6000u &&
              state.words[41u] == 0x6008u &&
              runtime.words.at(0x6000u) == 0u &&
              runtime.events == std::vector<std::string>{
                  "event", "write:24576", "event", "read:24576",
                  "release:771", "write:24576"},
          "partial generation reuses capacity then releases new element");
}

}  // namespace

int main() {
    TestGatesAndClear();
    TestGenerationAndFailureCleanup();
    TestAvailableCapacityAndPartialFailure();
}
