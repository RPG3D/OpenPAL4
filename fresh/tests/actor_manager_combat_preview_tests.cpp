#include "pal4/actor_manager_combat_preview.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kManager = 0x1000u;
constexpr std::uint32_t kEvent = 0x2000u;
constexpr std::uint32_t kActor = 0x3000u;

struct Runtime final : pal4::IActorManagerCombatPreviewRuntime {
    std::vector<std::string> trace;
    std::vector<std::uint32_t> actors{kActor, kActor, kActor};
    std::size_t actor_reads = 0;
    bool inactive = false;
    bool needs_update = true;
    bool throw_on_properties = false;
    bool throw_on_finalize = false;
    bool eff4 = false;
    bool eff3 = false;
    bool eff7 = false;
    bool eff6 = false;
    bool combat_active = true;
    std::uint32_t dialog_actor = 0x3123u;
    std::uint32_t actor_state = 0u;
    std::uint32_t text = 0x5000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if ((eff4 || eff7 || eff6) &&
            address == (eff4 ? kActor : dialog_actor) + 804u) {
            trace.push_back("actor-state");
            return actor_state;
        }
        Check(address == kEvent + (eff4 || eff3 ? 4u : 8u) &&
                  actor_reads < actors.size(),
              "event actor address and read count");
        trace.push_back("actor-read");
        return actors[actor_reads++];
    }
    std::uint32_t CombatSystem() override {
        trace.push_back("combat-system");
        return 0x6000u;
    }
    bool CombatInactive(std::uint32_t combat) override {
        Check(combat == 0x6000u, "combat system pointer");
        trace.push_back("inactive");
        return inactive;
    }
    bool CombatStateActive(std::uint32_t combat) override {
        Check(combat == 0x6000u, "active-state combat pointer");
        trace.push_back("combat-active");
        return combat_active;
    }
    std::uint32_t DialogData(std::uint32_t parameter) override {
        Check(parameter == kEvent, "dialog parameter");
        trace.push_back("dialog");
        return dialog_actor;
    }
    pal4::ActorManagerEffectHandle CreateEffect(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t position, std::uint32_t mode,
        std::uint32_t flags) override {
        Check(manager == kManager &&
                  name == (eff4 ? pal4::ActorManagerCombatPreview::kEFF4
                               : eff3 ? pal4::ActorManagerCombatPreview::kEFF3
                               : eff7 ? pal4::ActorManagerCombatPreview::kEFF7
                               : eff6 ? pal4::ActorManagerCombatPreview::kEFF6
                                      : pal4::ActorManagerCombatPreview::kCA46) &&
                  position == pal4::ActorManagerCombatPreview::kPosition &&
                  mode == 1u && flags == 0u,
              "CA46 creation constants");
        trace.push_back("create");
        return {0x7000u, 0x8000u};
    }
    bool NeedsUpdate(const pal4::ActorManagerEffectHandle& effect) override {
        Check(effect.id == 0x7000u, "update decision handle");
        trace.push_back("needs-update");
        return needs_update;
    }
    std::uint32_t CreateText(std::uint32_t literal) override {
        Check(literal == pal4::ActorManagerCombatPreview::kT1Text,
              "T1 literal address");
        trace.push_back("text-create");
        return text;
    }
    void ReleaseText(std::uint32_t value) override {
        Check(value == text && value != 0u, "released text handle");
        trace.push_back("text-release");
    }
    void SetProperties(const pal4::ActorManagerEffectHandle& effect,
                       std::uint32_t value, std::uint32_t source) override {
        Check(effect.id == 0x7000u && value == text &&
                  source == (actors[0] == 0u ? 0u : actors[0] + 724u),
              "preview properties arguments");
        trace.push_back("properties");
        if (throw_on_properties) throw std::runtime_error("properties");
    }
    std::uint32_t ChildPosition(std::uint32_t actor) override {
        Check(actor == (eff7 || eff6 ? dialog_actor : actors[1]),
              "position actor");
        trace.push_back("child-position");
        return 0x9000u;
    }
    std::uint32_t ChildMatrix(std::uint32_t actor) override {
        Check(actor == actors[2], "matrix actor");
        trace.push_back("child-matrix");
        return 0xa000u;
    }
    void UpdatePosition(const pal4::ActorManagerEffectHandle& effect,
                        std::uint32_t position,
                        std::uint8_t mode) override {
        Check(effect.id == 0x7000u && position == 0x9000u && mode == 0u,
              "position arguments");
        trace.push_back("position");
    }
    void UpdateMatrix(const pal4::ActorManagerEffectHandle& effect,
                      std::uint32_t matrix,
                      std::uint8_t mode) override {
        Check(effect.id == 0x7000u && matrix == 0xa000u && mode == 0u,
              "matrix arguments");
        trace.push_back("matrix");
    }
    void UpdateOrientation(
        const pal4::ActorManagerEffectHandle& effect,
        const std::array<float, 3>& angles,
        std::uint8_t mode) override {
        Check(effect.id == 0x7000u &&
                  angles == std::array<float, 3>{0.0f, 180.0f, 0.0f} &&
                  mode == 1u,
              "orientation arguments");
        trace.push_back("orientation");
    }
    void FinalizeEffect(const pal4::ActorManagerEffectHandle& effect) override {
        Check(effect.id == 0x7000u, "finalize handle");
        trace.push_back("finalize");
        if (throw_on_finalize) throw std::runtime_error("finalize");
    }
    void ResetTemporaryEffect(pal4::ActorManagerEffectHandle& effect) override {
        Check(effect.id == 0x7000u && effect.owner == 0x8000u,
              "temporary effect before reset");
        trace.push_back("reset");
        effect = {0xffffffffu, 0u};
    }
};

void InactiveAndSkippedUpdate() {
    Runtime inactive;
    inactive.inactive = true;
    Check(pal4::ActorManagerCombatPreview::Handle(
              kManager, kEvent, inactive) == 1u &&
              inactive.trace == std::vector<std::string>{
                  "combat-system", "inactive"},
          "inactive combat exits before CA46 creation");

    Runtime skipped;
    skipped.needs_update = false;
    Check(pal4::ActorManagerCombatPreview::Handle(
              kManager, kEvent, skipped) == 0u &&
              skipped.trace == std::vector<std::string>{
                  "combat-system", "inactive", "create", "needs-update",
                  "reset"},
          "unneeded preview skips properties but resets temporary handle");
}

void FullUpdateAndNullSource() {
    const std::vector<std::string> expected{
        "combat-system", "inactive", "create", "needs-update",
        "actor-read", "text-create", "properties", "text-release",
        "actor-read", "child-position", "position", "actor-read",
        "child-matrix", "matrix", "orientation", "finalize", "reset"};
    Runtime full;
    Check(pal4::ActorManagerCombatPreview::Handle(
              kManager, kEvent, full) == 0u &&
              full.trace == expected && full.actor_reads == 3u,
          "CA46 preview update order and three actor reads");

    Runtime null_source;
    null_source.actors[0] = 0u;
    null_source.text = 0u;
    Check(pal4::ActorManagerCombatPreview::Handle(
              kManager, kEvent, null_source) == 0u &&
              null_source.actor_reads == 3u &&
              std::find(null_source.trace.begin(), null_source.trace.end(),
                        "text-release") == null_source.trace.end(),
          "null actor source and empty temporary text");
}

void ExceptionCleanup() {
    for (bool property_failure : {true, false}) {
        Runtime runtime;
        runtime.throw_on_properties = property_failure;
        runtime.throw_on_finalize = !property_failure;
        bool caught = false;
        try {
            static_cast<void>(pal4::ActorManagerCombatPreview::Handle(
                kManager, kEvent, runtime));
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.trace.back() == "reset" &&
                  std::count(runtime.trace.begin(), runtime.trace.end(),
                             "text-release") == 1,
              "temporary text and effect are cleaned on failure");
    }
}

void Eff4StateAndTransforms() {
    Runtime inactive;
    inactive.eff4 = true;
    inactive.inactive = true;
    Check(pal4::ActorManagerCombatPreview::HandleEff4(
              kManager, kEvent, inactive) == 1u &&
              inactive.trace == std::vector<std::string>{
                  "combat-system", "inactive"},
          "inactive combat skips EFF4 state lookup");

    Runtime state_one;
    state_one.eff4 = true;
    state_one.actor_state = 1u;
    Check(pal4::ActorManagerCombatPreview::HandleEff4(
              kManager, kEvent, state_one) == 1u &&
              state_one.trace == std::vector<std::string>{
                  "combat-system", "inactive", "actor-read", "actor-state"},
          "state one returns before EFF4 creation");

    for (std::uint32_t state : {0u, 2u, 0xffffffffu}) {
        Runtime active;
        active.eff4 = true;
        active.actor_state = state;
        Check(pal4::ActorManagerCombatPreview::HandleEff4(
                  kManager, kEvent, active) == 0u &&
                  active.actor_reads == 3u &&
                  active.trace == std::vector<std::string>{
                      "combat-system", "inactive", "actor-read",
                      "actor-state", "create", "actor-read",
                      "child-position", "position", "actor-read",
                      "child-matrix", "matrix", "finalize", "reset"},
              "EFF4 state gate and transform order");
    }

    Runtime throwing;
    throwing.eff4 = true;
    throwing.throw_on_finalize = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::ActorManagerCombatPreview::HandleEff4(
            kManager, kEvent, throwing));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.trace.back() == "reset",
          "EFF4 temporary effect resets after finalization failure");
}

void Eff3PositionOnly() {
    Runtime inactive;
    inactive.eff3 = true;
    inactive.inactive = true;
    Check(pal4::ActorManagerCombatPreview::HandleEff3(
              kManager, kEvent, inactive) == 1u &&
              inactive.trace == std::vector<std::string>{
                  "combat-system", "inactive"},
          "inactive combat skips EFF3 creation");

    Runtime active;
    active.eff3 = true;
    Check(pal4::ActorManagerCombatPreview::HandleEff3(
              kManager, kEvent, active) == 0u &&
              active.actor_reads == 1u &&
              active.trace == std::vector<std::string>{
                  "combat-system", "inactive", "create", "actor-read",
                  "child-position", "position", "finalize", "reset"},
          "EFF3 updates position only and resets its handle");

    Runtime throwing;
    throwing.eff3 = true;
    throwing.throw_on_finalize = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::ActorManagerCombatPreview::HandleEff3(
            kManager, kEvent, throwing));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.trace.back() == "reset",
          "EFF3 temporary handle resets after finalization failure");
}

void DialogPositionEffects() {
    Runtime inactive;
    inactive.eff7 = true;
    inactive.inactive = true;
    Check(pal4::ActorManagerCombatPreview::HandleEff7(
              kManager, kEvent, inactive) == 1u &&
              inactive.trace == std::vector<std::string>{
                  "combat-system", "inactive"},
          "EFF7 inactive combat exits before second combat lookup");

    Runtime inactive_state;
    inactive_state.eff7 = true;
    inactive_state.combat_active = false;
    Check(pal4::ActorManagerCombatPreview::HandleEff7(
              kManager, kEvent, inactive_state) == 0u &&
              inactive_state.trace == std::vector<std::string>{
                  "combat-system", "inactive", "combat-system",
                  "combat-active"},
          "EFF7 requires active combat state");

    for (bool eff7 : {true, false}) {
        Runtime skipped;
        skipped.eff7 = eff7;
        skipped.eff6 = !eff7;
        skipped.actor_state = 1u;
        const auto skipped_result = eff7
            ? pal4::ActorManagerCombatPreview::HandleEff7(
                  kManager, kEvent, skipped)
            : pal4::ActorManagerCombatPreview::HandleEff6(
                  kManager, kEvent, skipped);
        Check(skipped_result == (eff7 ? 0x23u : 0x3123u) &&
                  skipped.trace.back() == "actor-state" &&
                  std::find(skipped.trace.begin(), skipped.trace.end(),
                            "create") == skipped.trace.end(),
              "non-two actor state returns dialog pointer width without effect");

        Runtime active;
        active.eff7 = eff7;
        active.eff6 = !eff7;
        active.actor_state = 2u;
        const auto active_result = eff7
            ? pal4::ActorManagerCombatPreview::HandleEff7(
                  kManager, kEvent, active)
            : pal4::ActorManagerCombatPreview::HandleEff6(
                  kManager, kEvent, active);
        Check(active_result == 0u &&
                  active.trace == (eff7
                      ? std::vector<std::string>{
                            "combat-system", "inactive", "combat-system",
                            "combat-active", "dialog", "actor-state", "create",
                            "dialog", "child-position", "position", "finalize",
                            "reset"}
                      : std::vector<std::string>{
                            "combat-system", "inactive", "dialog",
                            "actor-state", "create", "dialog",
                            "child-position", "position", "finalize", "reset"}),
              "EFF7/EFF6 state-two position update and reset order");

        Runtime throwing;
        throwing.eff7 = eff7;
        throwing.eff6 = !eff7;
        throwing.actor_state = 2u;
        throwing.throw_on_finalize = true;
        bool caught = false;
        try {
            if (eff7)
                static_cast<void>(pal4::ActorManagerCombatPreview::HandleEff7(
                    kManager, kEvent, throwing));
            else
                static_cast<void>(pal4::ActorManagerCombatPreview::HandleEff6(
                    kManager, kEvent, throwing));
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && throwing.trace.back() == "reset",
              "dialog position effect resets on finalization failure");
    }
}

}  // namespace

int main() {
    try {
        InactiveAndSkippedUpdate();
        FullUpdateAndNullSource();
        ExceptionCleanup();
        Eff4StateAndTransforms();
        Eff3PositionOnly();
        DialogPositionEffects();
        std::cout << "actor_manager_combat_preview_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
