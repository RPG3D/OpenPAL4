#include "pal4/particle_visual_object.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IParticleSimulationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> transforms;
    std::vector<std::uint32_t> normalized;
    std::uint32_t transform_result = 0x9000u;
    pal4::ParticleVisualPoint normalized_result{};
    pal4::ParticleVisualPoint last_direction{};
    std::uint32_t position_matrix{};

    std::uint32_t ObjectTransform(std::uint32_t frame) override {
        Check(frame == 0x1000u, "frame passed to GetObjectTransform");
        return transform_result;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    pal4::ParticleVisualPoint TransformVector(
        std::uint32_t matrix, pal4::ParticleVisualPoint vector) override {
        transforms.push_back(matrix);
        return {vector.x + 10.f, vector.y + 20.f, vector.z + 30.f};
    }
    pal4::ParticleVisualPoint TransformPosition(
        std::uint32_t matrix, pal4::ParticleVisualPoint point) override {
        position_matrix = matrix;
        return {point.x + 10.f, point.y + 20.f, point.z + 30.f};
    }
    pal4::ParticleVisualPoint Normalize(
        pal4::ParticleVisualPoint point) override {
        normalized.push_back(0x3044u);
        last_direction = point;
        return normalized_result;
    }
    void Float(std::uint32_t address, float value) {
        words[address] = std::bit_cast<std::uint32_t>(value);
    }
    float Float(std::uint32_t address) const {
        return std::bit_cast<float>(words.at(address));
    }
};

constexpr std::uint32_t item = 0x3000u;

void Seed(pal4::ParticleVisualState& state, Runtime& runtime) {
    state.WriteWord(44u, 0x1000u);
    state.WriteWord(160u, 0x2000u);
    state.WriteWord(164u, 0x2004u);
    state.WriteWord(208u, 1u);
    runtime.words[0x2000u] = item;
    runtime.bytes[item] = 1u;
    runtime.Float(item + 4u, 0.f);
    runtime.Float(item + 8u, 2.f);
    runtime.Float(item + 16u, 1.f);
    runtime.Float(item + 44u, 2.f);
    runtime.Float(item + 48u, 4.f);
    runtime.Float(item + 52u, 6.f);
    runtime.Float(item + 68u, 3.f);
    runtime.Float(item + 72u, 5.f);
    runtime.Float(item + 76u, 7.f);
    state.WriteWord(228u, std::bit_cast<std::uint32_t>(8.f));
    state.WriteWord(232u, std::bit_cast<std::uint32_t>(9.f));
}

void TestEmptyAndInactive() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    state.WriteWord(44u, 0x1000u);
    Check(pal4::ParticleVisualObject::SimulateParticles(
              state, 0.5, runtime) == 0x9000u,
          "empty array returns transform result");
    Seed(state, runtime);
    runtime.bytes[item] = 0u;
    Check(pal4::ParticleVisualObject::SimulateParticles(
              state, 0.5, runtime) == 1u &&
              runtime.Float(item + 4u) == 0.f &&
              runtime.transforms.empty(),
          "inactive item is skipped");
}

void TestAgeAndExpiry() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state, runtime);
    runtime.Float(item + 4u, 1.5f);
    Check(pal4::ParticleVisualObject::SimulateParticles(
              state, 0.5, runtime) == 1u &&
              runtime.bytes[item] == 0u &&
              state.ReadWord(208u) == 0u &&
              runtime.Float(item + 4u) == 2.f &&
              runtime.transforms.empty(),
          "item expires at equal lifetime before movement");

    Seed(state, runtime);
    runtime.Float(item + 4u, 1.4f);
    runtime.Float(item + 8u, 1.8f);
    (void)pal4::ParticleVisualObject::SimulateParticles(state, 0.5, runtime);
    Check(runtime.bytes[item] == 0u && state.ReadWord(208u) == 0u,
          "item expires after lifetime");

    Seed(state, runtime);
    runtime.Float(item + 4u, 0.5f);
    runtime.Float(item + 8u, 1.f);
    (void)pal4::ParticleVisualObject::SimulateParticles(
        state, 0.50000001, runtime);
    Check(runtime.bytes[item] == 0u && runtime.Float(item + 4u) == 1.f,
          "reached rounded lifetime expires before movement");
    Seed(state,runtime);
    runtime.Float(item+4u,0.5f);runtime.Float(item+8u,1.f);
    (void)pal4::ParticleVisualObject::SimulateParticles(state,0.49999999,runtime);
    Check(runtime.bytes[item]==0u && state.ReadWord(208u)==0u && runtime.Float(item+4u)==1.f,
          "PC24 age addition reaches lifetime before the original FCOMP");
}

void TestMovementAndTransformModes() {
    for (unsigned mode = 0; mode < 3; ++mode) {
        pal4::ParticleVisualState state;
        Runtime runtime;
        Seed(state, runtime);
        state.WriteWord(240u, std::bit_cast<std::uint32_t>(2.f));
        if (mode != 0u) {
            state.WriteWord(124u, 1u);
            state.WriteByte(53u, 1u);
            state.WriteByte(54u, mode == 2u ? 1u : 0u);
        }
        Check(pal4::ParticleVisualObject::SimulateParticles(
                  state, 0.5, runtime) == 1u,
              "one simulated item");
        Check(runtime.Float(item + 4u) == 0.5f &&
                  runtime.Float(item + 48u) == 3.f,
              "age and gravity use delta");
        Check(runtime.Float(item + 68u) == (mode == 2u ? 4.f : 14.f) &&
                  runtime.Float(item + 72u) == (mode == 2u ? 7.f : 27.f) &&
                  runtime.Float(item + 76u) == (mode == 2u ? 10.f : 40.f),
              "movement uses selected matrix path");
        Check(runtime.transforms ==
                  (mode == 2u ? std::vector<std::uint32_t>{} :
                   std::vector<std::uint32_t>{mode == 0u ? 0x9000u :
                                               0x1010u}),
              "matrix selection");
        Check(runtime.Float(item + 20u) == 8.f &&
                  runtime.Float(item + 24u) == 9.f,
              "default size copied");
    }
}

void TestOptionalUpdates() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state, runtime);
    state.WriteByte(4u, 1u);
    state.WriteByte(420u, 1u);
    state.WriteByte(316u, 1u);
    state.WriteByte(384u, 1u);
    state.WriteWord(244u, std::bit_cast<std::uint32_t>(0.0001f));
    runtime.words[item + 80u] = 0x4000u;
    runtime.words[item + 84u] = 0x5000u;
    runtime.words[item + 88u] = 0x6000u;
    runtime.Float(0x6004u, 6.f);
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        runtime.Float(0x4000u + i * 4u, static_cast<float>(i + 1u));
        runtime.Float(0x4010u + i * 4u, 2.f);
    }
    runtime.Float(0x5000u, 10.f);
    runtime.Float(0x5004u, 20.f);
    runtime.Float(0x5008u, 4.f);
    runtime.Float(0x500cu, 8.f);
    (void)pal4::ParticleVisualObject::SimulateParticles(state, 0.5, runtime);
    Check(runtime.normalized == std::vector<std::uint32_t>{item + 68u} &&
              runtime.transforms.empty(),
          "normalization and local movement gate");
    Check(runtime.Float(item + 16u) == 4.f &&
              runtime.Float(item + 28u) == 2.f &&
              runtime.Float(item + 40u) == 5.f &&
              runtime.Float(item + 20u) == 12.f &&
              runtime.Float(item + 24u) == 24.f,
          "optional rotation, color and size curves");

    state.WriteByte(3u, 1u);
    state.WriteByte(384u, 0u);
    for (std::uint32_t i = 0u; i < 4u; ++i)
        state.WriteWord(212u + i * 4u, 0xaaa00000u + i);
    runtime.normalized.clear();
    state.WriteWord(244u, std::bit_cast<std::uint32_t>(-0.00005f));
    (void)pal4::ParticleVisualObject::SimulateParticles(state, 0.5, runtime);
    Check(runtime.normalized.empty(), "subthreshold rate skips normalization");
    for (std::uint32_t i = 0u; i < 4u; ++i)
        Check(runtime.words[item + 28u + i * 4u] == 0xaaa00000u + i,
              "direct color overrides curve");
    state.WriteWord(244u, std::bit_cast<std::uint32_t>(-0.0001f));
    (void)pal4::ParticleVisualObject::SimulateParticles(state, 0.5, runtime);
    Check(runtime.normalized == std::vector<std::uint32_t>{item + 68u},
          "negative rate at threshold normalizes");

    runtime.normalized.clear();
    state.WriteWord(244u, 0u);
    (void)pal4::ParticleVisualObject::SimulateParticles(state, 0.5, runtime);
    Check(runtime.normalized.empty(), "zero rate skips normalization");
}

void TestNormalizedPositionModes() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    state.WriteWord(44u, 0x1000u);
    state.WriteByte(4u, 1u);
    state.WriteWord(244u, std::bit_cast<std::uint32_t>(2.f));
    state.WriteWord(248u, std::bit_cast<std::uint32_t>(4.f));
    state.WriteWord(252u, std::bit_cast<std::uint32_t>(5.f));
    state.WriteWord(256u, std::bit_cast<std::uint32_t>(6.f));
    runtime.Float(item + 68u, 1.f);
    runtime.Float(item + 72u, 2.f);
    runtime.Float(item + 76u, 3.f);
    runtime.normalized_result = {0.6f, 0.8f, 0.f};
    pal4::ParticleVisualObject::UpdatePositionNormalized(
        state, item + 68u, 0.5, runtime);
    Check(runtime.normalized.size() == 1u &&
              runtime.last_direction.x == 3.f &&
              runtime.last_direction.y == 3.f &&
              runtime.last_direction.z == 3.f &&
              runtime.position_matrix == 0u &&
              std::fabs(runtime.Float(item + 68u) - 1.6f) < 0.0001f &&
              std::fabs(runtime.Float(item + 72u) - 2.8f) < 0.0001f,
          "local target is normalized and scaled by rate times delta");

    state.WriteWord(124u, 1u);
    state.WriteByte(53u, 1u);
    state.WriteByte(54u, 0u);
    runtime.Float(item + 68u, 1.f);
    runtime.Float(item + 72u, 2.f);
    runtime.Float(item + 76u, 3.f);
    pal4::ParticleVisualObject::UpdatePositionNormalized(
        state, item + 68u, 0.5, runtime);
    Check(runtime.position_matrix == 0x1010u &&
              runtime.last_direction.x == 13.f &&
              runtime.last_direction.y == 23.f &&
              runtime.last_direction.z == 33.f,
          "collection child target uses frame plus 16");
    state.WriteByte(53u, 0u);
    state.WriteByte(4u, 0u);
    pal4::ParticleVisualObject::UpdatePositionNormalized(
        state, item + 68u, 0.5, runtime);
    Check(runtime.position_matrix == 0x9000u,
          "world target uses frame object transform");
    state.WriteByte(53u, 1u);
    state.WriteByte(54u, 1u);
    runtime.position_matrix = 0u;
    pal4::ParticleVisualObject::UpdatePositionNormalized(
        state, item + 68u, 0.5, runtime);
    Check(runtime.position_matrix == 0u,
          "collection local target bypasses transform");
}

}  // namespace

int main() {
    TestEmptyAndInactive();
    TestAgeAndExpiry();
    TestMovementAndTransformModes();
    TestOptionalUpdates();
    TestNormalizedPositionModes();
}
