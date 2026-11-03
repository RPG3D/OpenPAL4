#include "pal4/gob_obstacle.h"
#include "pal4/gob_obstacle_physics.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Near(float left, float right) {
    return std::fabs(left - right) < 0.0001f;
}

struct Runtime final : pal4::IGobObstaclePhysicsRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t control = 0x3000u;
    static constexpr std::uint32_t actor = 0x4000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::vector<pal4::GobObstacleVector> forces;
    pal4::GobObstacleVector forward{1.0f, 0.0f, 0.0f};
    float distance = 1.0f;
    float random_delay = 2.5f;
    double height = 10.0;
    std::uint32_t control_address = control;
    std::uint32_t found_object = 0x6000u;
    bool dialog_active = false;
    bool dialog_active_after_delay = false;
    unsigned dialog_checks = 0;

    Runtime() {
        words[control] = 1u;
        words[control + 4u] = actor;
        floats[object + 0xacu] = 10.0f;
        floats[object + 0xb0u] = 0.0f;
        floats[object + 0xb4u] = 2.0f;
        floats[object + 0x19cu] = 10.0f;
        floats[actor + 0xacu] = 0.0f;
        floats[actor + 0xb0u] = 0.0f;
        floats[actor + 0xb4u] = 1.0f;
        floats[actor + 0xecu] = 1.0f;
        floats[object + 0xecu] = 2.0f;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word " + std::to_string(address));
    }
    float ReadFloat(std::uint32_t address) override { return floats[address]; }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        calls.emplace_back("write float " + std::to_string(address));
    }
    std::string_view ReadCString(std::uint32_t address) override {
        if (address == 0x7000u) return "LaunchObject";
        if (address == 0x7100u) return "LaunchEffect";
        throw std::runtime_error("unknown string address");
    }
    void UpdateMachine(std::uint32_t address, float elapsed) override {
        Check(address == object && Near(elapsed, 0.5f), "machine update args");
        calls.emplace_back("update machine");
    }
    std::uint32_t ActorControl() override {
        calls.emplace_back("actor control");
        return control_address;
    }
    double ActorHeight(std::uint32_t address) override {
        Check(address == actor, "actor height address");
        calls.emplace_back("actor height");
        return height;
    }
    float HorizontalMagnitude(const pal4::GobObstacleVector& vector) override {
        Check(Near(vector.x, 10.0f) && Near(vector.y, 0.0f) &&
              Near(vector.z, 1.0f), "horizontal displacement");
        calls.emplace_back("magnitude");
        return distance;
    }
    void ClampVectorToRange(float range,
                            pal4::GobObstacleVector& vector) override {
        Check(Near(range, 2.4f), "clamp range");
        calls.emplace_back("clamp");
        vector = {1.0f, 0.0f, 0.0f};
    }
    void ApplyActorWorldForce(
        const pal4::GobObstacleVector& force) override {
        calls.emplace_back("apply force");
        forces.push_back(force);
    }
    void TickFloorSound(float elapsed) override {
        Check(Near(elapsed, 0.5f), "floor sound elapsed");
        calls.emplace_back("floor sound");
    }
    pal4::GobObstacleVector ObjectForward(
        std::uint32_t address) override {
        Check(address == object, "object forward address");
        calls.emplace_back("object forward");
        return forward;
    }
    void ApplyPlayerStatus(float duration) override {
        Check(Near(duration, 1.5f), "player status duration");
        calls.emplace_back("apply status");
    }
    void StartPlayerStatus(std::uint32_t first, std::uint32_t second,
                            float duration) override {
        Check(first == 11u && second == 12u && Near(duration, 1.5f),
              "start status args");
        calls.emplace_back("start status");
    }
    void BeginMovementStatus(std::uint32_t value) override {
        Check(value == 11u, "movement status arg");
        calls.emplace_back("movement status");
    }
    bool DialogNeedsUpdate(std::uint32_t handle) override {
        Check(handle == object + 0x32cu, "dialog handle");
        calls.emplace_back("dialog check");
        return dialog_checks++ == 0 ? dialog_active :
                                      dialog_active_after_delay;
    }
    void UpdateDialogContent(std::uint32_t handle) override {
        Check(handle == object + 0x32cu, "dialog update handle");
        calls.emplace_back("dialog update");
    }
    std::uint32_t FindObjectByName(std::string_view name) override {
        Check(name == "LaunchObject", "launch object name");
        calls.emplace_back("find object");
        return found_object;
    }
    void RemoveAnimatedObject(std::uint32_t address) override {
        Check(address == found_object, "remove object");
        calls.emplace_back("remove animated");
    }
    void ResetAnimatedObject(std::uint32_t address) override {
        Check(address == found_object, "reset object");
        calls.emplace_back("reset animated");
    }
    void PositionAnimatedObject(std::uint32_t address,
                                std::uint32_t position) override {
        Check(address == found_object && position == object + 0xacu,
              "position animated object");
        calls.emplace_back("position animated");
    }
    void SetAnimatedObjectVisibility(std::uint32_t address,
                                     std::uint32_t visible) override {
        Check(address == found_object && visible == 1u,
              "animated visibility");
        calls.emplace_back("set animated visible");
    }
    void OrientAnimatedObject(std::uint32_t address,
                              std::uint32_t position) override {
        Check(address == found_object && position == object + 0xacu,
              "orient animated object");
        calls.emplace_back("orient animated");
    }
    void SetAllAtomicMaterialFlags(std::uint32_t address,
                                   std::uint32_t flags) override {
        Check(address == found_object && flags == 0xffffffffu,
              "material flags");
        calls.emplace_back("set materials");
    }
    void AddAnimatedObject(std::uint32_t address) override {
        Check(address == found_object, "add animated object");
        calls.emplace_back("add animated");
    }
    std::uint32_t TimeMilliseconds() override {
        calls.emplace_back("time");
        return 1234u;
    }
    float RandomDelay(float minimum, float maximum, float seed) override {
        Check(Near(minimum, 1.0f) && Near(maximum, 3.0f) &&
              Near(seed, 1234.0f), "random delay args");
        calls.emplace_back("random delay");
        return random_delay;
    }
    pal4::GobObstacleEffectHandle CreateEffect(
        std::string_view name) override {
        Check(name == "LaunchEffect", "effect name");
        calls.emplace_back("create effect");
        return {0x9000u, 0x9100u};
    }
    void ResetTemporaryEffect() override {
        calls.emplace_back("reset temporary effect");
    }
    std::uint32_t UpdateEffectProperties(std::uint32_t handle,
                                         std::uint32_t clump) override {
        Check(handle == object + 0x32cu && clump == 0x8000u,
              "effect properties args");
        calls.emplace_back("update effect properties");
        return 0x99u;
    }
};

void CheckForceBranches() {
    Runtime absent;
    absent.words[Runtime::control] = 0;
    Check(pal4::GobObstacle::UpdateMachineAndApplyActorForce(
              Runtime::object, 0.5f, absent) == Runtime::control &&
          absent.calls == (std::vector<std::string>{
              "update machine", "actor control"}),
          "missing player control returns access pointer");
    Runtime no_actor;
    no_actor.words[Runtime::control + 4u] = 0;
    Check(pal4::GobObstacle::UpdateMachineAndApplyActorForce(
              Runtime::object, 0.5f, no_actor) == Runtime::control &&
          no_actor.calls == (std::vector<std::string>{
              "update machine", "actor control", "actor control"}),
          "missing active actor returns access pointer");
    Runtime outside;
    outside.distance = 20.0f;
    outside.words[Runtime::object + 0x2f0u] = 1;
    outside.floats[Runtime::object + 0x2ecu] = 2.0f;
    outside.floats[Runtime::object + 0x2e0u] = 2.0f;
    Check(pal4::GobObstacle::UpdateMachineAndApplyActorForce(
              Runtime::object, 0.5f, outside) == 0u &&
          Near(outside.floats[Runtime::object + 0x2ecu], 1.25f) &&
          outside.forces.size() == 1u &&
          Near(outside.forces[0].x, 1.25f),
          "outside range decays active thrust and applies residual force");
    Runtime expired;
    expired.distance = 20.0f;
    expired.words[Runtime::object + 0x2f0u] = 1;
    expired.floats[Runtime::object + 0x2ecu] = 0.5f;
    Check(pal4::GobObstacle::UpdateMachineAndApplyActorForce(
              Runtime::object, 0.5f, expired) == 0u &&
          expired.words[Runtime::object + 0x2f0u] == 0u &&
          expired.forces.empty(), "expired thrust clears active flag");
    Runtime too_high;
    too_high.height = 1.0;
    too_high.floats[Runtime::actor + 0xb0u] = 2.0f;
    too_high.words[Runtime::object + 0x2f0u] = 1;
    too_high.floats[Runtime::object + 0x2ecu] = 0.5f;
    Check(pal4::GobObstacle::UpdateMachineAndApplyActorForce(
              Runtime::object, 0.5f, too_high) == 0u &&
          too_high.words[Runtime::object + 0x2f0u] == 0u &&
          too_high.forces.empty(),
          "vertical separation also expires thrust");
    Runtime attraction;
    attraction.distance = 3.0f;
    attraction.floats[Runtime::object + 0x2d8u] = 2.0f;
    Check(pal4::GobObstacle::UpdateMachineAndApplyActorForce(
              Runtime::object, 0.5f, attraction) == 0u &&
          attraction.forces.size() == 1u &&
          Near(attraction.forces[0].x, 1.0f) &&
          attraction.calls[5] == "clamp",
          "range clamp and attraction force");
    Runtime touching;
    touching.distance = (touching.floats[Runtime::actor + 0xecu] +
        touching.floats[Runtime::object + 0xecu]) * 0.80000001f;
    touching.floats[Runtime::object + 0x2d8u] = 2.0f;
    Check(pal4::GobObstacle::UpdateMachineAndApplyActorForce(
              Runtime::object, 0.5f, touching) == 0u &&
          touching.forces.empty() &&
          std::find(touching.calls.begin(), touching.calls.end(), "clamp") == touching.calls.end(),
          "exact range equality skips normalization and suction as IDA5A4F1D");
    Runtime thrust;
    thrust.floats[Runtime::object + 0x2dcu] = 2.0f;
    thrust.words[Runtime::object + 0x2f8u] = 1u;
    thrust.floats[Runtime::object + 0x304u] = 1.5f;
    Check(pal4::GobObstacle::UpdateMachineAndApplyActorForce(
              Runtime::object, 0.5f, thrust) == 0u &&
          thrust.words[Runtime::object + 0x2f0u] == 1u &&
          Near(thrust.floats[Runtime::object + 0x2ecu], 2.0f) &&
          thrust.forces.size() == 1u &&
          Near(thrust.forces[0].x, -10.0f) &&
          Near(thrust.forces[0].z, -1.0f),
          "initial thrust direction, status and impulse");
    Runtime forward;
    forward.floats[Runtime::object + 0x2dcu] = 2.0f;
    forward.words[Runtime::object + 0x2f4u] = 1u;
    forward.distance = 2.0f;
    Check(pal4::GobObstacle::UpdateMachineAndApplyActorForce(
              Runtime::object, 0.5f, forward) == 0u &&
          forward.forces.size() == 1u &&
          Near(forward.forces[0].x, 2.0f),
          "forward transform thrust uses distance scale");
    Runtime sustained;
    sustained.words[Runtime::object + 0x2f0u] = 1;
    sustained.words[Runtime::object + 0x2f8u] = 1;
    sustained.floats[Runtime::object + 0x2dcu] = 2.0f;
    sustained.floats[Runtime::object + 0x2ecu] = 1.0f;
    sustained.floats[Runtime::object + 0x2e0u] = 4.0f;
    Check(pal4::GobObstacle::UpdateMachineAndApplyActorForce(
              Runtime::object, 0.5f, sustained) == 0u &&
          sustained.forces.size() == 1u &&
          Near(sustained.forces[0].x, 2.0f) &&
          std::find(sustained.calls.begin(), sustained.calls.end(),
                    "apply status") == sustained.calls.end(),
          "sustained thrust does not reapply status");
}

void CheckEffectTypes() {
    for (std::uint32_t type = 0; type <= 4; ++type) {
        Runtime runtime;
        runtime.words[Runtime::object + 0x2f8u] = type;
        runtime.words[Runtime::object + 0x2fcu] = 11;
        runtime.words[Runtime::object + 0x300u] = 12;
        runtime.floats[Runtime::object + 0x304u] = 1.5f;
        pal4::GobObstacle::ApplyEffectTypeToActor(Runtime::object, runtime);
        const auto expected = type == 1 ? "apply status" :
                              type == 2 ? "start status" :
                              type == 3 ? "movement status" : "";
        Check(runtime.calls == (expected[0] == '\0' ?
              std::vector<std::string>{} :
              std::vector<std::string>{expected}),
              "effect type dispatch");
    }
}

void CheckLaunch() {
    Runtime disabled;
    Check(pal4::GobObstacle::UpdateLaunchEffectOrDialog(
              Runtime::object, 0.5f, disabled) == 0u &&
          disabled.calls.empty(), "empty launch name disables timer");
    Runtime active;
    active.words[Runtime::object + 0x30cu] = 12;
    active.dialog_active = true;
    Check(pal4::GobObstacle::UpdateLaunchEffectOrDialog(
              Runtime::object, 0.5f, active) == 1u &&
          active.calls == (std::vector<std::string>{"dialog check"}),
          "active dialog blocks launch");
    Runtime effect;
    effect.words[Runtime::object + 0x30cu] = 12;
    effect.words[Runtime::object + 0x324u] = 12;
    effect.words[Runtime::object + 0x320u] = 0x7100u;
    effect.words[Runtime::object + 0x14u] = 0x8100u;
    effect.words[0x8104u] = 0x8000u;
    effect.floats[Runtime::object + 0x31cu] = 0.25f;
    Check(pal4::GobObstacle::UpdateLaunchEffectOrDialog(
              Runtime::object, 0.5f, effect) == 0x99u &&
          effect.words[Runtime::object + 0x32cu] == 0x9000u &&
          effect.words[Runtime::object + 0x330u] == 0x9100u &&
          Near(effect.floats[Runtime::object + 0x31cu], 0.0f) &&
          effect.calls == (std::vector<std::string>{
              "dialog check", "write float 4892", "write float 4892",
              "dialog check", "create effect", "write word 4908",
              "write word 4912", "reset temporary effect",
              "update effect properties"}),
          "effect countdown creates and binds handle");
    Runtime named;
    named.words[Runtime::object + 0x30cu] = 12;
    named.words[Runtime::object + 0x308u] = 0x7000u;
    named.floats[Runtime::object + 0x31cu] = 0.25f;
    named.floats[Runtime::object + 0x314u] = 1.0f;
    named.floats[Runtime::object + 0x318u] = 3.0f;
    Check(pal4::GobObstacle::UpdateLaunchEffectOrDialog(
              Runtime::object, 0.5f, named) == 0u &&
          Near(named.floats[Runtime::object + 0x31cu], 2.5f) &&
          named.words[named.found_object + 0x190u] == 1u &&
          named.calls == (std::vector<std::string>{
              "dialog check", "write float 4892", "find object",
              "remove animated", "reset animated", "position animated",
              "set animated visible", "write word 24976", "orient animated",
              "set materials", "add animated", "time", "random delay",
              "write float 4892"}),
          "named object launch, material and random delay order");
    Runtime immediate;
    immediate.words[Runtime::object + 0x30cu] = 12;
    immediate.words[Runtime::object + 0x324u] = 12;
    immediate.words[Runtime::object + 0x308u] = 0x7000u;
    immediate.floats[Runtime::object + 0x31cu] = -1.0f;
    immediate.floats[Runtime::object + 0x314u] = 1.0f;
    immediate.floats[Runtime::object + 0x318u] = 3.0f;
    Check(pal4::GobObstacle::UpdateLaunchEffectOrDialog(
              Runtime::object, 0.5f, immediate) == 0u &&
          immediate.calls[1] == "find object",
          "negative timer enters named object path immediately");
    Runtime waiting;
    waiting.words[Runtime::object + 0x30cu] = 12;
    waiting.words[Runtime::object + 0x308u] = 0x7000u;
    waiting.floats[Runtime::object + 0x31cu] = 2.0f;
    Check(pal4::GobObstacle::UpdateLaunchEffectOrDialog(
              Runtime::object, 0.5f, waiting) == 0u &&
          Near(waiting.floats[Runtime::object + 0x31cu], 1.5f) &&
          waiting.calls.size() == 2u,
          "positive named timer waits");
    Runtime missing;
    missing.words[Runtime::object + 0x30cu] = 12;
    missing.words[Runtime::object + 0x308u] = 0x7000u;
    missing.floats[Runtime::object + 0x31cu] = 0.25f;
    missing.found_object = 0u;
    Check(pal4::GobObstacle::UpdateLaunchEffectOrDialog(
              Runtime::object, 0.5f, missing) == 0u &&
          missing.calls == (std::vector<std::string>{
              "dialog check", "write float 4892", "find object"}),
          "missing named object stops activation");
    Runtime no_effect;
    no_effect.words[Runtime::object + 0x30cu] = 12;
    no_effect.floats[Runtime::object + 0x31cu] = -1.0f;
    Check(pal4::GobObstacle::UpdateLaunchEffectOrDialog(
              Runtime::object, 0.5f, no_effect) == 0u &&
          no_effect.calls == (std::vector<std::string>{"dialog check"}),
          "negative timer without effect does not launch");
}

}  // namespace

int main() {
    try {
        CheckForceBranches();
        CheckEffectTypes();
        CheckLaunch();
        std::cout << "gob_obstacle_physics_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
