#include "pal4/move_to_target_task.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

constexpr std::array<std::string_view, 12u> kKeys{
    "Attr_Task_Target_Position_x", "Attr_Task_Target_Position_y",
    "Attr_Task_Target_Position_z", "Attr_Task_Target_Rotate_x",
    "Attr_Task_Target_Rotate_y", "Attr_Task_Target_Rotate_z",
    "Attr_Task_Target_Position_Bias_x", "Attr_Task_Target_Position_Bias_y",
    "Attr_Task_Target_Position_Bias_z", "Attr_Task_Target_Rotate_Bias_x",
    "Attr_Task_Target_Rotate_Bias_y", "Attr_Task_Target_Rotate_Bias_z"};

struct Runtime final : pal4::IMoveToTargetConfigRuntime {
    std::vector<std::string> calls;
    std::array<float, 12u> values{};
    std::array<std::int32_t, 6u> random_values{};
    std::size_t float_index = 0u;
    std::size_t random_index = 0u;
    std::int32_t fixed = 1;
    float fixed_velocity = 7.5f;
    bool throw_on_assign = false;

    pal4::MoveToTargetStringTemporary ReadTargetName(
        std::uint32_t config, std::string_view key,
        std::uint32_t fallback) override {
        Check(config == 0x5000u && key == "Attr_Task_Target_Name" &&
                  fallback == 0x8e425cu, "target name key and fallback");
        calls.emplace_back("read-name");
        return {{0x7000u, 3u, 3u, 0u}};
    }
    void AssignTargetName(
        pal4::MoveToTargetTaskState& state,
        const pal4::MoveToTargetStringTemporary& temporary) override {
        Check(temporary.words[0u] == 0x7000u,
              "assign receives XML temporary");
        calls.emplace_back("assign-name");
        if (throw_on_assign) throw std::runtime_error("assign");
        state.words[28u] = 0x7000u;
    }
    void ReleaseTargetName(
        pal4::MoveToTargetStringTemporary& temporary) override {
        Check(temporary.words[0u] == 0x7000u,
              "release receives XML temporary");
        calls.emplace_back("release-name");
    }
    float ReadFloat(std::uint32_t config, std::string_view key,
                    float fallback) override {
        Check(config == 0x5000u && fallback == 0.0f &&
                  float_index < kKeys.size() && key == kKeys[float_index],
              "twelve float keys and zero defaults in IDA order");
        calls.emplace_back("float:" + std::string(key));
        return values[float_index++];
    }
    std::int32_t ReadTreeInt(std::uint32_t config, std::string_view key,
                             std::int32_t fallback) override {
        Check(config == 0x5000u && key == "Attr_Is_Fixed_Vel" &&
                  fallback == 0, "fixed velocity flag tree key");
        calls.emplace_back("fixed-flag");
        return fixed;
    }
    float ReadTreeFloat(std::uint32_t config, std::string_view key,
                        float fallback) override {
        Check(config == 0x5000u && key == "Attr_Fixed_Vel" &&
                  fallback == 0.0f, "fixed velocity tree key");
        calls.emplace_back("fixed-velocity");
        return fixed_velocity;
    }
    std::int32_t RandomInt() override {
        Check(random_index < random_values.size(),
              "exactly six random values consumed");
        calls.emplace_back("random");
        return random_values[random_index++];
    }
};

float Field(const pal4::MoveToTargetTaskState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}

}  // namespace

int main() {
    pal4::MoveToTargetTaskState state{};
    state.words.fill(0xa5a5a5a5u);
    Runtime runtime;
    Check(!pal4::MoveToTargetTask::LoadConfig(state, 0u, runtime) &&
              runtime.calls.empty() && state.words[21u] == 0xa5a5a5a5u,
          "null XML leaves state untouched");

    runtime.values = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f,
                      1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    runtime.random_values.fill(0);
    Check(pal4::MoveToTargetTask::LoadConfig(state, 0x5000u, runtime) &&
              runtime.float_index == 12u && runtime.random_index == 6u &&
              state.words[28u] == 0x7000u &&
              state.words[34u] == 0xa5a5a501u &&
              Field(state, 35u) == 7.5f,
          "target name, all keys and fixed velocity written");
    for (std::size_t index = 0u; index < 6u; ++index)
        Check(Field(state, 21u + index) ==
                  10.0f * static_cast<float>(index + 1u) -
                      static_cast<float>(index + 1u),
              "zero random result subtracts each signed bias");
    Check(runtime.calls[0u] == "read-name" &&
              runtime.calls[1u] == "assign-name" &&
              runtime.calls[2u] == "release-name" &&
              runtime.calls[9u] == "float:Attr_Task_Target_Position_Bias_x" &&
              runtime.calls[12u] == "random" &&
              runtime.calls[15u] == "float:Attr_Task_Target_Rotate_Bias_x" &&
              runtime.calls[18u] == "random" &&
              runtime.calls[21u] == "fixed-flag" &&
              runtime.calls[22u] == "fixed-velocity",
          "reads each bias group before its three random draws");

    Runtime positive;
    positive.values = runtime.values;
    positive.random_values.fill(32767);
    positive.fixed = -2;
    positive.fixed_velocity = 0.0f;
    pal4::MoveToTargetTaskState other{};
    other.words[34u] = 0x123456ffu;
    Check(pal4::MoveToTargetTask::LoadConfig(other, 0x5000u, positive) &&
              (other.words[34u] & 0xffu) == 0u &&
              (other.words[34u] & 0xffffff00u) == 0x12345600u,
          "nonpositive fixed velocity flag clears only low byte");
    for (std::size_t index = 0u; index < 6u; ++index)
        Check(std::fabs(Field(other, 21u + index) -
                        11.0f * static_cast<float>(index + 1u)) < 0.001f,
              "maximum CRT random result adds signed bias");

    Runtime failing;
    failing.throw_on_assign = true;
    bool caught = false;
    try { (void)pal4::MoveToTargetTask::LoadConfig(other, 0x5000u, failing); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && failing.calls == std::vector<std::string>{
                                       "read-name", "assign-name", "release-name"},
          "string temporary released when assignment throws");
}
