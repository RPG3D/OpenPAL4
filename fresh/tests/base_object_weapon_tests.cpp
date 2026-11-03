#include "pal4/base_object_weapon.h"

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

struct Runtime final : pal4::IBaseObjectWeaponRuntime {
    std::vector<Event> events;
    pal4::BaseObjectHand hand{pal4::BaseObjectHand::Right};
    bool hand_hook{true};
    bool effect_hook{true};
    bool body_hook{true};
    bool effect_available{true};
    bool update_effect{true};

    void CleanupWeapon(pal4::BaseObjectState&,
                       pal4::BaseObjectHand selected) override {
        Check(selected == hand, "correct hand cleanup is selected");
        events.emplace_back('C', selected == pal4::BaseObjectHand::Right ? 0u : 1u);
    }
    std::uint32_t GetUserData(std::uint32_t clump,
                              std::string_view name) override {
        if (name == "hook_hand") {
            Check(clump == 0x2000u, "weapon hand hook uses weapon clump");
            events.emplace_back('H', clump);
            return hand_hook ? 0x3000u : 0u;
        }
        if (name == "hook_effect") {
            Check(clump == 0x2000u, "effect hook uses weapon clump");
            events.emplace_back('E', clump);
            return effect_hook ? 0x3100u : 0u;
        }
        Check(clump == 0x1000u &&
                  name == (hand == pal4::BaseObjectHand::Right ?
                               "hook_RHWeapon" : "hook_LHWeapon"),
              "body hook follows selected hand and base clump");
        events.emplace_back('B', clump);
        return body_hook ? 0x3200u : 0u;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back('r', address);
        if (address == 0x2004u) return 0x5000u;
        if (address == 0x3004u) return 0x6000u;
        if (address == 0x3104u) return 0x7000u;
        if (address == 0x3204u) return 0x8000u;
        Check(false, "only observed frame pointers are read");
        return 0u;
    }
    void InvertTransform(std::uint32_t destination,
                        std::uint32_t source) override {
        Check(destination == 0x5010u && source == 0x6010u,
              "weapon frame receives inverse hand-hook transform");
        events.emplace_back('T', destination);
    }
    void ScaleFrame(std::uint32_t frame, std::uint32_t scale,
                    std::uint32_t mode) override {
        Check(frame == 0x5000u && scale == 0x55u && mode == 2u,
              "frame scale forwards raw scale and mode two");
        events.emplace_back('S', frame);
    }
    void UpdateFrame(std::uint32_t frame) override {
        Check(frame == 0x5000u, "frame update uses weapon frame");
        events.emplace_back('U', frame);
    }
    void RotateMatrix(std::uint32_t matrix, std::uint32_t axis,
                      float degrees, std::uint32_t mode) override {
        Check(matrix == 0x5010u && mode == 2u &&
                  ((axis == 0x8433a0u && degrees == 90.0f) ||
                   (axis == 0x8433acu && degrees == 180.0f)),
              "rotation preserves axis, angle and mode");
        events.emplace_back('X', axis);
    }
    void ProcessUserData(std::uint32_t clump, std::string_view hook,
                         std::uint32_t frame) override {
        Check(clump == 0x1000u && frame == 0x5000u &&
                  hook == (hand == pal4::BaseObjectHand::Right ?
                               "hook_RHWeapon" : "hook_LHWeapon"),
              "weapon attaches to the selected body hook");
        events.emplace_back('P', clump);
    }
    bool EffectNeedsUpdate(pal4::BaseObjectState&,
                            std::uint32_t offset) override {
        Check(offset == (hand == pal4::BaseObjectHand::Right ?
                             0x520u : 0x528u),
              "effect handle offset follows selected hand");
        events.emplace_back('N', offset);
        return update_effect;
    }
    void UpdateEffect(pal4::BaseObjectState&,
                      std::uint32_t offset) override {
        events.emplace_back('D', offset);
    }
    std::uint32_t EffectSingleton() override {
        events.emplace_back('I', 0u);
        return 0x9000u;
    }
    pal4::BaseObjectEffectCreation CreateEffect(
        std::uint32_t singleton, std::uint32_t name,
        std::uint32_t vector, std::uint32_t one,
        std::uint32_t zero) override {
        Check(singleton == 0x9000u && name == 0x99u &&
                  vector == 0x84b1dcu && one == 1u && zero == 0u,
              "effect creation forwards singleton, name and global vector");
        events.emplace_back('F', vector);
        return {{0xa1u, 0xa2u}, {0xb1u, 0xb2u}};
    }
    void ResetTemporary(pal4::BaseObjectEffectHandle& temporary) override {
        Check(temporary.first == 0xb1u && temporary.second == 0xb2u,
              "temporary effect handle is reset after persistent copy");
        events.emplace_back('Z', 0u);
    }
    void SetEffectProperties(pal4::BaseObjectState& state,
                             std::uint32_t offset,
                             std::uint32_t frame) override {
        Check(state.Read(offset) == 0xa1u &&
                  state.Read(offset + 4u) == 0xa2u,
              "persistent effect handle is stored before property update");
        events.emplace_back('Q', frame);
    }
    void SetEffectVisibility(pal4::BaseObjectState&,
                             std::uint32_t, std::uint32_t frame) override {
        events.emplace_back('v', frame);
    }
    void RefreshEffect(pal4::BaseObjectState&,
                       std::uint32_t offset) override {
        events.emplace_back('R', offset);
    }
    std::uint32_t ArenaManager() override {
        events.emplace_back('A', 0u);
        return 0xa000u;
    }
    void AddClump(std::uint32_t arena,
                  std::uint32_t weapon) override {
        Check(arena == 0xa000u && weapon == 0x2000u,
              "arena receives the weapon clump");
        events.emplace_back('a', weapon);
    }
    void RemoveClump(std::uint32_t arena,
                     std::uint32_t weapon) override {
        Check(arena == 0xa000u && weapon == 0x2000u,
              "cleanup removes selected weapon from arena");
        events.emplace_back('m', weapon);
    }
    void RemoveFrameChild(std::uint32_t clump,
                          std::string_view hook,
                          std::uint32_t frame) override {
        Check(clump == 0x1000u && frame == 0x5000u &&
                  hook == (hand == pal4::BaseObjectHand::Right ?
                               "hook_RHWeapon" : "hook_LHWeapon"),
              "cleanup detaches weapon frame from selected body hook");
        events.emplace_back('f', frame);
    }
    bool EffectAvailable(std::uint32_t name) override {
        Check(name == 0x99u,
              "no-weapon effect predicate receives original name");
        events.emplace_back('V', name);
        return effect_available;
    }
};

std::uint32_t Setup(pal4::BaseObjectState& state,
                    Runtime& runtime, std::uint32_t weapon) {
    return runtime.hand == pal4::BaseObjectHand::Right ?
        pal4::BaseObjectWeapon::SetupRight(state, weapon, 0x55u, 0x99u,
                                            runtime) :
        pal4::BaseObjectWeapon::SetupLeft(state, weapon, 0x55u, 0x99u,
                                           runtime);
}

std::vector<Event> ExpectedWeapon(pal4::BaseObjectHand hand,
                                   bool effect_hook, bool update_effect) {
    const bool right = hand == pal4::BaseObjectHand::Right;
    const auto effect_offset = right ? 0x520u : 0x528u;
    std::vector<Event> expected{
        {'C', right ? 0u : 1u}, {'H', 0x2000u},
        {'r', 0x2004u}, {'r', 0x3004u}, {'T', 0x5010u},
        {'S', 0x5000u}, {'U', 0x5000u}, {'X', 0x8433a0u}};
    if (right) expected.emplace_back('X', 0x8433acu);
    expected.insert(expected.end(), {
        {'U', 0x5000u}, {'r', 0x2004u}, {'P', 0x1000u},
        {'E', 0x2000u}, {'N', effect_offset}});
    if (update_effect) expected.emplace_back('D', effect_offset);
    expected.insert(expected.end(), {
        {'I', 0u}, {'F', 0x84b1dcu}, {'Z', 0u},
        {'r', effect_hook ? 0x3104u : 0x2004u},
        {'Q', effect_hook ? 0x7000u : 0x5000u},
        {'R', effect_offset}, {'A', 0u}, {'a', 0x2000u}});
    return expected;
}

void CheckWeaponPath(pal4::BaseObjectHand hand,
                     bool effect_hook, bool update_effect) {
    pal4::BaseObjectState state{};
    state.Write(0x14u, 0x1000u);
    Runtime runtime;
    runtime.hand = hand;
    runtime.effect_hook = effect_hook;
    runtime.update_effect = update_effect;
    Check(Setup(state, runtime, 0x2000u) == 1u &&
              runtime.events == ExpectedWeapon(hand, effect_hook, update_effect),
          "weapon path preserves hand-specific rotation and all effect calls");
    const auto weapon_offset = hand == pal4::BaseObjectHand::Right ?
        0x518u : 0x51cu;
    const auto other_offset = hand == pal4::BaseObjectHand::Right ?
        0x51cu : 0x518u;
    Check(state.Read(weapon_offset) == 0x2000u &&
              state.Read(other_offset) == 0u,
          "only the selected hand stores the new weapon clump");
}

void CheckEarlyPaths(pal4::BaseObjectHand hand) {
    pal4::BaseObjectState state{};
    Runtime runtime;
    runtime.hand = hand;
    Check(Setup(state, runtime, 0x2000u) == 0u &&
              runtime.events.empty(),
          "missing base clump stops before cleanup and field write");
    state.Write(0x14u, 0x1000u);
    runtime.hand_hook = false;
    Check(Setup(state, runtime, 0x2000u) == 0u &&
              runtime.events == std::vector<Event>{
                  {'C', hand == pal4::BaseObjectHand::Right ? 0u : 1u},
                  {'H', 0x2000u}},
          "missing hand hook stops after cleanup and weapon assignment");
    Check(state.Read(hand == pal4::BaseObjectHand::Right ?
                         0x518u : 0x51cu) == 0x2000u,
          "failed hook lookup retains the assigned weapon pointer");
}

void CheckEffectOnly(pal4::BaseObjectHand hand,
                     bool available, bool hook_present) {
    pal4::BaseObjectState state{};
    state.Write(0x14u, 0x1000u);
    Runtime runtime;
    runtime.hand = hand;
    runtime.effect_available = available;
    runtime.body_hook = hook_present;
    const auto result = Setup(state, runtime, 0u);
    Check(result == (available && !hook_present ? 0u : 1u),
          "effect-only return follows predicate and body-hook lookup");
    const auto effect_offset = hand == pal4::BaseObjectHand::Right ?
        0x520u : 0x528u;
    if (available && hook_present) {
        Check(runtime.events == std::vector<Event>{
                  {'C', hand == pal4::BaseObjectHand::Right ? 0u : 1u},
                  {'V', 0x99u}, {'B', 0x1000u}, {'N', effect_offset},
                  {'D', effect_offset}, {'I', 0u}, {'F', 0x84b1dcu},
                  {'Z', 0u}, {'r', 0x3204u}, {'v', 0x8000u},
                  {'R', effect_offset}},
              "effect-only path attaches visibility to selected body hook");
    } else {
        Check(runtime.events.size() == (available ? 3u : 2u),
              "effect-only early exits skip effect allocation");
    }
}

std::uint32_t Cleanup(pal4::BaseObjectState& state, Runtime& runtime) {
    return runtime.hand == pal4::BaseObjectHand::Right ?
        pal4::BaseObjectWeapon::CleanupRight(state, runtime) :
        pal4::BaseObjectWeapon::CleanupLeft(state, runtime);
}

void CheckCleanup(pal4::BaseObjectHand hand, bool update_effect) {
    pal4::BaseObjectState state{};
    Runtime runtime;
    runtime.hand = hand;
    runtime.update_effect = update_effect;
    const auto weapon_offset = hand == pal4::BaseObjectHand::Right ?
        0x518u : 0x51cu;
    const auto effect_offset = hand == pal4::BaseObjectHand::Right ?
        0x520u : 0x528u;
    state.Write(weapon_offset, 0x2000u);
    Check(Cleanup(state, runtime) == 0u &&
              state.Read(weapon_offset) == 0x2000u &&
              runtime.events.empty(),
          "cleanup without base clump leaves weapon untouched");

    state.Write(0x14u, 0x1000u);
    Check(Cleanup(state, runtime) == 1u &&
              state.Read(weapon_offset) == 0u,
          "cleanup clears selected weapon after detach");
    std::vector<Event> expected{{'N', effect_offset}};
    if (update_effect) expected.emplace_back('D', effect_offset);
    expected.insert(expected.end(), {{'A', 0u}, {'m', 0x2000u},
                                     {'r', 0x2004u}, {'f', 0x5000u}});
    Check(runtime.events == expected,
          "cleanup updates effect then removes clump and frame in IDA order");

    runtime.events.clear();
    Check(Cleanup(state, runtime) == 1u,
          "cleanup with no weapon still returns success");
    expected = {{'N', effect_offset}};
    if (update_effect) expected.emplace_back('D', effect_offset);
    Check(runtime.events == expected,
          "cleanup without weapon checks effect but skips arena calls");
}

void CheckDefaultCleanupIntegration() {
    struct DefaultRuntime final : pal4::IBaseObjectWeaponRuntime {
        unsigned effect_checks{};
        bool EffectNeedsUpdate(pal4::BaseObjectState&,
                               std::uint32_t offset) override {
            Check(offset == 0x520u,
                  "default setup cleanup selects right-hand effect");
            ++effect_checks;
            return false;
        }
    } runtime;
    pal4::BaseObjectState state{};
    state.Write(0x14u, 0x1000u);
    Check(pal4::BaseObjectWeapon::SetupRight(
              state, 0u, 0u, 0u, runtime) == 1u &&
              runtime.effect_checks == 1u,
          "default setup runtime delegates to reconstructed cleanup");
}

}  // namespace

int main() {
    for (auto hand : {pal4::BaseObjectHand::Right,
                      pal4::BaseObjectHand::Left}) {
        for (bool effect_hook : {false, true}) {
            for (bool update_effect : {false, true}) {
                CheckWeaponPath(hand, effect_hook, update_effect);
            }
        }
        CheckEarlyPaths(hand);
        CheckEffectOnly(hand, false, false);
        CheckEffectOnly(hand, true, false);
        CheckEffectOnly(hand, true, true);
        CheckCleanup(hand, false);
        CheckCleanup(hand, true);
    }
    CheckDefaultCleanupIntegration();
}
