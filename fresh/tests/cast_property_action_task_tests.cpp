#include "pal4/cast_property_action_task.h"

#include <array>
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
constexpr std::uint32_t kSource = 0x5000u;
constexpr std::uint32_t kTarget = 0x6000u;
constexpr std::uint32_t kProperty = 0x7000u;
constexpr std::uint32_t kEvent = 0x8000u;

struct Events final : pal4::IEventSubscriptionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x8b8b64u, 11u}, {0x8b5940u, 22u}};
    std::vector<std::array<std::uint32_t, 5>> registrations;
    std::uint32_t ReadWord(std::uint32_t a) override { return words[a]; }
    void WriteWord(std::uint32_t a, std::uint32_t v) override {
        words[a] = v;
    }
    std::uint32_t EventKey(std::string_view) override { return 0u; }
    std::uint32_t EventRouter() override { return 0x9000u; }
    std::uint32_t Register(std::uint32_t r, std::uint32_t m,
                           std::uint32_t k, std::uint32_t o,
                           std::uint32_t c) override {
        registrations.push_back({r, m, k, o, c});
        return 0xa000u + static_cast<std::uint32_t>(registrations.size());
    }
    std::uint32_t FindEventGroup(std::uint32_t,
                                 std::uint32_t) override { return 0u; }
    void ReleaseOwnedSlot(std::uint32_t) override {}
    void FreeNode(std::uint32_t) override {}
};

struct Property final : pal4::IUsePropertyTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 232u + 60u, kSource},
        {kTask + 232u + 64u, kTarget},
        {kTask + 232u + 68u, kProperty},
        {kProperty, 42u}, {kProperty + 148u, 7u},
        {kSource + 804u, 1u}, {kTarget + 804u, 1u}};
    int uses = 0;
    int status_calls = 0;
    bool needs_update = false;
    std::uint32_t ReadWord(std::uint32_t a) override { return words[a]; }
    void WriteWord(std::uint32_t a, std::uint32_t v) override {
        words[a] = v;
    }
    void UseProperty(std::uint32_t s, std::uint32_t p,
                     std::uint32_t t) override {
        Check(s == kSource && p == kProperty && t == kTarget,
              "elapsed cast composes embedded property use");
        ++uses;
    }
    void ConsumeProperty(std::uint32_t, std::uint32_t type,
                         std::int32_t amount) override {
        Check(type == 42u && amount == -1,
              "cast consumes one property");
    }
    std::uint32_t TargetProperty(std::uint32_t,
                                 std::uint32_t) override { return 1u; }
    std::uint32_t PropertyKind(std::uint32_t) override { return 1u; }
    std::uint32_t TargetIterator(std::uint32_t) override { return 0u; }
    bool IteratorAtEnd(std::uint32_t) override { return true; }
    std::uint32_t IteratorCurrent(std::uint32_t) override { return 0u; }
    void IteratorNext(std::uint32_t) override {}
    void DestroyIterator(std::uint32_t) override {}
    bool CandidateEligible(std::uint32_t) override { return true; }
    void ApplyToTarget(std::uint32_t, std::uint32_t,
                       std::uint32_t) override {}
    void NotifySource(std::uint32_t, std::uint32_t,
                      std::uint32_t) override {}
    std::array<float, 3> ActorScale(std::uint32_t) override {
        return {1.0f, 1.0f, 1.0f};
    }
    pal4::PropertyTaskEffectHandle CreateChestEffect(
        std::uint32_t, std::string_view,
        std::string_view) override { return {}; }
    void ResetTemporaryEffect(
        pal4::PropertyTaskEffectHandle&) override {}
    void SetEffectScale(std::uint32_t,
                        const std::array<float, 3>&,
                        std::uint8_t) override {}
    void NotifyTarget(std::uint32_t, std::uint32_t) override {}
    bool NeedsUpdate(std::uint32_t address) override {
        Check(address == kTask + 304u,
              "status checks effect at embedded +72");
        ++status_calls;
        return needs_update;
    }
};

struct Action final : pal4::ICastPropertyActionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 60u, kSource}, {kTask + 296u, kTarget},
        {kTask + 300u, 0xb000u}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, float> floats{{0xb000u + 164u, 1.0f}};
    std::vector<std::string> calls;
    std::uint32_t effect_actor = 0u;
    int effects = 0;
    int resets = 0;
    bool pending = false;
    bool fail_animation = false;
    std::string forwarded;
    std::string cleaned;
    std::uint32_t ReadWord(std::uint32_t a) override { return words[a]; }
    void WriteWord(std::uint32_t a, std::uint32_t v) override {
        words[a] = v;
        calls.push_back("word:" + std::to_string(a));
    }
    std::uint8_t ReadByte(std::uint32_t a) override { return bytes[a]; }
    void WriteByte(std::uint32_t a, std::uint8_t v) override {
        bytes[a] = v;
        calls.push_back("byte:" + std::to_string(a));
    }
    float ReadFloat(std::uint32_t a) override { return floats[a]; }
    void WriteFloat(std::uint32_t a, float v) override {
        floats[a] = v;
        calls.push_back("float:" + std::to_string(a));
    }
    void ResetEffect(std::uint32_t a) override {
        calls.push_back("reset:" + std::to_string(a));
    }
    void DestroyEmbedded(std::uint32_t a) override {
        calls.push_back("embedded:" + std::to_string(a));
    }
    void DestroyBase(std::uint32_t a) override {
        Check(a == kTask, "destructor receives action base");
        calls.emplace_back("base");
    }
    void StartAction(std::uint32_t a) override {
        Check(a == kTask, "start receives action task");
        calls.emplace_back("start");
    }
    void StopAction(std::uint32_t a, std::uint32_t frame) override {
        Check(a == kTask && frame == 123u,
              "stop receives task and frame");
        calls.emplace_back("stop");
    }
    void ResetEmbedded(std::uint32_t a) override {
        Check(a == kTask + 232u, "stop resets embedded property");
        calls.emplace_back("reset_embedded");
    }
    bool IsActionPending(std::uint32_t a) override {
        Check(a == kTask, "pending receives action task");
        return pending;
    }
    void ActorSetState(std::uint32_t actor,
                       std::uint32_t state) override {
        Check(actor == kSource && state == 1u,
              "pending cast sets source state one");
        calls.emplace_back("actor_state");
    }
    pal4::PropertyTaskEffectHandle CreateActorEffect(
        std::uint32_t actor, std::string_view name,
        std::uint32_t hook) override {
        Check(actor == kTarget && name == "EFF15" &&
              hook == pal4::CastPropertyActionTask::kEffectHook,
              "atk creates EFF15 on cast target and hook");
        effect_actor = actor;
        ++effects;
        return {31u, 32u};
    }
    void ResetTemporaryEffect(
        pal4::PropertyTaskEffectHandle& effect) override {
        Check(effect.first == 31u && effect.second == 32u,
              "atk resets temporary effect pair");
        ++resets;
    }
    void ProcessAnimationCommand(std::uint32_t a,
                                 std::string_view name) override {
        Check(a == kTask, "fallback receives action task");
        forwarded = name;
        if (fail_animation)
            throw std::runtime_error("animation failure");
    }
    int CleanupCommand(std::string_view name) override {
        cleaned = name;
        return 17;
    }
};

struct Effect final : pal4::IPropertyActionEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 296u, kTarget}, {kEvent, 31u},
        {kEvent + 4u, 32u}};
    std::uint32_t owner = kTarget;
    std::string name = "other";
    int copies = 0;
    int resets = 0;
    int completions = 0;
    std::uint32_t ReadWord(std::uint32_t a) override { return words[a]; }
    void WriteWord(std::uint32_t a, std::uint32_t v) override {
        words[a] = v;
    }
    void WriteByte(std::uint32_t a, std::uint8_t v) override {
        words[a] = v;
    }
    pal4::PropertyTaskEffectHandle CopyEffectPair(
        std::uint32_t a) override {
        Check(a == kEvent + 4u, "copy reads event +4");
        ++copies;
        return {31u, 32u};
    }
    std::string_view EffectName(
        const pal4::PropertyTaskEffectHandle&) override {
        return name;
    }
    int ResetEffectPair(pal4::PropertyTaskEffectHandle&) override {
        ++resets;
        return 41;
    }
    std::uint32_t EffectOwner(std::uint32_t event) override {
        Check(event == kEvent, "owner receives event");
        return owner;
    }
    std::uint32_t PrepareEffectCompletion(
        const pal4::PropertyTaskEffectHandle& effect) override {
        Check(effect.first == 31u && effect.second == 32u,
              "completion receives effect pair");
        ++completions;
        return 43u;
    }
};

void LifecycleAndCommand() {
    Action action;
    Events events;
    Check(pal4::CastPropertyActionTask::Start(
              kTask, action, events) == 1u &&
          events.registrations ==
              std::vector<std::array<std::uint32_t, 5>>{
                  {0x9000u, 2u, 11u, kTask, 0x55f050u},
                  {0x9000u, 2u, 22u, kTask, 0x55eec0u}} &&
          action.words[kTask + 320u] == 0u &&
          action.bytes[kTask + 324u] == 0u &&
          action.bytes[kTask + 325u] == 0u &&
          action.calls.back() == "start",
          "start registers callbacks and resets cast state");
    Check(pal4::CastPropertyActionTask::ProcessCommand(
              kTask, "ATK_extra", action) == 17 &&
          action.bytes[kTask + 325u] == 1u &&
          action.effects == 1 && action.resets == 1 &&
          action.cleaned == "atk_extra",
          "atk creates EFF15 and marks casting");
    Check(pal4::CastPropertyActionTask::ProcessCommand(
              kTask, "At", action) == 17 &&
          action.forwarded == "At" && action.cleaned == "at",
          "short command uses animation fallback");
    action.fail_animation = true;
    bool threw = false;
    try {
        (void)pal4::CastPropertyActionTask::ProcessCommand(
            kTask, "other", action);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && action.cleaned == "other",
          "command string is cleaned on exception");
    events.words[kTask + 216u] = 0u;
    events.words[kTask + 224u] = 0u;
    action.calls.clear();
    Check(pal4::CastPropertyActionTask::Stop(
              kTask, 123u, action, events) == 1u &&
          action.calls ==
              std::vector<std::string>{"stop", "reset_embedded"},
          "stop clears subscriptions, forwards frame and resets property");
    action.calls.clear();
    pal4::CastPropertyActionTask::Destroy(kTask, action, events);
    Check(action.words[kTask] ==
              pal4::CastPropertyActionTask::kVtable &&
          action.calls == std::vector<std::string>{
              "word:16384", "reset:16696", "reset:16688",
              "embedded:16616", "base"},
          "destructor resets both effects before property and base");
}

void TimingAndStatus() {
    Action action;
    Property property;
    action.pending = true;
    Check(pal4::CastPropertyActionTask::Update(
              kTask, 0.5f, action, property) == 1u &&
          action.calls.back() == "actor_state" && property.uses == 0,
          "pending action sets source actor state before casting");
    action.bytes[kTask + 325u] = 1u;
    Check(pal4::CastPropertyActionTask::Update(
              kTask, 0.5f, action, property) == 1u &&
          action.floats[kTask + 320u] == 0.5f &&
          property.uses == 0,
          "casting accumulates elapsed below duration");
    Check(pal4::CastPropertyActionTask::Update(
              kTask, 0.5f, action, property) == 1u &&
          action.floats[kTask + 320u] == 1.0f &&
          property.uses == 0,
          "elapsed equal to duration does not apply property");
    Check(pal4::CastPropertyActionTask::Update(
              kTask, 0.25f, action, property) == 1u &&
          action.floats[kTask + 320u] == 1.25f &&
          property.uses == 1 &&
          action.bytes[kTask + 324u] == 1u,
          "elapsed above duration applies property once");
    (void)pal4::CastPropertyActionTask::Update(
        kTask, 0.25f, action, property);
    Check(property.uses == 1,
          "completed cast skips repeated property use");
    action.bytes[kTask + 324u] = 0u;
    Check(!pal4::CastPropertyActionTask::CheckStatus(
              kTask, action, property) && property.status_calls == 0,
          "status short-circuits before property if not applied");
    action.bytes[kTask + 324u] = 1u;
    property.needs_update = true;
    Check(!pal4::CastPropertyActionTask::CheckStatus(
              kTask, action, property) && property.status_calls == 1,
          "pending property effect blocks completion");
    property.needs_update = false;
    action.bytes[kTask + 326u] = 1u;
    Check(pal4::CastPropertyActionTask::CheckStatus(
              kTask, action, property),
          "property and event completion satisfy status");
}

void EffectCallbacks() {
    Effect effect;
    effect.name = "EFF13";
    Check(pal4::CastPropertyActionTask::HandleEffectEvent(
              kTask, kEvent, effect) == 41u &&
          effect.copies == 1 && effect.resets == 1,
          "EFF13 is filtered");
    effect = Effect{};
    effect.name = "eff14";
    Check(pal4::CastPropertyActionTask::HandleEffectEvent(
              kTask, kEvent, effect) == 41u &&
          effect.copies == 2,
          "EFF14 is filtered after second lookup");
    effect = Effect{};
    effect.words[kTask + 326u] = 1u;
    Check(pal4::CastPropertyActionTask::HandleEffectEvent(
              kTask, kEvent, effect) == 41u &&
          effect.words[kTask + 312u] == 31u &&
          effect.words[kTask + 316u] == 32u &&
          effect.words[kTask + 326u] == 0u,
          "owned effect transfers to cast pair at +312");
    Check(pal4::CastPropertyActionTask::HandleCompletionEvent(
              kTask, kEvent, effect) == 43u &&
          effect.words[kTask + 326u] == 1u,
          "completion matches cast effect pair");
    effect.words[kTask + 326u] = 0u;
    Check(pal4::CastPropertyActionTask::HandleAnimationEvent(
              kTask, {31u, 32u}, "ENDAct", effect) == 41 &&
          effect.words[kTask + 326u] == 1u &&
          effect.completions == 2,
          "endact completes and resets cast effect");
}

}  // namespace

int main() {
    try {
        LifecycleAndCommand();
        TimingAndStatus();
        EffectCallbacks();
        std::cout << "cast_property_action_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
