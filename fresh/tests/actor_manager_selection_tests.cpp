#include "pal4/actor_manager_selection.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;

struct Runtime final : pal4::IActorManagerSelectionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::array<float, 3> position_seen{};
    std::array<float, 3> scale_seen{};
    std::uint32_t dispatch_result = 0x55u;
    std::uint32_t skill_touches = 0;
    double multiplier = 4.0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        trace.push_back("write:" + std::to_string(address));
    }
    void AdjustScale(std::uint32_t target,
                     std::array<float, 3>& scale) override {
        Check(target == actor && scale ==
              (std::array<float, 3>{1.0f, 1.0f, 1.0f}),
              "scale helper receives identity scale");
        trace.push_back("scale");
        scale[0] = 2.5f;
    }
    void UpdateDialog(std::uint32_t dialog) override {
        Check(dialog == actor + 852u, "dialog field address");
        trace.push_back("dialog-update");
    }
    pal4::ActorManagerHandle BuildDialog(
        std::uint32_t target, std::string_view text,
        std::uint32_t context) override {
        Check(target == actor && text == "pick" &&
              context == 0x8fa274u, "pick text and virtual context");
        trace.push_back("dialog-build");
        return {11u, 12u};
    }
    void ResetTemporaryHandle() override {
        trace.push_back("temp-reset");
    }
    void UpdateEffectProperties(
        std::uint32_t dialog, const std::array<float, 3>& scale,
        std::uint8_t flag) override {
        Check(dialog == actor + 852u && flag == 0u,
              "dialog effect properties target and flag");
        scale_seen = scale;
        trace.push_back("properties");
    }
    std::array<float, 3> ActorPosition(std::uint32_t target) override {
        Check(target == actor, "actor position target");
        trace.push_back("position");
        return {1.0f, 2.0f, 3.0f};
    }
    std::array<float, 8> ActorBoundingBox(
        std::uint32_t target) override {
        Check(target == actor, "actor bounding box target");
        trace.push_back("box");
        return {0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 5.0f};
    }
    void HandleEffect(std::uint32_t handle, std::uint32_t target,
                      bool force) override {
        Check(handle == actor + 860u && target == actor && !force,
              "select effect handle target and force");
        trace.push_back("effect-clear");
    }
    std::uint32_t CombatCamera() override {
        trace.push_back("camera");
        return 0xb000u;
    }
    pal4::ActorManagerHandle CreateEffect(
        std::uint32_t camera, std::string_view name,
        const std::array<float, 3>& position,
        std::uint32_t first_flag, std::uint32_t second_flag) override {
        Check(camera == 0xb000u && name == "OM03" &&
              first_flag == 1u && second_flag == 0u,
              "effect camera, name and flags");
        position_seen = position;
        trace.push_back("effect-create");
        return {21u, 22u};
    }
    std::uint32_t Dispatch(std::string_view event,
                           std::uint32_t target,
                           std::uint32_t mode) override {
        Check(target == actor && mode == 1u,
              "selection event actor and mode");
        trace.push_back(std::string(event));
        return dispatch_result;
    }
    double CriticalDamageMultiplier(std::uint32_t target) override {
        Check(target == actor, "facet points back to actor at -716");
        trace.push_back("multiplier");
        return multiplier;
    }
    void TouchSkillSystem() override { ++skill_touches; }
};

void SelectAndUnselect() {
    Runtime runtime;
    Check(pal4::ActorManagerSelection::SetSelected(
              actor, 0x101u, runtime) == 0x55u &&
          runtime.trace == std::vector<std::string>{
              "scale", "dialog-update", "dialog-build", "write:4948",
              "write:4952", "temp-reset", "properties", "box",
              "position", "effect-clear", "camera", "effect-create",
              "write:4956", "write:4960", "temp-reset", "ae_ui_select"} &&
          runtime.words[actor + 852u] == 11u &&
          runtime.words[actor + 856u] == 12u &&
          runtime.words[actor + 860u] == 21u &&
          runtime.words[actor + 864u] == 22u &&
          runtime.position_seen ==
              (std::array<float, 3>{1.0f, 27.0f, 3.0f}) &&
          runtime.scale_seen ==
              (std::array<float, 3>{2.5f, 1.0f, 1.0f}),
          "selection builds pick dialog and OM03 effect before dispatch");
    runtime.trace.clear();
    Check(pal4::ActorManagerSelection::SetSelected(
              actor, 0x100u, runtime) == 0x55u &&
          runtime.trace == std::vector<std::string>{
              "dialog-update", "effect-clear", "ae_ui_unselect"},
          "low byte zero chooses unselect without creating effects");
}

void RatioAndState() {
    Runtime runtime;
    Check(pal4::ActorManagerSelection::CriticalRatio(
              actor + 716u, runtime) == 16.0 &&
          runtime.trace == std::vector<std::string>{"multiplier"},
          "critical ratio forwards base actor and divides 64");
    runtime.multiplier = 0.0;
    Check(std::isinf(pal4::ActorManagerSelection::CriticalRatio(
              actor + 716u, runtime)),
          "zero multiplier retains floating-point infinity");
    const auto facet = actor + 716u;
    runtime.words[facet + 112u] = 1u;
    Check(pal4::ActorManagerSelection::IsStateOne(facet, runtime) &&
          runtime.skill_touches == 1u,
          "state one touches skill system and returns true");
    runtime.words[facet + 112u] = 7u;
    Check(!pal4::ActorManagerSelection::IsStateOne(facet, runtime) &&
          runtime.skill_touches == 2u,
          "state seven still touches skill system");
    runtime.words[facet + 112u] = 8u;
    Check(!pal4::ActorManagerSelection::IsStateOne(facet, runtime) &&
          runtime.skill_touches == 2u,
          "out-of-range state skips skill-system call");
    runtime.words[facet + 112u] = 0xffffffffu;
    Check(!pal4::ActorManagerSelection::IsStateOne(facet, runtime) &&
          runtime.skill_touches == 2u,
          "negative bit pattern uses unsigned switch bound");
}

}  // namespace

int main() {
    try {
        SelectAndUnselect();
        RatioAndState();
        std::cout << "actor_manager_selection_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
