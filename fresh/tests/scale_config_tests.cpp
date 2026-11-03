#include "pal4/scale_task.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IScaleTaskConfigRuntime {
    std::vector<std::string> keys;
    std::map<std::string, int> ints;
    std::map<std::string, float> floats;
    int ReadInt(std::uint32_t config, std::string_view key, int fallback) override {
        Check(config == 0x5000u && fallback == 0, "integer read contract");
        keys.emplace_back(key);
        const auto it = ints.find(std::string(key));
        return it == ints.end() ? fallback : it->second;
    }
    float ReadFloat(std::uint32_t config, std::string_view key,
                    float fallback) override {
        Check(config == 0x5000u && fallback == 0.0f, "float read contract");
        keys.emplace_back(key);
        const auto it = floats.find(std::string(key));
        return it == floats.end() ? fallback : it->second;
    }
};
const std::vector<std::string> kKeys{
    "Attr_Task_Scale_Result", "Attr_Task_Scale_x", "Attr_Task_Scale_y",
    "Attr_Task_Scale_z", "Attr_Inherit_Pre_Task",
    "Attr_Task_Scale_Rollback", "Attr_Task_Scale_Time_Bias",
    "Attr_Time_Interval", "Attr_Time_Interval2"};
}  // namespace

int main() {
    pal4::ScaleTaskState state{};
    state.words.fill(0xabcdef01u);
    Runtime runtime;
    Check(!pal4::ScaleTask::LoadConfig(state, 0u, runtime) &&
          runtime.keys.empty() && state.words[18u] == 0xabcdef01u,
          "null config leaves state unchanged");
    runtime.ints = {{"Attr_Task_Scale_Result", 3},
                    {"Attr_Inherit_Pre_Task", -2},
                    {"Attr_Task_Scale_Rollback", 1}};
    runtime.floats = {{"Attr_Task_Scale_x", 1.0f},
                      {"Attr_Task_Scale_y", 2.0f},
                      {"Attr_Task_Scale_z", 3.0f},
                      {"Attr_Task_Scale_Time_Bias", 4.0f},
                      {"Attr_Time_Interval", 5.0f},
                      {"Attr_Time_Interval2", 6.0f}};
    Check(pal4::ScaleTask::LoadConfig(state, 0x5000u, runtime) &&
          runtime.keys == kKeys, "nine IDA keys in order");
    for (std::size_t i = 0; i < 3u; ++i)
        Check(std::bit_cast<float>(state.words[22u + i]) == float(i + 1u),
              "target scale");
    Check(std::bit_cast<float>(state.words[29u]) == 4.0f &&
          std::bit_cast<float>(state.words[31u]) == 5.0f &&
          std::bit_cast<float>(state.words[34u]) == 6.0f,
          "bias and intervals");
    Check((state.words[18u] & 0xffu) == 1u &&
          ((state.words[15u] >> 8u) & 0xffu) == 0u &&
          (state.words[26u] & 0xffu) == 1u &&
          ((state.words[26u] >> 8u) & 0xffu) == 0xefu,
          "positive flags and neighboring byte preservation");
    Check(state.words[27u] == 0xabcdef01u,
          "configuration preserves elapsed animation state");
    pal4::ScaleTaskState missing{};
    missing.words.fill(0xabcdef01u);
    runtime = Runtime{};
    Check(pal4::ScaleTask::LoadConfig(missing, 0x5000u, runtime) &&
          runtime.keys == kKeys, "missing keys are still read");
    for (auto word : {22u, 23u, 24u, 29u, 31u, 34u})
        Check(missing.words[word] == 0u, "missing float defaults to zero");
    Check((missing.words[18u] & 0xffu) == 0u &&
          (missing.words[26u] & 0xffu) == 0u &&
          ((missing.words[15u] >> 8u) & 0xffu) == 0u,
          "missing flags default false");
}
