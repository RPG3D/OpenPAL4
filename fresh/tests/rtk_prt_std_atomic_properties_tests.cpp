#include "pal4/rtk_prt_std_atomic_properties.h"
#include "support/color_byte_x87_reference.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IRtkPrtStdAtomicPropertiesRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> reads;
    std::vector<std::uint32_t> writes;
    std::uint32_t frame_calls = 0u;
    std::uint32_t byte_writes = 0u;
    std::uint8_t frame_result = 0x6du;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words[address];
    }
    float ReadFloat(std::uint32_t address) override {
        return std::bit_cast<float>(ReadWord(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        reads.push_back(address);
        return bytes[address];
    }
    void WriteFloat(std::uint32_t address, float value) override {
        writes.push_back(address);
        words[address] = std::bit_cast<std::uint32_t>(value);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        writes.push_back(address);
        bytes[address] = value;
        ++byte_writes;
    }
    std::uint8_t UpdateFrame(std::uint32_t resource) override {
        Check(resource == 0x3107u, "facet frame update resource");
        ++frame_calls;
        return frame_result;
    }
};

void TestGetters() {
    pal4::RtkPrtStdAtomicObjectState state{};
    state.words[23u] = 0x3107u;
    Runtime runtime;
    std::array<std::uint32_t, 3> output{9u, 8u, 7u};
    Check(pal4::RtkPrtStdAtomicProperties::GetPosition(
              state, output, runtime) == 0u &&
              output == std::array<std::uint32_t, 3>{9u, 8u, 7u} &&
              runtime.reads.empty(), "inactive triple getter preserves output");
    state.words[1u] = 0x100u;
    Check(pal4::RtkPrtStdAtomicProperties::GetScale(
              state, output, runtime) == 0u && runtime.reads.empty(),
          "facet gate reads owner low activity byte");
    state.words[1u] = 0x101u;
    for (std::uint32_t i = 0u; i != 3u; ++i) {
        runtime.words[0x3107u + 100u + i * 4u] = 0x11111111u + i;
        runtime.words[0x3107u + 112u + i * 4u] = 0x22222221u + i;
    }
    Check(pal4::RtkPrtStdAtomicProperties::GetScale(
              state, output, runtime) == 0x13u &&
              output == std::array<std::uint32_t, 3>{
                  0x11111111u, 0x11111112u, 0x11111113u},
          "scale getter reads resource plus 100 and returns last low byte");
    Check(pal4::RtkPrtStdAtomicProperties::GetPosition(
              state, output, runtime) == 0x23u &&
              output == std::array<std::uint32_t, 3>{
                  0x22222221u, 0x22222222u, 0x22222223u},
          "position getter reads resource plus 112");
    output.fill(0u);
    Check(pal4::RtkPrtStdAtomicProperties::GetRotation(
              state, output, runtime) == 0x23u &&
              output == std::array<std::uint32_t, 3>{
                  0x22222221u, 0x22222222u, 0x22222223u},
          "rotation facet getter also reads plus 112 in IDA");
    Check(runtime.reads == std::vector<std::uint32_t>{
              0x316bu, 0x316fu, 0x3173u,
              0x3177u, 0x317bu, 0x317fu,
              0x3177u, 0x317bu, 0x317fu},
          "all getter addresses and order");
}

void TestSetters() {
    pal4::RtkPrtStdAtomicObjectState state{};
    state.words[23u] = 0x3107u;
    Runtime runtime;
    Check(pal4::RtkPrtStdAtomicProperties::SetRotation(
              state, {1.0f, 2.0f, 3.0f}, false, runtime) == 0u &&
              runtime.writes.empty(), "inactive setter does not touch resource");
    state.words[1u] = 1u;
    Check(pal4::RtkPrtStdAtomicProperties::SetPosition(
              state, {1.0f, 2.0f, 3.0f}, false, runtime) == 0x6du &&
              runtime.frame_calls == 1u &&
              runtime.writes == std::vector<std::uint32_t>{
                  0x3177u, 0x317bu, 0x317fu},
          "absolute position writes three floats then updates frame");
    runtime.writes.clear();
    Check(pal4::RtkPrtStdAtomicProperties::SetPosition(
              state, {4.0f, -1.0f, 0.5f}, true, runtime) == 0x6du &&
              std::bit_cast<float>(runtime.words[0x3177u]) == 5.0f &&
              std::bit_cast<float>(runtime.words[0x317bu]) == 1.0f &&
              std::bit_cast<float>(runtime.words[0x317fu]) == 3.5f,
          "relative position adds resource values component-wise");
    runtime.writes.clear();
    Check(pal4::RtkPrtStdAtomicProperties::SetRotation(
              state, {7.0f, 8.0f, 9.0f}, false, runtime) == 0x6du &&
              runtime.writes == std::vector<std::uint32_t>{
                  0x315fu, 0x3163u, 0x3167u},
          "rotation setter writes resource plus 88 despite getter quirk");
    runtime.writes.clear();
    Check(pal4::RtkPrtStdAtomicProperties::SetScale(
              state, {1.0f, 2.0f, 3.0f}, false, runtime) == 0x6du &&
              runtime.writes == std::vector<std::uint32_t>{
                  0x316bu, 0x316fu, 0x3173u} &&
              runtime.frame_calls == 4u,
          "scale setter writes plus 100 and updates frame");
}

void TestModeAndColor() {
    pal4::RtkPrtStdAtomicObjectState state{};
    state.words[23u] = 0x3107u;
    Runtime runtime;
    std::array<std::uint8_t, 4> output{11u, 22u, 33u, 44u};
    std::array<std::uint8_t*, 4> pointers{
        &output[0], &output[1], &output[2], &output[3]};
    Check(pal4::RtkPrtStdAtomicProperties::SetResourceMode(
              state, 3u, runtime) == 0u &&
              pal4::RtkPrtStdAtomicProperties::GetColor(
                  state, pointers, runtime) == 0u &&
              pal4::RtkPrtStdAtomicProperties::SetColor(
                  state, {1u, 2u, 3u, 4u}, runtime) == 0u &&
              output == std::array<std::uint8_t, 4>{11u, 22u, 33u, 44u} &&
              runtime.reads.empty() && runtime.writes.empty(),
          "inactive mode and color facets preserve native resource");
    state.words[1u] = 1u;
    Check(pal4::RtkPrtStdAtomicProperties::SetResourceMode(
              state, 0x5au, runtime) == 7u &&
              runtime.bytes[0x310au] == 0x5au,
          "mode writes resource plus 3 and returns pointer low byte");
    runtime.bytes[0x3243u] = 1u;
    const std::array<std::uint8_t, 4> color{0u, 64u, 128u, 255u};
    Check(pal4::RtkPrtStdAtomicProperties::SetColor(
              state, color, runtime) == 7u &&
              runtime.bytes[0x3243u] == 0u && runtime.byte_writes == 2u,
          "setting color disables active gradient and returns resource low byte");
    for (std::uint32_t i = 0u; i != 4u; ++i) {
        const auto actual = std::bit_cast<float>(
            runtime.words[0x31dbu + 4u * i]);
        const auto expected = pal4::test::ColorBytePc24(color[i]);
        Check(actual == expected, "color channels normalize to resource floats");
    }
    Check(pal4::RtkPrtStdAtomicProperties::GetColor(
              state, pointers, runtime) ==
              static_cast<std::uint8_t>(
                  reinterpret_cast<std::uintptr_t>(&output[3])) &&
              output == color,
          "color getter rounds channels and returns last output pointer byte");
    runtime.byte_writes = 0u;
    Check(pal4::RtkPrtStdAtomicProperties::SetColor(
              state, color, runtime) == 7u && runtime.byte_writes == 0u,
          "disabled gradient is left untouched");
    for(unsigned n=0;n<256u;++n) {
        const auto c=static_cast<std::uint8_t>(n);
        (void)pal4::RtkPrtStdAtomicProperties::SetColor(state,{c,c,c,c},runtime);
        for(unsigned channel=0;channel<4u;++channel)
            Check(runtime.words[0x31dbu+channel*4u]==std::bit_cast<std::uint32_t>(pal4::test::ColorBytePc24(n)),
                "color facet all256 bytes matches independent original x87 reference");
    }
}

}  // namespace

int main() {
    TestGetters();
    TestSetters();
    TestModeAndColor();
}
