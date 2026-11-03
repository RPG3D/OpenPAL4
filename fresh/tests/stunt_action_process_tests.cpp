#include "pal4/combat_animation_command_processor.h"
#include "pal4/stunt_action_task.h"

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

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kStunt = 0x7000u;
constexpr std::uint32_t kTarget = 0x6000u;
constexpr std::uint32_t kOther = 0x6100u;

struct ProcessRuntime final : pal4::IStuntActionProcessRuntime {
    struct Binding {
        std::uint32_t effect;
        std::string key;
        std::uint32_t value;
    };
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 324u, kStunt}, {kTask + 320u, kTarget},
        {kStunt + 4u, 1u}};
    std::map<std::uint32_t, bool> ready;
    std::string name = "ordinary";
    std::uint32_t kind = 1u;
    bool needs_update = false;
    bool throw_on_bind = false;
    std::uint8_t completion = 255u;
    float scale = 0.0f;
    std::vector<std::uint32_t> candidates;
    std::vector<std::uint32_t> executed_targets;
    std::vector<std::uint32_t> released_effects;
    std::vector<Binding> bindings;
    std::map<std::string, std::uint32_t> config;
    std::vector<std::string> config_reads;
    struct ActorCall {
        std::uint32_t actor, slot, argument;
    };
    std::vector<ActorCall> actor_calls;
    std::map<std::uint32_t, std::uint32_t> actor_results;
    std::uint32_t event_actor = kOther;
    int animation_resets = 0;
    bool throw_on_config = false;
    std::size_t index = 0u;
    int empty_effects = 0;
    int prepares = 0;
    int iterator_releases = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == kTask + 332u, "completion uses +332");
        completion = value;
    }
    void WriteFloat(std::uint32_t address, float value) override {
        Check(address == kTask + 328u, "group scale uses +328");
        scale = value;
    }
    std::string_view StuntName(std::uint32_t address) override {
        Check(address == kStunt + 196u, "stunt name uses +196");
        return name;
    }
    std::uint32_t StuntTargetKind(std::uint32_t stunt) override {
        Check(stunt == kStunt, "kind uses task stunt");
        return kind;
    }
    bool ActorReady(std::uint32_t actor) override {
        return ready[actor];
    }
    std::uint32_t EmptyEffect() override {
        ++empty_effects;
        return 0xe000u;
    }
    std::uint32_t ExecuteEffect(std::uint32_t command,
                                std::uint32_t target,
                                std::uint32_t context) override {
        Check(command == kStunt + 176u && context == kStunt + 192u,
              "effect uses stunt command/context offsets");
        executed_targets.push_back(target);
        return 0xa000u +
               static_cast<std::uint32_t>(executed_targets.size());
    }
    bool NeedsUpdate(std::uint32_t) override { return needs_update; }
    void BindEffect(std::uint32_t effect, std::string_view key,
                    std::uint32_t value) override {
        bindings.push_back({effect, std::string(key), value});
        if (throw_on_bind) throw std::runtime_error("bind failure");
    }
    void ReleaseEffect(std::uint32_t effect) override {
        released_effects.push_back(effect);
    }
    std::uint32_t ConfigValue(std::uint32_t effect,
                              std::string_view key) override {
        Check(effect != 0u, "config lookup receives effect");
        config_reads.emplace_back(key);
        if (throw_on_config) throw std::runtime_error("config failure");
        return config[std::string(key)];
    }
    int ResetAnimationEffect(std::uint32_t effect) override {
        Check(effect != 0u, "animation reset receives effect");
        ++animation_resets;
        return 0x55;
    }
    std::uint32_t EventActor(std::uint32_t event) override {
        Check(event != 0u, "event actor receives event");
        return event_actor;
    }
    std::uint32_t CallActor(std::uint32_t actor, std::uint32_t slot,
                            std::uint32_t argument) override {
        actor_calls.push_back({actor, slot, argument});
        return actor_results[slot];
    }
    void PrepareImmediateStunt() override { ++prepares; }
    std::uint32_t TargetIterator(std::uint32_t target) override {
        Check(target == kTarget, "group iterator uses target");
        index = 0u;
        return 0x9000u;
    }
    bool IteratorEmpty(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "empty probe uses iterator");
        return candidates.empty();
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "end probe uses iterator");
        return index == candidates.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(iterator == 0x9000u && index < candidates.size(),
              "current uses valid iterator position");
        return candidates[index];
    }
    void IteratorNext(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "next uses iterator");
        ++index;
    }
    int ReleaseIterator(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "release uses iterator");
        ++iterator_releases;
        return 0;
    }
};

struct StuntRuntime final : pal4::IStuntTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 256u + 60u, 0x5000u},
        {kTask + 256u + 64u, kTarget},
        {kTask + 256u + 68u, kStunt}};
    std::vector<std::uint32_t> actor_property_targets;
    int group_scale_writes = 0;
    std::uint32_t includes_self = 0u;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteFloat(std::uint32_t address, float value) override {
        Check(address == kTask + 256u + 72u && value == 1.0f,
              "single stunt uses unit scale");
        ++group_scale_writes;
    }
    void SetStuntTarget(std::uint32_t, std::uint32_t,
                        std::uint32_t) override {}
    void AdjustStuntCost(std::uint32_t, std::uint32_t,
                         std::int32_t) override {}
    void AdjustActorValue(std::uint32_t, std::int32_t) override {}
    std::uint32_t ApplyActorEffect(std::uint32_t, std::uint32_t,
                                   std::int32_t) override { return 0u; }
    std::uint32_t StuntTargetKind(std::uint32_t) override { return 1u; }
    std::uint32_t GameDatabase() override { return 0u; }
    float FloatArrayElement(std::uint32_t, std::uint32_t) override {
        return 0.0f;
    }
    std::uint32_t TargetIterator(std::uint32_t) override { return 0u; }
    bool IteratorAtEnd(std::uint32_t) override { return true; }
    std::uint32_t IteratorCurrent(std::uint32_t) override { return 0u; }
    void IteratorNext(std::uint32_t) override {}
    std::uint32_t DestroyIterator(std::uint32_t) override { return 0u; }
    bool CandidateEligible(std::uint32_t) override { return false; }
    std::uint32_t ActorProperty(std::uint32_t actor,
                                std::uint32_t mode) override {
        Check(mode == 2u, "stunt actor property uses mode two");
        actor_property_targets.push_back(actor);
        return 0u;
    }
    std::uint32_t ActorReady(std::uint32_t) override { return 0u; }
    std::uint32_t StuntIncludesSelf(std::uint32_t) override {
        return includes_self;
    }
    void ApplyTargetHit(std::uint32_t, std::uint32_t,
                        std::uint32_t) override {}
    void NotifySource(std::uint32_t, std::uint32_t,
                      std::uint32_t) override {}
    std::uint32_t SetTargetState12(std::uint32_t,
                                   std::uint32_t) override { return 0u; }
    std::uint32_t SetTargetStateOne(std::uint32_t) override { return 0u; }
};

void HookArea() {
    ProcessRuntime pending;
    StuntRuntime stunt;
    pending.name = "HOOK_AREA";
    pending.needs_update = true;
    pal4::StuntActionTask::ProcessInternal(kTask, pending, stunt);
    Check(pending.executed_targets == std::vector<std::uint32_t>{kTarget} &&
          pending.empty_effects == 0 && pending.completion == 0u &&
          pending.bindings.size() == 2u &&
          pending.bindings[0].key == "cbCast" &&
          pending.bindings[0].value == kTask &&
          pending.bindings[1].key == "cbTarget" &&
          pending.bindings[1].value == kTarget &&
          pending.released_effects ==
              std::vector<std::uint32_t>{0xa001u} &&
          stunt.actor_property_targets.empty(),
          "HookArea pending effect binds task and target");

    ProcessRuntime immediate;
    immediate.name = "hook_area";
    pal4::StuntActionTask::ProcessInternal(kTask, immediate, stunt);
    Check(immediate.completion == 1u && immediate.prepares == 1 &&
          stunt.actor_property_targets ==
              std::vector<std::uint32_t>{kTarget} &&
          immediate.released_effects ==
              std::vector<std::uint32_t>{0xa001u},
          "HookArea immediate effect invokes embedded StuntTask");
}

void SingleTarget() {
    ProcessRuntime unready;
    StuntRuntime stunt;
    pal4::StuntActionTask::ProcessInternal(kTask, unready, stunt);
    Check(unready.empty_effects == 1 &&
          unready.executed_targets.empty() &&
          unready.released_effects ==
              std::vector<std::uint32_t>{0xe000u} &&
          unready.completion == 1u && unready.prepares == 1 &&
          stunt.actor_property_targets.size() == 1u,
          "unready direct target uses empty effect then immediate handling");

    ProcessRuntime ready;
    ready.ready[kTarget] = true;
    ready.needs_update = true;
    pal4::StuntActionTask::ProcessInternal(kTask, ready, stunt);
    Check(ready.empty_effects == 1 &&
          ready.executed_targets == std::vector<std::uint32_t>{kTarget} &&
          ready.released_effects ==
              std::vector<std::uint32_t>{0xe000u, 0xa001u} &&
          ready.bindings.size() == 2u && ready.completion == 0u,
          "ready direct target replaces empty effect and waits for callback");

    ProcessRuntime type_five;
    type_five.words[kStunt + 4u] = 5u;
    pal4::StuntActionTask::ProcessInternal(kTask, type_five, stunt);
    Check(type_five.executed_targets ==
              std::vector<std::uint32_t>{kTarget} &&
          type_five.completion == 1u,
          "type five bypasses actor readiness");
}

void GroupTargets() {
    ProcessRuntime runtime;
    StuntRuntime stunt;
    runtime.kind = 3u;
    runtime.candidates = {kOther, kTarget};
    runtime.ready[kTarget] = true;
    pal4::StuntActionTask::ProcessInternal(kTask, runtime, stunt);
    Check(runtime.executed_targets ==
              std::vector<std::uint32_t>{kTarget} &&
          runtime.scale == 1.0f && runtime.completion == 1u &&
          runtime.prepares == 1 && runtime.iterator_releases == 1 &&
          stunt.actor_property_targets ==
              std::vector<std::uint32_t>{kTarget},
          "group immediate branch filters candidates and sets unit scale");

    ProcessRuntime pending;
    pending.kind = 4u;
    pending.words[kStunt + 4u] = 5u;
    pending.candidates = {kTarget, kOther};
    pending.needs_update = true;
    pal4::StuntActionTask::ProcessInternal(kTask, pending, stunt);
    Check(pending.executed_targets ==
              std::vector<std::uint32_t>{kTarget, kOther} &&
          pending.bindings.size() == 4u && pending.completion == 0u &&
          pending.released_effects ==
              std::vector<std::uint32_t>{0xa001u, 0xa002u} &&
          pending.iterator_releases == 1,
          "group type-five pending effects bind both candidates");

    ProcessRuntime empty;
    empty.kind = 3u;
    pal4::StuntActionTask::ProcessInternal(kTask, empty, stunt);
    Check(empty.completion == 1u && empty.prepares >= 1 &&
          empty.iterator_releases == 1,
          "empty target group marks completion and releases iterator");
}

void ExceptionCleanup() {
    ProcessRuntime runtime;
    StuntRuntime stunt;
    runtime.kind = 3u;
    runtime.candidates = {kTarget};
    runtime.ready[kTarget] = true;
    runtime.needs_update = true;
    runtime.throw_on_bind = true;
    bool threw = false;
    try {
        pal4::StuntActionTask::ProcessInternal(kTask, runtime, stunt);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.released_effects ==
              std::vector<std::uint32_t>{0xa001u} &&
          runtime.iterator_releases == 1,
          "binding failure releases effect and target iterator");
}

void AnimationCallbacks() {
    constexpr std::uint32_t kEffect = 0xb000u;
    ProcessRuntime attached;
    StuntRuntime stunt;
    attached.name = "HOOK_AREA";
    attached.config["cbAttachCast"] = kTask;
    Check(pal4::StuntActionTask::HandleAnimationEvent(
              kTask, kEffect, "ATK", attached, stunt) == 0x55 &&
          attached.executed_targets ==
              std::vector<std::uint32_t>{kTarget} &&
          attached.config_reads ==
              std::vector<std::string>{"cbAttachCast", "cbCast"} &&
          attached.animation_resets == 1,
          "attached caster reruns full stunt process before reset");

    ProcessRuntime hook;
    hook.name = "hook_area";
    hook.config["cbCast"] = kTask;
    stunt.actor_property_targets.clear();
    Check(pal4::StuntActionTask::HandleAnimationEvent(
              kTask, kEffect, "atk", hook, stunt) == 0x55 &&
          hook.executed_targets.empty() &&
          stunt.actor_property_targets ==
              std::vector<std::uint32_t>{kTarget} &&
          hook.animation_resets == 1,
          "owned HookArea callback handles embedded stunt directly");

    ProcessRuntime single;
    single.config["cbCast"] = kTask;
    single.config["cbTarget"] = kOther;
    stunt.actor_property_targets.clear();
    Check(pal4::StuntActionTask::HandleAnimationEvent(
              kTask, kEffect, "atk", single, stunt) == 0x55 &&
          single.scale == 1.0f &&
          stunt.actor_property_targets ==
              std::vector<std::uint32_t>{kOther} &&
          single.config_reads == std::vector<std::string>{
              "cbAttachCast", "cbCast", "cbTarget"},
          "owned non-HookArea callback uses configured target at scale one");

    ProcessRuntime end;
    end.config["cbCast"] = kTask;
    Check(pal4::StuntActionTask::HandleAnimationEvent(
              kTask, kEffect, "EndAct", end, stunt) == 0x55 &&
          end.completion == 1u && end.prepares == 1 &&
          end.animation_resets == 1,
          "owned endact marks completion");
    ProcessRuntime foreign;
    Check(pal4::StuntActionTask::HandleAnimationEvent(
              kTask, kEffect, "endact", foreign, stunt) == 0x55 &&
          foreign.completion == 255u && foreign.animation_resets == 1,
          "foreign endact leaves completion unchanged");
    ProcessRuntime unrelated;
    Check(pal4::StuntActionTask::HandleAnimationEvent(
              kTask, kEffect, "idle", unrelated, stunt) == 0x55 &&
          unrelated.config_reads.empty() &&
          unrelated.animation_resets == 1,
          "unrelated animation still resets effect");
    ProcessRuntime failing;
    failing.throw_on_config = true;
    bool threw = false;
    try {
        (void)pal4::StuntActionTask::HandleAnimationEvent(
            kTask, kEffect, "atk", failing, stunt);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing.animation_resets == 1,
          "callback exception resets animation effect");
}

void CompletionAndActorCallbacks() {
    constexpr std::uint32_t kEffect = 0xb000u;
    constexpr std::uint32_t kEvent = 0xc000u;
    ProcessRuntime runtime;
    runtime.words[kTask + 60u] = 0x5000u;
    runtime.config["cbCast"] = kTask;
    pal4::StuntActionTask::HandleCompletionEvent(kTask, kEffect, runtime);
    Check(runtime.completion == 1u && runtime.prepares == 1,
          "owned completion effect marks task ready");
    runtime.completion = 0u;
    runtime.config["cbCast"] = 0u;
    pal4::StuntActionTask::HandleCompletionEvent(kTask, kEffect, runtime);
    Check(runtime.completion == 0u && runtime.prepares == 1,
          "foreign completion effect is ignored");

    runtime.words[kEvent + 4u] = 7u;
    runtime.words[kEvent + 8u] = 10u;
    runtime.words[kEvent + 12u] = 5u;
    runtime.actor_results[52u] = 3u;
    runtime.actor_results[340u] = 99u;
    Check(pal4::StuntActionTask::HandleActorEvent(
              kTask, kEvent, runtime) == 99u &&
          runtime.actor_calls.size() == 2u &&
          runtime.actor_calls[0].slot == 52u &&
          runtime.actor_calls[0].argument == 7u &&
          runtime.actor_calls[1].slot == 340u &&
          runtime.actor_calls[1].argument == 0x5000u,
          "negative actor comparison notifies source through slot 340");
    runtime.actor_calls.clear();
    runtime.actor_results[52u] = 15u;
    Check(pal4::StuntActionTask::HandleActorEvent(
              kTask, kEvent, runtime) == 5u &&
          runtime.actor_calls.size() == 1u,
          "nonnegative actor value skips notification");
    runtime.words[kEvent + 4u] = 6u;
    runtime.actor_calls.clear();
    Check(pal4::StuntActionTask::HandleActorEvent(
              kTask, kEvent, runtime) == 0x5000u &&
          runtime.actor_calls.empty(),
          "other event states leave actor untouched");
}

void CombatCallbacks() {
    constexpr std::uint32_t kEvent = 0xc000u;
    ProcessRuntime runtime;
    StuntRuntime stunt;
    runtime.words[kTask + 60u] = 0x5000u;
    runtime.words[kEvent + 4u] = 0u;
    stunt.includes_self = 0x100u;
    Check(pal4::StuntActionTask::HandleCombatEvent(
              kTask, kEvent, runtime, stunt) == 0x100u &&
          runtime.actor_calls.empty(),
          "combat callback rejects zero low byte of stunt predicate");
    stunt.includes_self = 1u;
    runtime.event_actor = 0x5000u;
    Check(pal4::StuntActionTask::HandleCombatEvent(
              kTask, kEvent, runtime, stunt) == 0x5000u &&
          runtime.actor_calls.empty(),
          "combat callback ignores source actor");
    runtime.event_actor = kOther;
    runtime.actor_results[136u] = 1u;
    runtime.actor_results[340u] = 77u;
    Check(pal4::StuntActionTask::HandleCombatEvent(
              kTask, kEvent, runtime, stunt) == 77u &&
          runtime.actor_calls.size() == 2u &&
          runtime.actor_calls[0].slot == 136u &&
          runtime.actor_calls[1].slot == 340u,
          "state zero uses slot 136 then notification");
    runtime.actor_calls.clear();
    runtime.words[kEvent + 4u] = 1u;
    runtime.actor_results[144u] = 0u;
    Check(pal4::StuntActionTask::HandleCombatEvent(
              kTask, kEvent, runtime, stunt) == 0u &&
          runtime.actor_calls.size() == 1u &&
          runtime.actor_calls[0].slot == 144u,
          "failed state-one slot skips notification");
    runtime.actor_calls.clear();
    runtime.words[kEvent + 4u] = 2u;
    Check(pal4::StuntActionTask::HandleCombatEvent(
              kTask, kEvent, runtime, stunt) == 2u &&
          runtime.actor_calls.empty(),
          "other combat states forward state unchanged");
}

// Exercise both production endpoints through one case-sensitive config store.
// Hand-populating callback keys previously hid the AMF writer/reader mismatch.
void AnimationEffectCallbackRoundtrip() {
    ProcessRuntime process;
    process.ready[kTarget] = true;
    struct Animation final : pal4::ICombatAnimationCommandRuntime {
        ProcessRuntime& process;
        std::uint32_t facet{};
        explicit Animation(ProcessRuntime& p):process(p){}
        std::uint32_t Actor(std::uint32_t) override{return 0x5000u;}
        std::uint32_t FindAnimationEntry(std::uint32_t,std::string_view) override{return 1u;}
        std::uint32_t ChildClump(std::uint32_t) override{return 1u;}
        std::string Attribute(std::uint32_t,std::string_view key) override{return key=="ef"?"skill_effect":"hook_ef_pos";}
        float FloatAttribute(std::uint32_t,std::string_view,float v) override{return v;}
        int IntAttribute(std::uint32_t,std::string_view,int v) override{return v;}
        bool EffectActive(pal4::CombatEffectHandle) override{return true;}
        bool SlotActive(std::uint32_t) override{return false;}
        void ReleaseSlot(std::uint32_t) override{}
        pal4::CombatEffectHandle CreateEffect(std::string_view) override{return {59u,1u};}
        void StoreSlot(std::uint32_t,pal4::CombatEffectHandle,std::string_view) override{}
        void SetCallback(pal4::CombatEffectHandle,std::string_view key,std::uint32_t task) override{process.config[std::string(key)]=task;}
        std::uint32_t TaskTarget(std::uint32_t) override{return kTarget;}
        void SetTarget(pal4::CombatEffectHandle,std::string_view key,std::uint32_t value) override{Check(key=="T1","original target facet name");facet=value;}
        void AttachActorTransform(pal4::CombatEffectHandle,std::uint32_t) override{}
        void ApplyBindingRotation(pal4::CombatEffectHandle) override{}
        std::uint32_t EffectFrame(pal4::CombatEffectHandle) override{return 1u;}
        void ProcessBinding(std::uint32_t,std::string_view,std::uint32_t) override{}
        void UpdateEffect(pal4::CombatEffectHandle) override{}
        void ResetTemporaryEffect(pal4::CombatEffectHandle) override{}
        void SetTimeScale(float) override{}
        void SetHandsActive(std::uint32_t,bool) override{}
        std::uint32_t CleanupCommand(std::string_view) override{return 0u;}
    } animation(process);
    StuntRuntime stunt;
    (void)pal4::CombatAnimationCommandProcessor::Process(kTask,"ef1",animation);
    Check(process.config.at("cbAttachCast")==kTask && animation.facet==kTarget+724u,
          "AMF creates original exact-case callback and target binding");
    (void)pal4::StuntActionTask::HandleAnimationEvent(kTask,59u,"ATK",process,stunt);
    Check(process.executed_targets==std::vector<std::uint32_t>{kTarget},
          "AMF attached effect ATK reaches actual shared cast dispatch");
}

}  // namespace

int main() {
    try {
        AnimationEffectCallbackRoundtrip();
        HookArea();
        SingleTarget();
        GroupTargets();
        ExceptionCleanup();
        AnimationCallbacks();
        CompletionAndActorCallbacks();
        CombatCallbacks();
        std::cout << "stunt_action_process_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
