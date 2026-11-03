#include "pal4/effect_object_control.h"

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

struct Runtime final : pal4::IEffectObjectControlRuntime {
    std::vector<std::string> calls;
    bool finalize_result = true;

    void ResetPlaybackFacet(pal4::EffectObjectState&) override {
        calls.emplace_back("facet");
    }
    void ResetInstance(const std::uint32_t instance) override {
        Check(instance == 0x3000u, "reset instance handle");
        calls.emplace_back("instance_reset");
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check(address == 0x300cu && value == 0x5000u,
              "instance owner word");
        calls.emplace_back("owner");
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        Check((address == 0x3004u && value == 1u) ||
                  (address == 0x3050u && value == 0u) ||
                  (address == 0x3053u && value == 0x7bu),
              "instance control byte");
        calls.emplace_back(address == 0x3004u ? "active" :
                           address == 0x3050u ? "clear" : "visibility");
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        Check(address == 0x5053u, "owner visibility offset");
        calls.emplace_back("read_visibility");
        return 0x7bu;
    }
    void UpdateFrame(pal4::EffectObjectState& state) override {
        Check(state.words[13u] == state.words[33u],
              "cached transforms precede frame update");
        calls.emplace_back("frame_update");
    }
    bool FinalizeInstance(const std::uint32_t instance) override {
        Check(instance == 0x3000u, "finalize instance handle");
        calls.emplace_back("finalize");
        return finalize_result;
    }
    void CleanupInstance(const std::uint32_t instance,
                         const std::uint32_t context) override {
        Check(instance == 0x3000u && context == 0x6000u,
              "stop cleanup arguments");
        calls.emplace_back("cleanup");
    }
    void DestroyInstance(const std::uint32_t instance,
                         const std::uint32_t context) override {
        Check(instance == 0x3000u && context == 0x6000u,
              "stop destruction arguments");
        calls.emplace_back("destroy_instance");
    }
    void DestroyFrame(const std::uint32_t frame) override {
        Check(frame == 0x4000u, "stop frame handle");
        calls.emplace_back("destroy_frame");
    }
    void ProcessStop(const std::uint32_t system,
                     pal4::EffectObjectState& state) override {
        Check(system == 0x5000u && state.words[23u] == 0u &&
                  state.words[24u] == 0u && Byte(state, 4u) == 0u,
              "system notification follows resource release");
        calls.emplace_back("process_stop");
    }
    std::uint32_t DeleteObject(pal4::EffectObjectState&,
                               const std::uint32_t flags) override {
        Check(flags == 1u, "deleting destructor flag");
        calls.emplace_back("delete");
        return 0xdeadbeefu;
    }
};

pal4::EffectObjectState ActiveState() {
    pal4::EffectObjectState state{};
    state.words.fill(0xaaaaaaaau);
    state.words[3u] = 0x5000u;
    state.words[4u] = 0x5000u;
    state.words[23u] = 0x3000u;
    state.words[24u] = 0x4000u;
    SetByte(state, 4u, 1u);
    SetByte(state, 6u, 1u);
    SetByte(state, 7u, 1u);
    SetByte(state, 11u, 1u);
    SetByte(state, 88u, 1u);
    for (std::uint32_t i = 0u; i != 9u; ++i)
        state.words[33u + i] = 0x1000u + i;
    return state;
}

void TestReset() {
    auto state = ActiveState();
    Runtime runtime;
    Check(pal4::EffectObjectControl::Reset(state, runtime),
          "active reset returns native finalize result");
    for (std::uint32_t i = 0u; i != 9u; ++i)
        Check(state.words[13u + i] == 0x1000u + i,
              "reset restores all nine cached words");
    Check(Byte(state, 11u) == 0u && Byte(state, 4u) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "facet", "instance_reset", "owner", "active", "clear",
                  "read_visibility", "visibility", "frame_update", "finalize"},
          "reset call order and playback byte");
    runtime.finalize_result = false;
    runtime.calls.clear();
    Check(!pal4::EffectObjectControl::Reset(state, runtime) &&
              runtime.calls.back() == "finalize",
          "native finalize false propagates");
}

void TestResetGates() {
    auto state = ActiveState();
    Runtime runtime;
    SetByte(state, 4u, 0u);
    Check(!pal4::EffectObjectControl::Reset(state, runtime) &&
              runtime.calls.empty() && Byte(state, 11u) == 1u,
          "inactive reset has no effects");
    SetByte(state, 4u, 1u);
    state.words[23u] = 0u;
    Check(!pal4::EffectObjectControl::Reset(state, runtime) &&
              runtime.calls == std::vector<std::string>{"facet"} &&
              state.words[13u] == state.words[33u] && Byte(state, 11u) == 0u,
          "missing instance returns after restoring cache");
}

void TestStop() {
    auto state = ActiveState();
    Runtime runtime;
    Check(pal4::EffectObjectControl::Stop(state, 0x6000u, runtime) ==
              0xdeadbeefu,
          "stop returns deleting destructor result");
    constexpr std::array<std::uint32_t, 9u> expected{
        0x3f800000u, 0x3f800000u, 0x3f800000u,
        0u, 0u, 0u, 0u, 0x3f800000u, 0x3f800000u};
    for (std::uint32_t i = 0u; i != 9u; ++i)
        Check(state.words[13u + i] == expected[i],
              "stop writes exact IDA default constants");
    Check(Byte(state, 4u) == 0u && Byte(state, 6u) == 0u &&
              Byte(state, 7u) == 0u && Byte(state, 88u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "facet", "cleanup", "destroy_instance", "destroy_frame",
                  "process_stop", "delete"},
          "stop clears flags and releases resources in order");

    state = ActiveState();
    runtime.calls.clear();
    state.words[23u] = 0u;
    state.words[24u] = 0u;
    Check(pal4::EffectObjectControl::Stop(state, 0x6000u, runtime) ==
              0xdeadbeefu &&
              runtime.calls == std::vector<std::string>{
                  "facet", "process_stop", "delete"},
          "stop skips absent instance and frame");

    state = ActiveState();
    runtime.calls.clear();
    SetByte(state, 4u, 0u);
    Check(pal4::EffectObjectControl::Stop(state, 0x6000u, runtime) ==
              0xdeadbeefu &&
              runtime.calls == std::vector<std::string>{"delete"},
          "inactive stop still deletes");
}

}  // namespace

int main() {
    TestReset();
    TestResetGates();
    TestStop();
}
