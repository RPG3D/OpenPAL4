#include "pal4/strip_object.h"

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

constexpr std::array<std::uint32_t, 10> kDerived{
    0x84aa84u, 0x84aa78u, 0x84aa6cu, 0x84aa60u, 0x84aa58u,
    0x84aa4cu, 0x84aa3cu, 0x84aa2cu, 0x84aa14u, 0x84aa08u};
constexpr std::array<std::uint32_t, 10> kBase{
    0x84a1dcu, 0x84a1a0u, 0x84a194u, 0x84a1d0u, 0x84a1c8u,
    0x84a1bcu, 0x84a1acu, 0x84a184u, 0x84a408u, 0x84a178u};

void CheckFacets(const pal4::StripObjectState& state,
                 const std::array<std::uint32_t, 10>& facets) {
    for (std::uint32_t i = 0u; i < facets.size(); ++i)
        Check(state.words[5u + i] == facets[i],
              "all ten strip facet vtables and their order");
}

struct Runtime final : pal4::IStripObjectRuntime {
    pal4::StripObjectState* object = nullptr;
    std::vector<std::string> events;
    std::uint32_t count = 7u;
    bool fail_count = false;

    std::uint32_t SystemInstance() override {
        events.emplace_back("system");
        return 0x5000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x50a4u && object != nullptr &&
                  object->words[0u] == 0x84aa90u,
              "EffectSystem count read after derived vtables");
        CheckFacets(*object, kDerived);
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
    void FreeBuffer(std::uint32_t address) override {
        Check(address == object->words[72u],
              "buffer freed before clearing ownership words");
        events.emplace_back("free-buffer");
    }
    void FreeObject(pal4::StripObjectState& state) override {
        Check(&state == object && state.words[0u] == 0x84b240u &&
                  state.words[72u] == 0u,
              "object deleted after base and buffer destruction");
        CheckFacets(state, kBase);
        events.emplace_back("free-object");
    }
};

struct PlaybackRuntime final : pal4::IStripObjectPlaybackRuntime {
    pal4::StripObjectState* object = nullptr;
    std::vector<std::string> events;
    std::int32_t frame_count = 15;
    double duration = 6.0;

    std::int32_t StartFacet(pal4::StripObjectState& state,
                            std::uint32_t offset) override {
        Check(&state == object && offset == 44u &&
                  ((state.words[2u] >> 24u) & 0xffu) != 0u,
              "playback facet starts after mode byte changes");
        events.emplace_back("facet");
        return frame_count;
    }
    double Duration(pal4::StripObjectState& state) override {
        Check(&state == object, "duration uses strip owner");
        events.emplace_back("duration");
        return duration;
    }
};

void TestLifecycle() {
    pal4::StripObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    runtime.object = &state;
    Check(&pal4::StripObject::Construct(state, runtime) == &state &&
              state.words[0u] == 0x84aa90u && runtime.count == 8u &&
              state.words[33u] == 0u && state.words[34u] == 0u &&
              state.words[37u] == 0u && state.words[38u] == 0u &&
              state.words[39u] == 0u && state.words[72u] == 0u &&
              state.words[73u] == 0u && state.words[74u] == 0u,
          "300-byte StripObject constructor fields and instance count");
    CheckFacets(state, kDerived);
    for (std::uint32_t i = 1u; i < 5u; ++i)
        Check(state.words[i] == 0u, "base prefix cleared");
    Check((state.words[35u] & 0xffu) == 0u &&
              ((state.words[71u] & 0xffu) == 0x5au) &&
              state.words[36u] == 0xaaaaaaaau &&
              state.words[70u] == 0xaaaaaaaau,
          "individual bytes written without clearing adjacent fields");
    Check(pal4::StripObject::ClassName() == "StripObject",
          "native class name from vtable slot");

    state.words[72u] = 0x4000u;
    runtime.events.clear();
    pal4::StripObject::Destroy(state, runtime);
    Check(runtime.count == 7u && state.words[0u] == 0x84b240u &&
              state.words[72u] == 0u && state.words[73u] == 0u &&
              state.words[74u] == 0u,
          "destructor releases buffer and restores base");
    CheckFacets(state, kBase);
    Check(runtime.events == std::vector<std::string>{
              "system", "count-read", "count-write", "free-buffer"},
          "instance decrement precedes buffer free");
}

void TestDeletingDestructor() {
    for (const auto flags : {0u, 3u}) {
        pal4::StripObjectState state{};
        Runtime runtime;
        runtime.object = &state;
        (void)pal4::StripObject::Construct(state, runtime);
        runtime.events.clear();
        Check(pal4::StripObject::DeletingDestroy(state, flags, runtime) ==
                  reinterpret_cast<std::uintptr_t>(&state),
              "deleting destructor returns original object address");
        const std::vector<std::string> expected = (flags & 1u) != 0u
            ? std::vector<std::string>{
                  "system", "count-read", "count-write",
                  "free-buffer", "free-object"}
            : std::vector<std::string>{
                  "system", "count-read", "count-write", "free-buffer"};
        Check(runtime.events == expected,
              "conditional object free follows buffer cleanup");
    }
}

void TestConstructorUnwind() {
    pal4::StripObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    runtime.object = &state;
    runtime.fail_count = true;
    bool threw = false;
    try {
        (void)pal4::StripObject::Construct(state, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && state.words[0u] == 0x84b240u &&
              runtime.count == 7u,
          "constructor count failure restores base without increment");
    CheckFacets(state, kBase);
}

void TestPlaybackState() {
    pal4::StripObjectState state{};
    PlaybackRuntime runtime;
    runtime.object = &state;
    state.words[2u] = 0xaabbcc00u;
    state.words[36u] = 0x12345678u;
    pal4::StripObject::SetEnabled(state, 1u, runtime);
    Check(state.words[2u] == 0xaabbcc00u && runtime.events.empty(),
          "inactive strip ignores state changes");

    state.words[1u] = 0xabcde001u;
    pal4::StripObject::SetEnabled(state, 1u, runtime);
    Check(state.words[2u] == 0x01bbcc00u &&
              state.words[36u] == std::bit_cast<std::uint32_t>(2.5f) &&
              runtime.events == std::vector<std::string>{"facet", "duration"},
          "start callback and normalized playback ratio");
    runtime.events.clear();
    state.words[36u] = 0x87654321u;
    pal4::StripObject::SetEnabled(state, 1u, runtime);
    Check(runtime.events.empty() && state.words[36u] == 0x87654321u,
          "unchanged state skips native calls and ratio write");
    pal4::StripObject::SetEnabled(state, 0u, runtime);
    Check(state.words[2u] == 0x00bbcc00u && runtime.events.empty() &&
              state.words[36u] == 0x87654321u,
          "disable clears only playback byte");
    runtime.frame_count = -9;
    runtime.duration = 3.0;
    pal4::StripObject::SetEnabled(state, 255u, runtime);
    Check(state.words[2u] == 0xffbbcc00u &&
              state.words[36u] == std::bit_cast<std::uint32_t>(-3.0f),
          "nonunit playback byte and signed callback result");
}

}  // namespace

int main() {
    TestLifecycle();
    TestDeletingDestructor();
    TestConstructorUnwind();
    TestPlaybackState();
}
