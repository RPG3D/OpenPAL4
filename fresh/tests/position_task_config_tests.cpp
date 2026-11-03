#include "pal4/position_task.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
float Read(const pal4::PositionTaskState& state, std::size_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}
std::uint8_t Byte(const pal4::PositionTaskState& state, std::size_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}
struct Runtime final : pal4::IPositionTaskConfigRuntime {
    std::vector<std::string> keys;
    std::vector<std::pair<float, float>> ranges;
    bool result = false;
    std::int32_t ReadInt(std::uint32_t config, std::string_view key,
                         std::int32_t fallback) override {
        Check(config == 0x5000u && fallback == 0, "integer root fallback");
        keys.emplace_back(key);
        return key == "Attr_Task_Position_Result" ? int(result) : 1;
    }
    float ReadFloat(std::uint32_t config, std::string_view key,
                    float fallback) override {
        Check(config == 0x5000u && fallback == 0.0f,
              "float root fallback");
        keys.emplace_back(key);
        if (key == "Attr_Task_Position_x") return 10.0f;
        if (key == "Attr_Task_Position_y") return 20.0f;
        if (key == "Attr_Task_Position_z") return 30.0f;
        if (key == "Attr_Task_Position_Rot_Fixed_x") return 1.0f;
        if (key == "Attr_Task_Position_Rot_Fixed_y") return 2.0f;
        if (key == "Attr_Task_Position_Rot_Fixed_z") return 3.0f;
        if (key == "Attr_Task_Position_Pos_Bias_x") return 4.0f;
        if (key == "Attr_Task_Position_Pos_Bias_y") return 5.0f;
        if (key == "Attr_Task_Position_Pos_Bias_z") return 6.0f;
        if (key == "Attr_Task_Position_Rot_Bias_x") return 7.0f;
        if (key == "Attr_Task_Position_Rot_Bias_y") return 8.0f;
        if (key == "Attr_Task_Position_Rot_Bias_z") return 9.0f;
        if (key == "Attr_Add_Vel_x") return 11.0f;
        if (key == "Attr_Add_Vel_y") return 12.0f;
        if (key == "Attr_Add_Vel_z") return 13.0f;
        if (key == "Attr_Task_Position_Time_Bias") return 0.5f;
        if (key == "Attr_Time_Interval") return 1.5f;
        if (key == "Attr_Time_Interval2") return 2.5f;
        Check(false, "known position float key"); return 0.0f;
    }
    float RandomFloat(float lower, float upper) override {
        ranges.emplace_back(lower, upper); return upper;
    }
};
const std::vector<std::string> kExpectedKeys{
    "Attr_Task_Position_Result", "Attr_Task_Position_x",
    "Attr_Task_Position_y", "Attr_Task_Position_z",
    "Attr_Task_Position_Face_Target",
    "Attr_Task_Position_Rot_Fixed_x", "Attr_Task_Position_Rot_Fixed_y",
    "Attr_Task_Position_Rot_Fixed_z",
    "Attr_Task_Position_Pos_Bias_x", "Attr_Task_Position_Pos_Bias_y",
    "Attr_Task_Position_Pos_Bias_z",
    "Attr_Task_Position_Rot_Bias_x", "Attr_Task_Position_Rot_Bias_y",
    "Attr_Task_Position_Rot_Bias_z", "Attr_Inherit_Pre_Task",
    "Attr_Add_Vel", "Attr_Add_Vel_x", "Attr_Add_Vel_y",
    "Attr_Add_Vel_z", "Attr_Task_Position_Rollback",
    "Attr_Task_Position_Time_Bias", "Attr_Time_Interval",
    "Attr_Time_Interval2"};
}  // namespace

int main() {
    pal4::PositionTaskState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(!pal4::PositionTask::LoadConfig(state, 0u, runtime) &&
          state.words[21u] == 0xdeadbeefu && runtime.keys.empty(),
          "null config preserves state");
    Check(pal4::PositionTask::LoadConfig(state, 0x5000u, runtime) &&
          state.words[21u] == 0u && runtime.keys == kExpectedKeys &&
          Byte(state, 72u) == 0u && Byte(state, 73u) == 1u &&
          Byte(state, 61u) == 1u && Byte(state, 136u) == 1u &&
          Byte(state, 188u) == 1u &&
          Read(state, 88u) == 10.0f && state.words[25u] == 0xdeadbeefu &&
          Read(state, 140u) == 8.0f && Read(state, 144u) == 10.0f &&
          Read(state, 148u) == 12.0f &&
          Read(state, 204u) == 11.0f && Read(state, 208u) == 12.0f &&
          Read(state, 212u) == 13.0f &&
          Read(state, 80u) == 0.5f && Read(state, 168u) == 1.5f &&
          Read(state, 180u) == 2.5f,
          "ordered config values and native offsets");
    Check(runtime.ranges == std::vector<std::pair<float, float>>{
        {-7.0f, 7.0f}, {-8.0f, 8.0f}, {-9.0f, 9.0f}},
        "rotation bias samples even without result mode");

    state.words.fill(0u);
    runtime = Runtime{};
    runtime.result = true;
    Check(pal4::PositionTask::LoadConfig(state, 0x5000u, runtime) &&
          Byte(state, 72u) == 1u &&
          Read(state, 88u) == 14.0f && Read(state, 92u) == 25.0f &&
          Read(state, 96u) == 36.0f && runtime.ranges.size() == 6u &&
          runtime.ranges[0] == std::pair{-4.0f, 4.0f} &&
          runtime.ranges[5] == std::pair{-9.0f, 9.0f},
          "result mode applies position bias before rotation bias");
    Check(pal4::PositionTask::ReadOptionalFloat(runtime, 0x5000u,
          "Attr_Time_Interval", 0.0f) == 1.5f,
          "0x627580 optional float helper");
}
