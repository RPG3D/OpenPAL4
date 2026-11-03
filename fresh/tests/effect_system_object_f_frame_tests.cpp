#include "pal4/effect_system_object_f_frame.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemObjectFFrameRuntime {
    std::vector<std::string> events;
    std::array<std::int32_t, 4> values{0x12, 0x34, 0x56, 15};
    std::array<std::uint8_t, 4> bytes{};
    double converted_input = 0.0;
    std::uint32_t read_address = 0u;
    std::uint32_t object = 0u;
    std::uint32_t argument = 0u;
    std::uint32_t delta_low = 0u;
    std::uint32_t callback = 0u;
    std::int32_t converted_result = 12;

    void QueryTaskValues(pal4::EffectSystemObjectFState&,
                         std::array<std::int32_t, 4>& output) override {
        events.push_back("query");
        Check(output == std::array<std::int32_t, 4>{},
              "task outputs start at zero");
        output = values;
    }
    std::int32_t FloatToLong(double value) override {
        events.push_back("convert");
        converted_input = value;
        return converted_result;
    }
    void WriteTaskBytes(pal4::EffectSystemObjectFState&,
                        const std::array<std::uint8_t, 4>& output) override {
        events.push_back("write-task");
        bytes = output;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read-system");
        read_address = address;
        return 0x9000u;
    }
    void UpdateObject(std::uint32_t target, std::uint32_t input,
                      std::uint32_t delta_bits,
                      std::uint32_t system_callback) override {
        events.push_back("update-object");
        object = target;
        argument = input;
        delta_low = delta_bits;
        callback = system_callback;
    }
};

void TestGatesAndUntimedUpdate() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[4u] = 0x4000u;
    state.words[29u] = 0x5000u;
    pal4::EffectSystemObjectFFrame::Update(
        state, 1.1, runtime);
    Check(runtime.events.empty(), "inactive object skips update");
    state.words[1u] = 1u;
    pal4::EffectSystemObjectFFrame::Update(
        state, 1.1, runtime);
    Check(runtime.events.empty(), "stopped object skips update");
    state.words[1u] = 0x00010001u;
    pal4::EffectSystemObjectFFrame::Update(
        state, 1.1, runtime);
    Check(runtime.events == std::vector<std::string>{
              "read-system", "update-object"} &&
              runtime.read_address == 0x4030u &&
              runtime.object == 0x5000u && runtime.argument == static_cast<std::uint32_t>(std::bit_cast<std::uint64_t>(1.1)) &&
              runtime.delta_low == static_cast<std::uint32_t>(
                  std::bit_cast<std::uint64_t>(1.1) >> 32u) &&
              runtime.callback == 0x9000u,
          "untimed update forwards exact object, camera and both delta words");
}

void TestTimedUpdateAndClamp() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[1u] = 0x00010001u;
    state.words[2u] = 0x01000000u;
    state.words[4u] = 0x4000u;
    state.words[29u] = 0x5000u;
    state.words[30u] = std::bit_cast<std::uint32_t>(2.0f);
    pal4::EffectSystemObjectFFrame::Update(
        state, 1.1, runtime);
    Check(runtime.events == std::vector<std::string>{
              "query", "convert", "write-task", "read-system",
              "update-object"} &&
              runtime.converted_input > 12.79 &&
              runtime.converted_input < 12.81 &&
              runtime.bytes == std::array<std::uint8_t, 4>{
                  0x12u, 0x34u, 0x56u, 12u},
          "timed update preserves RGB and reduces alpha through four-pointer color ABI");
    runtime.events.clear();
    runtime.converted_result = -3;
    pal4::EffectSystemObjectFFrame::Update(
        state, 1.1, runtime);
    Check(runtime.bytes == std::array<std::uint8_t, 4>{
              0x12u, 0x34u, 0x56u, 0u} &&
              runtime.events.back() == "update-object",
          "negative converted time clamps before byte write");
}

}  // namespace

int main() {
    TestGatesAndUntimedUpdate();
    TestTimedUpdateAndClamp();
}
