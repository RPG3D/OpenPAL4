#include "pal4/uv_rotate_task.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IUVRotateTaskRuntime {
    std::vector<std::string> calls;
    std::array<float, 3u> axis{};
    float step = 0.0f;
    std::int32_t layer = 0;
    void ConstructBase(pal4::UVRotateTaskState& state) override {
        calls.emplace_back("construct"); state.words[1u] = 0x1234u;
    }
    void DestroyBase(pal4::UVRotateTaskState& state) override {
        Check(state.words[0u] == pal4::UVRotateTask::kVtable,
              "derived vtable before base cleanup");
        calls.emplace_back("destroy");
    }
    std::uint32_t ResolveFacet(std::uint32_t owner,
                               std::string_view name) override {
        Check(owner == 0x5000u && name == "UVRotateTask", "facet class key");
        calls.emplace_back("resolve"); return 0x6000u;
    }
    void StartRotation(std::uint32_t facet, std::int32_t value) override {
        Check(facet == 0x6000u, "start facet");
        layer = value; calls.emplace_back("start");
    }
    void UpdateRotation(std::uint32_t facet, std::int32_t value,
                        const std::array<float, 3u>& direction,
                        float angular_step) override {
        Check(facet == 0x6000u, "update facet");
        layer = value; axis = direction; step = angular_step;
        calls.emplace_back("update");
    }
    void StopRotation(std::uint32_t facet, std::int32_t value) override {
        Check(facet == 0x6000u, "stop facet");
        layer = value; calls.emplace_back("stop");
    }
};
struct Config final : pal4::IUVRotateTaskConfigRuntime {
    std::vector<std::string> keys;
    float ReadFloat(std::uint32_t config, std::string_view key,
                    float fallback) override {
        Check(config == 0x7000u && fallback == 0.0f, "float fallback");
        keys.emplace_back(key);
        return static_cast<float>(keys.size());
    }
    std::int32_t ReadInt(std::uint32_t config, std::string_view key,
                         std::int32_t fallback) override {
        Check(config == 0x7000u && fallback == 0, "integer fallback");
        keys.emplace_back(key); return -3;
    }
};
}  // namespace

int main() {
    pal4::UVRotateTaskState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(&pal4::UVRotateTask::Construct(state, runtime) == &state &&
          sizeof(state) == 96u && state.words[0u] == 0x84b0b0u &&
          state.words[1u] == 0x1234u && state.words[17u] == 0xdeadbeefu &&
          state.words[18u] == 0u && state.words[19u] == 0u &&
          state.words[20u] == 0u && state.words[21u] == 0u &&
          state.words[22u] == 0u && state.words[23u] == 0u &&
          pal4::UVRotateTask::ClassName() == "UVRotateTask",
          "native constructor, layout and name");
    Config config;
    Check(!pal4::UVRotateTask::LoadConfig(state, 0u, config) &&
          config.keys.empty(), "null config leaves state untouched");
    Check(pal4::UVRotateTask::LoadConfig(state, 0x7000u, config) &&
          config.keys == std::vector<std::string>{
              "Attr_Task_UVRotate_Axis_X", "Attr_Task_UVRotate_Axis_Y",
              "Attr_Task_UVRotate_Axis_Z", "Attr_Task_UVROtate_Angel",
              "Attr_Task_UVROtate_Texture_Layer"} &&
          std::bit_cast<float>(state.words[18u]) == 1.0f &&
          std::bit_cast<float>(state.words[21u]) == 4.0f &&
          static_cast<std::int32_t>(state.words[22u]) == -3,
          "five config fields preserve IDA spelling and offsets");
    state.words[16u] = 0x5000u;
    runtime.calls.clear();
    Check(pal4::UVRotateTask::Start(state, runtime) == 1u &&
          state.words[23u] == 0x6000u && runtime.layer == -3 &&
          runtime.calls == std::vector<std::string>{"resolve", "start"},
          "start resolves facet and starts selected layer");
    runtime.calls.clear();
    Check(pal4::UVRotateTask::Update(state, 0.25f, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{"update"} &&
          runtime.axis == std::array<float, 3u>{1.0f, 2.0f, 3.0f} &&
          runtime.step == 1.0f && runtime.layer == -3,
          "update multiplies elapsed time by angular speed");
    Check(pal4::UVRotateTask::Stop(state, runtime) == 1u &&
          state.words[23u] == 0u && runtime.layer == -3 &&
          runtime.calls.back() == "stop", "stop clears facet after dispatch");
    state.words[0u] = 0u;
    Check(&pal4::UVRotateTask::Destroy(state, runtime) == &state &&
          state.words[0u] == 0x84b0b0u && runtime.calls.back() == "destroy",
          "destructor restores derived vtable");
}
