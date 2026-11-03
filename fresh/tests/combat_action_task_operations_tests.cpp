#include "pal4/combat_action_task_operations.h"

#include <algorithm>
#include <cctype>
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
constexpr std::uint32_t kActor = 0x5000u;

struct ActionRuntime final : pal4::ICombatActionTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::string> calls;
    std::string animation_name = "Run";
    std::int32_t animation_flag = 0;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    std::string ReadCString(const std::uint32_t address) override {
        return strings.at(address);
    }
    std::uint32_t SkillSystem() override {
        calls.emplace_back("skill_system");
        return 0x8000u;
    }
    std::uint32_t GlobalSystem() override {
        calls.emplace_back("global_system");
        return 0x8100u;
    }
    void SetActorTarget(const std::uint32_t actor,
                        const std::uint32_t target,
                        const std::uint8_t mode) override {
        Check(actor == kActor && target == kTask + 64u && mode == 3u,
              "change-action target uses actor slot and task target block");
        calls.emplace_back("set_target");
    }
    void StartActorAction(const std::uint32_t actor,
                          const std::uint32_t target,
                          const std::uint32_t mode) override {
        Check(actor == kActor && target == kTask + 64u && mode == 0u,
              "play-action dispatches virtual slot +0x234");
        calls.emplace_back("start_action");
    }
    std::string GetAnimationName(const std::uint32_t actor) override {
        Check(actor == kActor, "animation name uses selected actor");
        calls.emplace_back("animation_name");
        return animation_name;
    }
    std::int32_t GetAnimationFlag(const std::uint32_t actor) override {
        Check(actor == kActor, "animation flag uses selected actor");
        calls.emplace_back("animation_flag");
        return animation_flag;
    }
    int CompareInsensitive(const std::string_view left,
                           const std::string_view right) override {
        calls.emplace_back("compare");
        const auto lower = [](const unsigned char ch) {
            return std::tolower(ch);
        };
        if (left.size() != right.size()) return 1;
        for (std::size_t index = 0u; index < left.size(); ++index)
            if (lower(static_cast<unsigned char>(left[index])) !=
                lower(static_cast<unsigned char>(right[index])))
                return 1;
        return 0;
    }
};

struct EventRuntime final : pal4::IEventSubscriptionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> released;
    std::vector<std::uint32_t> freed;
    std::uint32_t count = 0u;

    EventRuntime() {
        words[pal4::CombatProcessMessageSubscriptions::kAnimationKey] =
            0xa100u;
        words[pal4::CombatProcessMessageSubscriptions::kEffectKey] =
            0xa200u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t EventKey(const std::string_view) override { return 0u; }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return 0x7000u;
    }
    std::uint32_t Register(const std::uint32_t router,
                           const std::uint32_t mode,
                           const std::uint32_t key,
                           const std::uint32_t owner,
                           const std::uint32_t callback) override {
        Check(router == 0x7000u && mode == 2u && owner == kTask &&
              key == (count ? 0xa200u : 0xa100u) &&
              callback == (count
                  ? pal4::CombatProcessMessageSubscriptions::kEffectCallback
                  : pal4::CombatProcessMessageSubscriptions::kAnimationCallback),
              "message task subscribes two mode-2 event callbacks");
        const auto index = count++;
        const auto group = 0x9000u + 0x100u * index;
        const auto head = group + 0x20u;
        const auto node = 0xa000u + 0x100u * index;
        words[group + 4u] = head;
        words[group + 8u] = 1u;
        words[head] = node;
        words[head + 4u] = node;
        words[node] = head;
        words[node + 4u] = head;
        words[node + 12u] = 0xb000u + 0x100u * index;
        calls.emplace_back("register");
        return words[node + 12u];
    }
    std::uint32_t FindEventGroup(const std::uint32_t router,
                                 const std::uint32_t key) override {
        Check(router == 0x7000u, "cleanup uses event router");
        return key == 0xa100u ? 0x9000u :
               key == 0xa200u ? 0x9100u : 0u;
    }
    void ReleaseOwnedSlot(const std::uint32_t slot) override {
        released.push_back(slot);
        calls.emplace_back("release_slot");
    }
    void FreeNode(const std::uint32_t node) override {
        freed.push_back(node);
        calls.emplace_back("free_node");
    }
};

void ActorActionsAndAnimationState() {
    ActionRuntime runtime;
    Check(pal4::CombatActionTaskOperations::SetTarget(
              kTask, runtime) == 1u &&
          pal4::CombatActionTaskOperations::ProcessAction(
              kTask, runtime) == 1u && runtime.calls.empty(),
          "missing actor makes both action entries handled no-ops");
    runtime.words[kTask + 60u] = kActor;
    runtime.bytes[kTask + 80u] = 3u;
    Check(pal4::CombatActionTaskOperations::SetTarget(
              kTask, runtime) == 1u &&
          pal4::CombatActionTaskOperations::ProcessAction(
              kTask, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{
              "set_target", "skill_system", "global_system",
              "start_action"},
          "action entries dispatch target and start after singleton getters");

    runtime.words[kTask + 68u] = 0x6000u;
    runtime.strings[0x6000u] = "RUN";
    runtime.calls.clear();
    Check(!pal4::CombatActionTaskOperations::IsAnimationComplete(
              kTask, runtime) &&
          runtime.calls == std::vector<std::string>{
              "animation_name", "compare", "animation_flag"},
          "matching name and zero flag mean animation is finished");
    runtime.animation_flag = 1;
    Check(pal4::CombatActionTaskOperations::IsAnimationComplete(
              kTask, runtime),
          "positive actor animation flag keeps task pending");
    runtime.strings[0x6000u] = "Walk";
    runtime.calls.clear();
    Check(pal4::CombatActionTaskOperations::IsAnimationComplete(
              kTask, runtime) &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "animation_flag") == runtime.calls.end(),
          "different animation name returns pending before flag query");
}

void MessageSubscriptions() {
    EventRuntime runtime;
    Check(pal4::CombatProcessMessageSubscriptions::Register(
              kTask, runtime) == 0xb100u &&
          runtime.words[kTask + 80u] == 0xb000u &&
          runtime.words[kTask + 84u] == 0xa100u &&
          runtime.words[kTask + 88u] == 0xb100u &&
          runtime.words[kTask + 92u] == 0xa200u &&
          runtime.count == 2u,
          "message task stores two callback handles and keys");
    pal4::CombatProcessMessageSubscriptions::Clear(kTask, runtime);
    Check(runtime.words[kTask + 80u] == 0u &&
          runtime.words[kTask + 84u] == 0u &&
          runtime.words[kTask + 88u] == 0u &&
          runtime.words[kTask + 92u] == 0u &&
          runtime.words[0x9000u + 8u] == 0u &&
          runtime.words[0x9100u + 8u] == 0u &&
          runtime.released == std::vector<std::uint32_t>{
              0xa008u, 0xa108u} &&
          runtime.freed == std::vector<std::uint32_t>{
              0xa000u, 0xa100u},
          "cleanup unlinks both event nodes and releases owned slots");
    pal4::CombatProcessMessageSubscriptions::Clear(kTask, runtime);
    Check(runtime.freed.size() == 2u,
          "repeated cleanup is idempotent");
}

}  // namespace

int main() {
    try {
        ActorActionsAndAnimationState();
        MessageSubscriptions();
        std::cout << "combat_action_task_operations_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
