#include "pal4/combat_script_task_operations.h"

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

struct Runtime final : pal4::ICombatScriptTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> reads;
    std::vector<std::uint32_t> writes;
    std::uint32_t fired = 0;
    std::uint32_t protected_target = 0;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        reads.push_back(address);
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        writes.push_back(address);
        words[address] = value;
    }
    void FireEvent(const std::uint32_t callback,
                   const std::uint32_t actor,
                   const std::uint32_t argument) override {
        Check(callback == 0x6000u && actor == 0x5000u &&
              argument == 0x7000u, "fire-event callback arguments");
        ++fired;
        calls.emplace_back("fire");
    }
    void SetProtectTarget(const std::uint32_t actor,
                          const std::uint32_t target,
                          const std::uint32_t argument) override {
        Check(actor == 0x5000u && target == 0x6000u &&
              argument == 0x7000u, "actor slot +0x228 arguments");
        ++protected_target;
        calls.emplace_back("protect");
    }
    void ConstructBase(const std::uint32_t task) override {
        Check(task == kTask, "base construction address");
        calls.emplace_back("base");
    }
};

void FireEventGate() {
    Runtime runtime;
    Check(pal4::CombatScriptTaskOperations::FireEvent(kTask, runtime) == 1u &&
          runtime.reads == std::vector<std::uint32_t>{kTask + 60u} &&
          runtime.fired == 0u, "null actor stops event task");
    runtime.words[kTask + 60u] = 0x5000u;
    runtime.reads.clear();
    Check(pal4::CombatScriptTaskOperations::FireEvent(kTask, runtime) == 1u &&
          runtime.reads == std::vector<std::uint32_t>{kTask + 60u,
                                                     kTask + 64u} &&
          runtime.fired == 0u, "null callback stops event task");
    runtime.words[kTask + 64u] = 0x6000u;
    runtime.words[kTask + 68u] = 0x7000u;
    runtime.reads.clear();
    Check(pal4::CombatScriptTaskOperations::FireEvent(kTask, runtime) == 1u &&
          runtime.reads == std::vector<std::uint32_t>{kTask + 60u,
              kTask + 64u, kTask + 68u} && runtime.fired == 1u,
          "event task reads payload only when actor and callback exist");
}

void ProtectAndNormalAttack() {
    Runtime runtime;
    runtime.words[kTask + 64u] = 0x5000u;
    runtime.words[kTask + 60u] = 0x6000u;
    runtime.words[kTask + 68u] = 0x7000u;
    Check(pal4::CombatScriptTaskOperations::SetProtectTarget(
              kTask, runtime) == 1u &&
          runtime.protected_target == 1u &&
          runtime.reads == std::vector<std::uint32_t>{kTask + 64u,
              kTask + 68u, kTask + 60u},
          "protect task forwards actor, target and argument");

    runtime.calls.clear();
    Check(pal4::NormalAttackTask::Construct(kTask, runtime) == kTask &&
          runtime.words[kTask] == pal4::NormalAttackTask::kVtable &&
          runtime.words[kTask + 60u] == 0u &&
          runtime.words[kTask + 64u] == 0u &&
          runtime.writes == std::vector<std::uint32_t>{kTask,
              kTask + 64u, kTask + 60u} &&
          runtime.calls == std::vector<std::string>{"base"},
          "normal attack constructs base, vtable and two actor slots");
}

struct AttackRuntime final : pal4::INormalAttackProcessRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> candidates;
    std::vector<std::uint32_t> handled;
    std::size_t cursor = 0;
    std::uint32_t mode = 0;
    std::uint32_t releases = 0;
    bool throw_on_handle = false;
    float scale = 0;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words[address];
    }
    void WriteScale(const std::uint32_t address,
                    const float value) override {
        Check(address == kTask + 68u, "attack scale field");
        scale = value;
        calls.emplace_back("scale");
    }
    void PlayAttackSound(const std::uint32_t actor,
                         const std::uint32_t sound_mode) override {
        Check(actor == 0x5000u && sound_mode == 1u,
              "attack sound actor and mode");
        calls.emplace_back("sound");
    }
    std::uint32_t ActorMode(const std::uint32_t actor) override {
        Check(actor == 0x5000u, "attack mode actor");
        calls.emplace_back("mode");
        return mode;
    }
    float DamageScale(const std::uint32_t index) override {
        Check(index == 26u, "game database damage scale index");
        calls.emplace_back("damage_scale");
        return 0.4f;
    }
    std::uint32_t TargetIterator(const std::uint32_t target) override {
        Check(target == 0x6000u, "target iterator source");
        calls.emplace_back("iterator");
        cursor = 0;
        return 0x7000u;
    }
    bool IteratorAtEnd(const std::uint32_t iterator) override {
        Check(iterator == 0x7000u, "iterator end handle");
        calls.emplace_back("end");
        return cursor >= candidates.size();
    }
    std::uint32_t IteratorCurrent(const std::uint32_t iterator) override {
        Check(iterator == 0x7000u, "iterator current handle");
        calls.emplace_back("current");
        return candidates[cursor];
    }
    void IteratorNext(const std::uint32_t iterator) override {
        Check(iterator == 0x7000u, "iterator next handle");
        calls.emplace_back("next");
        ++cursor;
    }
    void ReleaseIterator(const std::uint32_t iterator) override {
        Check(iterator == 0x7000u, "iterator release handle");
        calls.emplace_back("release");
        ++releases;
    }
    bool ActorEligible(const std::uint32_t actor) override {
        calls.emplace_back("eligible");
        return actor != 0x9000u;
    }
    void HandleActor(const std::uint32_t task,
                     const std::uint32_t actor) override {
        Check(task == kTask, "attack handler task");
        if (throw_on_handle) throw std::runtime_error("handler failure");
        calls.emplace_back("handle");
        handled.push_back(actor);
    }
};

void NormalAttackProcessing() {
    AttackRuntime runtime;
    pal4::NormalAttackTask::ProcessInternal(kTask, runtime);
    Check(runtime.calls == std::vector<std::string>{"read"},
          "missing attacker does not play sound");
    runtime.words[kTask + 60u] = 0x5000u;
    runtime.calls.clear();
    pal4::NormalAttackTask::ProcessInternal(kTask, runtime);
    Check(runtime.calls == std::vector<std::string>{"read", "read"},
          "missing target does not play sound");

    runtime.words[kTask + 64u] = 0x6000u;
    runtime.calls.clear();
    Check(pal4::NormalAttackTask::Process(kTask, runtime) == 1u,
          "normal attack process wrapper returns handled");
    Check(runtime.scale == 1.0f &&
          runtime.handled == std::vector<std::uint32_t>{0x6000u} &&
          runtime.calls == std::vector<std::string>{"read", "read", "sound",
              "mode", "scale", "handle"},
          "ordinary attack processes only the selected target");

    runtime.mode = 1u;
    runtime.candidates = {0x8000u, 0x9000u, 0xa000u};
    runtime.handled.clear();
    runtime.calls.clear();
    pal4::NormalAttackTask::ProcessInternal(kTask, runtime);
    Check(runtime.scale == 0.4f && runtime.releases == 1u &&
          runtime.handled ==
              std::vector<std::uint32_t>{0x8000u, 0xa000u} &&
          runtime.calls.front() == "read" &&
          runtime.calls.back() == "release",
          "group attack scans eligible candidates and releases iterator");

    runtime.throw_on_handle = true;
    try {
        pal4::NormalAttackTask::ProcessInternal(kTask, runtime);
        throw std::runtime_error("missing handler exception");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "handler failure" &&
              runtime.releases == 2u,
              "group iterator is released during handler failure");
    }
}

struct ActorRuntime final : pal4::INormalAttackActorRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool has_property = true;
    bool hit = false;
    bool float_property = false;
    bool dodge = false;
    std::uint32_t mode = 2u;
    pal4::NormalAttackActorBranch branch =
        pal4::NormalAttackActorBranch::Support;

    ActorRuntime() {
        words[kTask + 60u] = 0x5000u;
        words[kTask + 64u] = 0x6000u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read_word");
        return words[address];
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        calls.emplace_back("read_byte");
        return bytes[address];
    }
    bool HasProperty8(const std::uint32_t actor) override {
        Check(actor == 0x7000u, "property actor");
        calls.emplace_back("property");
        return has_property;
    }
    std::uint8_t NoPropertyTarget(const std::uint32_t target,
                                  const std::uint32_t attacker) override {
        Check(target == 0x7000u && attacker == 0x5000u,
              "no-property actor virtual slot +0x150");
        calls.emplace_back("no_property");
        return 0x7du;
    }
    bool CheckHit(const std::uint32_t attacker_data,
                  const std::uint32_t target_data) override {
        Check(attacker_data == 0x5000u + 340u &&
              target_data == 0x7000u + 340u, "hit data offsets");
        calls.emplace_back("hit");
        return hit;
    }
    std::uint32_t TargetMode(const std::uint32_t target) override {
        Check(target == 0x6000u, "target mode source");
        calls.emplace_back("mode");
        return mode;
    }
    bool CheckFloatProperty(const std::uint32_t attacker_data,
                            const std::uint32_t target_data) override {
        Check(attacker_data == 0x5000u + 340u &&
              target_data == 0x7000u + 340u, "float property offsets");
        calls.emplace_back("float");
        return float_property;
    }
    bool CheckDodge(const std::uint32_t attacker_data,
                    const std::uint32_t target_data,
                    const float bonus) override {
        Check(attacker_data == 0x5000u + 340u &&
              target_data == 0x7000u + 340u && bonus == 0.0f,
              "dodge arguments");
        calls.emplace_back("dodge");
        return dodge;
    }
    std::uint8_t DispatchActor(
        const std::uint32_t task, const std::uint32_t actor,
        const pal4::NormalAttackActorBranch next) override {
        Check(task == kTask && actor == 0x7000u, "actor branch arguments");
        branch = next;
        calls.emplace_back("dispatch");
        return static_cast<std::uint8_t>(0x30u +
            static_cast<std::uint8_t>(next));
    }
};

void NormalAttackActorDispatch() {
    using Branch = pal4::NormalAttackActorBranch;
    ActorRuntime runtime;
    runtime.has_property = false;
    Check(pal4::NormalAttackTask::HandleActor(kTask, 0x7000u, runtime) ==
              0x7du &&
          runtime.calls == std::vector<std::string>{"property", "read_word",
              "no_property"}, "non-property actor uses its virtual fallback");

    runtime = {};
    runtime.bytes[kTask + 72u] = 1u;
    Check(pal4::NormalAttackTask::HandleActor(kTask, 0x7000u, runtime) ==
              0x30u && runtime.branch == Branch::Item &&
          runtime.calls == std::vector<std::string>{"property", "read_byte",
              "dispatch"}, "item flag takes priority before hit checks");

    runtime = {};
    runtime.hit = true;
    Check(pal4::NormalAttackTask::HandleActor(kTask, 0x7000u, runtime) ==
              0x31u && runtime.branch == Branch::Magic &&
          runtime.calls.back() == "dispatch",
          "hit preview routes to magic handler");

    runtime = {};
    runtime.float_property = true;
    Check(pal4::NormalAttackTask::HandleActor(kTask, 0x7000u, runtime) ==
              0x30u && runtime.branch == Branch::Item &&
          runtime.calls[runtime.calls.size() - 2] == "float",
          "float-property preview routes to item handler");

    runtime = {};
    runtime.mode = 4u;
    runtime.bytes[kTask + 73u] = 1u;
    runtime.float_property = true;
    Check(pal4::NormalAttackTask::HandleActor(kTask, 0x7000u, runtime) ==
              0x33u && runtime.branch == Branch::Attack &&
          runtime.calls.back() == "dispatch",
          "forced attack skips float-property check in mode four");
    for (const auto& call : runtime.calls)
        Check(call != "float", "mode four suppresses float-property preview");

    runtime = {};
    runtime.dodge = true;
    Check(pal4::NormalAttackTask::HandleActor(kTask, 0x7000u, runtime) ==
              0x33u && runtime.branch == Branch::Attack,
          "dodge result routes to attack handler");

    runtime = {};
    Check(pal4::NormalAttackTask::HandleActor(kTask, 0x7000u, runtime) ==
              0x32u && runtime.branch == Branch::Support,
          "remaining path routes to support handler");
}

struct CommandRuntime final : pal4::INormalAttackCommandRuntime {
    std::vector<std::string> calls;
    std::string original_name;
    std::string cleaned_name;
    std::uint32_t embedded_task = 0;
    std::uint32_t event_actor = 0;

    void ProcessAttack(const std::uint32_t task) override {
        embedded_task = task;
        calls.emplace_back("attack");
    }
    void ProcessAnimationCommand(
        const std::uint32_t task,
        const std::string_view original) override {
        Check(task == kTask, "animation command task");
        original_name = original;
        calls.emplace_back("animation");
    }
    std::uint32_t CleanupCommand(
        const std::string_view lowercase) override {
        cleaned_name = lowercase;
        calls.emplace_back("cleanup");
        return 0x55u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(address == kTask + 276u, "target actor field");
        calls.emplace_back("read");
        return 0x9000u;
    }
    void NotifyTargetState(const std::uint32_t actor,
                           const std::uint32_t event,
                           const std::uint32_t payload) override {
        Check(actor == 0x9000u && event == 14u && payload == actor,
              "actor slot +0x104 target notification");
        event_actor = actor;
        calls.emplace_back("notify");
    }
};

void NormalAttackCommands() {
    CommandRuntime runtime;
    Check(pal4::NormalAttackTask::ProcessCommand(
              kTask, "ATK", runtime) == 0x55u &&
          runtime.embedded_task == kTask + 0xd8u &&
          runtime.cleaned_name == "atk" &&
          runtime.calls == std::vector<std::string>{"attack", "cleanup"},
          "atk command uses embedded task and cleans lowered string");
    runtime.calls.clear();
    Check(pal4::NormalAttackTask::ProcessCommand(
              kTask, "ATK_extra", runtime) == 0x55u &&
          runtime.calls == std::vector<std::string>{"attack", "cleanup"},
          "atk prefix accepts a longer command");
    runtime.calls.clear();
    Check(pal4::NormalAttackTask::ProcessCommand(
              kTask, "Ef1", runtime) == 0x55u &&
          runtime.original_name == "Ef1" &&
          runtime.cleaned_name == "ef1" &&
          runtime.calls == std::vector<std::string>{"animation", "cleanup"},
          "other commands preserve original message for animation lookup");
    runtime.calls.clear();
    (void)pal4::NormalAttackTask::ProcessCommand(kTask, "at", runtime);
    Check(runtime.calls == std::vector<std::string>{"animation", "cleanup"},
          "short partial prefix falls through");
    runtime.calls.clear();
    Check(pal4::NormalAttackTask::SetTarget(kTask, runtime) == 1u &&
          runtime.event_actor == 0x9000u &&
          runtime.calls == std::vector<std::string>{"read", "notify"},
          "target notification forwards actor and event code fourteen");
}

}  // namespace

int main() {
    try {
        FireEventGate();
        ProtectAndNormalAttack();
        NormalAttackProcessing();
        NormalAttackActorDispatch();
        NormalAttackCommands();
        std::cout << "combat_script_task_operations_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
