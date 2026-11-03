#include "pal4/effect_system_object_f_transform.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IEffectSystemObjectFTransformRuntime {
    std::vector<std::string> events;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::array<float, 3> frame_position{5.0f, 0.0f, 3.0f};
    std::vector<std::array<float, 3>> translations;
    std::vector<std::uint32_t> axes;
    std::vector<float> angles;
    std::uint32_t transform_result = 0xabcdef23u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t InitializeTransform(std::uint32_t object) override {
        events.push_back("initialize:" + std::to_string(object));
        return transform_result;
    }
    std::uint32_t GetFrame(pal4::EffectSystemObjectFState&) override {
        events.push_back("frame");
        return 0x7000u;
    }
    void TranslateMatrix(std::uint32_t matrix,
                         const std::array<float, 3>& vector,
                         std::uint32_t mode) override {
        events.push_back("translate");
        Check(matrix == 0x7010u && mode == 2u,
              "RenderWare translation matrix and mode");
        translations.push_back(vector);
    }
    void RotateMatrix(std::uint32_t matrix,
                      std::uint32_t axis, float angle,
                      std::uint32_t mode) override {
        events.push_back("rotate");
        Check(matrix == 0x7010u && mode == 2u,
              "RenderWare rotation matrix and mode");
        axes.push_back(axis);
        angles.push_back(angle);
    }
    void UpdateFrameObjects(std::uint32_t frame) override {
        events.push_back("update-frame");
        Check(frame == 0x7000u, "frame update operand");
    }
    std::array<float, 3> ReadFramePosition(
        std::uint32_t frame) override {
        events.push_back("read-position");
        Check(frame == 0x7000u, "frame position operand");
        return frame_position;
    }
};

void TestInactiveAndSetters() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[29u] = 0x5000u;
    Check(pal4::EffectSystemObjectFTransform::SetScale(
              state, 1.0f, 2.0f, false, runtime) == 0u &&
              pal4::EffectSystemObjectFTransform::SetRotation(
                  state, 3.0f, false, runtime) == 0u &&
              pal4::EffectSystemObjectFTransform::SetPosition(
                  state, {1.0f, 2.0f, 3.0f}, false, runtime) == 0u &&
              pal4::EffectSystemObjectFTransform::ApplyMotion(
                  state, {1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f},
                  {1.0f, 1.0f, 1.0f}, runtime) == 0u &&
              runtime.events.empty(),
          "inactive owner skips all transform paths");

    state.words[1u] = 0x12340001u;
    Check(pal4::EffectSystemObjectFTransform::SetScale(
              state, 2.0f, 3.0f, false, runtime) == Bits(3.0f) &&
              runtime.words.at(0x5028u) == Bits(2.0f) &&
              runtime.words.at(0x502cu) == Bits(3.0f) &&
              runtime.events == std::vector<std::string>{
                  "write:20520", "write:20524"},
          "absolute scale writes two words and returns second float bits");
    runtime.events.clear();
    Check(pal4::EffectSystemObjectFTransform::SetScale(
              state, 1.0f, 4.0f, true, runtime) == Bits(7.0f) &&
              runtime.words.at(0x5028u) == Bits(3.0f) &&
              runtime.words.at(0x502cu) == Bits(7.0f) &&
              runtime.events == std::vector<std::string>{
                  "read:20520", "read:20524", "write:20520",
                  "write:20524"},
          "relative scale reads both old values before writes");

    runtime.events.clear();
    Check(pal4::EffectSystemObjectFTransform::SetRotation(
              state, 12.0f, false, runtime) == 0u &&
              runtime.words.at(0x5018u) == Bits(12.0f) &&
              runtime.events == std::vector<std::string>{"write:20504"},
          "absolute rotation writes only embedded +24");
    runtime.events.clear();
    Check(pal4::EffectSystemObjectFTransform::SetRotation(
              state, 3.0f, true, runtime) == 0x5000u &&
              runtime.words.at(0x5018u) == Bits(15.0f) &&
              runtime.events == std::vector<std::string>{
                  "read:20504", "write:20504"},
          "relative rotation adds angle and returns object pointer");

    runtime.events.clear();
    Check(pal4::EffectSystemObjectFTransform::SetPosition(
              state, {1.0f, 2.0f, 3.0f}, false, runtime) ==
              runtime.transform_result &&
              runtime.words.at(0x501cu) == Bits(1.0f) &&
              runtime.words.at(0x5020u) == Bits(2.0f) &&
              runtime.words.at(0x5024u) == Bits(3.0f) &&
              runtime.events.back() == "initialize:20480",
          "absolute position initializes transform after three writes");
    runtime.events.clear();
    Check(pal4::EffectSystemObjectFTransform::SetPosition(
              state, {4.0f, -1.0f, 2.0f}, true, runtime) ==
              runtime.transform_result &&
              runtime.words.at(0x501cu) == Bits(5.0f) &&
              runtime.words.at(0x5020u) == Bits(1.0f) &&
              runtime.words.at(0x5024u) == Bits(5.0f) &&
              runtime.events == std::vector<std::string>{
                  "read:20508", "read:20512", "read:20516",
                  "write:20508", "write:20512", "write:20516",
                  "initialize:20480"},
          "relative position reads all axes before writing and initializing");
}

void TestMotionSequenceAndClamps() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[1u] = 1u;
    state.words[29u] = 0x5000u;
    Check(pal4::EffectSystemObjectFTransform::ApplyMotion(
              state, {1.0f, 2.0f, 3.0f}, {10.0f, 20.0f, 30.0f},
              {2.0f, 1.0f, 1.0f}, runtime) == runtime.transform_result,
          "motion forwards final position setter result");
    Check(runtime.events == std::vector<std::string>{
              "frame", "translate", "rotate", "rotate", "rotate",
              "translate", "update-frame", "read-position",
              "write:20508", "write:20512", "write:20516",
              "initialize:20480"},
          "RenderWare operations precede component position setter");
    Check(runtime.translations == std::vector<std::array<float, 3>>{
              {-1.0f, -2.0f, -3.0f}, {1.0f, 2.0f, 3.0f}} &&
              runtime.axes == std::vector<std::uint32_t>{
                  0x84a08cu, 0x84a098u, 0x84a0a4u} &&
              runtime.angles == std::vector<float>{10.0f, 20.0f, 30.0f} &&
              runtime.words.at(0x501cu) == Bits(3.0f) &&
              runtime.words.at(0x5020u) == Bits(1.0f) &&
              runtime.words.at(0x5024u) == Bits(3.0f),
          "pivot translations, axis order and toward-target movement");

    runtime.events.clear();
    runtime.frame_position = {1.25f, 1.75f, 3.0f};
    static_cast<void>(pal4::EffectSystemObjectFTransform::ApplyMotion(
        state, {1.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f}, runtime));
    Check(runtime.words.at(0x501cu) == Bits(1.0f) &&
              runtime.words.at(0x5020u) == Bits(2.0f) &&
              runtime.words.at(0x5024u) == Bits(3.0f),
          "all three axes clamp overshoot and preserve equality");
}

void TestRawPositionWords() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[29u] = 0x5000u;
    const std::array<std::uint32_t, 3> words{0x7fc12345u, 0x80000000u,
                                              0x12345678u};
    Check(pal4::EffectSystemObjectFTransform::SetPositionWords(
              state, words, runtime) == 0u && runtime.events.empty(),
          "inactive raw position leaves object untouched");
    state.words[1u] = 1u;
    Check(pal4::EffectSystemObjectFTransform::SetPositionWords(
              state, words, runtime) == 0x23u &&
              runtime.words.at(0x501cu) == words[0u] &&
              runtime.words.at(0x5020u) == words[1u] &&
              runtime.words.at(0x5024u) == words[2u] &&
              runtime.events == std::vector<std::string>{
                  "write:20508", "write:20512", "write:20516",
                  "initialize:20480"},
          "raw position copies bits and returns initializer low byte");
}

}  // namespace

int main() {
    TestInactiveAndSetters();
    TestMotionSequenceAndClamps();
    TestRawPositionWords();
}
