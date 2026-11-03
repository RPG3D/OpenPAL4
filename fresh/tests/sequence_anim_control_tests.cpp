#include "pal4/sequence_anim_control.h"

#include <array>
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

struct Runtime final : pal4::ISequenceAnimControlRuntime {
    std::vector<std::string> events;
    std::array<std::uint32_t, 13> written{};
    std::uint32_t color = 0u;
    std::array<std::uint32_t, 4> corners{};
    void WriteResourceWord(std::uint32_t address,
                           std::uint32_t value) override {
        Check(address >= 0x3000u && address < 0x3034u,
              "resource word write address");
        written[(address - 0x3000u) / 4u] = value;
        events.push_back("write:" + std::to_string(address - 0x3000u));
    }
    std::uint32_t Geometry(std::uint32_t resource) override {
        Check(resource == 0x3000u, "geometry resource address");
        events.emplace_back("geometry");
        return 0x5000u;
    }
    void SetColor(std::uint32_t geometry, std::uint32_t rgba) override {
        Check(geometry == 0x5000u, "color geometry handle");
        color = rgba;
        events.emplace_back("color");
    }
    void InitializeTransform(std::uint32_t resource) override {
        Check(resource == 0x3000u, "transform resource address");
        events.emplace_back("transform");
    }
    void UpdateGeometry(std::uint32_t geometry,
                        const std::array<std::uint32_t, 4>& values) override {
        Check(geometry == 0x5000u, "update geometry handle");
        corners = values;
        events.emplace_back("update");
    }
    void StopPlaybackFacet(pal4::SequenceAnimObjectState& state) override {
        Check(state.words[5u] == 0x84a8b8u,
              "playback facet at owner +20");
        events.emplace_back("stop-facet");
    }
    void DeinitializeResource(std::uint32_t resource) override {
        Check(resource == 0x3000u, "deinitialize native resource");
        events.emplace_back("deinitialize");
    }
    void FreeResource(std::uint32_t resource) override {
        Check(resource == 0x3000u, "free native resource");
        events.emplace_back("free");
    }
    void ProcessEvent(std::uint32_t system,
                      pal4::SequenceAnimObjectState& state) override {
        Check(system == 0x6000u && state.words[30u] == 0u,
              "event after resource cleanup");
        events.emplace_back("event");
    }
    std::uint32_t DeletingDestroy(pal4::SequenceAnimObjectState&,
                                  std::uint32_t flags) override {
        Check(flags == 1u, "native deleting destructor flag");
        events.emplace_back("delete");
        return 0x12345678u;
    }
};

}  // namespace

void TestReset() {
    pal4::SequenceAnimObjectState state{};
    state.words.fill(0xdeadbeefu);
    state.words[1u] = 0x100u;
    Runtime runtime;
    Check(pal4::SequenceAnimControl::Reset(state, runtime) == 0u &&
              runtime.events.empty(), "activity uses low byte");
    state.words[1u] = 1u;
    state.words[2u] = 0xab123456u;
    state.words[15u] = 4u;
    state.words[30u] = 0x3000u;
    state.words[31u] = 11u;
    state.words[32u] = 12u;
    state.words[33u] = 13u;
    state.words[34u] = 14u;
    state.words[35u] = 0x12345678u;
    state.words[36u] = 0xaaaa0001u;
    state.words[37u] = 0xaaaa0002u;
    state.words[38u] = 0xaaaa0003u;
    state.words[39u] = 0xaaaa0004u;
    state.words[40u] = 0xaaaa0005u;
    state.words[41u] = 0xaaaa0006u;
    Check(pal4::SequenceAnimControl::Reset(state, runtime) == 1u &&
              state.words[20u] == 0u && state.words[16u] == 0u &&
              state.words[17u] == 3u && state.words[2u] == 0x00123456u,
          "reset counters and mode byte");
    Check(runtime.written[6u] == 0xaaaa0001u &&
              runtime.written[10u] == 0xaaaa0002u &&
              runtime.written[11u] == 0xaaaa0003u &&
              runtime.written[7u] == 0xaaaa0004u &&
              runtime.written[8u] == 0xaaaa0005u &&
              runtime.written[9u] == 0xaaaa0006u,
          "cached rotation, scale and position restored");
    Check(runtime.color == 0x12345678u &&
              runtime.corners == std::array<std::uint32_t, 4>{11u, 12u, 13u, 14u},
          "cached color and corners restored");
    Check(runtime.events == std::vector<std::string>{
              "write:24", "write:40", "write:44", "geometry", "color",
              "write:28", "write:32", "write:36", "transform", "geometry",
              "update"},
          "native reset callback order");
}

void TestStop() {
    Runtime inactive_runtime;
    pal4::SequenceAnimObjectState inactive{};
    inactive.words.fill(0xabcdef01u);
    inactive.words[1u] = 0u;
    Check(pal4::SequenceAnimControl::Stop(inactive, inactive_runtime) ==
              0x12345678u &&
              inactive_runtime.events == std::vector<std::string>{"delete"} &&
              inactive.words[13u] == 0xabcdef01u,
          "inactive stop only invokes deleting destructor");

    Runtime runtime;
    pal4::SequenceAnimObjectState state{};
    state.words.fill(0xabcdef01u);
    state.words[1u] = 0x03020101u;
    state.words[4u] = 0x6000u;
    state.words[5u] = 0x84a8b8u;
    state.words[30u] = 0x3000u;
    Check(pal4::SequenceAnimControl::Stop(state, runtime) == 0x12345678u,
          "active stop returns native destructor result");
    Check(runtime.events == std::vector<std::string>{
              "stop-facet", "deinitialize", "free", "event", "delete"},
          "active stop call order");
    Check(state.words[1u] == 0x00000100u && state.words[30u] == 0u,
          "active, running and pending bytes cleared");
    for (std::uint32_t i = 13u; i <= 20u; ++i)
        Check(state.words[i] == 0u, "atlas fields cleared");
    for (std::uint32_t i = 25u; i <= 28u; ++i)
        Check(state.words[i] == 0u, "texture dimensions cleared");
    Check(state.words[21u] == 0u && state.words[22u] == 0u &&
              state.words[23u] == 0x3f800000u &&
              state.words[24u] == 0x3f800000u,
          "static UV corner values restored");

    Runtime no_resource_runtime;
    pal4::SequenceAnimObjectState no_resource{};
    no_resource.words[1u] = 1u;
    no_resource.words[4u] = 0x6000u;
    Check(pal4::SequenceAnimControl::Stop(no_resource, no_resource_runtime) ==
              0x12345678u &&
              no_resource_runtime.events ==
                  std::vector<std::string>{"event", "delete"},
          "active stop without playback or resource");
}

int main() {
    TestReset();
    TestStop();
}
