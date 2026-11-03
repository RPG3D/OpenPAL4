#include "pal4/particle_visual_object.h"

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

struct Runtime final : pal4::IParticleCollectionStateRuntime {
    pal4::ParticleVisualState* state{};
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::vector<std::pair<std::uint32_t, std::uint8_t>> enabled;
    bool shorten{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
    void SetElementEnabled(std::uint32_t element,
                           std::uint8_t value) override {
        enabled.emplace_back(element, value);
        if (shorten) state->WriteWord(180u, 0x1004u);
    }
};

void TestCollectionState() {
    pal4::ParticleVisualState state;
    Runtime runtime;
    runtime.state = &state;
    state.WriteWord(176u, 0x1000u);
    state.WriteWord(180u, 0x1008u);
    runtime.words[0x1000u] = 0x2000u;
    runtime.words[0x1004u] = 0x3000u;
    Check(pal4::ParticleVisualObject::SetCollectionEnabled(
              state, 1u, runtime) == 0u &&
              runtime.reads.empty() && runtime.enabled.empty(),
          "non-collection skips element callbacks");

    state.WriteWord(124u, 1u);
    Check(pal4::ParticleVisualObject::SetCollectionEnabled(
              state, 0u, runtime) == 0x1008u &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x1000u, 0x1004u} &&
              runtime.enabled ==
                  std::vector<std::pair<std::uint32_t, std::uint8_t>>{
                      {0x2000u, 0u}, {0x3000u, 0u}},
          "collection forwards flag in pointer order and returns end");

    runtime.reads.clear();
    runtime.enabled.clear();
    runtime.shorten = true;
    Check(pal4::ParticleVisualObject::SetCollectionEnabled(
              state, 1u, runtime) == 0x1004u &&
              runtime.reads == std::vector<std::uint32_t>{0x1000u} &&
              runtime.enabled ==
                  std::vector<std::pair<std::uint32_t, std::uint8_t>>{
                      {0x2000u, 1u}},
          "end pointer is reloaded after each element callback");
    runtime.reads.clear();
    runtime.enabled.clear();
    Check(pal4::ParticleVisualObject::SetCollectionEnabled(
              state, 1u, runtime) == 0x1004u &&
              runtime.enabled.size() == 1u,
          "single-item collection remains valid after shrink");
    state.WriteWord(180u, 0x1000u);
    runtime.reads.clear();
    runtime.enabled.clear();
    Check(pal4::ParticleVisualObject::SetCollectionEnabled(
              state, 1u, runtime) == 0x1000u &&
              runtime.reads.empty(),
          "empty collection returns the current end pointer");
}

}  // namespace

int main() { TestCollectionState(); }
