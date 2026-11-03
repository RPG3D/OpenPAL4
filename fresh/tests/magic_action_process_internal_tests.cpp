#include "pal4/magic_action_task.h"

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
constexpr std::uint32_t kMagic = 0x7000u;
constexpr std::uint32_t kTarget = 0x6000u;

struct Runtime final : pal4::IMagicActionProcessRuntime,
                       pal4::ICastMagicTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{kTask + 324u, kMagic},
        {kTask + 320u, kTarget}, {kTask + 256u + 60u, 0x5000u},
        {kTask + 256u + 64u, kTarget}, {kTask + 256u + 68u, kMagic}};
    std::map<std::uint32_t, bool> ready;
    std::string name = "ordinary";
    std::uint32_t mode = 1u;
    bool needs_update = false;
    bool iterator_empty = false;
    bool throw_on_bind = false;
    std::vector<std::uint32_t> candidates;
    std::size_t index = 0;
    std::vector<std::string> order;
    std::vector<std::uint32_t> executed_targets;
    std::vector<std::uint32_t> released_effects;
    struct Binding { std::uint32_t effect, value; std::string key; };
    std::vector<Binding> bindings;
    std::uint8_t completion = 255u;
    float scale = 0.0f;
    int immediate_calls = 0;
    int property_checks = 0;
    int iterator_releases = 0;
    int empty_effects = 0;
    std::map<std::string, std::uint32_t> config;
    std::vector<std::string> config_reads;
    int animation_resets = 0;
    std::uint32_t last_property_actor = 0;
    std::uint32_t event_actor = 0;
    int self_rejection = 0;
    int first_actor_result = 1;
    struct ActorCall { std::uint32_t actor, slot, first; };
    std::vector<ActorCall> actor_calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::string_view MagicName(std::uint32_t address) override {
        Check(address == kMagic + 140u, "magic name offset");
        return name;
    }
    std::uint32_t MagicMode(std::uint32_t magic) override {
        Check(magic == kMagic, "magic mode object");
        return mode;
    }
    bool ActorReady(std::uint32_t actor) override {
        order.emplace_back("ready");
        return ready[actor];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == kTask + 332u, "completion byte offset");
        completion = value;
        order.emplace_back(value ? "complete" : "reset");
    }
    void WriteFloat(std::uint32_t address, float value) override {
        Check(address == kTask + 328u, "group scale offset");
        scale = value;
        order.emplace_back("scale");
    }
    std::uint32_t EmptyEffect() override {
        ++empty_effects;
        order.emplace_back("empty");
        return 0x8000u;
    }
    std::uint32_t ExecuteEffect(std::uint32_t command,
                                std::uint32_t target,
                                std::uint32_t context) override {
        Check(command == kMagic + 120u && context == kMagic + 136u,
              "effect command and context offsets");
        executed_targets.push_back(target);
        order.emplace_back("execute");
        return 0x9000u + static_cast<std::uint32_t>(executed_targets.size());
    }
    bool NeedsUpdate(std::uint32_t effect) override {
        Check(effect != 0u, "effect handle");
        order.emplace_back("needs_update");
        return needs_update;
    }
    void BindEffect(std::uint32_t effect, std::string_view key,
                    std::uint32_t value) override {
        bindings.push_back({effect, value, std::string(key)});
        order.emplace_back("bind");
        if (throw_on_bind) throw std::runtime_error("bind failed");
    }
    void ReleaseEffect(std::uint32_t effect) override {
        released_effects.push_back(effect);
        order.emplace_back("release_effect");
    }
    std::uint32_t ConfigValue(std::uint32_t effect,
                              std::string_view key) override {
        Check(effect == 0xb000u, "animation effect handle");
        config_reads.emplace_back(key);
        return config[std::string(key)];
    }
    int ResetAnimationEffect(std::uint32_t effect) override {
        Check(effect == 0xb000u, "animation reset handle");
        ++animation_resets;
        order.emplace_back("reset_animation");
        return -17;
    }
    std::uint32_t EventActor(std::uint32_t event) override {
        Check(event == 0xc000u, "combat event object");
        return event_actor;
    }
    void PrepareImmediateCast() override {
        ++immediate_calls;
        order.emplace_back("prepare");
    }
    std::uint32_t TargetIterator(std::uint32_t target) override {
        Check(target == kTarget, "group target source");
        order.emplace_back("iterator");
        return 0xa000u;
    }
    bool IteratorEmpty(std::uint32_t iterator) override {
        Check(iterator == 0xa000u, "group iterator identity");
        return iterator_empty;
    }
    bool IteratorAtEnd(std::uint32_t) override {
        return index == candidates.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t) override {
        return candidates.at(index);
    }
    void IteratorNext(std::uint32_t) override { ++index; }
    int ReleaseIterator(std::uint32_t iterator) override {
        Check(iterator == 0xa000u, "released iterator identity");
        ++iterator_releases;
        order.emplace_back("release_iterator");
        return 0;
    }

    void WriteScale(std::uint32_t, float) override {}
    void SetActorTarget(std::uint32_t, std::uint32_t,
                        std::uint32_t) override {}
    int ChangeActorValue(std::uint32_t, std::uint32_t,
                         std::int32_t) override { return 0; }
    float DamageScale(std::uint32_t) override { return 1.0f; }
    bool ActorEligible(std::uint32_t) override { return false; }
    int ActorProperty3(std::uint32_t actor) override {
        ++property_checks;
        last_property_actor = actor;
        order.emplace_back("cast_actor");
        return 0;
    }
    int MagicRejectsSelf(std::uint32_t magic) override {
        Check(magic == kMagic, "self rejection magic");
        return self_rejection;
    }
    float ReadFloat(std::uint32_t) override { return 0.0f; }
    bool CheckDodge(std::uint32_t, std::uint32_t, float) override {
        return false;
    }
    int CallActor(std::uint32_t actor, std::uint32_t slot,
                  std::uint32_t first, std::uint32_t) override {
        actor_calls.push_back({actor, slot, first});
        return slot == 340u ? 340 : first_actor_result;
    }
};

void HookArea() {
    Runtime runtime;
    runtime.name = "HoOk_ArEa";
    runtime.needs_update = true;
    pal4::MagicActionTask::ProcessInternal(kTask, runtime, runtime);
    Check(runtime.completion == 0u && runtime.empty_effects == 0 &&
          runtime.executed_targets == std::vector<std::uint32_t>{kTarget} &&
          runtime.bindings.size() == 2u &&
          runtime.bindings[0].key == "cbCast" &&
          runtime.bindings[0].value == kTask &&
          runtime.bindings[1].key == "cbTarget" &&
          runtime.bindings[1].value == kTarget &&
          runtime.released_effects ==
              std::vector<std::uint32_t>{0x9001u} &&
          runtime.immediate_calls == 0,
          "hook-area effect binds cast and target then releases handle");
    Runtime immediate;
    immediate.name = "hook_area";
    pal4::MagicActionTask::ProcessInternal(kTask, immediate, immediate);
    Check(immediate.completion == 1u && immediate.immediate_calls == 1 &&
          immediate.property_checks == 1 && immediate.bindings.empty() &&
          immediate.released_effects.size() == 1u,
          "hook-area without pending effect resolves cast immediately");
}

void Direct() {
    Runtime unready;
    pal4::MagicActionTask::ProcessInternal(kTask, unready, unready);
    Check(unready.empty_effects == 1 && unready.executed_targets.empty() &&
          unready.released_effects ==
              std::vector<std::uint32_t>{0x8000u} &&
          unready.completion == 1u && unready.property_checks == 1,
          "unready direct target uses empty effect and immediate cast");
    Runtime pending;
    pending.ready[kTarget] = true;
    pending.needs_update = true;
    pal4::MagicActionTask::ProcessInternal(kTask, pending, pending);
    Check(pending.empty_effects == 1 &&
          pending.executed_targets == std::vector<std::uint32_t>{kTarget} &&
          pending.released_effects ==
              std::vector<std::uint32_t>{0x8000u, 0x9001u} &&
          pending.bindings.size() == 2u && pending.completion == 0u,
          "ready direct target transfers to active effect and binds callbacks");
    Runtime type_five;
    type_five.words[kMagic + 8u] = 5u;
    pal4::MagicActionTask::ProcessInternal(kTask, type_five, type_five);
    Check(type_five.executed_targets.size() == 1u,
          "type five executes even when target is not ready");
}

void Group() {
    Runtime runtime;
    runtime.mode = 3u;
    runtime.candidates = {0x6100u, 0x6200u};
    runtime.ready[0x6100u] = true;
    pal4::MagicActionTask::ProcessInternal(kTask, runtime, runtime);
    Check(runtime.executed_targets ==
              std::vector<std::uint32_t>{0x6100u} &&
          runtime.completion == 1u && runtime.scale == 1.0f &&
          runtime.property_checks == 1 && runtime.iterator_releases == 1 &&
          runtime.released_effects ==
              std::vector<std::uint32_t>{0x9001u},
          "group immediate branch filters actors, scales and releases");
    Runtime pending;
    pending.mode = 4u;
    pending.words[kMagic + 8u] = 5u;
    pending.candidates = {0x6100u};
    pending.needs_update = true;
    pal4::MagicActionTask::ProcessInternal(kTask, pending, pending);
    Check(pending.bindings.size() == 2u &&
          pending.bindings[1].value == 0x6100u &&
          pending.completion == 0u && pending.iterator_releases == 1,
          "group type-five candidate binds pending effect");
    Runtime empty;
    empty.mode = 3u;
    empty.iterator_empty = true;
    pal4::MagicActionTask::ProcessInternal(kTask, empty, empty);
    Check(empty.immediate_calls == 2 && empty.completion == 1u &&
          empty.iterator_releases == 1,
          "empty group triggers both native completion gates");
    Runtime filtered;
    filtered.mode = 4u;
    filtered.candidates = {0x6200u};
    pal4::MagicActionTask::ProcessInternal(kTask, filtered, filtered);
    Check(filtered.immediate_calls == 1 &&
          filtered.executed_targets.empty() && filtered.iterator_releases == 1,
          "group without eligible candidate completes once");
    Runtime throwing;
    throwing.mode = 3u;
    throwing.candidates = {0x6100u};
    throwing.ready[0x6100u] = true;
    throwing.needs_update = true;
    throwing.throw_on_bind = true;
    try {
        pal4::MagicActionTask::ProcessInternal(kTask, throwing, throwing);
        throw std::runtime_error("missing bind exception");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "bind failed",
              "binding error propagated");
    }
    Check(throwing.released_effects.size() == 1u &&
          throwing.iterator_releases == 1,
          "effect and iterator release on group exception");
}

void AnimationEvents() {
    Runtime unrelated;
    Check(pal4::MagicActionTask::HandleAnimationEvent(
              kTask, 0xb000u, "other", unrelated, unrelated) == -17 &&
          unrelated.config_reads.empty() && unrelated.animation_resets == 1,
          "unrelated message only resets animation effect");

    Runtime attached;
    attached.config["cbAttachCast"] = kTask;
    attached.config["cbCast"] = kTask;
    Check(pal4::MagicActionTask::HandleAnimationEvent(
              kTask, 0xb000u, "AtK", attached, attached) == -17 &&
          attached.config_reads == std::vector<std::string>{
              "cbAttachCast", "cbCast"} &&
          attached.completion == 1u && attached.property_checks == 1 &&
          attached.animation_resets == 1,
          "attached atk takes full process path before caster branch");

    Runtime caster_hook;
    caster_hook.config["cbCast"] = kTask;
    caster_hook.name = "HOOK_AREA";
    Check(pal4::MagicActionTask::HandleAnimationEvent(
              kTask, 0xb000u, "atk", caster_hook, caster_hook) == -17 &&
          caster_hook.property_checks == 1 &&
          caster_hook.config_reads.size() == 2u &&
          caster_hook.completion == 255u,
          "hook-area caster callback handles target without completing task");

    Runtime caster_direct;
    caster_direct.config["cbCast"] = kTask;
    caster_direct.config["cbTarget"] = 0x6100u;
    Check(pal4::MagicActionTask::HandleAnimationEvent(
              kTask, 0xb000u, "aTk", caster_direct, caster_direct) == -17 &&
          caster_direct.config_reads == std::vector<std::string>{
              "cbAttachCast", "cbCast", "cbTarget"} &&
          caster_direct.scale == 1.0f &&
          caster_direct.last_property_actor == 0x6100u &&
          caster_direct.completion == 255u,
          "direct caster callback uses configured target and scale one");

    Runtime end;
    end.config["cbCast"] = kTask;
    Check(pal4::MagicActionTask::HandleAnimationEvent(
              kTask, 0xb000u, "EnDaCt", end, end) == -17 &&
          end.config_reads == std::vector<std::string>{
              "cbAttachCast", "cbCast"} &&
          end.immediate_calls == 1 && end.completion == 1u &&
          end.animation_resets == 1,
          "endact matching caster marks task complete");
    Runtime end_other;
    Check(pal4::MagicActionTask::HandleAnimationEvent(
              kTask, 0xb000u, "endact", end_other, end_other) == -17 &&
          end_other.completion == 255u && end_other.animation_resets == 1,
          "unowned endact leaves completion unchanged");

    Runtime failing;
    failing.config["cbAttachCast"] = kTask;
    failing.name = "hook_area";
    failing.needs_update = true;
    failing.throw_on_bind = true;
    try {
        (void)pal4::MagicActionTask::HandleAnimationEvent(
            kTask, 0xb000u, "atk", failing, failing);
        throw std::runtime_error("missing animation failure");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "bind failed",
              "animation handler propagates effect failure");
    }
    Check(failing.released_effects.size() == 1u &&
          failing.animation_resets == 1,
          "animation effect resets during callback exception");
}

void RegisteredCallbacks() {
    Runtime completion;
    pal4::MagicActionTask::HandleCompletionEvent(
        kTask, 0xb000u, completion);
    Check(completion.config_reads ==
              std::vector<std::string>{"cbCast"} &&
          completion.completion == 255u && completion.immediate_calls == 0,
          "foreign completion event is ignored");
    completion.config["cbCast"] = kTask;
    pal4::MagicActionTask::HandleCompletionEvent(
        kTask, 0xb000u, completion);
    Check(completion.completion == 1u && completion.immediate_calls == 1,
          "owned completion event marks task done");

    Runtime combat;
    combat.words[kTask + 60u] = 0x5000u;
    combat.event_actor = 0x6100u;
    combat.words[0xc000u + 4u] = 0u;
    combat.self_rejection = 0x100;
    Check(pal4::MagicActionTask::HandleCombatEvent(
              kTask, 0xc000u, combat, combat) == 0x100 &&
          combat.actor_calls.empty(),
          "false low byte returns full magic predicate result");
    combat.self_rejection = 1;
    combat.event_actor = 0x5000u;
    Check(pal4::MagicActionTask::HandleCombatEvent(
              kTask, 0xc000u, combat, combat) == 0x5000 &&
          combat.actor_calls.empty(), "self event returns caster word");
    combat.event_actor = 0x6100u;
    combat.words[0xc000u + 4u] = 2u;
    Check(pal4::MagicActionTask::HandleCombatEvent(
              kTask, 0xc000u, combat, combat) == 2 &&
          combat.actor_calls.empty(), "unsupported state returns state");
    combat.words[0xc000u + 4u] = 0u;
    combat.first_actor_result = 0;
    Check(pal4::MagicActionTask::HandleCombatEvent(
              kTask, 0xc000u, combat, combat) == 0 &&
          combat.actor_calls.size() == 1u &&
          combat.actor_calls.back().slot == 136u,
          "state zero false predicate skips caster notification");
    combat.first_actor_result = 1;
    combat.actor_calls.clear();
    Check(pal4::MagicActionTask::HandleCombatEvent(
              kTask, 0xc000u, combat, combat) == 340 &&
          combat.actor_calls.size() == 2u &&
          combat.actor_calls[0].actor == 0x6100u &&
          combat.actor_calls[0].slot == 136u &&
          combat.actor_calls[1].slot == 340u &&
          combat.actor_calls[1].first == 0x5000u,
          "state zero success notifies actor with caster");
    combat.actor_calls.clear();
    combat.words[0xc000u + 4u] = 1u;
    Check(pal4::MagicActionTask::HandleCombatEvent(
              kTask, 0xc000u, combat, combat) == 340 &&
          combat.actor_calls[0].slot == 144u &&
          combat.actor_calls[1].slot == 340u,
          "state one uses actor slot 144 before notification");
}

}  // namespace

int main() {
    try {
        HookArea();
        Direct();
        Group();
        AnimationEvents();
        RegisteredCallbacks();
        std::cout << "magic_action_process_internal_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
