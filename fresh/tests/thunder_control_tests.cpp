#include "pal4/thunder_control.h"

#include <array>
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

struct ResetRuntime final : pal4::IThunderResetRuntime {
    std::vector<std::string> events;
    std::vector<std::array<std::uint32_t, 9u>> snapshots;

    void RefreshTarget(pal4::ThunderObjectState& state,
                       const std::uint32_t owner) override {
        Check(owner == 0x7000u, "reset resolves owner target");
        events.emplace_back("target");
        state.words[70u] = 41u;
        state.words[71u] = 42u;
        state.words[72u] = 43u;
    }
    void UpdateFrame(pal4::ThunderObjectState& state) override {
        events.emplace_back("frame");
        std::array<std::uint32_t, 9u> snapshot{};
        for (std::uint32_t i = 0u; i < snapshot.size(); ++i)
            snapshot[i] = state.words[18u + i];
        snapshots.push_back(snapshot);
    }
};

void TestReset() {
    pal4::ThunderObjectState state{};
    ResetRuntime runtime;
    state.words[3u] = 0x7000u;
    state.words[2u] = 0xff123456u;
    for (std::uint32_t i = 0u; i < 9u; ++i) {
        state.words[18u + i] = 90u + i;
        state.words[61u + i] = 10u + i;
    }
    state.words[27u] = 0x11223344u;
    state.words[60u] = 0xaabbccddu;
    state.words[70u] = 20u;
    state.words[71u] = 21u;
    state.words[72u] = 22u;

    Check(pal4::ThunderControl::Reset(state, runtime) == 0u &&
              runtime.events.empty(), "inactive reset skips all work");
    state.words[1u] = 1u;
    Check(pal4::ThunderControl::Reset(state, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "target", "frame", "frame", "frame"},
          "nonlocal target refresh precedes three frame updates");
    Check(runtime.snapshots == std::vector<std::array<std::uint32_t, 9u>>{
              {10u, 11u, 12u, 93u, 94u, 95u, 96u, 97u, 98u},
              {10u, 11u, 12u, 13u, 14u, 15u, 96u, 97u, 98u},
              {10u, 11u, 12u, 13u, 14u, 15u, 16u, 17u, 18u}},
          "reset writes scale, rotation and position in stages");
    Check(state.words[27u] == 0xaabbccddu &&
              state.words[2u] == 0x00123456u &&
              state.words[37u] == 41u && state.words[38u] == 42u &&
              state.words[39u] == 43u,
          "reset restores color, clears playback and copies refreshed target");

    state.words[58u] = 1u;
    runtime.events.clear();
    runtime.snapshots.clear();
    Check(pal4::ThunderControl::Reset(state, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "frame", "frame", "frame"},
          "local model skips target resolution");
}

struct StopRuntime final : pal4::IThunderStopRuntime {
    std::vector<std::string> events;
    void ResetVisual(pal4::ThunderObjectState& state) override {
        Check((state.words[1u] & 0xffu) != 0u,
              "visual reset occurs while active");
        events.emplace_back("visual");
    }
    void ClearTargetName(pal4::ThunderObjectState& state) override {
        Check(state.words[1u] == 0x00005600u,
              "active and visibility bytes cleared before text cleanup");
        events.emplace_back("name");
    }
    void ProcessEffectEvent(const std::uint32_t system,
                            pal4::ThunderObjectState& state) override {
        Check(system == 0x6000u &&
                  state.words[14u] == 0x11223300u &&
                  state.words[58u] == 0xaabbcc00u,
              "event sees local and movement flags cleared");
        events.emplace_back("event");
    }
    std::uint32_t DeleteObject(pal4::ThunderObjectState&,
                               const std::uint32_t flags) override {
        Check(flags == 1u, "deleting destructor flag");
        events.emplace_back("delete");
        return 0x4567u;
    }
};

void TestStop() {
    pal4::ThunderObjectState state{};
    StopRuntime runtime;
    Check(pal4::ThunderControl::Stop(state, runtime) == 0x4567u &&
              runtime.events == std::vector<std::string>{"delete"},
          "inactive stop still deletes object");
    runtime.events.clear();
    state.words[1u] = 0x12345601u;
    state.words[4u] = 0x6000u;
    state.words[14u] = 0x11223301u;
    state.words[58u] = 0xaabbcc01u;
    Check(pal4::ThunderControl::Stop(state, runtime) == 0x4567u &&
              runtime.events == std::vector<std::string>{
                  "visual", "name", "event", "delete"},
          "active stop cleans resources, dispatches event then deletes");
}

}  // namespace

int main() {
    TestReset();
    TestStop();
}
