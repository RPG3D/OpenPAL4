#include "pal4/pal_actor_control_floor_sound.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IActorControlFloorSoundRuntime {
    pal4::ActorControlFloorText floor{};
    std::vector<std::string> events;

    pal4::ActorControlFloorText CurrentFloorText() override {
        events.emplace_back("floor");
        return floor;
    }
    std::uint32_t ClassifyFloorSound(
        std::string_view path, pal4::PalActorControlState&) override {
        events.emplace_back("classify " + std::string(path));
        return 0x77u;
    }
};

}  // namespace

int main() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    state.words[224u] = std::bit_cast<std::uint32_t>(0.15f);
    Check(pal4::PalActorControlFloorSound::Tick(
              state, 0.1f, runtime) == 0u &&
              state.words[224u] == std::bit_cast<std::uint32_t>(0.15f) &&
              runtime.events.empty(),
          "absent actor leaves timer and floor lookup untouched");

    state.words[1u] = 0x1234u;
    Check(pal4::PalActorControlFloorSound::Tick(
              state, 0.01f, runtime) == 0x100u &&
              std::abs(std::bit_cast<float>(state.words[224u]) - 0.16f) <
                  0.00001f && runtime.events.empty(),
          "below threshold stores timer and returns FPU C0 bit");

    Check(pal4::PalActorControlFloorSound::Tick(
              state, 0.04f, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"floor"} &&
              state.words[224u] != 0u,
          "empty floor text does not reset timer");

    runtime.events.clear();
    runtime.floor = {5u, "WF011"};
    Check(pal4::PalActorControlFloorSound::Tick(
              state, 0.0f, runtime) == 0x77u &&
              state.words[224u] == 0u &&
              runtime.events == std::vector<std::string>{
                  "floor", "classify WF011"},
          "nonempty floor text resets timer before classification");

    runtime.events.clear();
    runtime.floor = {1u, {}};
    state.words[224u] = std::bit_cast<std::uint32_t>(0.2f);
    Check(pal4::PalActorControlFloorSound::Tick(
              state, 0.0f, runtime) == 0x77u &&
              runtime.events == std::vector<std::string>{
                  "floor", "classify "},
          "positive text length with null data uses empty path");

    runtime.events.clear();
    state.words[224u] = std::bit_cast<std::uint32_t>(NAN);
    Check(pal4::PalActorControlFloorSound::Tick(
              state, 0.0f, runtime) == 0x100u &&
              std::isnan(std::bit_cast<float>(state.words[224u])) &&
              runtime.events.empty(),
          "unordered FPU comparison skips floor lookup");
}
