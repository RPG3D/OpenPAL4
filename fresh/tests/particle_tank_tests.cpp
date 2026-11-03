#include "pal4/particle_visual_object.h"

#include <array>
#include <bit>
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

struct Runtime final : pal4::IParticleTankUpdateRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> locks;
    std::vector<std::uint32_t> transforms;
    std::uint32_t unlock_count{};

    pal4::ParticleTankLock Lock(std::uint32_t atomic,
                                std::uint32_t stream,
                                std::uint32_t access) override {
        Check(atomic == 0x9000u && access == 0x40000000u,
              "PTank lock arguments");
        locks.push_back(stream);
        switch (stream) {
            case 1u: return {0x10000u, 20u};
            case 2u: return {0x20000u, 8u};
            case 4u: return {0x30000u, 12u};
            case 128u: return {0x40000u, 20u};
            case 32u: return {0x50000u, 8u};
            default: std::exit(2);
        }
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
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t ObjectTransform(std::uint32_t frame) override {
        Check(frame == 0x5000u, "position transform uses object frame");
        return 0x5100u;
    }
    pal4::ParticleVisualPoint TransformPosition(
        std::uint32_t matrix, pal4::ParticleVisualPoint point) override {
        transforms.push_back(matrix);
        return {point.x + 10.f, point.y + 20.f, point.z + 30.f};
    }
    int Unlock(std::uint32_t atomic) override {
        Check(atomic == 0x9000u, "PTank unlock atomic");
        ++unlock_count;
        return 77;
    }
    void Float(std::uint32_t address, float value) {
        words[address] = std::bit_cast<std::uint32_t>(value);
    }
    float Float(std::uint32_t address) const {
        return std::bit_cast<float>(words.at(address));
    }
};

void Seed(pal4::ParticleVisualState& state, Runtime& runtime) {
    state.WriteWord(40u, 0x9000u);
    state.WriteWord(44u, 0x5000u);
    state.WriteWord(160u, 0x6000u);
    state.WriteWord(204u, 2u);
    runtime.words[0x6000u] = 0x7000u;
    runtime.words[0x6004u] = 0x8000u;
    runtime.bytes[0x7000u] = 1u;
    runtime.bytes[0x8000u] = 0u;
    runtime.Float(0x7004u, 0.5f);
    runtime.Float(0x7008u, 2.f);
    runtime.Float(0x7010u, 0.4f);
    runtime.Float(0x7014u, 2.f);
    runtime.Float(0x7018u, 4.f);
    runtime.Float(0x701cu, 1.f);
    runtime.Float(0x7020u, 0.5f);
    runtime.Float(0x7024u, 0.25f);
    runtime.Float(0x7028u, 1.f);
    runtime.Float(0x7044u, 1.f);
    runtime.Float(0x7048u, 2.f);
    runtime.Float(0x704cu, 3.f);
}

void TestEmptyStillLocksAndUnlocks() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    state.WriteWord(40u, 0x9000u);
    Check(pal4::ParticleVisualObject::RenderTank(state, runtime) == 77 &&
              runtime.locks == std::vector<std::uint32_t>{
                  1u, 2u, 4u, 128u, 32u} &&
              runtime.unlock_count == 1u && runtime.words.empty(),
          "empty PTank locks all streams and returns unlock result");
}

void TestActiveAndInactiveStreams() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state, runtime);
    state.WriteByte(2u, 1u);
    state.WriteByte(4u, 1u);
    runtime.words[0x10014u] = 0xdeadbeefu;
    runtime.words[0x3000cu] = 0xdeadbeefu;
    runtime.words[0x40014u] = 0xdeadbeefu;
    runtime.words[0x50008u] = 0xdeadbeefu;
    Check(pal4::ParticleVisualObject::RenderTank(state, runtime) == 77 &&
              runtime.transforms == std::vector<std::uint32_t>{0x5100u} &&
              runtime.unlock_count == 1u,
          "active position transforms once and unlock result is returned");
    Check(runtime.Float(0x10000u) == 11.f &&
              runtime.Float(0x10004u) == 22.f &&
              runtime.Float(0x10008u) == 33.f &&
              runtime.bytes[0x20000u] == 255u &&
              runtime.bytes[0x20001u] == 128u &&
              runtime.bytes[0x20002u] == 64u &&
              runtime.bytes[0x20003u] == 191u,
          "active position, RGB and lifetime alpha");
    for (std::uint32_t channel = 0u; channel < 4u; ++channel)
        Check(runtime.bytes[0x20008u + channel] == 0u,
              "inactive slot clears its strided color");
    Check(runtime.Float(0x30000u) == 2.f &&
              runtime.Float(0x30004u) == 4.f &&
              runtime.words[0x40000u] == 0u &&
              runtime.words[0x40004u] == 0u &&
              runtime.Float(0x40008u) == 1.f &&
              runtime.Float(0x4000cu) == 1.f &&
              runtime.Float(0x50000u) == 0.4f,
          "active size, UV and rotation streams");
    Check(runtime.words[0x10014u] == 0xdeadbeefu &&
              runtime.words[0x3000cu] == 0xdeadbeefu &&
              runtime.words[0x40014u] == 0xdeadbeefu &&
              runtime.words[0x50008u] == 0xdeadbeefu,
          "inactive position, size, UV and rotation remain untouched");
}

void TestOpaqueAndLowByteColor() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state, runtime);
    state.WriteWord(204u, 1u);
    runtime.Float(0x701cu, 1.5f);
    runtime.Float(0x7020u, -0.1f);
    runtime.Float(0x7028u, 0.5f);
    (void)pal4::ParticleVisualObject::RenderTank(state, runtime);
    Check(runtime.transforms.empty() &&
              runtime.bytes[0x20000u] == 127u &&
              runtime.bytes[0x20001u] == 231u &&
              runtime.bytes[0x20003u] == 128u,
          "no transform or fade; color conversion keeps low byte");
}

}  // namespace

int main() {
    TestEmptyStillLocksAndUnlocks();
    TestActiveAndInactiveStreams();
    TestOpaqueAndLowByteColor();
}
