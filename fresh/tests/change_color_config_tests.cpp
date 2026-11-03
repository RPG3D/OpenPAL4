#include "pal4/change_color_task.h"
#include "support/color_byte_x87_reference.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IChangeColorConfigRuntime {
    std::map<std::string, std::int32_t> integers;
    std::map<std::string, float> floats;
    std::vector<std::string> keys;
    std::int32_t ReadInt(const std::uint32_t config,
                         const std::string_view key,
                         const std::int32_t default_value) override {
        Check(config == 0x1234u && default_value == 0,
              "integer XML read receives original config and zero default");
        keys.emplace_back(key);
        const auto it = integers.find(std::string(key));
        return it == integers.end() ? default_value : it->second;
    }
    float ReadFloat(const std::uint32_t config,
                    const std::string_view key,
                    const float default_value) override {
        Check(config == 0x1234u && default_value == 0.0f,
              "float XML read receives original config and zero default");
        keys.emplace_back(key);
        const auto it = floats.find(std::string(key));
        return it == floats.end() ? default_value : it->second;
    }
};

constexpr std::array<std::string_view, 11u> kKeys{
    "Attr_Task_ChangeColor_Alpha", "Attr_Task_ChangeColor_End_Red",
    "Attr_Task_ChangeColor_End_Green", "Attr_Task_ChangeColor_End_Blue",
    "Attr_Task_ChangeColor_End_Alpha", "Attr_Inherit_Pre_Task",
    "Attr_Task_ChangeColor_Total", "Attr_Task_ChangeColor_Rollback",
    "Attr_Task_ChangeColor_Time_Bias", "Attr_Time_Interval",
    "Attr_Time_Interval2"};

void CheckKeyOrder(const Runtime& runtime) {
    Check(runtime.keys.size() == kKeys.size(), "all eleven attributes read");
    for (std::size_t i = 0u; i < kKeys.size(); ++i)
        Check(runtime.keys[i] == kKeys[i], "IDA XML key read order");
}

}  // namespace

int main() {
    pal4::ChangeColorTaskState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(!pal4::ChangeColorTask::LoadConfig(state, 0u, runtime) &&
              runtime.keys.empty() && state.words[34u] == 0xdeadbeefu,
          "null config returns without touching task");

    runtime.integers = {
        {"Attr_Task_ChangeColor_Alpha", 2},
        {"Attr_Task_ChangeColor_End_Red", 300},
        {"Attr_Task_ChangeColor_End_Green", -1},
        {"Attr_Task_ChangeColor_End_Blue", 128},
        {"Attr_Task_ChangeColor_End_Alpha", 255},
        {"Attr_Inherit_Pre_Task", -1},
        {"Attr_Task_ChangeColor_Total", 3},
        {"Attr_Task_ChangeColor_Rollback", 1}};
    runtime.floats = {{"Attr_Task_ChangeColor_Time_Bias", 1.25f},
                      {"Attr_Time_Interval", 2.5f},
                      {"Attr_Time_Interval2", 3.75f}};
    Check(pal4::ChangeColorTask::LoadConfig(state, 0x1234u, runtime),
          "valid config returns success");
    CheckKeyOrder(runtime);
    Check(state.words[34u] == 0xdead0101u &&
              state.words[15u] == 0xdead00efu &&
              state.words[36u] == 0xdeadbe01u,
          "alpha, total, inherit and rollback boolean bytes");
    const std::array<std::uint8_t, 4u> channels{44u, 255u, 128u, 255u};
    for (std::uint32_t i = 0u; i < channels.size(); ++i) {
        const auto expected = pal4::test::ColorBytePc24(channels[i]);
        Check(state.words[22u + i] == std::bit_cast<std::uint32_t>(expected),
              "end channel truncation and normalization");
    }
    Check(state.words[39u] == std::bit_cast<std::uint32_t>(1.25f) &&
              state.words[41u] == std::bit_cast<std::uint32_t>(2.5f) &&
              state.words[44u] == std::bit_cast<std::uint32_t>(3.75f),
          "three float parameters stored at native offsets");

    runtime.keys.clear();
    runtime.integers.clear();
    runtime.floats.clear();
    Check(pal4::ChangeColorTask::LoadConfig(state, 0x1234u, runtime),
          "missing keys retain success");
    CheckKeyOrder(runtime);
    Check((state.words[34u] & 0xffffu) == 0u &&
              ((state.words[15u] >> 8u) & 0xffu) == 0u &&
              (state.words[36u] & 0xffu) == 0u &&
              state.words[22u] == 0u && state.words[44u] == 0u,
          "missing attributes apply zero defaults");
    for(unsigned n=0;n<256u;++n) {
        runtime.integers["Attr_Task_ChangeColor_End_Red"]=n;
        runtime.integers["Attr_Task_ChangeColor_End_Green"]=n;
        runtime.integers["Attr_Task_ChangeColor_End_Blue"]=n;
        runtime.integers["Attr_Task_ChangeColor_End_Alpha"]=n;
        (void)pal4::ChangeColorTask::LoadConfig(state,0x1234u,runtime);
        for(unsigned channel=0;channel<4u;++channel)
            Check(state.words[22u+channel]==std::bit_cast<std::uint32_t>(pal4::test::ColorBytePc24(n)),
                "all byte channels agree with original x87 float-constant conversion");
    }

}
