#include "pal4/pal_actor_control_actions.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct MotionRuntime : pal4::ActorControlMotionRuntimeStub {
    std::vector<std::string> events;
    bool throw_on_monster{};
    void ProcessUiEvents(std::uint32_t manager,
                         std::uint32_t mode) override {
        Check(manager == 0u && mode == 0u, "begin motion UI event");
        events.emplace_back("ui");
    }
    std::uint32_t SetMonsterFlags(std::uint32_t manager) override {
        Check(manager == 0u, "begin motion monster manager");
        events.emplace_back("monster");
        if (throw_on_monster) throw std::runtime_error("monster callback");
        return 0x55u;
    }
};

struct ActionRuntime : pal4::IActorControlActionRuntime {
    std::uint32_t* component{};
    std::uint32_t actor{};
    std::uint32_t position{};
    unsigned calls{};
    std::uint32_t movement_state{};
    unsigned movement_calls{};

    std::uint32_t UpdateCameraAndWorldView(
        std::uint32_t* value, std::uint32_t current_actor,
        std::uint32_t position_address) override {
        component = value;
        actor = current_actor;
        position = position_address;
        ++calls;
        return 0x789u;
    }
    std::uint32_t MovementAdapterState() override {
        ++movement_calls;
        return movement_state;
    }
};

}  // namespace

int main() {
    pal4::PalActorControlState state{};
    MotionRuntime motion;
    const std::array<std::uint32_t, 3> start{
        std::bit_cast<std::uint32_t>(1.0f),
        std::bit_cast<std::uint32_t>(2.0f),
        std::bit_cast<std::uint32_t>(3.0f)};
    const std::array<std::uint32_t, 3> finish{
        std::bit_cast<std::uint32_t>(4.0f),
        std::bit_cast<std::uint32_t>(2.0f),
        std::bit_cast<std::uint32_t>(3.0f)};
    Check(pal4::PalActorControlActions::BeginMotion(
              state, 0x100000u, start, finish, 0x40400000u, 7u,
              motion) == 0x55u &&
              state.words[183u] == 0x100294u &&
              state.words[165u + 1u] == 1u &&
              state.words[165u + 3u] == start[0] &&
              state.words[165u + 6u] == finish[0] &&
              state.words[165u + 12u] == 1u &&
              state.words[165u + 13u] == 1u &&
              state.words[165u + 14u] == 7u &&
              state.words[165u + 17u] == 0x40400000u &&
              motion.events == std::vector<std::string>{"ui", "monster"},
          "motion wrapper composes Begin and selects subobject address");

    state.words[183u] = 0x2222u;
    motion.throw_on_monster = true;
    state.words[165u + 1u] = 0u;
    try {
        static_cast<void>(pal4::PalActorControlActions::BeginMotion(
            state, 0x100000u, start, finish, 0x40400000u, 7u,
            motion));
        Check(false, "motion callback failure must propagate");
    } catch (const std::runtime_error&) {
        Check(state.words[165u + 1u] == 1u &&
                  state.words[183u] == 0x2222u,
              "partial motion writes survive exception without selecting it");
    }

    state.words[20u] = 1u;
    state.words[21u] = 2u;
    state.words[22u] = 3u;
    state.words[184u] = 9u;
    Check(pal4::PalActorControlActions::ClearTargets(state) == 0u &&
              state.words[183u] == 0u && state.words[20u] == 0u &&
              state.words[21u] == 0u && state.words[22u] == 0u &&
              state.words[184u] == 9u,
          "target reset changes only four observed words");

    ActionRuntime action;
    Check(pal4::PalActorControlActions::UpdateWorldView(
              state, 0x2000u, action) == 0u && action.calls == 0u,
          "world-view wrapper exits with null actor");
    state.words[1u] = 0x1234u;
    Check(pal4::PalActorControlActions::UpdateWorldView(
              state, 0x2000u, action) == 0x789u &&
              action.calls == 1u &&
              action.component == state.words.data() + 23u &&
              action.actor == 0x1234u && action.position == 0x2000u,
          "world-view wrapper forwards exact subobject and arguments");

    Check(pal4::PalActorControlActions::SetWord28(
              state, 0xfedcba98u) == 0xfedcba98u &&
              state.words[28u] == 0xfedcba98u &&
              state.words[27u] == 0u && state.words[29u] == 0u,
          "word-28 setter returns value and preserves neighbors");

    state.words[1u] = 0u;
    Check(!pal4::PalActorControlActions::CanStartAction(state, action) &&
              action.movement_calls == 0u,
          "availability gate skips singleton without actor");
    state.words[1u] = 0x1234u;
    state.words[183u] = 1u;
    Check(!pal4::PalActorControlActions::CanStartAction(state, action) &&
              action.movement_calls == 0u,
          "availability gate skips singleton with selected subobject");
    state.words[183u] = 0u;
    action.movement_state = 1u;
    Check(!pal4::PalActorControlActions::CanStartAction(state, action) &&
              action.movement_calls == 1u,
          "nonzero adapter state rejects action");
    action.movement_state = 0u;
    Check(pal4::PalActorControlActions::CanStartAction(state, action) &&
              action.movement_calls == 2u,
          "zero adapter state permits action");
}
