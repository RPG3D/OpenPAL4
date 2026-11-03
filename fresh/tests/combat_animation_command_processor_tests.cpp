#include "pal4/combat_animation_command_processor.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kActor = 0x5000u;
constexpr std::uint32_t kEntry = 0x6000u;
constexpr pal4::CombatEffectHandle kEffect{0x7000u, 3u};

struct Runtime final : pal4::ICombatAnimationCommandRuntime {
    std::vector<std::string> calls;
    std::map<std::string, std::string> attributes;
    std::string lookup_name;
    std::string cleaned_command;
    std::uint32_t entry = kEntry;
    std::uint32_t clump = 0x8000u;
    std::uint32_t target = 0;
    bool effect_active = true;
    bool slot_active = false;
    float scale = 0;
    int type_fallback = 0;
    std::uint32_t slot = 0;
    std::uint32_t resets = 0;
    bool hands_active = false;

    std::uint32_t Actor(const std::uint32_t task) override {
        Check(task == kTask, "task address");
        calls.emplace_back("actor");
        return kActor;
    }
    std::uint32_t FindAnimationEntry(const std::uint32_t actor,
                                      const std::string_view name) override {
        Check(actor == kActor, "animation actor");
        lookup_name = name;
        calls.emplace_back("find");
        return entry;
    }
    std::uint32_t ChildClump(const std::uint32_t actor) override {
        Check(actor == kActor, "child clump actor");
        calls.emplace_back("clump");
        return clump;
    }
    std::string Attribute(const std::uint32_t source,
                          const std::string_view key) override {
        Check(source == kEntry, "attribute entry");
        calls.emplace_back(std::string(key));
        return attributes[std::string(key)];
    }
    float FloatAttribute(const std::uint32_t source,
                         const std::string_view key,
                         const float fallback) override {
        Check(source == kEntry && key == "scale" && fallback == 1.0f,
              "time scale attribute");
        calls.emplace_back("float");
        return 0.35f;
    }
    int IntAttribute(const std::uint32_t source,
                     const std::string_view key,
                     const int fallback) override {
        Check(source == kEntry && key == "type", "camera attribute");
        type_fallback = fallback;
        calls.emplace_back("int");
        return 9;
    }
    bool EffectActive(const pal4::CombatEffectHandle handle) override {
        Check(handle.object == kEffect.object &&
              handle.generation == kEffect.generation, "created handle");
        calls.emplace_back("effect_active");
        return effect_active;
    }
    bool SlotActive(const std::uint32_t address) override {
        slot = address;
        calls.emplace_back("slot_active");
        return slot_active;
    }
    void ReleaseSlot(const std::uint32_t address) override {
        Check(address == slot, "release selected slot");
        calls.emplace_back("release");
    }
    pal4::CombatEffectHandle CreateEffect(
        const std::string_view resource) override {
        Check(resource == attributes["ef"], "effect resource name");
        calls.emplace_back("create");
        return kEffect;
    }
    void StoreSlot(const std::uint32_t address,
                   const pal4::CombatEffectHandle handle,
                   const std::string_view bind) override {
        Check(address == slot && handle.object == kEffect.object &&
              bind == attributes["bind"], "store effect slot and binding");
        calls.emplace_back("store");
    }
    void SetCallback(const pal4::CombatEffectHandle,
                     const std::string_view property,
                     const std::uint32_t task) override {
        Check(property == "cbAttachCast" && task == kTask,
              "effect callback property");
        calls.emplace_back("callback");
    }
    std::uint32_t TaskTarget(const std::uint32_t task) override {
        Check(task == kTask, "target task");
        calls.emplace_back("target");
        return target;
    }
    void SetTarget(const pal4::CombatEffectHandle,
                   const std::string_view property,
                   const std::uint32_t address) override {
        Check(property == "T1" && address == target + 724u,
              "target property offset");
        calls.emplace_back("set_target");
    }
    void AttachActorTransform(const pal4::CombatEffectHandle,
                              const std::uint32_t actor) override {
        Check(actor == kActor, "hook actor transform");
        calls.emplace_back("attach_actor");
    }
    void ApplyBindingRotation(const pal4::CombatEffectHandle) override {
        calls.emplace_back("default_matrix");
    }
    std::uint32_t EffectFrame(const pal4::CombatEffectHandle) override {
        calls.emplace_back("matrix");
        return 0xa000u;
    }
    void ProcessBinding(const std::uint32_t source,
                        const std::string_view bind,
                        const std::uint32_t matrix) override {
        Check(source == clump && bind == attributes["bind"] &&
              matrix == 0xa000u, "RenderWare binding arguments");
        calls.emplace_back("bind");
    }
    void UpdateEffect(const pal4::CombatEffectHandle) override {
        calls.emplace_back("update");
    }
    void ResetTemporaryEffect(const pal4::CombatEffectHandle handle) override {
        Check(handle.object == kEffect.object, "temporary effect handle");
        ++resets;
        calls.emplace_back("reset");
    }
    void SetTimeScale(const float value) override {
        scale = value;
        calls.emplace_back("scale");
    }
    void SetHandsActive(const std::uint32_t actor,
                        const bool active) override {
        Check(actor == kActor, "hand actor");
        hands_active = active;
        calls.emplace_back("hands");
    }
    std::uint32_t CleanupCommand(const std::string_view command) override {
        cleaned_command = command;
        calls.emplace_back("cleanup");
        return 0x55u;
    }
};

void GateAndSlotRelease() {
    Runtime runtime;
    runtime.clump = 0;
    Check(pal4::CombatAnimationCommandProcessor::Process(
              kTask, "EFS2", runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"actor", "find", "clump"} &&
          runtime.lookup_name == "EFS2", "clump gate precedes command copy");

    runtime = {};
    runtime.entry = 0;
    runtime.slot_active = true;
    Check(pal4::CombatAnimationCommandProcessor::Process(
              kTask, "EFS2", runtime) == 0x55u &&
          runtime.slot == kTask + 96u + 2u * 24u &&
          runtime.cleaned_command == "2" &&
          runtime.calls.back() == "cleanup", "efs releases selected slot");
    Check(pal4::CombatAnimationCommandProcessor::Process(
              kTask, "efs5", runtime) == 0x55u &&
          runtime.slot == kTask + 96u + 2u * 24u,
          "slot five is outside the native limit");
    Check(pal4::CombatAnimationCommandProcessor::Process(
              kTask, "efs-1", runtime) == 0x55u &&
          runtime.slot == kTask + 96u + 2u * 24u,
          "negative native index is blocked by C++ boundary");
}

void Effects() {
    Runtime hook;
    hook.attributes = {{"ef", "spark"}, {"bind", "HoOk_Ef_PoS"}};
    hook.slot_active = true;
    hook.target = 0x9000u;
    Check(pal4::CombatAnimationCommandProcessor::Process(
              kTask, "EF1", hook) == 0x55u &&
          hook.lookup_name == "EF1" && hook.cleaned_command == "1" &&
          hook.slot == kTask + 96u + 24u && hook.resets == 1u &&
          hook.calls == std::vector<std::string>{"actor", "find", "clump",
              "ef", "bind", "create", "effect_active", "slot_active",
              "release", "store", "callback", "target", "target",
              "set_target", "attach_actor", "update", "reset", "cleanup"},
          "hook effect updates slot, target, transform and cleanup in order");

    Runtime bound;
    bound.attributes = {{"ef", "smoke"}, {"bind", "left_hand"}};
    Check(pal4::CombatAnimationCommandProcessor::Process(
              kTask, "ef0", bound) == 0x55u &&
          bound.calls == std::vector<std::string>{"actor", "find", "clump",
              "ef", "bind", "create", "effect_active", "slot_active",
              "store", "callback", "target", "default_matrix", "matrix",
              "clump", "bind", "update", "reset", "cleanup"},
          "non-hook binding uses default matrix and clump user data");

    Runtime inactive;
    inactive.attributes = {{"ef", "spark"}, {"bind", ""}};
    inactive.effect_active = false;
    Check(pal4::CombatAnimationCommandProcessor::Process(
              kTask, "ef4", inactive) == 0x55u &&
          inactive.resets == 1u &&
          inactive.calls == std::vector<std::string>{"actor", "find", "clump",
              "ef", "bind", "create", "effect_active", "reset", "cleanup"},
          "inactive created effect is still reset");
}

void OtherCommands() {
    Runtime runtime;
    Check(pal4::CombatAnimationCommandProcessor::Process(
              kTask, "tms", runtime) == 0x55u &&
          std::fabs(runtime.scale - 0.35f) < 0.0001f,
          "tms reads scale with unit fallback");
    runtime.entry = 0;
    runtime.scale = 0;
    (void)pal4::CombatAnimationCommandProcessor::Process(kTask, "tms", runtime);
    Check(runtime.scale == 0.0f, "tms requires animation entry");
    (void)pal4::CombatAnimationCommandProcessor::Process(kTask, "rtm", runtime);
    Check(runtime.scale == 1.0f, "rtm resets time scale without entry");

    runtime.entry = kEntry;
    (void)pal4::CombatAnimationCommandProcessor::Process(kTask, "cg", runtime);
    Check(runtime.type_fallback == 1 && runtime.calls.back() == "cleanup",
          "cg reads type but does not use result");
    runtime.calls.clear();
    (void)pal4::CombatAnimationCommandProcessor::Process(kTask, "rcg", runtime);
    Check(runtime.calls == std::vector<std::string>{"actor", "find", "clump",
          "cleanup"}, "rcg is a consumed no-op");

    (void)pal4::CombatAnimationCommandProcessor::Process(kTask, "WPHIDE", runtime);
    Check(!runtime.hands_active && runtime.calls[runtime.calls.size() - 2] ==
          "hands", "wphide disables both hands");
    (void)pal4::CombatAnimationCommandProcessor::Process(kTask, "wpshow", runtime);
    Check(runtime.hands_active, "wpshow enables both hands");
    runtime.calls.clear();
    (void)pal4::CombatAnimationCommandProcessor::Process(kTask, "wpshow_extra", runtime);
    Check(runtime.calls == std::vector<std::string>{"actor", "find", "clump",
          "cleanup"}, "hand command requires exact name");
}

}  // namespace

int main() {
    try {
        GateAndSlotRelease();
        Effects();
        OtherCommands();
        std::cout << "combat_animation_command_processor_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
