#include "pal4/pal_actor_control_loop.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IPalActorControlLoopRuntime {
    std::uint32_t* database{};
    std::uint32_t actor{};
    std::uint32_t event_context{};
    unsigned calls{};

    void ResetPlayerDatabase(std::uint32_t* value,
                             std::uint32_t current_actor,
                             std::uint32_t context) override {
        database = value;
        actor = current_actor;
        event_context = context;
        ++calls;
    }
};

}  // namespace

int main() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    pal4::PalActorControlLoop::ResetPlayerToJumpStart(
        state, 0x5000u, runtime);
    Check(runtime.calls == 0u, "absent actor skips database reset");

    state.words[1u] = 0x1234u;
    pal4::PalActorControlLoop::ResetPlayerToJumpStart(
        state, 0x5000u, runtime);
    Check(runtime.calls == 1u &&
              runtime.database == state.words.data() + 35u &&
              runtime.actor == 0x1234u &&
              runtime.event_context == 0x5000u &&
              state.words[1u] == 0x1234u,
          "actor reset forwards database offset, actor and context");
}
