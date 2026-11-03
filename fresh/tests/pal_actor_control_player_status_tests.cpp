#include "pal4/pal_actor_control_player_status.h"

#include <bit>
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

struct Runtime : pal4::IActorControlPlayerStatusRuntime {
    std::vector<std::string> events;
    std::uint32_t player{};
    bool check_started_fields{};

    std::uint32_t PlayerManager() override {
        events.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t FindPlayer(std::uint32_t manager,
                             std::uint32_t index) override {
        Check(manager == 0x4000u && index == 7u,
              "player manager lookup arguments");
        events.emplace_back("find");
        return player;
    }
    std::uint32_t HandleDamageOrStatus(
        pal4::PalActorControlState& state, float value) override {
        Check(std::bit_cast<std::uint32_t>(value) == 0x3f400000u,
              "status value forwarded without conversion");
        if (check_started_fields) {
            Check(state.words[188u] == 1u &&
                      state.words[189u] == 0x11u &&
                      state.words[190u] == 0u &&
                      state.words[191u] == 0x22u &&
                      state.words[192u] == 0u &&
                      state.words[193u] == 0x3f400000u &&
                      state.words[201u] == 3u,
                  "all status fields written before handler call");
        }
        events.emplace_back("handle");
        return 0x66u;
    }
};

}  // namespace

int main() {
    pal4::PalActorControlState state{};
    state.words[19u] = 7u;
    state.words[33u] = 3u;
    Runtime runtime;
    Check(pal4::PalActorControlPlayerStatus::ApplyIfPlayerExists(
              state, 0.75f, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"manager", "find"},
          "missing player skips damage/status handler");

    runtime.events.clear();
    runtime.player = 0x5000u;
    Check(pal4::PalActorControlPlayerStatus::ApplyIfPlayerExists(
              state, 0.75f, runtime) == 0x66u &&
              runtime.events == std::vector<std::string>{
                  "manager", "find", "handle"},
          "existing player forwards float and handler result");

    runtime.events.clear();
    state.words[188u] = 0x99u;
    Check(pal4::PalActorControlPlayerStatus::Start(
              state, 0x11u, 0x22u, 0.75f, runtime) == 0x99u &&
              runtime.events.empty() && state.words[188u] == 0x99u,
          "already active state exits before manager lookup");

    state.words[188u] = 0u;
    runtime.player = 0u;
    Check(pal4::PalActorControlPlayerStatus::Start(
              state, 0x11u, 0x22u, 0.75f, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"manager", "find"} &&
              state.words[188u] == 0u,
          "missing player leaves status fields untouched");

    runtime.events.clear();
    runtime.player = 0x5000u;
    runtime.check_started_fields = true;
    state.words[190u] = 0xdeadu;
    state.words[192u] = 0xbeefu;
    Check(pal4::PalActorControlPlayerStatus::Start(
              state, 0x11u, 0x22u, 0.75f, runtime) == 0x66u &&
              runtime.events == std::vector<std::string>{
                  "manager", "find", "handle"} &&
              state.words[194u] == 0u && state.words[200u] == 0u,
          "new status writes all observed fields and preserves neighbors");
}
