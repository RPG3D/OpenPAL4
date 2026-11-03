#include "pal4/base_object_hand_effect.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using Event = std::pair<char, std::uint32_t>;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IBaseObjectNamedWeaponRuntime {
    std::vector<Event> events;
    bool needs_update{};
    bool available{true};
    bool has_hook{true};
    std::uint32_t expected_offset{0x520u};

    bool EffectNeedsUpdate(pal4::BaseObjectState&,
                           std::uint32_t offset) override {
        Check(offset == expected_offset || offset == 0x528u,
              "effect update checks a hand-effect handle");
        events.emplace_back('?', offset);
        return needs_update;
    }
    void UpdateEffect(pal4::BaseObjectState&,
                      std::uint32_t offset) override {
        events.emplace_back('U', offset);
    }
    std::uint32_t GetUserData(std::uint32_t clump,
                              std::string_view name) override {
        Check(clump == 0x1000u &&
                  name == (expected_offset == 0x520u ?
                               "hook_ef_rhand" : "hook_ef_lhand"),
              "setup queries the exact hand-specific hook");
        events.emplace_back('H', clump);
        return has_hook ? 0x2000u : 0u;
    }
    bool EffectAvailable(std::uint32_t name) override {
        Check(name == 0x55u, "effect predicate receives the name pointer");
        events.emplace_back('A', name);
        return available;
    }
    std::uint32_t EffectSingleton() override {
        events.emplace_back('S', 0u);
        return 0x3000u;
    }
    pal4::BaseObjectEffectCreation CreateEffect(
        std::uint32_t singleton, std::uint32_t name,
        std::uint32_t vector, std::uint32_t one,
        std::uint32_t zero) override {
        Check(singleton == 0x3000u && name == 0x55u &&
                  vector == 0x84b1dcu && one == 1u && zero == 0u,
              "effect creator receives the original arguments");
        events.emplace_back('C', name);
        return {{0x111u, 0x222u}, {0x333u, 0x444u}};
    }
    void ResetTemporary(pal4::BaseObjectEffectHandle& handle) override {
        Check(handle.first == 0x333u && handle.second == 0x444u,
              "temporary effect handle is released after transfer");
        events.emplace_back('R', 0u);
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        Check(address == 0x2004u || address == 0x5004u ||
                  address == 0x6004u || address == 0x7004u ||
                  address == 0x8004u,
              "effect and weapon cleanup read their frame fields");
        events.emplace_back('D', address);
        return address == 0x2004u ? 0x4000u : address + 0x100u;
    }
    void SetEffectProperties(pal4::BaseObjectState&,
                             std::uint32_t offset,
                             std::uint32_t frame) override {
        Check(frame == 0x4000u, "effect uses the hook frame");
        events.emplace_back('P', offset);
    }
    void RefreshEffect(pal4::BaseObjectState&,
                       std::uint32_t offset) override {
        events.emplace_back('F', offset);
    }
    std::uint32_t ArenaManager() override {
        events.emplace_back('a', 0u);
        return 0x9000u;
    }
    void RemoveClump(std::uint32_t arena, std::uint32_t clump) override {
        Check(arena == 0x9000u, "cleanup uses the arena singleton");
        events.emplace_back('m', clump);
    }
    void RemoveFrameChild(std::uint32_t body, std::string_view hook,
                          std::uint32_t frame) override {
        Check(body == 0x1000u &&
                  ((hook == "hook_RHWeapon" &&
                    (frame == 0x5104u || frame == 0x7104u)) ||
                   (hook == "hook_LHWeapon" &&
                    (frame == 0x6104u || frame == 0x8104u))),
              "cleanup detaches the matching right or left frame");
        events.emplace_back('f', frame);
    }
    bool EffectReady(pal4::BaseObjectState&,
                     std::uint32_t offset) override {
        events.emplace_back('e', offset);
        return true;
    }
    void DisplayAndCleanupEffect(pal4::BaseObjectState&,
                                 std::uint32_t offset) override {
        events.emplace_back('d', offset);
    }
    std::uint32_t GameInstance() override {
        events.emplace_back('g', 0u);
        return 0xa000u;
    }
    void CleanupClump(pal4::BaseObjectState&,
                      std::uint32_t offset,
                      std::uint32_t game) override {
        Check(game == 0xa1acu,
              "named weapon cleanup receives game resource owner");
        events.emplace_back('k', offset);
    }
};

std::uint32_t Setup(pal4::BaseObjectState& state, Runtime& runtime,
                    std::uint32_t name) {
    return runtime.expected_offset == 0x520u ?
        pal4::BaseObjectHandEffect::SetupRight(state, name, runtime) :
        pal4::BaseObjectHandEffect::SetupLeft(state, name, runtime);
}

void CheckSetup(std::uint32_t offset) {
    pal4::BaseObjectState state{};
    Runtime runtime;
    runtime.expected_offset = offset;
    Check(Setup(state, runtime, 0x55u) == 0u && runtime.events.empty(),
          "missing body returns before effect update");

    state.Write(0x14u, 0x1000u);
    runtime.has_hook = false;
    Check(Setup(state, runtime, 0x55u) == 0u &&
              runtime.events == std::vector<Event>{{'?', offset},
                                                   {'H', 0x1000u}},
          "missing hook returns after first update check");

    runtime.events.clear();
    runtime.has_hook = true;
    runtime.needs_update = true;
    Check(Setup(state, runtime, 0u) == 1u &&
              runtime.events == std::vector<Event>{{'?', offset},
                                                   {'U', offset},
                                                   {'H', 0x1000u},
                                                   {'?', offset},
                                                   {'U', offset}},
          "null effect retains both update checks and returns success");

    runtime.events.clear();
    runtime.available = false;
    Check(Setup(state, runtime, 0x55u) == 1u &&
              runtime.events.back() == Event{'A', 0x55u} &&
              state.Read(offset) == 0u,
          "unavailable effect leaves the handle unchanged");

    runtime.events.clear();
    runtime.available = true;
    Check(Setup(state, runtime, 0x55u) == 1u &&
              state.Read(offset) == 0x111u &&
              state.Read(offset + 4u) == 0x222u,
          "created effect transfers both handle words");
    Check(runtime.events == std::vector<Event>{{'?', offset},
                                               {'U', offset},
                                               {'H', 0x1000u},
                                               {'?', offset},
                                               {'U', offset},
                                               {'A', 0x55u},
                                               {'S', 0u},
                                               {'C', 0x55u},
                                               {'R', 0u},
                                               {'D', 0x2004u},
                                               {'P', offset},
                                               {'F', offset}},
          "setup follows update, hook, creation and property order");
}

void CheckUpdateAndCleanup() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    runtime.needs_update = true;
    Check(pal4::BaseObjectHandEffect::UpdateRight(state, runtime) == 1u &&
              pal4::BaseObjectHandEffect::UpdateLeft(state, runtime) == 1u &&
              runtime.events == std::vector<Event>{{'?', 0x520u},
                                                   {'U', 0x520u},
                                                   {'?', 0x528u},
                                                   {'U', 0x528u}},
          "individual updates preserve hand offsets");
    runtime.events.clear();
    Check(pal4::BaseObjectHandEffect::CleanupWeapons(state, runtime) == 1u &&
              pal4::BaseObjectHandEffect::CleanupWeaponEffects(state, runtime) == 1u &&
              runtime.events.empty(),
          "cleanup wrappers return one when the owner is absent");
    Check(pal4::BaseObjectHandEffect::UpdateWeaponEffects(state, runtime) == 1u &&
              runtime.events == std::vector<Event>{{'?', 0x520u},
                                                   {'U', 0x520u},
                                                   {'?', 0x528u},
                                                   {'U', 0x528u}},
          "combined update invokes right before left");
    runtime.events.clear();
    Check(pal4::BaseObjectHandEffect::CleanupAll(state, runtime) == 1u &&
              runtime.events == std::vector<Event>{{'?', 0x520u},
                                                   {'U', 0x520u},
                                                   {'?', 0x528u},
                                                   {'U', 0x528u}},
          "overall cleanup invokes its three component stages");
}

void CheckCombinedCleanupOrder() {
    pal4::BaseObjectState state{};
    state.Write(0x14u, 0x1000u);
    state.Write(0x518u, 0x5000u);
    state.Write(0x51cu, 0x6000u);
    state.Write(0x22cu, 0x7000u);
    state.Write(0x39cu, 0x8000u);
    Runtime runtime;
    Check(pal4::BaseObjectHandEffect::CleanupAll(state, runtime) == 1u,
          "combined cleanup returns one after all stages");
    Check(state.Read(0x518u) == 0u && state.Read(0x51cu) == 0u,
          "simple weapon pointers clear during cleanup");
    Check(runtime.events == std::vector<Event>{
              {'?', 0x520u}, {'a', 0u}, {'m', 0x5000u},
              {'D', 0x5004u}, {'f', 0x5104u},
              {'?', 0x528u}, {'a', 0u}, {'m', 0x6000u},
              {'D', 0x6004u}, {'f', 0x6104u},
              {'a', 0u}, {'m', 0x7000u}, {'D', 0x7004u},
              {'f', 0x7104u}, {'e', 0x4f8u}, {'d', 0x4f8u},
              {'e', 0x500u}, {'d', 0x500u}, {'g', 0u},
              {'k', 0x218u},
              {'a', 0u}, {'m', 0x8000u}, {'D', 0x8004u},
              {'f', 0x8104u}, {'e', 0x508u}, {'d', 0x508u},
              {'e', 0x510u}, {'d', 0x510u}, {'g', 0u},
              {'k', 0x388u},
              {'?', 0x520u}, {'?', 0x528u}},
          "combined cleanup preserves simple, named and hand-effect order");
}

}  // namespace

int main() {
    CheckSetup(0x520u);
    CheckSetup(0x528u);
    CheckUpdateAndCleanup();
    CheckCombinedCleanupOrder();
}
