#include "pal4/uv_translate_task.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IUVTranslateTaskRuntime {
    std::vector<std::string> calls;
    std::array<float, 3u> delta{};
    std::int32_t layer = 0;
    void ConstructBase(pal4::UVTranslateTaskState& state) override {
        calls.emplace_back("construct"); state.words[1u] = 0x1234u;
    }
    void DestroyBase(pal4::UVTranslateTaskState& state) override {
        Check(state.words[0u] == pal4::UVTranslateTask::kVtable,
              "derived vtable before base cleanup");
        calls.emplace_back("destroy");
    }
    std::uint32_t ResolveFacet(std::uint32_t owner,
                               std::string_view name) override {
        Check(owner == 0x5000u && name == "UVTranslateTask",
              "facet class key");
        calls.emplace_back("resolve"); return 0x6000u;
    }
    void StartTranslation(std::uint32_t facet, std::int32_t value) override {
        Check(facet == 0x6000u, "start facet");
        layer = value; calls.emplace_back("start");
    }
    void UpdateTranslation(std::uint32_t facet, std::int32_t value,
                           const std::array<float, 3u>& movement) override {
        Check(facet == 0x6000u, "update facet");
        layer = value; delta = movement; calls.emplace_back("update");
    }
    void StopTranslation(std::uint32_t facet, std::int32_t value) override {
        Check(facet == 0x6000u, "stop facet");
        layer = value; calls.emplace_back("stop");
    }
};
struct Config final : pal4::IUVTranslateTaskConfigRuntime {
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
    pal4::UVTranslateTaskState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(&pal4::UVTranslateTask::Construct(state, runtime) == &state &&
          sizeof(state) == 92u && state.words[0u] == 0x84b0f0u &&
          state.words[1u] == 0x1234u && state.words[17u] == 0xdeadbeefu &&
          state.words[18u] == 0u && state.words[19u] == 0u &&
          state.words[20u] == 0u && state.words[21u] == 0u &&
          state.words[22u] == 0u &&
          pal4::UVTranslateTask::ClassName() == "UVTranslateTask",
          "native constructor, layout and name");
    Config config;
    Check(!pal4::UVTranslateTask::LoadConfig(state, 0u, config) &&
          config.keys.empty(), "null config leaves state untouched");
    Check(pal4::UVTranslateTask::LoadConfig(state, 0x7000u, config) &&
          config.keys == std::vector<std::string>{
              "Attr_Task_UVTranslate_X", "Attr_Task_UVTranslate_Y",
              "Attr_Task_UVTranslate_Z",
              "Attr_Task_UVTranslate_Texture_Layer"} &&
          std::bit_cast<float>(state.words[18u]) == 1.0f &&
          std::bit_cast<float>(state.words[20u]) == 3.0f &&
          static_cast<std::int32_t>(state.words[21u]) == -3,
          "four config fields preserve IDA keys and offsets");
    state.words[16u] = 0x5000u;
    runtime.calls.clear();
    Check(pal4::UVTranslateTask::Start(state, runtime) == 1u &&
          state.words[22u] == 0x6000u && runtime.layer == -3 &&
          runtime.calls == std::vector<std::string>{"resolve", "start"},
          "start resolves facet and selected texture layer");
    runtime.calls.clear();
    Check(pal4::UVTranslateTask::Update(state, 0.25f, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{"update"} &&
          runtime.delta == std::array<float, 3u>{0.25f, 0.5f, 0.75f} &&
          runtime.layer == -3,
          "update scales each translation component by elapsed time");
    Check(pal4::UVTranslateTask::Stop(state, runtime) == 1u &&
          state.words[22u] == 0u && runtime.layer == -3 &&
          runtime.calls.back() == "stop", "stop clears facet after dispatch");
    state.words[0u] = 0u;
    Check(&pal4::UVTranslateTask::Destroy(state, runtime) == &state &&
          state.words[0u] == 0x84b0f0u && runtime.calls.back() == "destroy",
          "destructor restores derived vtable");
}
