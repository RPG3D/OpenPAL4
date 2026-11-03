#include "pal4/particle_visual_object.h"

#include <bit>
#include <algorithm>
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

struct Runtime final : pal4::IParticleEmissionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::int32_t> random_values;
    std::size_t next_random{};
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> initialized;
    std::vector<pal4::ParticleRandomSite> sites;
    std::uint32_t next_allocation = 0x3000u;

    std::int32_t Random() override {
        if (next_random == random_values.size()) return 0;
        return random_values[next_random++];
    }
    std::int32_t RandomFor(pal4::ParticleRandomSite site) override {
        sites.push_back(site);
        return Random();
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        allocations.push_back(size);
        const auto address = next_allocation;
        next_allocation += 0x100u;
        return address;
    }
    void FreeBuffer(std::uint32_t address) override {
        frees.push_back(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
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
        if (value == 1u) initialized.push_back(address);
    }
    pal4::ParticleVisualPoint Normalize(
        pal4::ParticleVisualPoint point) override { return point; }
    std::uint32_t ObjectTransform(std::uint32_t frame) override {
        return frame;
    }
    pal4::ParticleVisualPoint TransformPosition(
        std::uint32_t, pal4::ParticleVisualPoint point) override {
        return point;
    }
    std::uint32_t PluginOffset() override { return 0x10u; }
    void Float(std::uint32_t address, float value) {
        words[address] = std::bit_cast<std::uint32_t>(value);
    }
};

void Seed(pal4::ParticleVisualState& state, Runtime& runtime) {
    state.WriteWord(40u, 0x9000u);
    state.WriteWord(200u, 5u);
    state.WriteWord(260u, 3u);
    state.WriteWord(264u, 0u);
    state.WriteWord(268u, std::bit_cast<std::uint32_t>(1.f));
    state.WriteWord(272u, 0u);
    runtime.words[0x9010u] = 0xa000u;
    runtime.words[0xa040u] = 0x10u;
}

void TestThresholdAndPluginSync() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state, runtime);
    state.WriteWord(148u, std::bit_cast<std::uint32_t>(0.25f));
    state.WriteWord(152u, std::bit_cast<std::uint32_t>(1.f));
    state.WriteWord(204u, 3u);
    Check(pal4::ParticleVisualObject::EmitTankParticles(
              state, 0.5, runtime) == 0xa000 &&
              std::bit_cast<float>(state.ReadWord(148u)) == 0.75f &&
              runtime.next_random == 0u && runtime.allocations.empty() &&
              runtime.words[0xa040u] == 0x00800010u &&
              runtime.words[0xa004u] == 3u,
          "below threshold still synchronizes PTank plugin");

    state.WriteWord(148u, std::bit_cast<std::uint32_t>(0.5f));
    state.WriteWord(200u, 0u);
    runtime.random_values = {0};
    Check(pal4::ParticleVisualObject::EmitTankParticles(
              state, 0.50000001, runtime) == 0xa000 &&
              state.ReadWord(148u) == 0u &&
              std::bit_cast<float>(state.ReadWord(152u)) == 1.f &&
              runtime.next_random == 1u,
          "threshold reached resets elapsed and draws the next period");
}

void TestPc24Threshold() {
    for (bool tank : {false, true}) {
        for (const auto delta : {0.017, 0.49999999}) {
            pal4::ParticleVisualState state;
            Runtime runtime;
            Seed(state, runtime);
            state.WriteWord(200u, 0u);
            const float elapsed = delta == 0.017 ? 0.f : 0.5f;
            const float period = delta == 0.017 ? 0.017f : 1.f;
            state.WriteWord(148u, std::bit_cast<std::uint32_t>(elapsed));
            state.WriteWord(152u, std::bit_cast<std::uint32_t>(period));
            runtime.random_values = {0};
            if (tank) (void)pal4::ParticleVisualObject::EmitTankParticles(state, delta, runtime);
            else (void)pal4::ParticleVisualObject::EmitCollectionParticles(state, delta, runtime);
            Check(state.ReadWord(148u) == 0u && runtime.next_random == 1u,
                  "PC24 FADD reaches threshold before FCOMP in both emission paths");
        }
    }
}

void TestNonpositiveRequestReuse() {
    for (bool tank : {false, true}) {
        for (const auto requested : {0, -1}) {
            pal4::ParticleVisualState state;
            Runtime runtime;
            Seed(state, runtime);
            state.WriteWord(260u, static_cast<std::uint32_t>(requested));
            state.WriteWord(160u, 0x1000u);
            state.WriteWord(164u, 0x100cu);
            state.WriteWord(168u, 0x100cu);
            state.WriteWord(204u, 3u);
            state.WriteWord(208u, 1u);
            runtime.words[0x1000u] = 0x2000u;
            runtime.words[0x1004u] = 0x2100u;
            runtime.words[0x1008u] = 0x2200u;
            runtime.bytes[0x2000u] = 1u;
            runtime.bytes[0x2100u] = 0u;
            runtime.bytes[0x2200u] = 0u;
            runtime.random_values.assign(64, 0);
            if (tank) (void)pal4::ParticleVisualObject::EmitTankParticles(state, 0.5, runtime);
            else (void)pal4::ParticleVisualObject::EmitCollectionParticles(state, 0.5, runtime);
            Check(runtime.initialized == std::vector<std::uint32_t>{0x2100u} &&
                      state.ReadWord(204u) == 3u && state.ReadWord(208u) == 2u &&
                      runtime.allocations.empty() && runtime.next_random > 1u,
                  "nonpositive request still revives the first inactive slot before testing count");
        }
    }
}

void TestReuseAndGrowth() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state, runtime);
    state.WriteWord(160u, 0x1000u);
    state.WriteWord(164u, 0x1008u);
    state.WriteWord(168u, 0x1008u);
    state.WriteWord(204u, 2u);
    state.WriteWord(208u, 1u);
    state.WriteByte(316u, 1u);
    state.WriteByte(384u, 1u);
    state.WriteByte(420u, 1u);
    runtime.words[0x1000u] = 0x2000u;
    runtime.words[0x1004u] = 0x2100u;
    runtime.bytes[0x2000u] = 1u;
    runtime.bytes[0x2100u] = 0u;
    runtime.random_values = {0};
    Check(pal4::ParticleVisualObject::EmitTankParticles(
              state, 0.5, runtime) == 0xa000 &&
              runtime.initialized == std::vector<std::uint32_t>{
                  0x2100u, 0x3000u, 0x3500u} &&
              state.ReadWord(204u) == 4u && state.ReadWord(208u) == 4u,
          "inactive slot is reused before allocating two particles");
    using Site = pal4::ParticleRandomSite;
    Check(runtime.sites.size() == 67u &&
              std::count(runtime.sites.begin(), runtime.sites.end(), Site::period) == 1 &&
              std::count(runtime.sites.begin(), runtime.sites.end(), Site::properties) == 15 &&
              std::count(runtime.sites.begin(), runtime.sites.end(), Site::position) == 9 &&
              std::count(runtime.sites.begin(), runtime.sites.end(), Site::color) == 24 &&
              std::count(runtime.sites.begin(), runtime.sites.end(), Site::size) == 12 &&
              std::count(runtime.sites.begin(), runtime.sites.end(), Site::rotation) == 6,
          "random site observation preserves one draw per sample across all birth channels");
    Check(runtime.allocations == std::vector<std::uint32_t>{
              92u, 32u, 16u, 8u, 16u, 92u, 32u, 16u, 8u} &&
              runtime.frees == std::vector<std::uint32_t>{0x1000u} &&
              state.ReadWord(160u) == 0x3400u &&
              state.ReadWord(164u) == 0x3410u &&
              state.ReadWord(168u) == 0x3410u,
          "new particle buffers and pointer-array capacity grow in order");
    Check(runtime.words[0x3400u] == 0x2000u &&
              runtime.words[0x3404u] == 0x2100u &&
              runtime.words[0x3408u] == 0x3000u &&
              runtime.words[0x340cu] == 0x3500u &&
              runtime.words[0x3050u] == 0x3100u &&
              runtime.words[0x3054u] == 0x3200u &&
              runtime.words[0x3058u] == 0x3300u &&
              runtime.words[0x3550u] == 0x3600u &&
              runtime.words[0x3554u] == 0x3700u &&
              runtime.words[0x3558u] == 0x3800u &&
              runtime.words[0xa004u] == 4u,
          "array pointers, optional buffers and PTank count");
}

void TestBudgetAndJitter() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state, runtime);
    state.WriteByte(5u, 1u);
    state.WriteWord(8u, 2u);
    state.WriteWord(12u, 5u);
    state.WriteWord(260u, 1u);
    runtime.random_values = {0};
    (void)pal4::ParticleVisualObject::EmitTankParticles(
        state, 1.5, runtime);
    Check(state.ReadWord(12u) == 2u &&
              state.ReadWord(204u) == 1u &&
              state.ReadWord(208u) == 1u,
          "budget decrements but old limit governs this emission");

    pal4::ParticleVisualState varied;
    Runtime random_runtime;
    Seed(varied, random_runtime);
    varied.WriteWord(260u, 1u);
    varied.WriteWord(264u, static_cast<std::uint32_t>(-2));
    varied.WriteWord(200u, 1u);
    random_runtime.random_values = {0, 0};
    (void)pal4::ParticleVisualObject::EmitTankParticles(
        varied, 0.1, random_runtime);
    Check(varied.ReadWord(204u) == 0u &&
              random_runtime.next_random == 2u &&
              random_runtime.sites == std::vector<pal4::ParticleRandomSite>{
                  pal4::ParticleRandomSite::period, pal4::ParticleRandomSite::count},
          "negative variation draws a negative jitter and skips creation");
    varied.WriteWord(152u, 0u);
    random_runtime.random_values = {0, 0, 0, 4};
    (void)pal4::ParticleVisualObject::EmitTankParticles(
        varied, 0.1, random_runtime);
    Check(varied.ReadWord(204u) == 1u &&
              varied.ReadWord(208u) == 1u &&
              random_runtime.next_random == 4u,
          "positive jitter from negative variation is capped by limit");
}

void TestCollectionEmissionOmitsPluginUpdate() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    Seed(state, runtime);
    state.WriteWord(152u, std::bit_cast<std::uint32_t>(1.f));
    runtime.words[0xa004u] = 55u;
    Check(pal4::ParticleVisualObject::EmitCollectionParticles(
              state, 0.5, runtime) == 0x100 &&
              runtime.next_random == 0u &&
              runtime.words[0xa040u] == 0x10u &&
              runtime.words[0xa004u] == 55u,
          "collection below threshold returns compare bit without PTank writes");

    state.WriteWord(152u, 0u);
    state.WriteWord(260u, 1u);
    runtime.random_values = {0};
    Check(pal4::ParticleVisualObject::EmitCollectionParticles(
              state, 0.5, runtime) == 0 &&
              state.ReadWord(204u) == 1u &&
              state.ReadWord(208u) == 1u &&
              runtime.initialized == std::vector<std::uint32_t>{0x3000u} &&
              runtime.words[0xa040u] == 0x10u &&
              runtime.words[0xa004u] == 55u,
          "collection emission shares creation but leaves PTank plugin alone");
}

}  // namespace

int main() {
    for (const bool tank : {false,true}) {
        pal4::ParticleVisualState state;Runtime runtime;Seed(state,runtime);
        state.WriteWord(200u,0u);state.WriteWord(268u,std::bit_cast<std::uint32_t>(2.f));
        state.WriteWord(272u,std::bit_cast<std::uint32_t>(1.f));runtime.random_values={33};
        if(tank)(void)pal4::ParticleVisualObject::EmitTankParticles(state,0.5,runtime);
        else (void)pal4::ParticleVisualObject::EmitCollectionParticles(state,0.5,runtime);
        Check(state.ReadWord(152u)==0x3f804200u && runtime.next_random==1u,
              "both next-period paths preserve the original PC24 arithmetic");
        state.WriteWord(152u,0u);state.WriteByte(5u,1u);state.WriteWord(8u,2u);
        state.WriteWord(12u,5u);state.WriteWord(260u,0u);
        if(tank)(void)pal4::ParticleVisualObject::EmitTankParticles(state,1.00000001,runtime);
        else (void)pal4::ParticleVisualObject::EmitCollectionParticles(state,1.00000001,runtime);
        Check(state.ReadWord(12u)==3u,"PC24 budget multiply/subtract occurs before truncation");
    }
    TestNonpositiveRequestReuse();
    TestPc24Threshold();
    TestThresholdAndPluginSync();
    TestReuseAndGrowth();
    TestBudgetAndJitter();
    TestCollectionEmissionOmitsPluginUpdate();
}
