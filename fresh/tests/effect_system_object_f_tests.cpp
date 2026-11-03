#include "pal4/effect_system_object_f.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemObjectFRuntime {
    std::vector<std::uint32_t> events;
    std::uint32_t count = 6u;
    bool saw_base_facets = false;
    bool saw_derived_on_destroy = false;
    std::uint32_t freed = 0xffffffffu;

    void InitializeXmlDocument(
        pal4::EffectSystemObjectFState& state,
        const std::uint32_t offset) override {
        events.push_back(offset);
        constexpr std::array<std::uint32_t, 9u> expected{
            0x84a1dcu, 0x84a1d0u, 0x84a1c8u,
            0x84a1bcu, 0x84a1acu, 0x84a1a0u,
            0x84a194u, 0x84a184u, 0x84a178u};
        saw_base_facets = state.words[0u] == 0x84b240u;
        for (std::uint32_t index = 0u; index < expected.size(); ++index) {
            saw_base_facets &= state.words[5u + index] == expected[index];
        }
        state.words[14u] = 0x11223344u;
    }
    void DestroyXmlDocument(
        pal4::EffectSystemObjectFState& state,
        const std::uint32_t offset) override {
        events.push_back(offset);
        saw_derived_on_destroy = state.words[0u] == 0x84a120u &&
            state.words[5u] == 0x84a114u &&
            state.words[13u] == 0x84a0b0u &&
            state.words[39u] == 0u && state.words[40u] == 0u &&
            state.words[41u] == 0u;
        state.words[14u] = 0u;
    }
    void Free(const std::uint32_t address) override {
        events.push_back(address);
        freed = address;
    }
    std::uint32_t EffectSystemInstance() override {
        events.push_back(0x11111111u);
        return 0x5000u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        events.push_back(address);
        return count;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        events.push_back(address);
        count = value;
    }
};

struct PlaybackRuntime final : pal4::IEffectSystemObjectFPlaybackRuntime {
    std::vector<std::uint32_t> events;
    std::int32_t task_value = 8;
    double duration = 2.0;

    void QueryTaskValues(pal4::EffectSystemObjectFState& state,
                         std::array<std::int32_t, 4>& values) override {
        events.push_back(1u);
        Check((state.words[2u] >> 24u) == 1u,
              "mode byte changes before task query");
        Check(values == std::array<std::int32_t, 4>{},
              "task query receives four zero initialized values");
        values[3u] = task_value;
    }

    double Duration(pal4::EffectSystemObjectFState&) override {
        events.push_back(2u);
        return duration;
    }
};

void TestConstructionAndCount() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words.fill(0xa5a5a5a5u);
    Check(&pal4::EffectSystemObjectF::Construct(
              state, 0x6bu, runtime) == &state,
          "constructor returns object address");
    constexpr std::array<std::uint32_t, 9u> derived{
        0x84a114u, 0x84a108u, 0x84a100u,
        0x84a0f4u, 0x84a0e4u, 0x84a0d8u,
        0x84a0ccu, 0x84a0bcu, 0x84a0b0u};
    Check(runtime.saw_base_facets && state.words[0u] == 0x84a120u,
          "XML initialization sees base facets before derived vtable");
    for (std::uint32_t index = 0u; index < derived.size(); ++index) {
        Check(state.words[5u + index] == derived[index],
              "derived facet table retains native address order");
    }
    Check(state.words[1u] == 0u && state.words[2u] == 0u &&
              state.words[3u] == 0u && state.words[4u] == 0u &&
              state.words[14u] == 0x11223344u &&
              state.words[29u] == 0u &&
              state.words[38u] == 0xa5a5a56bu &&
              state.words[39u] == 0u && state.words[40u] == 0u &&
              state.words[41u] == 0u &&
              state.words[30u] == 0xa5a5a5a5u &&
              state.words[37u] == 0xa5a5a5a5u,
          "constructor writes exact observed fields and preserves other words");
    Check(runtime.events == std::vector<std::uint32_t>{
              56u, 0x11111111u, 0x50a4u, 0x50a4u} &&
              runtime.count == 7u,
          "XML init precedes singleton count read and increment at +164");

    runtime.events.clear();
    runtime.count = 0xffffffffu;
    static_cast<void>(pal4::EffectSystemObjectF::Construct(
        state, 0u, runtime));
    Check(runtime.count == 0u && state.words[38u] == 0xa5a5a500u,
          "count wraps at 32 bits and explicit stack byte replaces low byte");
}

void TestDestructionOrder() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    static_cast<void>(pal4::EffectSystemObjectF::Construct(
        state, 0x55u, runtime));
    state.words[39u] = 0x3000u;
    state.words[40u] = 0x3004u;
    state.words[41u] = 0x3008u;
    state.words[37u] = 0xabcdu;
    runtime.events.clear();
    pal4::EffectSystemObjectF::Destroy(state, runtime);
    constexpr std::array<std::uint32_t, 9u> base{
        0x84a1dcu, 0x84a1d0u, 0x84a1c8u,
        0x84a1bcu, 0x84a1acu, 0x84a1a0u,
        0x84a194u, 0x84a184u, 0x84a178u};
    Check(runtime.events == std::vector<std::uint32_t>{
              0x11111111u, 0x50a4u, 0x50a4u, 0x3000u, 56u} &&
              runtime.freed == 0x3000u && runtime.count == 6u &&
              runtime.saw_derived_on_destroy,
          "destructor decrements singleton count, frees buffer, then XML");
    for (std::uint32_t index = 0u; index < base.size(); ++index) {
        Check(state.words[5u + index] == base[index],
              "destructor restores base facet vtables");
    }
    Check(state.words[0u] == 0x84b240u &&
              state.words[14u] == 0u &&
              state.words[39u] == 0u && state.words[40u] == 0u &&
              state.words[41u] == 0u && state.words[37u] == 0xabcdu &&
              state.words[38u] == 0x55u,
          "destructor ends with base object and retains unrelated fields");
}

void TestObjectAccessAndTimedMode() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[29u] = 0x4000u;
    Check(pal4::EffectSystemObjectF::ObjectHandle(state, runtime) == 0u &&
              runtime.events.empty(),
          "inactive object has no handle and avoids pointer read");
    state.words[1u] = 0x12340001u;
    Check(pal4::EffectSystemObjectF::ObjectHandle(state, runtime) == 6u &&
              runtime.events == std::vector<std::uint32_t>{0x4014u},
          "active object returns embedded handle at +20");
    Check(pal4::EffectSystemObjectF::ClassName() == "BillBordObject",
          "class name preserves native spelling");

    PlaybackRuntime playback;
    state.words[2u] = 0xaabbccddu;
    state.words[30u] = 0xffffffffu;
    pal4::EffectSystemObjectF::SetTimedMode(state, 0x101u, playback);
    Check(state.words[2u] == 0x01bbccddu &&
              state.words[30u] == std::bit_cast<std::uint32_t>(4.0f) &&
              playback.events == std::vector<std::uint32_t>{1u, 2u},
          "enabling low byte queries task then writes duration ratio");

    playback.events.clear();
    pal4::EffectSystemObjectF::SetTimedMode(state, 1u, playback);
    Check(playback.events.empty(), "unchanged mode makes no callbacks");
    pal4::EffectSystemObjectF::SetTimedMode(state, 0x100u, playback);
    Check(state.words[2u] == 0x00bbccddu &&
              state.words[30u] == std::bit_cast<std::uint32_t>(4.0f) &&
              playback.events.empty(),
          "disabling updates only the mode byte");

    state.words[1u] = 0u;
    pal4::EffectSystemObjectF::SetTimedMode(state, 1u, playback);
    Check(state.words[2u] == 0x00bbccddu && playback.events.empty(),
          "inactive object ignores mode changes");
}

}  // namespace

int main() {
    TestConstructionAndCount();
    TestDestructionOrder();
    TestObjectAccessAndTimedMode();
}
