#include "pal4/cylinder_object_collection.h"

#include <cstdint>
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

struct Runtime final : pal4::ICylinderObjectCollectionRuntime {
    std::vector<std::uint32_t> events;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t next_memory = 0x1000u;

    std::uint32_t HandleEvent(pal4::CylinderObjectState& state,
                              std::uint32_t xml, std::uint32_t context,
                              std::uint32_t event) override {
        Check(xml == 0xccu && context == 0xaau && event == 0xbbu &&
                  (state.words[1u] & 0xffu) == 1u,
              "event receives XML and owner fields");
        calls.push_back("event");
        Check(!events.empty(), "event response available");
        const auto result = events.front();
        events.erase(events.begin());
        return result;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        const auto result = next_memory;
        next_memory += 0x100u;
        return result;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void Release(std::uint32_t object) override {
        calls.push_back("release:" + std::to_string(object));
    }
};

pal4::CylinderObjectState ActiveState() {
    pal4::CylinderObjectState state{};
    state.words[1u] = 1u;
    state.words[3u] = 0xaau;
    state.words[4u] = 0xbbu;
    state.words[23u] = 0xccu;
    return state;
}

void TestGatesAndGrowth() {
    pal4::CylinderObjectState state{};
    state.words[1u] = 0x100u;
    Runtime runtime;
    using pal4::CylinderObjectCollection;
    Check(!CylinderObjectCollection::Update(state, 2u, runtime) &&
              CylinderObjectCollection::Clear(state, runtime) == 0u &&
              CylinderObjectCollection::Get(state, 0u, runtime) == 0u &&
              runtime.calls.empty(),
          "inactive collection makes no runtime calls");
    state = ActiveState();
    Check(CylinderObjectCollection::Update(state, 0u, runtime) &&
              runtime.calls.empty(),
          "zero request succeeds without allocation");
    runtime.events = {0x2000u, 0x3000u, 0x4000u};
    Check(CylinderObjectCollection::Update(state, 3u, runtime) &&
              state.words[36u] == 0x1200u &&
              state.words[37u] == 0x120cu &&
              state.words[38u] == 0x1210u &&
              runtime.words[0x1200u] == 0x2000u &&
              runtime.words[0x1204u] == 0x3000u &&
              runtime.words[0x1208u] == 0x4000u,
          "collection grows through capacities 1, 2, 4");
    Check(CylinderObjectCollection::Get(state, 0u, runtime) == 0x2000u &&
              CylinderObjectCollection::Get(state, 2u, runtime) == 0x4000u &&
              CylinderObjectCollection::Get(state, 3u, runtime) == 0u,
          "indexed lookup enforces end boundary");
    runtime.calls.clear();
    Check(CylinderObjectCollection::Update(state, 20u, runtime) &&
              runtime.calls.empty(),
          "nonempty collection short circuits generation");
    Check(CylinderObjectCollection::Clear(state, runtime) == 0x0cu &&
              state.words[36u] == 0x1200u &&
              state.words[37u] == 0x1200u &&
              state.words[38u] == 0x1210u,
          "clear retains array capacity and returns old end byte");
    Check(runtime.calls == std::vector<std::string>{
              "read:4608", "release:8192", "write:4608",
              "read:4612", "release:12288", "write:4612",
              "read:4616", "release:16384", "write:4616"},
          "clear releases nonnull elements in order");
    runtime.calls.clear();
    runtime.events = {0x5000u, 0x6000u};
    Check(CylinderObjectCollection::Update(state, 2u, runtime) &&
              state.words[37u] == 0x1208u &&
              runtime.words[0x1200u] == 0x5000u &&
              runtime.words[0x1204u] == 0x6000u &&
              runtime.next_memory == 0x1300u,
          "cleared storage is reused");
}

void TestFailure() {
    auto state = ActiveState();
    Runtime runtime;
    state.words[36u] = 0x1100u;
    state.words[37u] = 0x1100u;
    state.words[38u] = 0x1108u;
    runtime.events = {0x7000u, 0u};
    using pal4::CylinderObjectCollection;
    Check(!CylinderObjectCollection::Update(state, 2u, runtime) &&
              state.words[37u] == 0x1100u &&
              runtime.words[0x1100u] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "event", "write:4352", "event", "read:4352",
                  "release:28672", "write:4352"},
          "failed event releases earlier output and restores empty end");
    runtime.calls.clear();
    state.words[37u] = 0x1108u;
    runtime.words[0x1100u] = 0u;
    runtime.words[0x1104u] = 0x8000u;
    Check(CylinderObjectCollection::Clear(state, runtime) == 0x08u &&
              runtime.calls == std::vector<std::string>{
                  "read:4352", "read:4356", "release:32768",
                  "write:4356"},
          "clear skips null entries");
}

}  // namespace

int main() {
    TestGatesAndGrowth();
    TestFailure();
}
