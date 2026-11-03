#include "pal4/effect_camera_configuration.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
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

std::uint8_t Byte(const pal4::EffectCameraObjectState& state,
                  std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

struct Runtime final : pal4::IEffectCameraConfigurationRuntime {
    std::vector<std::string> events;
    std::unordered_map<std::string, std::int32_t> ints;
    std::unordered_map<std::string, float> floats;
    std::unordered_map<std::string, float> tree_floats;
    std::vector<std::uint32_t> axes;
    std::vector<float> angles;
    bool fail_assign_name = false;

    std::int32_t ReadInt(std::uint32_t document, std::string_view key,
                         std::int32_t fallback) override {
        Check(document == 0x5000u && fallback == 0,
              "integer document and fallback");
        events.push_back("int:" + std::string(key));
        const auto it = ints.find(std::string(key));
        return it == ints.end() ? fallback : it->second;
    }
    float ReadFloat(std::uint32_t document, std::string_view key,
                    float fallback) override {
        Check(document == 0x5000u, "float document");
        events.push_back("float:" + std::string(key));
        const auto it = floats.find(std::string(key));
        return it == floats.end() ? fallback : it->second;
    }
    float ReadTreeFloat(std::uint32_t document, std::string_view key,
                        float fallback) override {
        Check(document == 0x5000u &&
                  fallback == (key == "Attr_Camera_RandRang" ?
                      1.0f : 0.0f),
              "tree float document and fallback");
        events.push_back("tree:" + std::string(key));
        const auto it = tree_floats.find(std::string(key));
        return it == tree_floats.end() ? fallback : it->second;
    }
    std::uint32_t ReadTargetName(std::uint32_t document,
                                  std::string_view key,
                                  std::uint32_t fallback) override {
        Check(document == 0x5000u &&
                  key == "Attr_Camera_Target_Name" &&
                  fallback == 0x8e425cu, "target name inputs");
        events.push_back("name-read");
        return 0x7000u;
    }
    void AssignTargetName(pal4::EffectCameraObjectState& state,
                          std::uint32_t temporary) override {
        Check(temporary == 0x7000u && events.size() >= 4u &&
                  events[events.size() - 4u] ==
                      "int:Attr_Camera_Inherit_Current" &&
                  events.back() == "name-read",
              "target name follows flag reads");
        events.push_back("name-assign");
        if (fail_assign_name)
            throw std::runtime_error("name assignment");
    }
    void ReleaseTargetName(std::uint32_t temporary) override {
        Check(temporary == 0x7000u, "target temporary cleanup");
        events.push_back("name-release");
    }
    std::uint32_t CreateFrame() override {
        events.push_back("frame-create");
        return 0x8000u;
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x8000u, "identity frame");
        events.push_back("frame-identity");
    }
    void RotateFrame(std::uint32_t frame, std::uint32_t axis,
                     float angle, std::uint32_t mode) override {
        Check(frame == 0x8000u && mode == 2u,
              "native RenderWare frame and mode");
        events.push_back("frame-rotate");
        axes.push_back(axis);
        angles.push_back(angle);
    }
    void UpdateFrameObjects(std::uint32_t frame) override {
        Check(frame == 0x8000u, "frame update address");
        events.push_back("frame-update");
    }
    void AssignXml(pal4::EffectCameraObjectState& state,
                   std::uint32_t document) override {
        Check(document == 0x5000u && state.words[31u] == 0x8000u &&
                  Byte(state, 4u) == 1u && Byte(state, 11u) == 0u,
              "XML assignment follows frame setup and active flag");
        events.push_back("xml-assign");
    }
    std::uint32_t CachedDocument(
        pal4::EffectCameraObjectState& state) override {
        Check(Byte(state, 4u) == 1u && state.words[31u] == 0x8000u,
              "cached document resolution follows active frame");
        events.push_back("cached-document");
        return 0x5000u;
    }
    void StartCameraFacet(pal4::EffectCameraObjectState& state,
                          std::uint32_t offset) override {
        Check(offset == 24u && Byte(state, 4u) == 1u,
              "reload starts the +24 facet while active");
        events.push_back("facet-start");
    }
};

void TestNullAndDefault() {
    Runtime runtime;
    pal4::EffectCameraObjectState state{};
    Check(!pal4::EffectCameraConfiguration::Load(state, 0u, runtime) &&
              runtime.events.empty(),
          "null document does not mutate or invoke services");
    state.words[20u] = 0xffffffffu;
    state.words[2u] = 0xffffffffu;
    Check(pal4::EffectCameraConfiguration::Load(state, 0x5000u, runtime) &&
              Byte(state, 44u) == 0u && Byte(state, 47u) == 0u &&
              Byte(state, 60u) == 0u && Byte(state, 80u) == 0u &&
              Byte(state, 4u) == 1u && Byte(state, 11u) == 0u &&
              state.words[21u] == Bits(1.0f) &&
              state.words[12u] == Bits(0.0f) &&
              state.words[22u] == Bits(0.0f) &&
              state.words[27u] == Bits(0.0f) &&
              state.words[31u] == 0x8000u,
          "default values and state flags match native offsets");
    const std::vector<std::string> expected{
        "int:Attr_Camera_Inherit_Current",
        "int:Attr_Camera_Lock_LookAt",
        "int:Attr_Camera_Target_Mode",
        "name-read", "name-assign", "name-release",
        "float:Attr_Camera_LookAt_x",
        "float:Attr_Camera_LookAt_y",
        "float:Attr_Camera_LookAt_z",
        "float:Attr_Camera_Pitch",
        "float:Attr_Camera_Yaw",
        "tree:Attr_Camera_Roll",
        "float:Attr_Camera_Position_X",
        "tree:Attr_Camera_Position_Y",
        "float:Attr_Camera_Position_Z",
        "float:Attr_Camera_RandRang",
        "frame-create", "frame-identity",
        "frame-rotate", "frame-rotate", "frame-rotate",
        "frame-update", "xml-assign"};
    Check(runtime.events == expected &&
              runtime.axes == std::vector<std::uint32_t>{
                  0x84a1e8u, 0x84a1f4u, 0x84a200u} &&
              runtime.angles == std::vector<float>{360.0f, 0.0f, 360.0f},
          "configuration lookup and RenderWare setup order");
}

void TestOverridesAndModes() {
    Runtime runtime;
    runtime.ints = {{"Attr_Camera_Inherit_Current", 1},
                    {"Attr_Camera_Lock_LookAt", 1},
                    {"Attr_Camera_Target_Mode", 2}};
    runtime.floats = {{"Attr_Camera_LookAt_x", 1.0f},
                      {"Attr_Camera_LookAt_y", 2.0f},
                      {"Attr_Camera_LookAt_z", 3.0f},
                      {"Attr_Camera_Pitch", 4.0f},
                      {"Attr_Camera_Yaw", 5.0f},
                      {"Attr_Camera_Position_X", 7.0f},
                      {"Attr_Camera_Position_Z", 9.0f},
                      {"Attr_Camera_RandRang", 10.0f}};
    runtime.tree_floats = {{"Attr_Camera_Roll", 6.0f},
                           {"Attr_Camera_Position_Y", 8.0f}};
    pal4::EffectCameraObjectState state{};
    Check(pal4::EffectCameraConfiguration::Load(state, 0x5000u, runtime) &&
              Byte(state, 44u) == 1u && Byte(state, 47u) == 0u &&
              Byte(state, 60u) == 1u &&
              state.words[12u] == Bits(1.0f) &&
              state.words[13u] == Bits(2.0f) &&
              state.words[14u] == Bits(3.0f) &&
              state.words[21u] == Bits(10.0f) &&
              state.words[22u] == 0u && state.words[23u] == 0u &&
              state.words[24u] == 0u && state.words[25u] == 0u &&
              state.words[26u] == Bits(1.0f) &&
              state.words[27u] == Bits(1.0f),
          "inherit mode overrides lock, orientation and position values");

    runtime.ints["Attr_Camera_Inherit_Current"] = 0;
    runtime.events.clear();
    pal4::EffectCameraObjectState locked{};
    Check(pal4::EffectCameraConfiguration::Load(locked, 0x5000u, runtime) &&
              Byte(locked, 44u) == 0u && Byte(locked, 47u) == 1u &&
              locked.words[22u] == 0u && locked.words[23u] == 0u &&
              locked.words[24u] == 0u &&
              locked.words[25u] == Bits(7.0f) &&
              locked.words[26u] == Bits(8.0f) &&
              locked.words[27u] == Bits(9.0f),
          "lock mode clears rotation but preserves position");

    runtime.ints["Attr_Camera_Lock_LookAt"] = -1;
    pal4::EffectCameraObjectState free{};
    Check(pal4::EffectCameraConfiguration::Load(free, 0x5000u, runtime) &&
              Byte(free, 47u) == 0u &&
              free.words[22u] == Bits(4.0f) &&
              free.words[23u] == Bits(5.0f) &&
              free.words[24u] == Bits(6.0f) &&
              free.words[25u] == Bits(7.0f) &&
              free.words[26u] == Bits(8.0f) &&
              free.words[27u] == Bits(9.0f),
          "nonpositive flags preserve parsed rotation and position");
}

void TestNameCleanupOnException() {
    Runtime runtime;
    runtime.fail_assign_name = true;
    runtime.ints["Attr_Camera_Target_Mode"] = 1;
    pal4::EffectCameraObjectState state{};
    bool threw = false;
    try {
        static_cast<void>(pal4::EffectCameraConfiguration::Load(
            state, 0x5000u, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events.back() == "name-release" &&
              runtime.events.size() == 6u,
          "temporary target name is released when assignment throws");
}

void TestReloadUsesCachedTreeAndFrame() {
    Runtime runtime;
    pal4::EffectCameraObjectState inactive{};
    Check(!pal4::EffectCameraConfiguration::Reload(inactive, runtime) &&
              runtime.events.empty(),
          "inactive reload does not resolve XML or touch the frame");

    pal4::EffectCameraObjectState state{};
    state.words[1u] = 1u;
    state.words[31u] = 0x8000u;
    runtime.floats = {{"Attr_Camera_Pitch", 4.0f},
                      {"Attr_Camera_Yaw", 99.0f},
                      {"Attr_Camera_Roll", 6.0f},
                      {"Attr_Camera_Position_X", 7.0f},
                      {"Attr_Camera_Position_Z", 9.0f},
                      {"Attr_Camera_RandRang", 99.0f}};
    runtime.tree_floats = {{"Attr_Camera_Yaw", 5.0f},
                           {"Attr_Camera_Position_Y", 8.0f},
                           {"Attr_Camera_RandRang", 10.0f}};
    Check(pal4::EffectCameraConfiguration::Reload(state, runtime) &&
              runtime.events[0u] == "facet-start" &&
              runtime.events[1u] == "cached-document" &&
              runtime.events.back() == "frame-update" &&
              state.words[21u] == Bits(10.0f) &&
              state.words[22u] == Bits(4.0f) &&
              state.words[23u] == Bits(5.0f) &&
              state.words[24u] == Bits(6.0f) &&
              state.words[25u] == Bits(7.0f) &&
              state.words[26u] == Bits(8.0f) &&
              state.words[27u] == Bits(9.0f) &&
              state.words[31u] == 0x8000u,
          "reload uses tree Yaw/RandRang and preserves existing frame");
    for (const auto& event : runtime.events)
        Check(event != "frame-create" && event != "xml-assign" &&
                  event != "float:Attr_Camera_Yaw" &&
                  event != "float:Attr_Camera_RandRang",
              "reload skips frame allocation, XML assignment and direct Yaw");
}

}  // namespace

int main() {
    TestNullAndDefault();
    TestOverridesAndModes();
    TestNameCleanupOnException();
    TestReloadUsesCachedTreeAndFrame();
}
