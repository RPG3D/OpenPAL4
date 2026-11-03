#include "pal4/strip_control.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IStripControlRuntime {
    pal4::StripObjectState* state = nullptr;
    std::vector<std::string> events;
    void SetColor(std::uint32_t model, std::uint32_t rgba) override {
        Check(model == 0x3000u && rgba == 0x44332211u,
              "straight color restored before callback");
        events.emplace_back("color");
    }
    void SetRotation(std::uint32_t model,
                     const std::array<float, 3>& values) override {
        Check(model == 0x3000u && values == std::array{7.0f, 8.0f, 9.0f},
              "rotation restored before callback");
        events.emplace_back("rotation");
    }
    void SetScale(std::uint32_t model,
                  const std::array<float, 3>& values) override {
        Check(model == 0x3000u && values == std::array{4.0f, 5.0f, 6.0f},
              "scale restored before callback");
        events.emplace_back("scale");
    }
    void SetPosition(std::uint32_t model,
                     const std::array<float, 3>& values) override {
        Check(model == 0x3000u && values == std::array{1.0f, 2.0f, 3.0f},
              "position restored before callback");
        events.emplace_back("position");
    }
    void UpdateGeometryVertices(
        std::uint32_t model, const std::array<float, 4>& uv) override {
        Check(model == 0x3000u &&
                  uv == std::array{0.0f, 0.5f, 0.25f, 1.0f} &&
                  state->words[44u] == 0u && state->words[45u] == 1u,
              "sequence counters and UVs restored before geometry update");
        events.emplace_back("vertices");
    }
    void UpdateFrameProperties(pal4::StripObjectState& value) override {
        Check(&value == state && value.words[25u] == 0x44332211u &&
                  value.words[19u] == Bits(7.0f),
              "cylinder frame sees restored state");
        events.emplace_back("frame");
    }
};

struct StopRuntime final : pal4::IStripStopRuntime {
    pal4::StripObjectState* state = nullptr;
    std::vector<std::string> events;
    void StopFacet(pal4::StripObjectState& value,
                   std::uint32_t offset) override {
        Check(&value == state && (value.words[1u] & 0xffu) == 1u,
              "stop facets run while active");
        events.emplace_back(offset == 48u ? "facet48" : "facet20");
        Check(offset == 48u || offset == 20u, "stop facet offset");
    }
    void DestroyDocument(std::uint32_t address) override {
        Check(address == 0x4000u && (state->words[1u] & 0xffu) == 0u,
              "document cleanup follows active flag clear");
        events.emplace_back("document");
    }
    void DestroyAtomic(std::uint32_t address) override {
        Check(address == 0x4100u, "atomic resource address");
        events.emplace_back("atomic");
    }
    void DestroyStraight(std::uint32_t address) override {
        Check(address == 0x4200u, "straight resource address");
        events.emplace_back("straight");
    }
    void DestroyClump(std::uint32_t address) override {
        Check(address == 0x4300u, "clump resource address");
        events.emplace_back("clump");
    }
    void DestroyFrame(std::uint32_t address) override {
        Check(address == 0x4400u, "frame resource address");
        events.emplace_back("frame");
    }
    void FreeMemory(std::uint32_t address) override {
        Check(address == 0x4000u || address == 0x4100u ||
                  address == 0x4200u, "owned resource allocation");
        events.emplace_back(address == 0x4000u ? "free-document" :
                            address == 0x4100u ? "free-atomic" :
                                                 "free-straight");
    }
    void ProcessEvent(std::uint32_t system_handle,
                      pal4::StripObjectState& value) override {
        Check(system_handle == 0x4500u && &value == state &&
                  value.words[33u] == 0u && value.words[34u] == 0u &&
                  value.words[37u] == 0u && value.words[38u] == 0u &&
                  value.words[39u] == 0u,
              "system event follows owned resource cleanup");
        events.emplace_back("event");
    }
    std::uint32_t DeleteObject(pal4::StripObjectState& value,
                               std::uint32_t flags) override {
        Check(&value == state && flags == 1u,
              "deleting destructor receives flag one");
        events.emplace_back("delete");
        return 0x1234u;
    }
};

pal4::StripObjectState State() {
    pal4::StripObjectState state{};
    state.words.fill(0xaaaaaaaau);
    state.words[1u] = 1u;
    state.words[2u] = 0x77000000u;
    state.words[35u] = 0u;
    state.words[40u] = 0u;
    state.words[38u] = 0x3000u;
    state.words[57u] = 0x44332211u;
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        state.words[61u + i] = Bits(7.0f + static_cast<float>(i));
        state.words[58u + i] = Bits(4.0f + static_cast<float>(i));
        state.words[64u + i] = Bits(1.0f + static_cast<float>(i));
    }
    state.words[67u] = Bits(0.0f);
    state.words[68u] = Bits(0.5f);
    state.words[69u] = Bits(0.25f);
    state.words[70u] = Bits(1.0f);
    state.words[43u] = 2u;
    return state;
}

void TestInactive() {
    auto state = State();
    state.words[1u] = 0u;
    const auto before = state;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::StripControl::Reset(state, runtime) == 0u &&
              state.words == before.words && runtime.events.empty(),
          "inactive reset preserves all state");
}

void TestCylinderAndStraight() {
    auto state = State();
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::StripControl::Reset(state, runtime) == 1u &&
              runtime.events == std::vector<std::string>{"frame"} &&
              state.words[25u] == 0x44332211u &&
              state.words[16u] == Bits(4.0f) &&
              state.words[22u] == Bits(1.0f) &&
              state.words[19u] == Bits(7.0f) &&
              (state.words[2u] >> 24u) == 0u,
          "cylinder reset copies cached transforms and clears playback");

    state = State();
    runtime.events.clear();
    state.words[35u] = 1u;
    state.words[40u] = 0u;
    Check(pal4::StripControl::Reset(state, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "color", "rotation", "scale", "position"} &&
              state.words[49u] == 0xaaaaaaaau,
          "straight reset preserves UVs when sequence is disabled");

    state = State();
    runtime.events.clear();
    state.words[35u] = 1u;
    state.words[40u] = 1u;
    Check(pal4::StripControl::Reset(state, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "color", "rotation", "scale", "position", "vertices"} &&
              state.words[49u] == state.words[67u] &&
              state.words[52u] == state.words[70u],
          "straight sequence restores UV cache after transforms");
}

void TestStop() {
    auto state = State();
    StopRuntime runtime;
    runtime.state = &state;
    state.words[1u] = 0x01010101u;
    state.words[4u] = 0x4500u;
    state.words[39u] = 0x4000u;
    state.words[37u] = 0x4100u;
    state.words[38u] = 0x4200u;
    state.words[34u] = 0x4300u;
    state.words[33u] = 0x4400u;
    Check(pal4::StripControl::Stop(state, runtime) == 0x1234u &&
              runtime.events == std::vector<std::string>{
                  "facet48", "facet20", "document", "free-document",
                  "atomic", "free-atomic", "straight", "free-straight",
                  "clump", "frame", "event", "delete"} &&
              state.words[1u] == 0x00000100u,
          "cylinder stop releases resources in native order");

    state = State();
    runtime.events.clear();
    state.words[1u] = 0x01010101u;
    state.words[4u] = 0x4500u;
    state.words[35u] = 1u;
    state.words[37u] = 0u;
    state.words[38u] = 0u;
    state.words[39u] = 0u;
    state.words[34u] = 0x4300u;
    state.words[33u] = 0x4400u;
    Check(pal4::StripControl::Stop(state, runtime) == 0x1234u &&
              runtime.events == std::vector<std::string>{
                  "facet48", "facet20", "event", "delete"} &&
              state.words[34u] == 0u && state.words[33u] == 0u,
          "straight stop clears shared handles without cylinder destruction");

    state = State();
    runtime.events.clear();
    state.words[1u] = 0u;
    Check(pal4::StripControl::Stop(state, runtime) == 0x1234u &&
              runtime.events == std::vector<std::string>{"delete"},
          "inactive stop still invokes deleting destructor");
}

}  // namespace

int main() {
    TestInactive();
    TestCylinderAndStraight();
    TestStop();
}
