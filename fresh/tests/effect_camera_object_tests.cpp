#include "pal4/effect_camera_object.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectCameraObjectRuntime {
    std::vector<std::string> events;
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::uint32_t system = 0x7000u;
    bool fail_xml = false;
    bool fail_system = false;
    std::uint32_t start_calls = 0u;
    std::uint8_t start_result = 0x5au;

    void ResetBase(pal4::EffectCameraObjectState& state) override {
        events.push_back("reset-base");
        state.words[0u] = 0x84a12cu;
    }
    void InitializeXml(pal4::EffectCameraObjectState& state,
                       std::uint32_t offset) override {
        events.push_back("xml-init");
        Check(offset == 132u && state.words[5u] == 0x84a1bcu &&
                  state.words[10u] == 0x84a178u &&
                  state.words[16u] == 0xffffff77u,
              "base facets and stack byte precede XML initialization");
        if (fail_xml)
            throw std::runtime_error("xml");
    }
    void DestroyXml(pal4::EffectCameraObjectState& state,
                    std::uint32_t offset) override {
        events.push_back("xml-destroy");
        Check(offset == 132u && state.words[5u] == 0x84a25cu,
              "derived facets remain installed during XML destruction");
    }
    void FinalizeField64(pal4::EffectCameraObjectState& state,
                         std::uint32_t offset, std::uint32_t flag) override {
        events.push_back("field64");
        Check(offset == 64u && flag == 1u &&
                  (state.words[10u] == 0x84a20cu ||
                   state.words[10u] == 0x84a178u),
              "field finalization precedes base facet restoration");
    }
    void FinalizeBase(pal4::EffectCameraObjectState& state) override {
        events.push_back("finalize-base");
        Check(state.words[5u] == 0x84a1bcu &&
                  state.words[10u] == 0x84a178u,
              "base facets restored before base finalization");
    }
    std::uint32_t SystemInstance() override {
        events.push_back("system");
        if (fail_system)
            throw std::runtime_error("system");
        return system;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read");
        return memory.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.push_back("write");
        memory[address] = value;
    }
    std::uint8_t StartCameraFacet(pal4::EffectCameraObjectState& state,
                                  std::uint32_t offset) override {
        Check(offset == 24u &&
                  static_cast<std::uint8_t>(state.words[2u] >> 24u) == 1u,
              "start facet sees updated byte +11");
        ++start_calls;
        return start_result;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    runtime.memory[0x70a4u] = 0xffffffffu;
    pal4::EffectCameraObjectState state{};
    state.words[11u] = 0xffffffffu;
    state.words[15u] = 0xffffffffu;
    state.words[16u] = 0xffffffffu;
    for (std::uint32_t index = 12u; index <= 32u; ++index)
        if (index != 15u && index != 16u)
            state.words[index] = 0xaaaaaaaa;
    Check(&pal4::EffectCameraObject::Construct(
              state, 0x77u, runtime) == &state &&
              runtime.events == std::vector<std::string>{
                  "reset-base", "xml-init", "system", "read", "write"} &&
              runtime.memory[0x70a4u] == 0u &&
              state.words[0u] == 0x84a268u,
          "construct installs facets and increments system count with wrap");
    constexpr std::array<std::uint32_t, 6> derived{
        0x84a25cu, 0x84a238u, 0x84a230u,
        0x84a224u, 0x84a218u, 0x84a20cu};
    for (std::uint32_t index = 0u; index < derived.size(); ++index)
        Check(state.words[5u + index] == derived[index],
              "derived facet table matches each native slot");
    Check(state.words[11u] == 0x00ffff00u &&
              state.words[15u] == 0xffffff00u &&
              state.words[16u] == 0xffffff77u &&
              state.words[17u] == 0u && state.words[19u] == 0u &&
              state.words[31u] == 0u && state.words[32u] == 0u &&
              state.words[12u] == 0u && state.words[14u] == 0u &&
              state.words[22u] == 0u && state.words[27u] == 0u &&
              state.words[33u] == 0u,
          "construct clears exact fields while preserving other bytes");

    runtime.events.clear();
    pal4::EffectCameraObject::Destroy(state, runtime);
    Check(runtime.events == std::vector<std::string>{
              "system", "read", "write", "xml-destroy", "field64",
              "finalize-base"} &&
              runtime.memory[0x70a4u] == 0xffffffffu &&
              state.words[0u] == 0x84a268u,
          "destroy decrements count before XML and base finalization");
    constexpr std::array<std::uint32_t, 6> base{
        0x84a1bcu, 0x84a2c0u, 0x84a1c8u,
        0x84a1d0u, 0x84a194u, 0x84a178u};
    for (std::uint32_t index = 0u; index < base.size(); ++index)
        Check(state.words[5u + index] == base[index],
              "destroy restores each base facet vtable");

    pal4::EffectCameraObjectState failed{};
    failed.words[16u] = 0xffffffffu;
    runtime.events.clear();
    runtime.fail_xml = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::EffectCameraObject::Construct(
            failed, 0x77u, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<std::string>{
              "reset-base", "xml-init", "field64", "finalize-base"},
          "failed XML initialization unwinds field and base only");

    runtime.fail_xml = false;
    runtime.fail_system = true;
    runtime.events.clear();
    threw = false;
    try {
        static_cast<void>(pal4::EffectCameraObject::Construct(
            failed, 0x77u, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<std::string>{
              "reset-base", "xml-init", "system", "xml-destroy",
              "field64", "finalize-base"},
          "failure after XML creation destroys XML then base resources");

    pal4::EffectCameraObjectState controls{};
    controls.words[2u] = 0xa5000000u;
    controls.words[31u] = 0xdeadbeefu;
    Check(pal4::EffectCameraObject::CachedWord124(controls) ==
              0xdeadbeefu &&
              pal4::EffectCameraObject::ClassName() == "CameraObject" &&
              pal4::EffectCameraObject::SetStartTaskFlag(
                  controls, 1u, runtime) == 0u &&
              runtime.start_calls == 0u,
          "cached word, native name and inactive flag gate");
    controls.words[1u] = 0x12340001u;
    Check(pal4::EffectCameraObject::SetStartTaskFlag(
              controls, 1u, runtime) == runtime.start_result &&
              controls.words[2u] == 0x01000000u &&
              runtime.start_calls == 1u,
          "enabling start flag writes byte and invokes facet");
    Check(pal4::EffectCameraObject::SetStartTaskFlag(
              controls, 1u, runtime) == 1u &&
              pal4::EffectCameraObject::SetStartTaskFlag(
                  controls, 0u, runtime) == 0u &&
              controls.words[2u] == 0u && runtime.start_calls == 1u,
          "unchanged flag and disable avoid facet callback");

    pal4::EffectCameraObjectState properties{};
    properties.words[12u] = 0x111u;
    properties.words[13u] = 0x222u;
    properties.words[14u] = 0x333u;
    const std::array<std::uint32_t, 3> look_at{
        0xabcdef01u, 0x12345602u, 0x87654303u};
    Check(pal4::EffectCameraObject::SetLookAtPosition(
              properties, look_at) == 0u &&
              properties.words[12u] == 0x111u,
          "inactive and invisible LookAt setter is gated");
    properties.words[20u] = 0x00000001u;
    Check(pal4::EffectCameraObject::SetLookAtPosition(
              properties, look_at) == 0x03u &&
              properties.words[12u] == look_at[0u] &&
              properties.words[13u] == look_at[1u] &&
              properties.words[14u] == look_at[2u],
          "visible facet writes raw LookAt coordinate words");
    properties.words[20u] = 0u;
    const std::array<std::uint32_t, 3> rotation{
        0x12345601u, 0x87654302u, 0xabcdefa5u};
    Check(pal4::EffectCameraObject::SetRotation(
              properties, rotation) == 0u &&
              properties.words[22u] == 0u,
          "rotation setter requires active camera");
    properties.words[1u] = 1u;
    Check(pal4::EffectCameraObject::SetRotation(
              properties, rotation) == 0u &&
              properties.words[22u] == 0u,
          "rotation setter requires ready camera");
    properties.words[20u] = 1u;
    Check(pal4::EffectCameraObject::SetRotation(
              properties, rotation) == 0xa5u &&
              properties.words[22u] == rotation[0u] &&
              properties.words[23u] == rotation[1u] &&
              properties.words[24u] == rotation[2u],
          "ready camera stores raw rotation words in order");
    properties.words[20u] = 0u;
    properties.words[11u] = 0x66000000u;
    properties.words[28u] = 0x11111111u;
    properties.words[29u] = 0x22222222u;
    properties.words[30u] = 0x333333a5u;
    Check(pal4::EffectCameraObject::InheritCurrentFlag(properties) == 0u &&
              pal4::EffectCameraObject::LockLookAtFlag(properties) == 0x66u &&
              pal4::EffectCameraObject::SetLockLookAtFlag(
                  properties, 1u) == 0xa5u &&
              pal4::EffectCameraObject::LockLookAtFlag(properties) == 1u &&
              properties.words[22u] == properties.words[28u] &&
              properties.words[23u] == properties.words[29u] &&
              properties.words[24u] == properties.words[30u],
          "enabling lock copies cached rotation and returns last low byte");
    Check(pal4::EffectCameraObject::SetLockLookAtFlag(
              properties, 0u) == 0u &&
              properties.words[22u] == 0x11111111u,
          "disabling lock preserves copied rotation");
    properties.words[1u] = 0u;
    Check(pal4::EffectCameraObject::SetLockLookAtFlag(
              properties, 1u) == 0u &&
              pal4::EffectCameraObject::LockLookAtFlag(properties) == 0u,
          "inactive lock setter leaves flag unchanged");
}
