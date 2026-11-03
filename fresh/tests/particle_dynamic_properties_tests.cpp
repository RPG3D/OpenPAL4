#include "pal4/particle_visual_object.h"
#include "support/particle_x87_reference.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.002f;
}

struct Runtime final : pal4::IParticleEmissionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::int32_t> draws;
    std::size_t next{};
    std::uint32_t transform{};
    std::uint32_t transformed_with{};
    int normalize_calls{};

    std::int32_t Random() override {
        Check(next < draws.size(), "unexpected random draw");
        return draws[next++];
    }
    std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    void FreeBuffer(std::uint32_t) override {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    pal4::ParticleVisualPoint Normalize(
        pal4::ParticleVisualPoint point) override {
        ++normalize_calls;
        return {point.x * 2.f, point.y * 3.f, point.z * 4.f};
    }
    std::uint32_t ObjectTransform(std::uint32_t frame) override {
        transform = frame;
        return 0x7777u;
    }
    pal4::ParticleVisualPoint TransformPosition(
        std::uint32_t matrix, pal4::ParticleVisualPoint point) override {
        transformed_with = matrix;
        return {point.x + 10.f, point.y + 20.f, point.z + 30.f};
    }
    std::uint32_t PluginOffset() override { return 0u; }
    void Float(std::uint32_t address, float value) {
        words[address] = std::bit_cast<std::uint32_t>(value);
    }
    float Float(std::uint32_t address) {
        return std::bit_cast<float>(words[address]);
    }
};

void SetFloat(pal4::ParticleVisualState& state,
              std::size_t offset, float value) {
    state.WriteWord(offset, std::bit_cast<std::uint32_t>(value));
}

void Seed(pal4::ParticleVisualState& state) {
    SetFloat(state, 276u, 4.f);
    SetFloat(state, 284u, 2.f);
    SetFloat(state, 292u, 1.f);
    SetFloat(state, 296u, 2.f);
    SetFloat(state, 300u, 3.f);
    SetFloat(state, 132u, 4.f);
    SetFloat(state, 136u, 6.f);
    SetFloat(state, 140u, 8.f);
    state.WriteWord(236u, 0x12345678u);
    state.WriteByte(4u, 1u);
}

void TestNullAndBox() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state);
    pal4::ParticleVisualObject::InitializeDynamicProperties(
        state, 0u, runtime);
    Check(runtime.next == 0u && runtime.words.empty(),
          "null particle is untouched");
    runtime.draws.assign(8u, 0);
    pal4::ParticleVisualObject::InitializeDynamicProperties(
        state, 0x1000u, runtime);
    Check(runtime.next == 8u && runtime.normalize_calls == 1 &&
              runtime.bytes[0x1000u] == 1u &&
              runtime.words[0x1004u] == 0u &&
              runtime.words[0x1010u] == 0x12345678u,
          "box initializer consumes lifetime, speed, direction and position");
    Check(Near(runtime.Float(0x1008u), 4.f) &&
              Near(runtime.Float(0x100cu), 2.f) &&
              Near(runtime.Float(0x1038u), 2.f) &&
              Near(runtime.Float(0x103cu), 6.f) &&
              Near(runtime.Float(0x1040u), 12.f) &&
              Near(runtime.Float(0x102cu), 4.f) &&
              Near(runtime.Float(0x1030u), 12.f) &&
              Near(runtime.Float(0x1034u), 24.f),
          "normalized direction and scaled velocity follow native layout");
    Check(Near(runtime.Float(0x1044u), -2.f) &&
              Near(runtime.Float(0x1048u), -4.f) &&
              Near(runtime.Float(0x104cu), -3.f) &&
              runtime.transformed_with == 0u,
          "box stores x, y, z in native sampling order");
}

void TestSkinAndTransform() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state);
    state.WriteByte(55u, 1u);
    state.WriteWord(64u, 0x5000u);
    state.WriteWord(68u, 0x5018u);
    SetFloat(state, 76u, 0.f);
    SetFloat(state, 80u, 0.f);
    SetFloat(state, 84u, 0.f);
    runtime.Float(0x500cu, 7.f);
    runtime.Float(0x5010u, 8.f);
    runtime.Float(0x5014u, 9.f);
    runtime.draws.assign(9u, 0);
    runtime.draws[5u] = 1;
    pal4::ParticleVisualObject::InitializeDynamicProperties(
        state, 0x1000u, runtime);
    Check(runtime.next == 9u &&
              Near(runtime.Float(0x1044u), 7.f) &&
              Near(runtime.Float(0x1048u), 8.f) &&
              Near(runtime.Float(0x104cu), 9.f) &&
              runtime.transformed_with == 0u,
          "skin vertex selection skips world transform");

    pal4::ParticleVisualState frame_state;
    Runtime frame_runtime;
    Seed(frame_state);
    frame_state.WriteByte(4u, 0u);
    frame_state.WriteWord(44u, 0x4000u);
    frame_state.WriteWord(124u, 1u);
    frame_state.WriteByte(53u, 1u);
    frame_state.WriteByte(54u, 0u);
    frame_runtime.draws.assign(8u, 0);
    pal4::ParticleVisualObject::InitializeDynamicProperties(
        frame_state, 0x1000u, frame_runtime);
    Check(frame_runtime.transformed_with == 0x4010u &&
              frame_runtime.transform == 0u &&
              Near(frame_runtime.Float(0x1044u), 8.f) &&
              Near(frame_runtime.Float(0x1048u), 16.f) &&
              Near(frame_runtime.Float(0x104cu), 27.f),
          "collection child frame uses direct matrix at frame plus 16");
    frame_state.WriteByte(53u, 0u);
    frame_runtime.next = 0u;
    pal4::ParticleVisualObject::InitializeDynamicProperties(
        frame_state, 0x1100u, frame_runtime);
    Check(frame_runtime.transform == 0x4000u &&
              frame_runtime.transformed_with == 0x7777u,
          "ordinary frame path obtains object transform");
}

void TestCylinderAndSphere() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state);
    state.WriteWord(128u, 1u);
    runtime.draws.assign(9u, 0);
    pal4::ParticleVisualObject::InitializeDynamicProperties(
        state, 0x1000u, runtime);
    Check(runtime.next == 9u && Near(runtime.Float(0x1044u), 0.f) &&
              Near(runtime.Float(0x104cu), 2.f) &&
              Near(runtime.Float(0x1048u), -4.f),
          "cylinder draws angle, two radii and height");

    state.WriteWord(128u, 2u);
    runtime.next = 0u;
    runtime.draws.assign(11u, 0);
    runtime.draws[5u] = 32767;
    runtime.draws[6u] = 32767;
    runtime.draws[7u] = 32767;
    runtime.draws[8u] = 16384;
    runtime.draws[9u] = 16384;
    runtime.draws[10u] = 16384;
    pal4::ParticleVisualObject::InitializeDynamicProperties(
        state, 0x1100u, runtime);
    Check(runtime.next == 11u &&
              Near(runtime.Float(0x1144u), 0.f) &&
              Near(runtime.Float(0x1148u), 0.f) &&
              Near(runtime.Float(0x114cu), 0.f),
          "sphere rejects first outlying triple and accepts second");
}

void TestCurvesAndDrawOrder() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state);
    state.WriteByte(316u, 1u);
    state.WriteByte(384u, 1u);
    state.WriteByte(420u, 1u);
    state.WriteWord(212u, std::bit_cast<std::uint32_t>(0.25f));
    for (std::size_t i = 0; i < 4u; ++i) {
        SetFloat(state, 320u + i * 4u, static_cast<float>(i + 1u));
        SetFloat(state, 352u + i * 4u, static_cast<float>(i + 5u));
    }
    SetFloat(state, 388u, 2.f);
    SetFloat(state, 392u, 3.f);
    SetFloat(state, 404u, 6.f);
    SetFloat(state, 408u, 7.f);
    SetFloat(state, 424u, 10.f);
    SetFloat(state, 432u, 14.f);
    runtime.words[0x1050u] = 0x2000u;
    runtime.words[0x1054u] = 0x3000u;
    runtime.words[0x1058u] = 0x4000u;
    runtime.draws.assign(22u, 0);
    pal4::ParticleVisualObject::InitializeDynamicProperties(
        state, 0x1000u, runtime);
    Check(runtime.next == 22u &&
              Near(runtime.Float(0x2000u), 1.f) &&
              Near(runtime.Float(0x200cu), 4.f) &&
              Near(runtime.Float(0x2010u), 1.f) &&
              Near(runtime.Float(0x201cu), 1.f) &&
              Near(runtime.Float(0x3000u), 2.f) &&
              Near(runtime.Float(0x3004u), 3.f) &&
              Near(runtime.Float(0x3008u), 1.f) &&
              Near(runtime.Float(0x300cu), 1.f) &&
              Near(runtime.Float(0x4000u), 10.f) &&
              Near(runtime.Float(0x4004u), 4.f) &&
              Near(runtime.Float(0x1010u), 10.f),
          "optional curves store starts, slopes and initial rotation");
    state.WriteByte(3u, 1u);
    runtime.next = 0u;
    runtime.draws.assign(14u, 0);
    pal4::ParticleVisualObject::InitializeDynamicProperties(
        state, 0x1100u, runtime);
    Check(runtime.next == 14u &&
              runtime.words[0x111cu] == state.ReadWord(212u),
          "fixed color copies four words without eight color draws");
}

void TestDistinctRandomDraws() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state);
    SetFloat(state, 280u, 1.f);
    SetFloat(state, 288u, 1.f);
    SetFloat(state, 304u, 1.f);
    SetFloat(state, 308u, 1.f);
    SetFloat(state, 312u, 1.f);
    state.WriteByte(316u, 1u);
    for (std::size_t i = 0; i < 4u; ++i) {
        SetFloat(state, 320u + i * 4u, static_cast<float>(i + 1u));
        SetFloat(state, 336u + i * 4u, 1.f);
        SetFloat(state, 352u + i * 4u, static_cast<float>(i + 5u));
        SetFloat(state, 368u + i * 4u, 1.f);
    }
    runtime.words[0x1050u] = 0x2000u;
    runtime.draws = {0, 32767, 0, 16384, 32767,
                     32767, 0, 16384,
                     0, 32767, 16384, 0,
                     32767, 0, 0, 32767};
    pal4::ParticleVisualObject::InitializeDynamicProperties(
        state, 0x1000u, runtime);
    Check(runtime.next == 16u &&
              Near(runtime.Float(0x1008u), 3.f) &&
              Near(runtime.Float(0x100cu), 3.f) &&
              Near(runtime.Float(0x1038u), 0.f) &&
              Near(runtime.Float(0x103cu), 6.f) &&
              Near(runtime.Float(0x1040u), 16.f) &&
              Near(runtime.Float(0x1044u), 2.f) &&
              Near(runtime.Float(0x1048u), 0.f) &&
              Near(runtime.Float(0x104cu), -3.f),
          "distinct draws map to lifetime, speed, axes and box coordinates");
    Check(Near(runtime.Float(0x2000u), 0.f) &&
              Near(runtime.Float(0x2004u), 3.f) &&
              Near(runtime.Float(0x2008u), 3.f) &&
              Near(runtime.Float(0x200cu), 3.f) &&
              Near(runtime.Float(0x2010u), 2.f) &&
              Near(runtime.Float(0x2014u), 2.f / 3.f) &&
              Near(runtime.Float(0x2018u), 1.f) &&
              Near(runtime.Float(0x201cu), 2.f),
          "four color starts precede all four end draws");
}

}  // namespace

void TestPc24LifetimeSamples() {
    // Actual491 parameters are center2/radius1. This fingerprint differs by
    // one bit if the multiply/add chain is kept wide until the final store.
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state);
    SetFloat(state,276u,2.f);SetFloat(state,280u,1.f);
    runtime.draws.assign(8u,0);runtime.draws[0]=33;
    pal4::ParticleVisualObject::InitializeDynamicProperties(state,0x1000u,runtime);
    Check(runtime.words[0x1008u]==0x3f804200u && runtime.next==8u,
          "PC24 lifetime rounds each original arithmetic operation without another draw");
#if defined(_MSC_VER) && defined(_M_IX86)
    for (const auto sample : {0,1,33,101,8191,16384,24577,32766,32767}) {
        for (const auto pair : {std::pair{2.f,1.f},std::pair{1.f,1.f},
                                std::pair{0.3f,0.1f},std::pair{10.f,7.f}}) {
            Runtime values;pal4::ParticleVisualState item_state;Seed(item_state);
            SetFloat(item_state,276u,pair.first);SetFloat(item_state,280u,pair.second);
            values.draws.assign(8u,0);values.draws[0]=sample;
            pal4::ParticleVisualObject::InitializeDynamicProperties(item_state,0x1000u,values);
            Check(values.words[0x1008u]==std::bit_cast<std::uint32_t>(
                      pal4::test::ParticleRangePc24(sample,pair.first,pair.second)) && values.next==8u,
                  "lifetime bits match the original x87 instruction sequence at PC24");
        }
    }
#endif
}

void TestPc24SphereRejection() {
    for(const bool accept_boundary:{true,false}) {
        const std::array<std::int32_t,3> draws=accept_boundary?
            std::array<std::int32_t,3>{5359,13000,28021}:
            std::array<std::int32_t,3>{5271,13000,27937};
        pal4::ParticleVisualState state;Runtime runtime;Seed(state);
        state.WriteWord(128u,2u);SetFloat(state,132u,2.f);
        runtime.draws.assign(5u,0);
        runtime.draws.insert(runtime.draws.end(),draws.begin(),draws.end());
        runtime.draws.insert(runtime.draws.end(),3u,16384);
#if defined(_MSC_VER) && defined(_M_IX86)
        const auto original=pal4::test::ParticleSpherePc24(draws[0],draws[1],draws[2],2.f);
        Check(original.accepted==accept_boundary &&
                  std::bit_cast<std::uint32_t>(original.squared)==
                  (accept_boundary?0x3f800000u:0x3f800002u),
              "actual x87 accepts the boundary but rejects the rounded outside point");
        const auto wanted=accept_boundary?original:
            pal4::test::ParticleSpherePc24(16384,16384,16384,2.f);
#endif
        pal4::ParticleVisualObject::InitializeDynamicProperties(state,0x1000u,runtime);
        Check(runtime.next==(accept_boundary?8u:11u),
              "particle sphere retains original PC24 acceptance and rejection draw counts");
#if defined(_MSC_VER) && defined(_M_IX86)
        Check(std::array{runtime.words[0x1044u],runtime.words[0x1048u],runtime.words[0x104cu]}==
                  std::bit_cast<std::array<std::uint32_t,3>>(wanted.position),
              "particle sphere output bits match the actual x87 sequence");
#endif
    }
#if defined(_MSC_VER) && defined(_M_IX86)
    for(const auto sample:{0,1,33,8191,10000,15000,20000,24577,32766,32767}) {
        for(std::size_t axis=0;axis<3u;++axis) {
            for(const float diameter:{0.f,0.3f,2.f,5.f}) {
                std::array<std::int32_t,3> draws{16384,16384,16384};draws[axis]=sample;
                const auto first=pal4::test::ParticleSpherePc24(draws[0],draws[1],draws[2],diameter);
                const auto wanted=first.accepted?first:
                    pal4::test::ParticleSpherePc24(16384,16384,16384,diameter);
                pal4::ParticleVisualState state;Runtime runtime;Seed(state);
                state.WriteWord(128u,2u);SetFloat(state,132u,diameter);
                runtime.draws.assign(5u,0);
                runtime.draws.insert(runtime.draws.end(),draws.begin(),draws.end());
                runtime.draws.insert(runtime.draws.end(),3u,16384);
                pal4::ParticleVisualObject::InitializeDynamicProperties(state,0x1000u,runtime);
                Check(runtime.next==(first.accepted?8u:11u) &&
                          std::array{runtime.words[0x1044u],runtime.words[0x1048u],runtime.words[0x104cu]}==
                          std::bit_cast<std::array<std::uint32_t,3>>(wanted.position),
                      "sphere axes, diameter scaling and draw counts match actual x87 instructions");
            }
        }
    }
#endif
}

int main() {
    TestPc24SphereRejection();
    TestPc24LifetimeSamples();
    TestNullAndBox();
    TestSkinAndTransform();
    TestCylinderAndSphere();
    TestCurvesAndDrawOrder();
    TestDistinctRandomDraws();
}
