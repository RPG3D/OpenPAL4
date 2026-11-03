#include "pal4/sound3d_object.h"

#include <array>
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

constexpr std::array<std::uint32_t, 5> kDerived{
    0x84a970u, 0x84a968u, 0x84a95cu, 0x84a950u, 0x84a944u};
constexpr std::array<std::uint32_t, 5> kBase{
    0x84a1d0u, 0x84a1c8u, 0x84a9d8u, 0x84a1bcu, 0x84a178u};

void CheckFacets(const pal4::Sound3DObjectState& state,
                 const std::array<std::uint32_t, 5>& facets) {
    for (std::uint32_t i = 0u; i < facets.size(); ++i)
        Check(state.words[5u + i] == facets[i],
              "all five facet vtables and order");
}

struct Runtime final : pal4::ISound3DObjectRuntime {
    pal4::Sound3DObjectState* state = nullptr;
    std::vector<std::string> events;
    std::uint32_t count = 7u;
    std::uint8_t refs = 0u;
    bool fail_count = false;
    std::uint8_t playback_result = 0x7eu;
    std::uint32_t SystemInstance() override {
        events.emplace_back("system");
        return 0x5000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x50a4u && state != nullptr &&
                  state->words[0u] == 0x84a980u,
              "EffectSystem count read after derived vtables");
        CheckFacets(*state, kDerived);
        events.emplace_back("count-read");
        if (fail_count) throw std::runtime_error("count");
        return count;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x50a4u, "EffectSystem count write address");
        count = value;
        events.emplace_back("count-write");
    }
    std::uint8_t ConstructorStackByte() override {
        events.emplace_back("stack-byte");
        return 0x5au;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x3fffu, "string reference count address");
        events.emplace_back("ref-read");
        return refs;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x3fffu, "string reference decrement address");
        refs = value;
        events.emplace_back("ref-write");
    }
    void FreeText(std::uint32_t allocation) override {
        Check(allocation == 0x3fffu, "string allocation starts before data");
        events.emplace_back("free-text");
    }
    void FreeObject(pal4::Sound3DObjectState& object) override {
        Check(&object == state && object.words[0u] == 0x84b240u &&
                  object.words[17u] == 0u,
              "object freed after base and string destruction");
        CheckFacets(object, kBase);
        events.emplace_back("free-object");
    }
    std::uint8_t StartPlayback(pal4::Sound3DObjectState& object,
                               std::uint32_t facet_offset) override {
        Check(&object == state && facet_offset == 28u,
              "playback facet offset");
        events.emplace_back("start-playback");
        return playback_result;
    }
};

void TestLifecycle() {
    pal4::Sound3DObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    runtime.state = &state;
    Check(&pal4::Sound3DObject::Construct(state, runtime) == &state &&
              state.words[0u] == 0x84a980u && runtime.count == 8u &&
              state.words[13u] == 0u && state.words[15u] == 0u &&
              state.words[17u] == 0u && state.words[18u] == 0u &&
              state.words[19u] == 0u,
          "constructor layout, fields and instance count");
    CheckFacets(state, kDerived);
    for (std::uint32_t i = 1u; i < 5u; ++i)
        Check(state.words[i] == 0u, "base prefix cleared");
    Check((state.words[16u] & 0xffu) == 0x5au &&
              ((state.words[14u] >> 8u) & 0xffu) == 0u &&
              state.words[20u] == 0xaaaaaaaau,
          "constructor stack byte, flag and unrelated field");
    Check(pal4::Sound3DObject::ClassName() == "Sound3DObject",
          "native class string");
    runtime.events.clear();
    state.words[17u] = 0x4000u;
    runtime.refs = 3u;
    pal4::Sound3DObject::Destroy(state, runtime);
    Check(state.words[0u] == 0x84b240u && runtime.count == 7u &&
              runtime.refs == 2u && state.words[17u] == 0u &&
              state.words[18u] == 0u && state.words[19u] == 0u,
          "destructor decrements string ref and restores base");
    CheckFacets(state, kBase);
    Check(runtime.events == std::vector<std::string>{
              "system", "count-read", "count-write", "ref-read", "ref-write"},
          "destructor count then text cleanup order");
}

void TestDeleteAndTextFree() {
    for (const auto refs : {0u, 255u}) {
        pal4::Sound3DObjectState state{};
        Runtime runtime;
        runtime.state = &state;
        (void)pal4::Sound3DObject::Construct(state, runtime);
        runtime.events.clear();
        runtime.refs = static_cast<std::uint8_t>(refs);
        state.words[17u] = 0x4000u;
        Check(pal4::Sound3DObject::DeletingDestroy(state, 0u, runtime) ==
                  reinterpret_cast<std::uintptr_t>(&state) &&
                  runtime.events == std::vector<std::string>{
                      "system", "count-read", "count-write",
                      "ref-read", "free-text"},
              "zero and sentinel refcounts free text only");
    }
    pal4::Sound3DObjectState state{};
    Runtime runtime;
    runtime.state = &state;
    (void)pal4::Sound3DObject::Construct(state, runtime);
    runtime.events.clear();
    Check(pal4::Sound3DObject::DeletingDestroy(state, 3u, runtime) ==
              reinterpret_cast<std::uintptr_t>(&state) &&
              runtime.events == std::vector<std::string>{
                  "system", "count-read", "count-write", "free-object"},
          "deleting destructor frees object when bit zero is set");
}

void TestConstructorUnwind() {
    pal4::Sound3DObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    runtime.state = &state;
    runtime.fail_count = true;
    bool threw = false;
    try {
        (void)pal4::Sound3DObject::Construct(state, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && state.words[0u] == 0x84b240u &&
              state.words[17u] == 0u && runtime.count == 7u,
          "constructor failure restores base without increment");
    CheckFacets(state, kBase);
}

void TestPropertiesAndPlayback() {
    pal4::Sound3DObjectState state{};
    Runtime runtime;
    runtime.state = &state;
    state.words[13u] = 0x12345678u;
    Check(pal4::Sound3DObject::ResourceProperty(state) == 0x12345678u,
          "resource getter returns word at +52 without activity gate");
    Check(pal4::Sound3DObject::SetEnabled(state, 1u, runtime) == 0u &&
              runtime.events.empty(),
          "inactive object does not change playback");
    state.words[1u] = 1u;
    Check(pal4::Sound3DObject::SetEnabled(state, 1u, runtime) == 0x7eu &&
              ((state.words[2u] >> 24u) & 0xffu) == 1u &&
              runtime.events == std::vector<std::string>{"start-playback"},
          "enabling stores byte and returns facet low byte");
    runtime.events.clear();
    Check(pal4::Sound3DObject::SetEnabled(state, 1u, runtime) == 1u &&
              runtime.events.empty(), "unchanged state skips playback");
    Check(pal4::Sound3DObject::SetEnabled(state, 0u, runtime) == 0u &&
              ((state.words[2u] >> 24u) & 0xffu) == 0u &&
              runtime.events.empty(), "disabling only clears flag");
    state.words[2u] = 0x12345678u;
    Check(pal4::Sound3DObject::SetEnabled(state, 0x80u, runtime) == 0x7eu &&
              state.words[2u] == 0x80345678u,
          "setter preserves neighboring bytes and returns callback byte");
}

}  // namespace

int main() {
    TestLifecycle();
    TestDeleteAndTextFree();
    TestConstructorUnwind();
    TestPropertiesAndPlayback();
}
