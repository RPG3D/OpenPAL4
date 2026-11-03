#include "pal4/strip_collection.h"

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

struct Runtime final : pal4::IStripCollectionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> generated;
    std::vector<std::string> events;
    std::uint32_t next_memory = 0x7000u;
    std::uint32_t calls = 0u;

    std::uint32_t HandleEvent(pal4::StripObjectState&,
                              std::uint32_t document,
                              std::uint32_t first,
                              std::uint32_t second) override {
        Check(document == 0x3333u && first == 0x1111u &&
                  second == 0x2222u,
              "event arguments use owner document and context words");
        events.emplace_back("event");
        return calls < generated.size() ? generated[calls++] : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        events.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.emplace_back("alloc:" + std::to_string(bytes));
        const auto address = next_memory;
        next_memory += 0x100u;
        return address;
    }
    void Free(std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
    }
    void Release(std::uint32_t object) override {
        events.emplace_back("release:" + std::to_string(object));
    }
};

pal4::StripObjectState State() {
    pal4::StripObjectState state{};
    state.words[3u] = 0x1111u;
    state.words[4u] = 0x2222u;
    state.words[39u] = 0x3333u;
    return state;
}

void TestGatesAndClear() {
    Runtime runtime;
    auto state = State();
    Check(!pal4::StripCollection::Update(state, 2u, runtime) &&
              pal4::StripCollection::Clear(state, runtime) == 0u &&
              pal4::StripCollection::Get(state, 0u, runtime) == 0u &&
              runtime.events.empty(),
          "inactive collection skips all work");
    state.words[1u] = 1u;
    Check(pal4::StripCollection::Update(state, 0u, runtime) &&
              runtime.events.empty(),
          "zero request succeeds without creating an element");
    state.words[72u] = 0x6010u;
    state.words[73u] = 0x6018u;
    state.words[74u] = 0x6020u;
    runtime.words[0x6010u] = 0x111u;
    runtime.words[0x6014u] = 0u;
    Check(pal4::StripCollection::Update(state, 4u, runtime) &&
              runtime.events.empty() &&
              pal4::StripCollection::Get(state, 0u, runtime) == 0x111u &&
              pal4::StripCollection::Get(state, 1u, runtime) == 0u &&
              pal4::StripCollection::Get(state, 2u, runtime) == 0u,
          "existing entries satisfy request and lookup is bounded");
    runtime.events.clear();
    Check(pal4::StripCollection::Clear(state, runtime) == 0x18u &&
              state.words[72u] == 0x6010u &&
              state.words[73u] == 0x6010u &&
              state.words[74u] == 0x6020u &&
              runtime.words[0x6010u] == 0u &&
              runtime.events == std::vector<std::string>{
                  "read:24592", "release:273", "write:24592",
                  "read:24596"},
          "clear releases nonzero entries and retains capacity");
}

void TestGrowthAndFailure() {
    Runtime runtime;
    runtime.generated = {0x101u, 0x202u};
    auto state = State();
    state.words[1u] = 1u;
    Check(pal4::StripCollection::Update(state, 2u, runtime) &&
              state.words[72u] == 0x7100u &&
              state.words[73u] == 0x7108u &&
              state.words[74u] == 0x7108u &&
              runtime.words[0x7100u] == 0x101u &&
              runtime.words[0x7104u] == 0x202u &&
              runtime.events == std::vector<std::string>{
                  "event", "alloc:4", "write:28672", "free:0",
                  "event", "alloc:8", "read:28672", "write:28928",
                  "write:28932", "free:28672"},
          "full collection doubles capacity and copies pointers");
    runtime.events.clear();
    Check(pal4::StripCollection::Get(state, 1u, runtime) == 0x202u &&
              pal4::StripCollection::Get(state, 2u, runtime) == 0u,
          "generated entries use four-byte x86 slots");

    state.words[73u] = state.words[72u];
    runtime.events.clear();
    Check(!pal4::StripCollection::Update(state, 1u, runtime) &&
              state.words[73u] == state.words[72u] &&
              runtime.events == std::vector<std::string>{"event"},
          "failed event on empty collection returns false");

    runtime = Runtime{};
    runtime.generated = {0x303u};
    state = State();
    state.words[1u] = 1u;
    state.words[72u] = 0x6000u;
    state.words[73u] = 0x6000u;
    state.words[74u] = 0x6008u;
    Check(!pal4::StripCollection::Update(state, 2u, runtime) &&
              state.words[73u] == 0x6000u &&
              runtime.words.at(0x6000u) == 0u &&
              runtime.events == std::vector<std::string>{
                  "event", "write:24576", "event", "read:24576",
                  "release:771", "write:24576"},
          "partial generation releases newly inserted pointer");
}

}  // namespace

int main() {
    TestGatesAndClear();
    TestGrowthAndFailure();
}
