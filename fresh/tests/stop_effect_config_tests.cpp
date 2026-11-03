#include "pal4/stop_effect_task.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IStopEffectConfigRuntime {
    std::vector<std::string> keys;
    std::int32_t value = 0;
    std::int32_t ReadInt(std::uint32_t config, std::string_view key,
                         std::int32_t fallback) override {
        Check(config == 0x5000u && fallback == 0,
              "integer config and zero fallback");
        keys.emplace_back(key); return value;
    }
};
}  // namespace

int main() {
    pal4::StopEffectTaskState state{};
    state.words.fill(0xabcdef01u);
    Runtime runtime;
    Check(!pal4::StopEffectTask::LoadConfig(state, 0u, runtime) &&
          runtime.keys.empty() && state.words[18u] == 0xabcdef01u,
          "null config preserves all task fields");
    runtime.value = 3;
    Check(pal4::StopEffectTask::LoadConfig(state, 0x5000u, runtime) &&
          runtime.keys == std::vector<std::string>{
              "Attr_Task_StopEffect_Smooth"} &&
          (state.words[18u] & 0xffu) == 1u &&
          (state.words[18u] & 0xffffff00u) == 0xabcdef00u &&
          state.words[19u] == 0u && state.words[20u] == 0u &&
          state.words[21u] == 0xabcdef01u &&
          state.words[22u] == 0xabcdef01u,
          "positive flag and timer reset preserve neighbors");
    runtime.value = -1;
    Check(pal4::StopEffectTask::LoadConfig(state, 0x5000u, runtime) &&
          (state.words[18u] & 0xffu) == 0u,
          "nonpositive smooth flag is false");
    runtime.value = 0;
    Check(pal4::StopEffectTask::LoadConfig(state, 0x5000u, runtime) &&
          runtime.keys.size() == 3u && (state.words[18u] & 0xffu) == 0u,
          "missing smooth key falls back to zero");
}
