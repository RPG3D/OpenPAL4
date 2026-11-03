#include "pal4/rump_shahdow_task.h"

#include <bit>
#include <cmath>
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
float Get(const pal4::RumpShahdowTaskState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}

struct Runtime final : pal4::IRumpShahdowConfigRuntime {
    std::vector<std::string> keys;
    std::map<std::string, float> floats;
    std::map<std::string, std::int32_t> integers;
    float ReadFloat(std::uint32_t config, std::string_view key,
                    float fallback) override {
        Check(config == 0x5000u, "float config handle");
        keys.emplace_back(key);
        const auto found = floats.find(std::string(key));
        return found == floats.end() ? fallback : found->second;
    }
    std::int32_t ReadInt(std::uint32_t config, std::string_view key,
                         std::int32_t fallback) override {
        Check(config == 0x5000u, "integer config handle");
        keys.emplace_back(key);
        const auto found = integers.find(std::string(key));
        return found == integers.end() ? fallback : found->second;
    }
};

const std::vector<std::string> kKeys{
    "Attr_Task_RumpShadow_Life", "Attr_Task_RumpShadow_Fps",
    "Attr_Task_RumpShadow_MaxCount", "Attr_Task_RumpShadow_CheckChange"};

}  // namespace

int main() {
    pal4::RumpShahdowTaskState state{};
    state.words.fill(0xabcdef01u);
    Runtime runtime;
    Check(!pal4::RumpShahdowTask::LoadConfig(state, 0u, runtime) &&
          runtime.keys.empty() && state.words[23u] == 0xabcdef01u,
          "null config leaves state intact");
    Check(pal4::RumpShahdowTask::LoadConfig(state, 0x5000u, runtime) &&
          runtime.keys == kKeys && Get(state, 20u) == 0.5f &&
          std::fabs(Get(state, 21u) - 1.0f / 24.0f) < 0.000001f &&
          state.words[23u] == 5u &&
          (state.words[28u] & 0xffu) == 0u &&
          ((state.words[28u] >> 8u) & 0xffu) == 0xefu,
          "native defaults and neighboring byte preservation");

    runtime.keys.clear();
    runtime.floats = {{"Attr_Task_RumpShadow_Life", -1.0f},
                      {"Attr_Task_RumpShadow_Fps", 0.0f}};
    runtime.integers = {{"Attr_Task_RumpShadow_MaxCount", -4},
                        {"Attr_Task_RumpShadow_CheckChange", 3}};
    Check(pal4::RumpShahdowTask::LoadConfig(state, 0x5000u, runtime) &&
          runtime.keys == kKeys && Get(state, 20u) == 0.5f &&
          std::fabs(Get(state, 21u) - 1.0f / 24.0f) < 0.000001f &&
          state.words[23u] == 0u &&
          (state.words[28u] & 0xffu) == 1u,
          "negative lifetime/count and zero FPS normalize per IDA");

    runtime.keys.clear();
    runtime.floats = {{"Attr_Task_RumpShadow_Life", 2.0f},
                      {"Attr_Task_RumpShadow_Fps", 50.0f}};
    runtime.integers = {{"Attr_Task_RumpShadow_MaxCount", 7},
                        {"Attr_Task_RumpShadow_CheckChange", -1}};
    Check(pal4::RumpShahdowTask::LoadConfig(state, 0x5000u, runtime) &&
          Get(state, 20u) == 2.0f &&
          std::fabs(Get(state, 21u) - 0.02f) < 0.000001f &&
          state.words[23u] == 7u &&
          (state.words[28u] & 0xffu) == 0u &&
          state.words[24u] == 0xabcdef01u,
          "positive settings and nonpositive predicate");
}
