#include "pal4/dynamic_strip_object.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

constexpr std::array<std::uint32_t, 8> kBase{
    0x84a1a0u, 0x84a194u, 0x84a1d0u, 0x84a1c8u,
    0x84a1bcu, 0x84a1dcu, 0x84a1acu, 0x84a178u};
constexpr std::array<std::uint32_t, 8> kDerived{
    0x84a5c0u, 0x84a5b4u, 0x84a5a8u, 0x84a5a0u,
    0x84a594u, 0x84a588u, 0x84a578u, 0x84a56cu};

void CheckFacets(const pal4::DynamicStripObjectState& state,
                 const std::array<std::uint32_t, 8>& expected) {
    for (std::uint32_t i = 0u; i < expected.size(); ++i)
        Check(state.words[5u + i] == expected[i],
              "dynamic strip facet offset and vtable");
}

struct Runtime final : pal4::IDynamicStripObjectRuntime {
    std::vector<std::string> calls;
    std::uint32_t count = 9u;
    bool fail_instance = false;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 36u, "36-byte linked-list sentinel");
        calls.push_back("allocate");
        return 0x3000u;
    }
    std::uint32_t SystemInstance() override {
        calls.push_back("instance");
        if (fail_instance)
            throw std::runtime_error("instance");
        return 0x5000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x50a4u, "instance counter offset +164");
        calls.push_back("read_count");
        return count;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        if (address == 0x50a4u) {
            count = value;
            calls.push_back("write_count");
            return;
        }
        Check((address == 0x3000u || address == 0x3004u) &&
                  value == 0x3000u,
              "sentinel is self-linked in both directions");
        calls.push_back(address == 0x3000u ? "link_next" : "link_previous");
    }
    void CleanupResources(pal4::DynamicStripObjectState& state) override {
        Check(state.words[0u] == 0x84a5d0u ||
                  state.words[0u] == 0x84b240u,
              "cleanup sees valid derived or base owner");
        calls.push_back("cleanup_resources");
    }
    void ReleaseRange(std::uint32_t begin, std::uint32_t end) override {
        Check(begin == 0x4000u && end == 0x4008u,
              "range cleanup receives +176/+180");
        calls.push_back("release_range");
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x4000u, "free array at +176");
        calls.push_back("free_array");
    }
    void CleanupConfig(pal4::DynamicStripObjectState& state) override {
        Check(state.words[41u] == 0x3000u &&
                  state.words[44u] == 0u &&
                  state.words[45u] == 0u &&
                  state.words[46u] == 0u,
              "list cleanup follows array reset");
        calls.push_back("cleanup_config");
    }
    void FreeObject(pal4::DynamicStripObjectState& state) override {
        Check(state.words[0u] == 0x84b240u,
              "storage free follows base restoration");
        calls.push_back("free_object");
    }
};

void TestConstructAndDestroy() {
    pal4::DynamicStripObjectState state{};
    state.words.fill(0xa5a5a5a5u);
    Runtime runtime;
    using pal4::DynamicStripObject;
    Check(&DynamicStripObject::Construct(state, 0x5au, 0x6bu, runtime) ==
              &state && runtime.count == 10u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "link_next", "link_previous",
                  "instance", "read_count", "write_count"} &&
              state.words[0u] == 0x84a5d0u,
          "constructor allocates self-linked sentinel before count increment");
    CheckFacets(state, kDerived);
    for (std::uint32_t i = 1u; i <= 4u; ++i)
        Check(state.words[i] == 0u, "base reset fields");
    Check((state.words[13u] & 0x00ffffffu) == 0x00010100u &&
              (state.words[14u] & 0xffu) == 0u &&
              state.words[27u] == 0u && state.words[28u] == 0u &&
              state.words[29u] == 0u && state.words[38u] == 0u &&
              state.words[39u] == 0u &&
              (state.words[40u] & 0xffu) == 0x5au &&
              state.words[41u] == 0x3000u && state.words[42u] == 0u &&
              (state.words[43u] & 0xffu) == 0x6bu &&
              state.words[44u] == 0u && state.words[45u] == 0u &&
              state.words[46u] == 0u,
          "constructor writes exact fields and explicit stack-derived bytes");
    for (std::uint32_t i = 47u; i < state.words.size(); ++i)
        Check(state.words[i] == 0xa5a5a5a5u,
              "constructor leaves derived configuration cache untouched");
    state.words[28u] = std::bit_cast<std::uint32_t>(3.75f);
    Check(DynamicStripObject::GetLife(state) == 3.75f,
          "life getter reads float at +112 without altering bits");
    state.words[27u] = 0xabcdef01u;
    Check(DynamicStripObject::ResourceHandle(state) == 0u &&
              DynamicStripObject::ClassName() == "DynamicStripObject",
          "inactive resource access and vtable class-name slot");
    state.words[1u] = 0x01000001u;
    Check(DynamicStripObject::ResourceHandle(state) == 0xabcdef01u,
          "active resource access returns object +108");
    state.words[2u] = 0x12345678u;
    Check(DynamicStripObject::SetModeByte(state, 0xabu) == 0xabu &&
              state.words[2u] == 0xab345678u,
          "mode setter writes only owner byte +11 and returns input");
    state.words[1u] = 0u;
    state.words[44u] = 0x4000u;
    state.words[45u] = 0x4008u;
    state.words[46u] = 0x4010u;
    runtime.calls.clear();
    DynamicStripObject::Destroy(state, runtime);
    Check(runtime.count == 9u && state.words[0u] == 0x84b240u &&
              state.words[41u] == 0u && state.words[42u] == 0u &&
              state.words[44u] == 0u && state.words[45u] == 0u &&
              state.words[46u] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "instance", "read_count", "write_count",
                  "cleanup_resources", "release_range", "free_array",
                  "cleanup_config"},
          "destructor decrements count and frees resources in native order");
    CheckFacets(state, kBase);
}

void TestDeletingDestructor() {
    pal4::DynamicStripObjectState state{};
    Runtime runtime;
    static_cast<void>(pal4::DynamicStripObject::Construct(
        state, 0u, 0u, runtime));
    state.words[44u] = 0x4000u;
    state.words[45u] = 0x4008u;
    runtime.calls.clear();
    Check(pal4::DynamicStripObject::DeletingDestroy(
              state, 2u, runtime) ==
              reinterpret_cast<std::uintptr_t>(&state) &&
              runtime.calls.back() == "cleanup_config",
          "deleting thunk retains storage without flag bit zero");
    static_cast<void>(pal4::DynamicStripObject::Construct(
        state, 0u, 0u, runtime));
    state.words[44u] = 0x4000u;
    state.words[45u] = 0x4008u;
    runtime.calls.clear();
    Check(pal4::DynamicStripObject::DeletingDestroy(
              state, 3u, runtime) ==
              reinterpret_cast<std::uintptr_t>(&state) &&
              runtime.calls.back() == "free_object",
          "deleting thunk frees storage after normal destruction");
}

void TestConstructorUnwind() {
    pal4::DynamicStripObjectState state{};
    Runtime runtime;
    runtime.fail_instance = true;
    try {
        static_cast<void>(pal4::DynamicStripObject::Construct(
            state, 7u, 8u, runtime));
        Check(false, "constructor must throw");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "allocate", "link_next", "link_previous",
                  "instance", "cleanup_resources"} &&
                  state.words[0u] == 0x84b240u,
              "constructor unwind cleans resource block and base facets");
        CheckFacets(state, kBase);
    }
}

}  // namespace

int main() {
    TestConstructAndDestroy();
    TestDeletingDestructor();
    TestConstructorUnwind();
}
