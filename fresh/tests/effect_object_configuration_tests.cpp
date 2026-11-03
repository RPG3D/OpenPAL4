#include "pal4/effect_object_configuration.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
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

struct Runtime final : pal4::IEffectObjectConfigurationRuntime {
    std::vector<std::string> calls;
    std::vector<std::string> keys;
    std::unordered_map<std::string, float> overrides;
    std::uint32_t child = 0x2000u;
    bool init_result = true;
    bool update_result = true;

    float ReadFloat(std::uint32_t document, std::string_view key,
                    float default_value) override {
        Check(document == 0x1000u, "read attributes from caller document");
        keys.emplace_back(key);
        calls.push_back("read");
        const auto found = overrides.find(std::string(key));
        return found == overrides.end() ? default_value : found->second;
    }
    std::uint32_t GetChildByIndex(std::uint32_t document,
                                  std::uint32_t index) override {
        Check(document == 0x1000u && index == 0u,
              "first child is queried after attributes");
        calls.push_back("child");
        return child;
    }
    std::uint32_t CreateInstance() override {
        calls.push_back("create_instance");
        return 0x3000u;
    }
    bool InitializeInstance(std::uint32_t instance,
                            std::uint32_t input_child) override {
        Check(instance == 0x3000u && input_child == child,
              "child initializes newly created instance");
        calls.push_back("initialize");
        return init_result;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x300cu && value == 0x5000u,
              "EffectSystem owner word propagates to instance +12");
        calls.push_back("write_owner");
    }
    bool UpdateInstance(std::uint32_t instance) override {
        Check(instance == 0x3000u, "update newly initialized instance");
        calls.push_back("update_instance");
        return update_result;
    }
    void CleanupInstance(std::uint32_t instance,
                         std::uint32_t context) override {
        Check(instance == 0x3000u && context == 0x6000u,
              "failure cleanup receives native context");
        calls.push_back("cleanup");
    }
    void DestroyInstance(std::uint32_t instance,
                         std::uint32_t context) override {
        Check(instance == 0x3000u && context == 0x6000u,
              "failure destruction receives native context");
        calls.push_back("destroy");
    }
    std::uint32_t CreateFrame() override {
        calls.push_back("create_frame");
        return 0x4000u;
    }
    void UpdateFrameProperties(pal4::EffectObjectState& state) override {
        Check(state.words[24u] == 0x4000u &&
                  state.words[33u] == state.words[13u] &&
                  state.words[36u] == state.words[16u] &&
                  state.words[39u] == state.words[19u],
              "frame update follows cache and frame creation");
        calls.push_back("update_frame");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x5053u, "owner visibility byte +83");
        calls.push_back("read_visibility");
        return 0x7bu;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == 0x3053u && value == 0x7bu,
              "instance visibility byte +83");
        calls.push_back("write_visibility");
    }
};

void TestSuccess() {
    pal4::EffectObjectState state{};
    state.words.fill(0xaaaaaaaau);
    state.words[4u] = 0x5000u;
    state.words[3u] = 0x5000u;
    Runtime runtime;
    runtime.overrides["Attr_EffectObject_Position_y"] = 2.5f;
    runtime.overrides["Attr_EffectObject_Scale_z"] = 4.0f;
    runtime.overrides["Attr_EffectObject_Rotate_x"] = -3.0f;
    Check(pal4::EffectObjectConfiguration::Load(
              state, 0x1000u, 0x6000u, runtime) == 1u &&
              runtime.keys == std::vector<std::string>{
                  "Attr_EffectObject_Position_x",
                  "Attr_EffectObject_Position_y",
                  "Attr_EffectObject_Position_z",
                  "Attr_EffectObject_Scale_x",
                  "Attr_EffectObject_Scale_y",
                  "Attr_EffectObject_Scale_z",
                  "Attr_EffectObject_Rotate_x",
                  "Attr_EffectObject_Rotate_y",
                  "Attr_EffectObject_Rotate_z"} &&
              state.words[19u] == 0u &&
              state.words[20u] == std::bit_cast<std::uint32_t>(2.5f) &&
              state.words[21u] == 0u &&
              state.words[13u] == std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[14u] == std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[15u] == std::bit_cast<std::uint32_t>(4.0f) &&
              state.words[16u] == std::bit_cast<std::uint32_t>(-3.0f) &&
              state.words[17u] == 0u && state.words[18u] == 0u,
          "nine XML attributes use exact order, defaults and overrides");
    for (std::uint32_t i = 0u; i != 9u; ++i)
        Check(state.words[33u + i] == state.words[13u + i],
              "successful setup caches all nine transform words");
    Check(state.words[23u] == 0x3000u && state.words[24u] == 0x4000u &&
              (state.words[1u] & 0xffu) == 1u &&
              ((state.words[2u] >> 24u) & 0xffu) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read", "read", "read", "read", "read", "read",
                  "read", "read", "read", "child", "create_instance",
                  "initialize", "write_owner", "update_instance",
                  "create_frame", "update_frame", "read_visibility",
                  "write_visibility"},
          "success creates instance/frame then copies owner visibility");
}

void TestFailures() {
    pal4::EffectObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    Check(pal4::EffectObjectConfiguration::Load(
              state, 0u, 0x6000u, runtime) == 0u &&
              runtime.calls.empty() && state.words[23u] == 0xaaaaaaaau,
          "null document leaves state untouched");
    state.words[4u] = 0x5000u;
    runtime.child = 0u;
    Check(pal4::EffectObjectConfiguration::Load(
              state, 0x1000u, 0x6000u, runtime) == 0u &&
              state.words[23u] == 0u &&
              runtime.calls.back() == "destroy" &&
              runtime.calls[runtime.calls.size() - 3u] == "create_instance",
          "missing child still creates and releases instance");
    runtime.calls.clear();
    runtime.child = 0x2000u;
    runtime.init_result = false;
    Check(pal4::EffectObjectConfiguration::Load(
              state, 0x1000u, 0x6000u, runtime) == 0u &&
              state.words[23u] == 0u &&
              runtime.calls[runtime.calls.size() - 3u] == "initialize" &&
              runtime.calls.back() == "destroy",
          "initialization failure cleans and destroys instance");
    runtime.calls.clear();
    runtime.init_result = true;
    runtime.update_result = false;
    Check(pal4::EffectObjectConfiguration::Load(
              state, 0x1000u, 0x6000u, runtime) == 0u &&
              state.words[23u] == 0u &&
              runtime.calls[runtime.calls.size() - 3u] == "update_instance" &&
              runtime.calls.back() == "destroy" &&
              state.words[24u] == 0xaaaaaaaau,
          "update failure cleans instance before frame creation");
}

}  // namespace

int main() {
    TestSuccess();
    TestFailures();
}
