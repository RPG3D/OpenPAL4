#include "pal4/wobble_task.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IWobbleTaskRuntime {
    std::vector<std::string> calls;
    std::uint32_t ticks = 1000u;
    float movement = 0.0f;
    void ConstructBase(pal4::WobbleTaskState& state) override {
        calls.emplace_back("construct"); state.words[1u] = 0x1234u;
    }
    void DestroyBase(pal4::WobbleTaskState& state) override {
        Check(state.words[0u] == pal4::WobbleTask::kVtable,
              "derived vtable before base cleanup");
        calls.emplace_back("destroy");
    }
    std::uint32_t ResolvePositionFacet(std::uint32_t owner,
                                       std::string_view name) override {
        Check(owner == 0x5000u && name == "PositionTask",
              "wobble resolves PositionTask facet");
        calls.emplace_back("resolve"); return 0x6000u;
    }
    std::uint32_t TickCountMilliseconds() override {
        calls.emplace_back("ticks"); return ticks;
    }
    void MovePosition(std::uint32_t facet, float x, float y,
                      float z, bool incremental) override {
        Check(facet == 0x6000u && x == 0.0f && z == 0.0f && incremental,
              "vertical incremental movement");
        calls.emplace_back("move"); movement = y;
    }
};
struct Config final : pal4::IWobbleTaskConfigRuntime {
    std::vector<std::string> keys;
    int inherit = 0;
    float range = 20.0f;
    float fps = 0.0f;
    std::int32_t ReadInt(std::uint32_t config, std::string_view key,
                         std::int32_t fallback) override {
        Check(config == 0x7000u && key == "Attr_Inherit_Pre_Task" &&
              fallback == 0, "inherit key and fallback");
        keys.emplace_back(key); return inherit;
    }
    float ReadFloat(std::uint32_t config, std::string_view key,
                    float fallback) override {
        Check(config == 0x7000u && fallback == 0.0f,
              "float config fallback");
        keys.emplace_back(key);
        if (key == "Attr_Task_Wobble_Range") return range;
        Check(key == "Attr_Task_Wobble_Fps", "frequency key");
        return fps;
    }
};
std::uint8_t Inherit(const pal4::WobbleTaskState& state) {
    return static_cast<std::uint8_t>(state.words[15u] >> 8u);
}
}  // namespace

int main() {
    pal4::WobbleTaskState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(&pal4::WobbleTask::Construct(state, runtime) == &state &&
          sizeof(state) == 84u && state.words[0u] == 0x84b130u &&
          state.words[1u] == 0x1234u && state.words[19u] == 0xdeadbeefu &&
          state.words[20u] == 0u &&
          pal4::WobbleTask::ClassName() == "WobbleTask",
          "native constructor, size and class name");
    Config config;
    Check(!pal4::WobbleTask::LoadConfig(state, 0u, config) &&
          config.keys.empty(), "null config preserves state");
    Check(pal4::WobbleTask::LoadConfig(state, 0x7000u, config) &&
          config.keys == std::vector<std::string>{
              "Attr_Inherit_Pre_Task", "Attr_Task_Wobble_Range",
              "Attr_Task_Wobble_Fps"} && Inherit(state) == 0u &&
          std::bit_cast<float>(state.words[19u]) == 20.0f &&
          std::bit_cast<float>(state.words[18u]) == 10.0f,
          "config order and nonpositive frequency fallback");
    config.inherit = 2;
    config.fps = 4.0f;
    Check(pal4::WobbleTask::LoadConfig(state, 0x7000u, config) &&
          Inherit(state) == 1u &&
          std::bit_cast<float>(state.words[18u]) == 4.0f,
          "positive frequency and inherit flag");
    state.words[16u] = 0x5000u;
    runtime.calls.clear();
    Check(pal4::WobbleTask::Start(state, runtime) == 1u &&
          state.words[20u] == 0x6000u &&
          runtime.calls == std::vector<std::string>{"resolve"},
          "start resolves position facet");
    runtime.calls.clear();
    Check(pal4::WobbleTask::Update(state, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{"ticks", "move"} &&
          std::fabs(runtime.movement - std::sin(4.0) * 20.0) < 0.0001,
          "tick-driven sinusoidal vertical motion");
    runtime.calls.clear();
    Check(pal4::WobbleTask::Stop(state) == 1u &&
          state.words[20u] == 0u && runtime.calls.empty(),
          "stop clears facet without dispatch");
    state.words[0u] = 0u;
    Check(&pal4::WobbleTask::Destroy(state, runtime) == &state &&
          state.words[0u] == 0x84b130u && runtime.calls.back() == "destroy",
          "destructor restores derived vtable");
}
