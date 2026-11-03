#include "pal4/effect_rotate_task.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}
float Get(const pal4::EffectRotateTaskState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}

struct Runtime final : pal4::IEffectRotateConfigRuntime {
    std::vector<std::string> keys;
    std::map<std::string, std::int32_t> integers;
    std::map<std::string, float> floats;

    std::int32_t ReadInt(std::uint32_t config, std::string_view key,
                         std::int32_t fallback) override {
        Check(config == 0x5000u && fallback == 0, "integer XML boundary");
        keys.emplace_back(key);
        const auto found = integers.find(std::string(key));
        return found == integers.end() ? fallback : found->second;
    }
    float ReadFloat(std::uint32_t config, std::string_view key,
                    float fallback) override {
        Check(config == 0x5000u && fallback == 0.0f, "float XML boundary");
        keys.emplace_back(key);
        const auto found = floats.find(std::string(key));
        return found == floats.end() ? fallback : found->second;
    }
};

const std::vector<std::string> kKeys{
    "Attr_Task_Rotate_Result", "Attr_Task_Rotate_x", "Attr_Task_Rotate_y",
    "Attr_Task_Rotate_z", "Attr_Add_Vel", "Attr_Add_Vel_x",
    "Attr_Add_Vel_y", "Attr_Add_Vel_z", "Attr_Inherit_Pre_Task",
    "Attr_Task_Rotate_Rollback", "Attr_Task_Rotate_Time_Bias",
    "Attr_Time_Interval", "Attr_Time_Interval2",
    "Attr_Task_Rotate_LockedLookAt"};

}  // namespace

int main() {
    pal4::EffectRotateTaskState state{};
    state.words.fill(0xabcdef01u);
    Runtime runtime;
    Check(!pal4::EffectRotateTask::LoadConfig(state, 0u, runtime) &&
          runtime.keys.empty() && state.words[19u] == 0xabcdef01u,
          "null configuration makes no mutations");

    runtime.integers = {
        {"Attr_Task_Rotate_Result", 3}, {"Attr_Add_Vel", 1},
        {"Attr_Inherit_Pre_Task", -2}, {"Attr_Task_Rotate_Rollback", 5},
        {"Attr_Task_Rotate_LockedLookAt", 0}};
    runtime.floats = {
        {"Attr_Task_Rotate_x", 1.0f}, {"Attr_Task_Rotate_y", 2.0f},
        {"Attr_Task_Rotate_z", 3.0f}, {"Attr_Add_Vel_x", 4.0f},
        {"Attr_Add_Vel_y", 5.0f}, {"Attr_Add_Vel_z", 6.0f},
        {"Attr_Task_Rotate_Time_Bias", 7.0f},
        {"Attr_Time_Interval", 8.0f}, {"Attr_Time_Interval2", 9.0f}};
    Check(pal4::EffectRotateTask::LoadConfig(state, 0x5000u, runtime) &&
          runtime.keys == kKeys && state.words[19u] == 0u,
          "all fourteen keys in IDA order and transient reset");
    for (std::size_t i = 0; i < 3u; ++i) {
        Check(Get(state, 20u + i) == float(i + 1u), "target rotation");
        Check(Get(state, 23u + i) == float(i + 4u), "added velocity");
    }
    Check(Get(state, 36u) == 7.0f && Get(state, 38u) == 8.0f &&
          Get(state, 41u) == 9.0f, "bias and both wait intervals");
    Check((state.words[18u] & 0xffffffu) == 0x000101u &&
          ((state.words[15u] >> 8u) & 0xffu) == 0u &&
          (state.words[33u] & 0xffu) == 1u &&
          ((state.words[33u] >> 8u) & 0xffu) == 0xefu,
          "positive and nonpositive flags preserve neighboring bytes");
    Check(state.words[34u] == 0xabcdef01u &&
          state.words[37u] == 0xabcdef01u,
          "config preserves animation state");

    pal4::EffectRotateTaskState defaults{};
    defaults.words.fill(0xabcdef01u);
    runtime.integers.clear(); runtime.floats.clear(); runtime.keys.clear();
    Check(pal4::EffectRotateTask::LoadConfig(defaults, 0x5000u, runtime) &&
          runtime.keys == kKeys, "missing XML keys still queried in order");
    for (auto word : {19u, 20u, 21u, 22u, 23u, 24u, 25u,
                      36u, 38u, 41u})
        Check(defaults.words[word] == 0u, "missing numeric default is zero");
    Check((defaults.words[18u] & 0xffffffu) == 0u &&
          (defaults.words[33u] & 0xffu) == 0u &&
          ((defaults.words[15u] >> 8u) & 0xffu) == 0u,
          "missing predicate defaults to false");
}
