#include "pal4/slow_motion_task.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::ISlowMotionTaskRuntime,
                       pal4::ISlowMotionConfigRuntime {
    std::vector<std::string> calls;
    std::int32_t id = -1;
    float configured = 0.5f;
    std::uint32_t facet = 0x6000u;
    std::uint32_t seen_facet = 0u;
    std::int32_t seen_id = 0;
    float seen_scale = 0.0f;

    std::uint32_t ResolveFacet(std::uint32_t owner,
                                std::string_view name) override {
        Check(owner == 0x5000u && name == "MessageTask",
              "resolve MessageTask through owner +64");
        calls.emplace_back("resolve"); return facet;
    }
    std::int32_t ReadOwnerId(std::uint32_t owner, std::string_view key,
                             std::int32_t fallback) override {
        Check(owner == 0x5000u && key == "ID" && fallback == -1,
              "owner XML ID and native fallback");
        calls.emplace_back("read-id"); return id;
    }
    void ApplyScaleTime(std::uint32_t value, std::int32_t object_id,
                        float scale_time) override {
        calls.emplace_back("apply");
        seen_facet = value; seen_id = object_id; seen_scale = scale_time;
    }
    float ReadFloat(std::uint32_t config, std::string_view key,
                    float fallback) override {
        Check(config == 0x7000u &&
              key == "Attr_Task_SlowMotion_ScaleTime" && fallback == 0.0f,
              "config key and zero fallback");
        calls.emplace_back("read-scale"); return configured;
    }
};
}  // namespace

int main() {
    pal4::SlowMotionTaskState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(sizeof(state) == 80u &&
          pal4::SlowMotionTask::kVtable == 0x84b318u,
          "native object layout and vtable");
    Check(!pal4::SlowMotionTask::LoadConfig(state, 0u, runtime) &&
          state.words[18u] == 0xdeadbeefu && runtime.calls.empty(),
          "null config leaves state unchanged");
    Check(pal4::SlowMotionTask::LoadConfig(state, 0x7000u, runtime) &&
          std::bit_cast<float>(state.words[18u]) == 0.5f &&
          state.words[19u] == 0xdeadbeefu,
          "configured scale at +72 preserves facet");
    state.words[16u] = 0x5000u;
    runtime.calls.clear();
    runtime.id = 42;
    Check(pal4::SlowMotionTask::Start(state, runtime) == 1u &&
          state.words[19u] == 0x6000u &&
          runtime.calls == std::vector<std::string>{
              "resolve", "read-id", "apply"} &&
          runtime.seen_facet == 0x6000u && runtime.seen_id == 42 &&
          runtime.seen_scale == 0.5f,
          "start dispatches ID and scale through MessageTask facet");

    runtime.calls.clear();
    runtime.id = -1;
    runtime.facet = 0u;
    Check(pal4::SlowMotionTask::Start(state, runtime) == 1u &&
          state.words[19u] == 0u && runtime.seen_facet == 0u &&
          runtime.seen_id == -1 && runtime.calls.size() == 3u,
          "missing ID and facet follow native dispatch path");
    Check(pal4::SlowMotionTask::Reset(state) == 0u &&
          state.words[18u] == 0u && state.words[19u] == 0u &&
          state.words[17u] == 0xdeadbeefu,
          "reset clears only scale and facet");

    runtime.calls.clear();
    runtime.configured = 0.0f;
    Check(pal4::SlowMotionTask::LoadConfig(state, 0x7000u, runtime) &&
          state.words[18u] == 0u &&
          runtime.calls == std::vector<std::string>{"read-scale"},
          "missing scale attribute uses zero fallback");
}
