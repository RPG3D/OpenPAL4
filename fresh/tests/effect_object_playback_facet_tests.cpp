#include "pal4/effect_object_playback_facet.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint8_t Byte(const pal4::EffectObjectState& state,
                  const std::uint32_t offset) {
    return static_cast<std::uint8_t>(state.words[offset / 4u] >>
                                     ((offset % 4u) * 8u));
}

void SetByte(pal4::EffectObjectState& state, const std::uint32_t offset,
             const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

pal4::EffectObjectState State() {
    pal4::EffectObjectState state{};
    state.words[23u] = 0x3000u;
    SetByte(state, 4u, 1u);
    SetByte(state, 6u, 1u);
    SetByte(state, 7u, 1u);
    SetByte(state, 88u, 1u);
    return state;
}

struct Runtime final : pal4::IEffectObjectPlaybackFacetRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> resource_values{0u};
    std::uint8_t pending = 0u;
    std::uint8_t paused = 0u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(address == 0x3008u && !resource_values.empty(),
              "resource word address");
        calls.emplace_back("resource");
        const auto value = resource_values.front();
        resource_values.erase(resource_values.begin());
        return value;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        Check(address == 0x3051u || address == 0x3052u,
              "instance playback byte address");
        calls.emplace_back(address == 0x3051u ? "pending" : "paused");
        return address == 0x3051u ? pending : paused;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        Check((address == 0x3052u && value <= 1u) ||
                  (address == 0x3004u && value == 1u) ||
                  (address == 0x3050u && value == 0u),
              "instance playback write");
        calls.emplace_back(address == 0x3004u ? "activate" :
                           address == 0x3050u ? "clear" :
                           value == 0u ? "resume" : "pause");
    }
    void ResetInstance(const std::uint32_t instance) override {
        Check(instance == 0x3000u, "reset instance handle");
        calls.emplace_back("reset");
    }
};

void TestPause() {
    auto state = State();
    Runtime runtime;
    SetByte(state, 6u, 0u);
    Check(pal4::EffectObjectPlaybackFacet::Pause(state, runtime) == 0u &&
              runtime.calls.empty(), "not running cannot pause");
    SetByte(state, 6u, 1u);
    SetByte(state, 4u, 0u);
    Check(pal4::EffectObjectPlaybackFacet::Pause(state, runtime) == 0u &&
              runtime.calls.empty(), "inactive cannot pause");
    SetByte(state, 4u, 1u);
    runtime.resource_values = {0x1234abu};
    Check(pal4::EffectObjectPlaybackFacet::Pause(state, runtime) == 0xabu &&
              runtime.calls == std::vector<std::string>{"resource"},
          "loaded resource returns its low byte without tail");

    runtime.calls.clear();
    runtime.resource_values = {0u};
    runtime.pending = 0u;
    Check(pal4::EffectObjectPlaybackFacet::Pause(state, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"resource", "pending"},
          "missing pending byte exits external tail");
    runtime.calls.clear();
    runtime.resource_values = {0u, 0x56u};
    runtime.pending = 1u;
    Check(pal4::EffectObjectPlaybackFacet::Pause(state, runtime) == 0x56u &&
              runtime.calls == std::vector<std::string>{
                  "resource", "pending", "resource", "pause"},
          "tail rechecks resource and marks pause on transition");
    runtime.calls.clear();
    runtime.resource_values = {0u, 0u};
    Check(pal4::EffectObjectPlaybackFacet::Pause(state, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "resource", "pending", "resource"},
          "tail leaves pause byte alone while resource remains absent");
}

void TestResume() {
    auto state = State();
    Runtime runtime;
    runtime.resource_values = {0u};
    Check(pal4::EffectObjectPlaybackFacet::Resume(state, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"resource"},
          "missing resource cannot resume");
    runtime.calls.clear();
    runtime.resource_values = {1u};
    runtime.paused = 0u;
    Check(pal4::EffectObjectPlaybackFacet::Resume(state, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"resource", "paused"},
          "clear pause byte is returned without write");
    runtime.calls.clear();
    runtime.resource_values = {1u};
    runtime.paused = 0x7bu;
    Check(pal4::EffectObjectPlaybackFacet::Resume(state, runtime) == 0x7bu &&
              runtime.calls == std::vector<std::string>{
                  "resource", "paused", "resume"},
          "external tail clears nonzero pause byte");
}

void TestClear() {
    auto state = State();
    Runtime runtime;
    SetByte(state, 4u, 0u);
    Check(pal4::EffectObjectPlaybackFacet::Clear(state, 0x99u, runtime) ==
              0x99u && runtime.calls.empty() && Byte(state, 6u) == 1u,
          "inactive clear retains entry EAX and state");
    SetByte(state, 4u, 1u);
    Check(pal4::EffectObjectPlaybackFacet::Clear(state, 0x99u, runtime) ==
              0x3000u &&
              runtime.calls == std::vector<std::string>{
                  "reset", "activate", "clear"} &&
              Byte(state, 4u) == 1u && Byte(state, 6u) == 0u &&
              Byte(state, 7u) == 0u && Byte(state, 88u) == 0u,
          "active clear resets instance and playback flags");
    state = State();
    state.words[23u] = 0u;
    runtime.calls.clear();
    Check(pal4::EffectObjectPlaybackFacet::Clear(state, 0x99u, runtime) ==
              0x99u && runtime.calls.empty() && Byte(state, 6u) == 0u,
          "null instance still clears state and preserves entry EAX");
}

void TestMetric() {
    struct MetricRuntime final : pal4::IEffectObjectMetricRuntime {
        std::vector<std::string> calls;
        void Prime(pal4::EffectObjectState&, std::uint32_t mode) override {
            Check(mode == 1u, "metric prime mode");
            calls.emplace_back("prime");
        }
        double Read(pal4::EffectObjectState&) override {
            calls.emplace_back("read");
            return 2.75;
        }
    } runtime;
    auto state = State();
    SetByte(state, 4u, 0u);
    Check(pal4::EffectObjectMetric::GetProgress(state, runtime) == 0.0 &&
              runtime.calls.empty(), "inactive metric returns IDA zero");
    SetByte(state, 4u, 1u);
    SetByte(state, 6u, 0u);
    Check(pal4::EffectObjectMetric::GetProgress(state, runtime) == 0.0 &&
              runtime.calls.empty(), "nonrunning metric returns IDA zero");
    SetByte(state, 6u, 1u);
    Check(pal4::EffectObjectMetric::GetProgress(state, runtime) == 2.75 &&
              runtime.calls == std::vector<std::string>{"prime", "read"},
          "running metric dispatches two virtual slots in order");
}

void TestFacetA718Lifetime() {
    struct LifetimeRuntime final : pal4::IEffectObjectFacetA718Runtime {
        std::uint32_t frees = 0u;
        void FreeFacet(pal4::EffectObjectFacetA718Storage& facet) override {
            Check(facet.vtable == pal4::EffectObjectFacetA718::kVtable,
                  "vtable restored before free");
            ++frees;
        }
    } runtime;
    pal4::EffectObjectFacetA718Storage facet{0x12345678u};
    pal4::EffectObjectFacetA718::Initialize(facet);
    Check(facet.vtable == 0x84a718u, "initializer writes native vtable");
    facet.vtable = 0u;
    Check(pal4::EffectObjectFacetA718::DeletingDestroy(
              facet, 0u, runtime) ==
              reinterpret_cast<std::uintptr_t>(&facet) &&
              facet.vtable == 0x84a718u && runtime.frees == 0u,
          "nondeleting destructor restores vtable and pointer");
    Check(pal4::EffectObjectFacetA718::DeletingDestroy(
              facet, 3u, runtime) ==
              reinterpret_cast<std::uintptr_t>(&facet) &&
              runtime.frees == 1u,
          "low flag bit frees facet exactly once");
}

}  // namespace

int main() {
    TestPause();
    TestResume();
    TestClear();
    TestMetric();
    TestFacetA718Lifetime();
}
