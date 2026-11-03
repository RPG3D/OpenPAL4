#include "pal4/particle_visual_object.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IParticleVisualRenderRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::array<std::uint8_t, 4u>> colors;
    std::vector<std::uint32_t> properties;
    pal4::ParticleVisualState* state{};
    bool shrink_after_update{};
    bool change_property_after_color{};

    std::uint32_t ObjectTransform(std::uint32_t frame) override {
        Check(frame == 0x1000u, "collection reads object frame");
        calls.emplace_back("object_transform");
        return 0x2000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    pal4::ParticleVisualPoint TransformPosition(
        std::uint32_t matrix, pal4::ParticleVisualPoint point) override {
        Check(matrix == 0x2000u, "collection position matrix");
        calls.emplace_back("transform_position");
        return {point.x + 100.f, point.y + 200.f, point.z + 300.f};
    }
    void InitializeTransform(std::uint32_t visual) override {
        calls.push_back("initialize:" + std::to_string(visual));
    }
    void SetColor(std::uint32_t property,
                  std::array<std::uint8_t, 4u> color) override {
        properties.push_back(property);
        colors.push_back(color);
        calls.push_back("color:" + std::to_string(property));
        if (change_property_after_color && property == 0x7000u)
            words[0x6030u] = 0x7200u;
    }
    void UpdateVisual(std::uint32_t visual, std::uint32_t low,
                      std::uint32_t high,
                      std::uint32_t argument) override {
        Check(low == 0x12345678u && high == 0x3ff80000u &&
                  argument == 0x9876u,
              "collection visual gets forwarded render words");
        calls.push_back("update:" + std::to_string(visual));
        if (shrink_after_update) state->WriteWord(204u, 1u);
    }
    void Float(std::uint32_t address, float value) {
        words[address] = std::bit_cast<std::uint32_t>(value);
    }
    float Float(std::uint32_t address) const {
        return std::bit_cast<float>(words.at(address));
    }
};

void Seed(pal4::ParticleVisualState& state, Runtime& runtime) {
    state.WriteWord(44u, 0x1000u);
    state.WriteWord(124u, 1u);
    state.WriteWord(160u, 0x3000u);
    state.WriteWord(176u, 0x4000u);
    state.WriteWord(204u, 2u);
    runtime.state = &state;
    runtime.Float(0x2030u, 10.f);
    runtime.Float(0x2034u, 20.f);
    runtime.Float(0x2038u, 30.f);
    runtime.words[0x3000u] = 0x5000u;
    runtime.words[0x3004u] = 0x5100u;
    runtime.words[0x4000u] = 0x6000u;
    runtime.words[0x4004u] = 0x6100u;
    runtime.bytes[0x5000u] = 1u;
    runtime.bytes[0x5100u] = 0u;
    runtime.words[0x6030u] = 0x7000u;
    runtime.words[0x6130u] = 0x7100u;
    runtime.Float(0x7014u, 0.f);
    runtime.Float(0x7018u, -2.f);
    runtime.Float(0x5010u, 0.25f);
    runtime.Float(0x5014u, 4.f);
    runtime.Float(0x5018u, 8.f);
    runtime.Float(0x501cu, 1.f);
    runtime.Float(0x5020u, 0.5f);
    runtime.Float(0x5024u, 0.25f);
    runtime.Float(0x5028u, 1.f);
    runtime.Float(0x5004u, 0.5f);
    runtime.Float(0x5008u, 2.f);
    runtime.Float(0x5044u, 1.f);
    runtime.Float(0x5048u, 2.f);
    runtime.Float(0x504cu, 3.f);
}

int Render(pal4::ParticleVisualState& state, Runtime& runtime) {
    return pal4::ParticleVisualObject::RenderCollection(
        state, 0x12345678u, 0x3ff80000u, 0x9876u, runtime);
}

void TestEmptyAndNegativeCount() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state, runtime);
    state.WriteWord(204u, 0u);
    Check(Render(state, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"object_transform"},
          "empty collection still reads frame translation");
    runtime.calls.clear();
    state.WriteWord(204u, 0xffffffffu);
    Check(Render(state, runtime) == -1 &&
              runtime.calls == std::vector<std::string>{"object_transform"},
          "negative collection count skips elements");
}

void TestActiveInactiveAndFade() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state, runtime);
    state.WriteByte(4u, 1u);
    state.WriteByte(2u, 1u);
    const auto result = Render(state, runtime);
    const std::vector<std::string> expected{
        "object_transform", "object_transform", "transform_position",
        "initialize:24576", "color:28672", "update:24576",
        "color:28928", "update:24832"};
    Check(result == 2 && runtime.calls == expected,
          "active visual is transformed and both slots update in order");
    Check(runtime.Float(0x601cu) == 101.f &&
              runtime.Float(0x6020u) == 202.f &&
              runtime.Float(0x6024u) == 303.f &&
              runtime.Float(0x6028u) == 4.f &&
              runtime.Float(0x602cu) == 8.f &&
              runtime.Float(0x6018u) == 0.25f,
          "active position, fallback size divisor and rotation");
    Check(runtime.Float(0x6000u) == 10.f &&
              runtime.Float(0x6004u) == 20.f &&
              runtime.Float(0x6008u) == 30.f &&
              runtime.colors == std::vector<std::array<std::uint8_t, 4u>>{
                  {255u, 128u, 64u, 191u}, {0u, 0u, 0u, 0u}},
          "origin, faded RGBA and inactive zero color");
    Check(runtime.words.find(0x611cu) == runtime.words.end(),
          "inactive visual transform fields remain untouched");
}

void TestLocalPositionAndChangedProperty() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state, runtime);
    state.WriteWord(204u, 1u);
    state.WriteByte(4u, 1u);
    state.WriteByte(53u, 1u);
    runtime.change_property_after_color = true;
    runtime.Float(0x7214u, 2.f);
    runtime.Float(0x7218u, 4.f);
    Check(Render(state, runtime) == 1 &&
              runtime.Float(0x601cu) == 1.f &&
              runtime.Float(0x6028u) == 2.f &&
              runtime.Float(0x602cu) == 2.f &&
              runtime.colors.front()[3] == 255u &&
              runtime.properties == std::vector<std::uint32_t>{0x7000u},
          "local mode skips transform and rereads property after color");
    runtime.calls.clear();
    runtime.shrink_after_update = true;
    state.WriteWord(204u, 2u);
    Check(Render(state, runtime) == 1 &&
              runtime.calls == std::vector<std::string>{
                  "object_transform", "initialize:24576",
                  "color:29184", "update:24576"},
          "collection count is reread after each visual update");
}

}  // namespace

int main() {
    TestEmptyAndNegativeCount();
    TestActiveInactiveAndFade();
    TestLocalPositionAndChangedProperty();
}
