#include "pal4/clump_object_collection.h"

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

struct Runtime final : pal4::IClumpObjectCollectionRuntime {
    std::vector<std::uint32_t> events;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t next_memory = 0x1000u;

    std::uint32_t HandleEvent(pal4::ClumpObjectState& state,
                              std::uint32_t xml_offset,
                              std::uint32_t context,
                              std::uint32_t event) override {
        Check(xml_offset == 108u && context == 0xaau &&
                  event == 0xbbu && state.words[1u] == 1u,
              "event uses clump XML and owner words");
        calls.push_back("event");
        Check(!events.empty(), "event response available");
        const auto value = events.front();
        events.erase(events.begin());
        return value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate:" + std::to_string(size));
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

pal4::ClumpObjectState ActiveState() {
    pal4::ClumpObjectState state{};
    state.words[1u] = 1u;
    state.words[3u] = 0xaau;
    state.words[4u] = 0xbbu;
    return state;
}

void TestGates() {
    pal4::ClumpObjectState state{};
    state.words[1u] = 0x100u;
    Runtime runtime;
    using pal4::ClumpObjectCollectionFacet;
    Check(!ClumpObjectCollectionFacet::Update(state, 2u, runtime) &&
              ClumpObjectCollectionFacet::Clear(state, runtime) == 0u &&
              ClumpObjectCollectionFacet::Get(state, 0u, runtime) == 0u &&
              runtime.calls.empty(),
          "inactive collection does not access runtime");
    state = ActiveState();
    Check(ClumpObjectCollectionFacet::Update(state, 0u, runtime) &&
              state.words[78u] == 0u && runtime.calls.empty(),
          "zero requested count succeeds without allocation");
    Check(ClumpObjectCollectionFacet::Get(state, 0u, runtime) == 0u,
          "empty collection has no element");
}

void TestGrowthClearAndReuse() {
    auto state = ActiveState();
    Runtime runtime;
    runtime.events = {0x2000u, 0x3000u, 0x4000u};
    using pal4::ClumpObjectCollectionFacet;
    Check(ClumpObjectCollectionFacet::Update(state, 3u, runtime) &&
              state.words[78u] == 0x1200u &&
              state.words[79u] == 0x120cu &&
              state.words[80u] == 0x1210u &&
              runtime.words[0x1200u] == 0x2000u &&
              runtime.words[0x1204u] == 0x3000u &&
              runtime.words[0x1208u] == 0x4000u,
          "array grows through native capacities one, two and four");
    Check(runtime.bytes[0x2000u + 168u] == 1u &&
              runtime.bytes[0x3000u + 168u] == 1u &&
              runtime.bytes[0x4000u + 168u] == 1u,
          "every generated object receives byte +168");
    Check(ClumpObjectCollectionFacet::Get(state, 0u, runtime) == 0x2000u &&
              ClumpObjectCollectionFacet::Get(state, 2u, runtime) == 0x4000u &&
              ClumpObjectCollectionFacet::Get(state, 3u, runtime) == 0u,
          "indexed lookup enforces size boundary");
    runtime.calls.clear();
    Check(ClumpObjectCollectionFacet::Update(state, 20u, runtime) &&
              runtime.calls.empty(),
          "nonempty collection satisfies update without generating more");
    Check(ClumpObjectCollectionFacet::Clear(state, runtime) == 0x0cu &&
              state.words[78u] == 0x1200u &&
              state.words[79u] == 0x1200u &&
              state.words[80u] == 0x1210u &&
              runtime.words[0x1200u] == 0u &&
              runtime.words[0x1204u] == 0u &&
              runtime.words[0x1208u] == 0u,
          "clear releases elements, preserves capacity and returns old end byte");
    Check(runtime.calls == std::vector<std::string>{
              "read:4608", "release:8192", "write:4608", "read:4612",
              "release:12288", "write:4612", "read:4616",
              "release:16384", "write:4616"},
          "clear invokes release slot +68 in element order");
    runtime.calls.clear();
    runtime.events = {0x5000u, 0x6000u};
    Check(ClumpObjectCollectionFacet::Update(state, 2u, runtime) &&
              state.words[79u] == 0x1208u &&
              runtime.words[0x1200u] == 0x5000u &&
              runtime.words[0x1204u] == 0x6000u &&
              runtime.next_memory == 0x1300u,
          "cleared capacity is reused without allocation");
}

void TestEventFailureAndNullEntries() {
    auto state = ActiveState();
    Runtime runtime;
    state.words[78u] = 0x1100u;
    state.words[79u] = 0x1100u;
    state.words[80u] = 0x1108u;
    runtime.events = {0x7000u, 0u};
    using pal4::ClumpObjectCollectionFacet;
    Check(!ClumpObjectCollectionFacet::Update(state, 2u, runtime) &&
              state.words[79u] == 0x1100u &&
              runtime.words[0x1100u] == 0u &&
              runtime.bytes[0x7000u + 168u] == 1u,
          "failed generation releases partially filled collection");
    Check(runtime.calls == std::vector<std::string>{
              "event", "byte:28840", "write:4352", "event", "read:4352",
              "release:28672", "write:4352"},
          "generation failure releases previous element in order");
    runtime.calls.clear();
    state.words[79u] = 0x1108u;
    runtime.words[0x1100u] = 0u;
    runtime.words[0x1104u] = 0x8000u;
    Check(ClumpObjectCollectionFacet::Clear(state, runtime) == 0x08u &&
              runtime.calls == std::vector<std::string>{
                  "read:4352", "read:4356", "release:32768",
                  "write:4356"},
          "null elements are skipped during clear");
}

}  // namespace

int main() {
    TestGates();
    TestGrowthClearAndReuse();
    TestEventFailureAndNullEntries();
}
