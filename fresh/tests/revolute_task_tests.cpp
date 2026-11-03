#include "pal4/revolute_task.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}
void Put(pal4::RevoluteTaskState& state, std::size_t index, float value) {
    state.words[index] = std::bit_cast<std::uint32_t>(value);
}
float Get(const pal4::RevoluteTaskState& state, std::size_t index) {
    return std::bit_cast<float>(state.words[index]);
}
bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}

struct Runtime final : pal4::IRevoluteTaskRuntime,
                       pal4::IRevoluteConfigRuntime {
    std::vector<std::string> calls;
    std::vector<std::string> keys;
    std::vector<float> float_values;
    std::vector<int> int_values;
    std::array<float, 3u> camera_target{7.0f, 8.0f, 9.0f};
    std::array<float, 3u> origin{};
    std::array<float, 6u> motion{};
    std::uint32_t applied_facet = 0u;
    std::uint32_t camera_facet = 0x3000u;
    bool active = false;
    bool has_target = true;
    int apply_count = 0;

    void ConstructBase(pal4::RevoluteTaskState& state) override {
        calls.emplace_back("construct"); state.words[0u] = 0xdeadbeefu;
    }
    void DestroyBase(pal4::RevoluteTaskState& state) override {
        Check(state.words[0u] == pal4::RevoluteTask::kVtable,
              "base destructor sees derived vtable");
        calls.emplace_back("destroy");
    }
    std::uint32_t ResolveFacet(std::uint32_t owner,
                                std::string_view name) override {
        Check(owner == 0x2000u, "facet owner");
        calls.emplace_back("resolve:" + std::string(name));
        return name == "RevoluteTask" ? 0x4000u : camera_facet;
    }
    bool CameraIsActive(std::uint32_t facet) override {
        Check(facet == camera_facet, "camera facet");
        calls.emplace_back("active"); return active;
    }
    bool CameraHasTarget(std::uint32_t facet) override {
        Check(facet == camera_facet, "camera target facet");
        calls.emplace_back("target"); return has_target;
    }
    void ReadCameraTarget(std::uint32_t facet,
                          std::array<float, 3u>& target) override {
        Check(facet == camera_facet, "camera read facet");
        calls.emplace_back("read-camera"); target = camera_target;
    }
    void ApplyRevolution(std::uint32_t facet,
                         const std::array<float, 3u>& position,
                         const std::array<float, 6u>& movement) override {
        applied_facet = facet; origin = position; motion = movement;
        ++apply_count; calls.emplace_back("apply");
    }
    float ReadFloat(std::uint32_t config, std::string_view key,
                    float fallback) override {
        Check(config == 0x5000u && fallback == 0.0f, "float config boundary");
        keys.emplace_back(key);
        return float_values.empty() ? 0.0f : float_values.at(keys.size() - 1u -
                            (keys.size() > 9u ? 1u : 0u));
    }
    std::int32_t ReadInt(std::uint32_t config, std::string_view key,
                          std::int32_t fallback) override {
        Check(config == 0x5000u && fallback == 0, "integer config boundary");
        keys.emplace_back(key);
        const auto index = static_cast<std::size_t>(
            std::count_if(keys.begin(), keys.end(), [](const auto& item) {
                return item == "Attr_Add_Vel" || item == "Attr_Inherit_Pre_Task" ||
                    item == "Attr_Task_Revolute_Camera_Target" ||
                    item == "Attr_Task_Revolute_Correct_Result";
            }) - 1);
        return int_values.empty() ? 0 : int_values.at(index);
    }
};

void CheckMotion(const Runtime& runtime,
                 const std::array<float, 6u>& expected) {
    for (std::size_t i = 0; i < expected.size(); ++i)
        Check(Near(runtime.motion[i], expected[i]), "facet motion component");
}

}  // namespace

int main() {
    pal4::RevoluteTaskState state{};
    state.words.fill(0xabcdef01u);
    Runtime runtime;
    Check(&pal4::RevoluteTask::Construct(state, runtime) == &state &&
          state.words[0u] == pal4::RevoluteTask::kVtable &&
          runtime.calls == std::vector<std::string>{"construct"},
          "constructor base order and vtable");
    for (auto index : {33u, 24u, 23u, 22u, 27u, 26u, 25u})
        Check(state.words[index] == 0u, "constructor zeroed word");
    Check(state.words[31u] == 0xabcdef01u && state.words[28u] == 0xabcdef01u,
          "constructor preserves other fields");
    Check(pal4::RevoluteTask::ClassName() == "RevoluteTask", "IDA class string");
    Check(!pal4::RevoluteTask::LoadConfig(state, 0u, runtime) &&
          runtime.keys.empty(), "null config leaves state and XML untouched");

    runtime.float_values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    runtime.int_values = {1, -1, 1, 1};
    Check(pal4::RevoluteTask::LoadConfig(state, 0x5000u, runtime),
          "config succeeds");
    Check(runtime.keys == std::vector<std::string>{
          "Attr_Task_Revolute_OriginPos_x", "Attr_Task_Revolute_OriginPos_y",
          "Attr_Task_Revolute_OriginPos_z", "Attr_Task_Revolute_Angle_x",
          "Attr_Task_Revolute_Angle_y", "Attr_Task_Revolute_Angle_z",
          "Attr_Task_Revolute_Center_Vel_x", "Attr_Task_Revolute_Center_Vel_y",
          "Attr_Task_Revolute_Center_Vel_z", "Attr_Add_Vel",
          "Attr_Add_Vel_x", "Attr_Add_Vel_y", "Attr_Add_Vel_z",
          "Attr_Inherit_Pre_Task", "Attr_Task_Revolute_Camera_Target",
          "Attr_Task_Revolute_Correct_Result"}, "ordered config keys");
    for (std::size_t i = 0; i < 9u; ++i)
        Check(Near(Get(state, 22u + i), float(i + 1u)), "rotation config field");
    for (std::size_t i = 0; i < 3u; ++i)
        Check(Near(Get(state, 19u + i), float(i + 10u)), "add velocity field");
    Check((state.words[18u] & 0xffffu) == 0x101u &&
          ((state.words[15u] >> 8u) & 0xffu) == 0u &&
          (state.words[31u] & 0xffu) == 1u,
          "config flags preserve neighboring bytes");

    state.words[16u] = 0x2000u;
    Put(state, 5u, 2.0f);
    runtime.calls.clear();
    Check(pal4::RevoluteTask::Start(state, runtime) == 1u &&
          state.words[33u] == 0x4000u &&
          runtime.calls == std::vector<std::string>{
            "resolve:RevoluteTask", "resolve:StartCameraTask", "active",
            "target", "read-camera"}, "start facet and camera call order");
    Check(Near(Get(state, 19u), 3.0f) && Near(Get(state, 20u), 3.0f) &&
          Near(Get(state, 21u), 3.0f) && Near(Get(state, 22u), 7.0f) &&
          Near(Get(state, 23u), 8.0f) && Near(Get(state, 24u), 9.0f) &&
          state.words[32u] == 0u, "start converts velocity and camera origin");

    runtime.calls.clear();
    Check(pal4::RevoluteTask::Update(state, 0.5f, runtime) == 1u &&
          runtime.applied_facet == 0x4000u &&
          runtime.origin == std::array<float, 3u>{7, 8, 9} &&
          Near(Get(state, 32u), 0.5f), "update dispatch and elapsed time");
    CheckMotion(runtime, {2.75f, 3.25f, 3.75f, 3.5f, 4.0f, 4.5f});
    Check(Near(Get(state, 25u), 5.5f), "update integrates center velocity");

    runtime.calls.clear();
    Check(pal4::RevoluteTask::StopAndCorrect(state, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{"apply"} &&
          state.words[33u] == 0u && state.words[32u] == 0u,
          "stop correction and reset");
    CheckMotion(runtime, {15.0f, 16.5f, 18.0f, 10.5f, 12.0f, 13.5f});

    state.words[33u] = 0x4000u;
    Put(state, 32u, 3.0f);
    runtime.calls.clear();
    (void)pal4::RevoluteTask::StopAndCorrect(state, runtime);
    CheckMotion(runtime, {-8.5f, -9.5f, -10.5f, 7.0f, 8.0f, 9.0f});
    Check(runtime.calls.size() == 1u, "negative remaining time branch");

    state.words[33u] = 0x4000u;
    Put(state, 5u, 0.0f);
    runtime.calls.clear();
    (void)pal4::RevoluteTask::StopAndCorrect(state, runtime);
    Check(runtime.calls.empty() && state.words[33u] == 0u,
          "zero duration skips correction");

    state.words[33u] = 0x4000u;
    Put(state, 5u, 2.0f);
    state.words[31u] &= 0xffffff00u;
    runtime.calls.clear();
    (void)pal4::RevoluteTask::StopAndCorrect(state, runtime);
    Check(runtime.calls.empty(), "disabled correction skips facet");

    pal4::RevoluteTaskState defaults{};
    defaults.words.fill(0xabcdef01u);
    runtime.keys.clear();
    runtime.float_values.clear();
    runtime.int_values.clear();
    Check(pal4::RevoluteTask::LoadConfig(defaults, 0x5000u, runtime) &&
          runtime.keys.size() == 16u, "all default fields are queried");
    for (std::size_t i = 19u; i <= 30u; ++i)
        Check(defaults.words[i] == 0u, "missing float defaults to zero");
    Check((defaults.words[18u] & 0xffffu) == 0u &&
          ((defaults.words[15u] >> 8u) & 0xffu) == 0u &&
          (defaults.words[31u] & 0xffu) == 0u,
          "missing integer flags default to false");

    defaults.words[16u] = 0x2000u;
    Put(defaults, 5u, 2.0f);
    Put(defaults, 22u, 1.0f);
    Put(defaults, 23u, 2.0f);
    Put(defaults, 24u, 3.0f);
    Put(defaults, 25u, 4.0f);
    Put(defaults, 26u, 5.0f);
    Put(defaults, 27u, 6.0f);
    defaults.words[18u] |= 0x100u;
    runtime.camera_facet = 0u;
    runtime.calls.clear();
    (void)pal4::RevoluteTask::Start(defaults, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "resolve:RevoluteTask", "resolve:StartCameraTask"} &&
          Near(Get(defaults, 22u), 1.0f),
          "missing camera facet leaves origin unchanged");

    runtime.camera_facet = 0x3000u;
    runtime.active = true;
    runtime.calls.clear();
    (void)pal4::RevoluteTask::Start(defaults, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "resolve:RevoluteTask", "resolve:StartCameraTask", "active"},
          "active camera short circuits target lookup");
    runtime.active = false;
    runtime.has_target = false;
    runtime.calls.clear();
    (void)pal4::RevoluteTask::Start(defaults, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "resolve:RevoluteTask", "resolve:StartCameraTask", "active", "target"},
          "camera without target does not overwrite origin");

    defaults.words[18u] &= 0xffff00ffu;
    runtime.calls.clear();
    (void)pal4::RevoluteTask::Update(defaults, 0.5f, runtime);
    Check(Near(Get(defaults, 25u), 4.0f) &&
          Near(Get(defaults, 32u), 0.5f),
          "disabled added velocity preserves angle and advances time");
    CheckMotion(runtime, {2.0f, 2.5f, 3.0f, 0.0f, 0.0f, 0.0f});

    defaults.words[31u] |= 1u;
    Put(defaults, 32u, 2.0f);
    runtime.calls.clear();
    (void)pal4::RevoluteTask::StopAndCorrect(defaults, runtime);
    Check(runtime.calls.size() == 1u, "zero remaining time still dispatches");
    CheckMotion(runtime, {0, 0, 0, 0, 0, 0});
    defaults.words[33u] = 0x4000u;
    Put(defaults, 5u, -1.0f);
    runtime.calls.clear();
    (void)pal4::RevoluteTask::StopAndCorrect(defaults, runtime);
    Check(runtime.calls.empty() && defaults.words[33u] == 0u,
          "negative duration skips correction and resets facet");

    runtime.calls.clear();
    Check(&pal4::RevoluteTask::Destroy(state, runtime) == &state &&
          runtime.calls == std::vector<std::string>{"destroy"},
          "destructor restores vtable and calls base");
}
