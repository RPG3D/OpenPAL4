#include "pal4/combat_process_message_task.h"
#include "pal4/physical_attack_child_task.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kEvent = 0x6000u;

struct Runtime final : pal4::IPhysicalAttackChildDestructionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> resets;
    std::uint32_t dialog_actor = 0u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t EventKey(const std::string_view) override { return 0u; }
    std::uint32_t EventRouter() override { return 0u; }
    std::uint32_t Register(std::uint32_t, std::uint32_t,
                           std::uint32_t, std::uint32_t,
                           std::uint32_t) override {
        return 0u;
    }
    std::uint32_t FindEventGroup(std::uint32_t,
                                 std::uint32_t) override {
        return 0u;
    }
    void ReleaseOwnedSlot(std::uint32_t) override {}
    void FreeNode(std::uint32_t) override {}
    void Free(const std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free");
    }
    void ResetEffect(const std::uint32_t effect) override {
        resets.push_back(effect);
        calls.emplace_back("reset");
    }
    std::uint32_t BaseDestruct(const std::uint32_t task) override {
        Check(task == kTask, "base destructor receives task");
        calls.emplace_back("base_destruct");
        return 0x88u;
    }
    void DestroyEmbeddedBase(std::uint32_t embedded) override {
        Check(embedded == kTask + 216u,
              "embedded attack base begins at +216");
        calls.emplace_back("embedded_base_destruct");
    }
    std::uint32_t GetDialogData(const std::uint32_t event) override {
        Check(event == kEvent, "dialog data helper receives event");
        calls.emplace_back("dialog_data");
        return dialog_actor;
    }
    std::uint32_t DispatchAnimationMessage(
        const std::uint32_t task,
        const std::uint32_t payload) override {
        Check(task == kTask && payload == 0x33u,
              "animation message invokes task virtual slot +28");
        calls.emplace_back("animation_message");
        return 0x99u;
    }
    std::uint32_t DispatchEffectMessage(
        const std::uint32_t task, const std::uint32_t first,
        const std::uint32_t second,
        const std::uint32_t payload) override {
        Check(task == kTask && first == 0x11u && second == 0x33u &&
              payload == kEvent + 8u,
              "effect wrapper copies two event words and passes payload tail");
        calls.emplace_back("effect_message");
        return 0xaau;
    }
    std::uint32_t ResetTemporaryEffect(
        const std::uint32_t first, const std::uint32_t second,
        const std::uint32_t third) override {
        Check(first == 1u && second == 2u && third == 3u,
              "temporary effect wrapper forwards three-word record");
        calls.emplace_back("temporary_reset");
        return 0xbbu;
    }
};

void DestructionAndEffectSlots() {
    Runtime runtime;
    runtime.words[kTask + 68u] = 0x7001u;
    runtime.bytes[0x7000u] = 0u;
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto slot = kTask + 96u + 24u * index;
        runtime.words[slot + 12u] = 0x8001u + 0x100u * index;
        runtime.words[slot + 16u] = 3u;
        runtime.words[slot + 20u] = 4u;
        runtime.bytes[0x8000u + 0x100u * index] = 0u;
    }
    Check(pal4::CombatProcessMessageTask::Destruct(
              kTask, runtime) == 0x88u &&
          runtime.words[kTask] ==
              pal4::CombatProcessMessageTask::kVtable &&
          runtime.words[kTask + 68u] == 0u &&
          runtime.words[kTask + 72u] == 0u &&
          runtime.words[kTask + 76u] == 0u &&
          runtime.resets.size() == 10u &&
          runtime.freed.size() == 6u &&
          runtime.freed.back() == 0x7000u &&
          runtime.calls.back() == "base_destruct",
          "destructor clears subscriptions, five slots, name and base");
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto slot = kTask + 96u + 24u * index;
        Check(runtime.resets[index] == slot &&
              runtime.resets[5u + index] ==
                  kTask + 96u + 24u * (4u - index) &&
              runtime.words[slot + 12u] == 0u &&
              runtime.words[slot + 16u] == 0u &&
              runtime.words[slot + 20u] == 0u,
              "effect slots reset forward then destruct in reverse");
    }

    Runtime shared;
    shared.words[0x5000u + 12u] = 0x7101u;
    shared.bytes[0x7100u] = 2u;
    pal4::CombatProcessMessageTask::DestroyEffectSlot(0x5000u, shared);
    Check(shared.bytes[0x7100u] == 1u && shared.freed.empty() &&
          shared.words[0x5000u + 12u] == 0u &&
          shared.resets == std::vector<std::uint32_t>{0x5000u},
          "shared effect string decrements reference without free");
}

void PhysicalAttackChildDestruction() {
    Runtime runtime;
    Check(pal4::PhysicalAttackChildTask::DestroyBase(kTask, runtime) ==
              0x88u &&
          runtime.calls.front() == "embedded_base_destruct" &&
          runtime.calls.back() == "base_destruct" &&
          runtime.resets.size() == 10u &&
          runtime.words[kTask] ==
              pal4::CombatProcessMessageTask::kVtable,
          "physical attack child destroys embedded task before message base");
}

void EventBridges() {
    Runtime runtime;
    runtime.words[kTask + 60u] = 0x5000u;
    runtime.words[kEvent] = 0x11u;
    runtime.words[kEvent + 4u] = 0x33u;
    runtime.dialog_actor = 0x6000u;
    Check(pal4::CombatProcessMessageTask::HandleAnimationMessage(
              kTask, kEvent, runtime) == 0x6000u &&
          runtime.calls == std::vector<std::string>{"dialog_data"},
          "unrelated actor does not dispatch animation message");
    runtime.dialog_actor = 0x5000u;
    Check(pal4::CombatProcessMessageTask::HandleAnimationMessage(
              kTask, kEvent, runtime) == 0x99u &&
          runtime.calls.back() == "animation_message" &&
          pal4::CombatProcessMessageTask::HandleEffectMessage(
              kTask, kEvent, runtime) == 0xaau &&
          runtime.calls.back() == "effect_message" &&
          pal4::CombatProcessMessageTask::ResetTemporaryEffect(
              1u, 2u, 3u, runtime) == 0xbbu &&
          runtime.calls.back() == "temporary_reset",
          "matching actor and effect event dispatch through virtual bridges");
}

struct FlowRuntime final : pal4::ICombatProcessMessageFlowRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> slots;
    bool animation_complete = true;
    std::uint32_t active_slot = 0;

    void RegisterSubscriptions(const std::uint32_t task) override {
        Check(task == kTask, "register task");
        calls.emplace_back("register");
    }
    std::uint8_t ProcessAction(const std::uint32_t task) override {
        Check(task == kTask, "process task");
        calls.emplace_back("action");
        return 7u;
    }
    void ClearSubscriptions(const std::uint32_t task) override {
        Check(task == kTask, "clear task");
        calls.emplace_back("clear");
    }
    void HandleEffectEvent(const std::uint32_t slot,
                           const std::uint32_t context,
                           const bool active) override {
        Check(context == 0x9000u && !active,
              "effect event cleanup arguments");
        slots.push_back(slot);
        calls.emplace_back("handle");
    }
    bool IsAnimationComplete(const std::uint32_t task) override {
        Check(task == kTask, "animation task");
        calls.emplace_back("animation");
        return animation_complete;
    }
    bool EffectSlotActive(const std::uint32_t slot) override {
        slots.push_back(slot);
        calls.emplace_back("effect");
        return slot == active_slot;
    }
    void CallActorCommand(const std::uint32_t actor,
                          const std::uint32_t first,
                          const std::uint32_t second,
                          const std::uint32_t fourth) override {
        Check(actor == 0x3000u && first == 11u &&
              second == 22u && fourth == 44u,
              "actor virtual slot +384 argument order");
        calls.emplace_back("command");
    }
};

void ActionFlow() {
    FlowRuntime runtime;
    Check(pal4::CombatProcessMessageTask::StartAction(kTask, runtime) == 7u &&
          runtime.calls == std::vector<std::string>{"register", "action"},
          "start registers before processing action");
    runtime.calls.clear();
    Check(pal4::CombatProcessMessageTask::StopAction(
              kTask, 0x9000u, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"clear", "handle",
              "handle", "handle", "handle", "handle"} &&
          runtime.slots.size() == 5u, "stop clears and handles five slots");
    for (std::uint32_t index = 0; index < 5u; ++index)
        Check(runtime.slots[index] == kTask + 96u + 24u * index,
              "effect slot stride and order");

    runtime.calls.clear();
    runtime.slots.clear();
    runtime.active_slot = kTask + 96u + 2u * 24u;
    Check(!pal4::CombatProcessMessageTask::IsActionComplete(kTask, runtime) &&
          runtime.slots.size() == 3u &&
          runtime.calls.front() == "animation",
          "active effect stops scan and overrides pending animation");
    runtime.active_slot = 0u;
    runtime.slots.clear();
    Check(pal4::CombatProcessMessageTask::IsActionComplete(kTask, runtime) &&
          runtime.slots.size() == 5u,
          "no active effects returns animation pending state");
    runtime.animation_complete = false;
    Check(!pal4::CombatProcessMessageTask::IsActionComplete(kTask, runtime),
          "completed animation and effects return false");
    Check(pal4::CombatProcessMessageTask::CallGameObjectCommand(
              11u, 22u, 0x3000u, 44u, runtime) == 11u &&
          runtime.calls.back() == "command",
          "game object command forwards and returns first argument");
}

}  // namespace

int main() {
    try {
        DestructionAndEffectSlots();
        PhysicalAttackChildDestruction();
        EventBridges();
        ActionFlow();
        std::cout << "combat_process_message_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
