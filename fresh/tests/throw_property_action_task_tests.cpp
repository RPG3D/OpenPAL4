#include "pal4/throw_property_action_task.h"

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

struct ThrowRuntime final : pal4::IThrowPropertyTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 240u + 60u, kSource},
        {kTask + 240u + 64u, kTarget},
        {kTask + 240u + 68u, kProperty}};
    std::vector<std::string>* trace = nullptr;
    std::vector<std::string> calls;
    int effects = 0;
    int moves = 0;
    std::uint32_t ReadWord(std::uint32_t a) override { return words[a]; }
    void WriteWord(std::uint32_t a, std::uint32_t v) override {
        words[a] = v;
    }
    float ReadFloat(std::uint32_t) override { return 0.0f; }
    void WriteFloat(std::uint32_t, float) override {}
    void ConstructBase(std::uint32_t) override {}
    void DestroyBase(std::uint32_t) override {
        if (trace) trace->emplace_back("throw_base");
    }
    void FreeTask(std::uint32_t) override {}
    pal4::PropertyTaskEffectHandle CreateActorEffect(
        std::uint32_t actor, std::string_view name,
        std::uint32_t hook) override {
        Check(actor == kSource && name == "EFF13" &&
              hook == pal4::ThrowPropertyTask::kEffectHook,
              "action composes throw effect creation");
        ++effects;
        return {1u, 2u};
    }
    void ResetEffect(pal4::PropertyTaskEffectHandle&) override {}
    void LoadScripts() override {}
    std::uint32_t CreateComponent() override { return 0xb000u; }
    std::string_view PropertyName(std::uint32_t,
                                  std::uint32_t offset) override {
        return offset == 84u ? "model" : "texture";
    }
    void LoadComponentClump(std::uint32_t, std::string_view,
                            std::string_view) override {}
    void SetComponentTexture(std::uint32_t,
                             std::string_view) override {}
    pal4::ThrowPropertyPosition ActorPosition(
        std::uint32_t) override { return {0.0f, 0.0f, 0.0f}; }
    float ActorBoundingHeight(std::uint32_t) override { return 0.0f; }
    void SetComponentPosition(
        std::uint32_t, const pal4::ThrowPropertyPosition&) override {
        ++moves;
    }
    float VectorLength(
        const pal4::ThrowPropertyPosition&) override { return 0.0f; }
    pal4::ThrowPropertyPosition Normalize(
        const pal4::ThrowPropertyPosition&) override {
        return {0.0f, 0.0f, 0.0f};
    }
    void CleanupScripts() override {}
    void DeleteComponent(std::uint32_t) override {
        if (trace) trace->emplace_back("throw_component");
    }
};

struct PropertyRuntime final : pal4::IUsePropertyTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 328u + 60u, kSource},
        {kTask + 328u + 64u, kTarget},
        {kTask + 328u + 68u, kProperty},
        {kProperty, 0x31u}, {kProperty + 148u, 7u},
        {kSource + 804u, 1u}, {kTarget + 804u, 1u}};
    int uses = 0;
    int applies = 0;
    bool needs_update = false;
    int status_calls = 0;
    std::uint32_t ReadWord(std::uint32_t a) override { return words[a]; }
    void WriteWord(std::uint32_t a, std::uint32_t v) override {
        words[a] = v;
    }
    void UseProperty(std::uint32_t s, std::uint32_t p,
                     std::uint32_t t) override {
        Check(s == kSource && p == kProperty && t == kTarget,
              "completed throw composes embedded property task");
        ++uses;
    }
    void ConsumeProperty(std::uint32_t, std::uint32_t type,
                         std::int32_t amount) override {
        Check(type == 0x31u && amount == -1,
              "completed throw consumes one property");
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
                       std::uint32_t) override { ++applies; }
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
        Check(address == kTask + 400u,
              "status checks embedded property effect at +400");
        ++status_calls;
        return needs_update;
    }
};

struct ActionRuntime final : pal4::IThrowPropertyActionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{kTask + 60u, kSource}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::string>* trace = nullptr;
    bool pending = false;
    bool throw_complete = false;
    bool fail_animation = false;
    int elapsed_calls = 0;
    int completion_checks = 0;
    std::string forwarded;
    std::string cleaned;
    std::uint32_t ReadWord(std::uint32_t a) override { return words[a]; }
    void WriteWord(std::uint32_t a, std::uint32_t v) override {
        words[a] = v;
        calls.emplace_back("write_vtable");
    }
    std::uint8_t ReadByte(std::uint32_t a) override { return bytes[a]; }
    void WriteByte(std::uint32_t a, std::uint8_t v) override {
        bytes[a] = v;
        calls.push_back("byte:" + std::to_string(a));
    }
    void ResetEffect(std::uint32_t address) override {
        if (trace) trace->push_back("effect:" + std::to_string(address));
    }
    void DestroyEmbeddedProperty(std::uint32_t address) override {
        Check(address == kTask + 328u,
              "destructor destroys embedded property base");
        if (trace) trace->emplace_back("property_base");
    }
    void DestroyBase(std::uint32_t address) override {
        Check(address == kTask, "destructor receives action base");
        if (trace) trace->emplace_back("action_base");
    }
    void StartAction(std::uint32_t address) override {
        Check(address == kTask, "start receives action task");
        calls.emplace_back("start");
    }
    bool IsActionPending(std::uint32_t address) override {
        Check(address == kTask, "pending query receives action task");
        return pending;
    }
    void ActorSetState(std::uint32_t actor,
                       std::uint32_t state) override {
        Check(actor == kSource && state == 1u,
              "pending action sets source actor state one");
        calls.emplace_back("actor_state");
    }
    void AccumulateElapsed(float delta) override {
        Check(delta == 0.25f, "update forwards delta to elapsed service");
        ++elapsed_calls;
        calls.emplace_back("elapsed");
    }
    bool ThrowComplete(std::uint32_t embedded) override {
        Check(embedded == kTask + 240u,
              "completion query receives embedded throw");
        ++completion_checks;
        return throw_complete;
    }
    void ProcessAnimationCommand(std::uint32_t task,
                                 std::string_view name) override {
        Check(task == kTask, "fallback receives action task");
        forwarded = name;
        if (fail_animation)
            throw std::runtime_error("animation failure");
    }
    int CleanupCommand(std::string_view name) override {
        cleaned = name;
        return 17;
    }
};

struct EffectRuntime final : pal4::IPropertyActionEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 392u, kTarget}, {kEvent, 31u},
        {kEvent + 4u, 32u}};
    std::string name = "other";
    std::uint32_t owner = kTarget;
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
        Check(a == kEvent + 4u, "copy reads event effect at +4");
        ++copies;
        return {31u, 32u};
    }
    std::string_view EffectName(
        const pal4::PropertyTaskEffectHandle&) override {
        return name;
    }
    int ResetEffectPair(
        pal4::PropertyTaskEffectHandle&) override {
        ++resets;
        return 41;
    }
    std::uint32_t EffectOwner(std::uint32_t a) override {
        Check(a == kEvent, "owner query receives event");
        return owner;
    }
    std::uint32_t PrepareEffectCompletion(
        const pal4::PropertyTaskEffectHandle& effect) override {
        Check(effect.first == 31u && effect.second == 32u,
              "completion receives matching effect pair");
        ++completions;
        return 43u;
    }
};

void LifecycleAndCommands() {
    Events events;
    ActionRuntime action;
    ThrowRuntime throwing;
    Check(pal4::ThrowPropertyActionTask::Start(
              kTask, action, events) == 1u &&
          events.registrations ==
              std::vector<std::array<std::uint32_t, 5>>{
                  {0x9000u, 2u, 11u, kTask, 0x55e7b0u},
                  {0x9000u, 2u, 22u, kTask, 0x55e620u}} &&
          events.words[kTask + 216u] == 0xa001u &&
          events.words[kTask + 224u] == 0xa002u &&
          action.bytes[kTask + 408u] == 0u &&
          action.bytes[kTask + 409u] == 0u &&
          action.calls.back() == "start",
          "start registers two callbacks and resets progress flags");
    Check(pal4::ThrowPropertyActionTask::ProcessCommand(
              kTask, "ATK_extra", action, throwing) == 17 &&
          throwing.effects == 1 &&
          action.bytes[kTask + 409u] == 1u &&
          action.cleaned == "atk_extra",
          "atk starts embedded throw once and marks it launched");
    Check(pal4::ThrowPropertyActionTask::ProcessCommand(
              kTask, "atk", action, throwing) == 17 &&
          throwing.effects == 1,
          "repeated atk does not restart throw");
    Check(pal4::ThrowPropertyActionTask::ProcessCommand(
              kTask, "At", action, throwing) == 17 &&
          action.forwarded == "At" && action.cleaned == "at",
          "short command falls back and cleans lowercase copy");
    action.fail_animation = true;
    bool threw = false;
    try {
        (void)pal4::ThrowPropertyActionTask::ProcessCommand(
            kTask, "other", action, throwing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && action.cleaned == "other",
          "command string is cleaned when fallback throws");

    std::vector<std::string> trace;
    action.trace = &trace;
    throwing.trace = &trace;
    events.words[kTask + 216u] = 0u;
    events.words[kTask + 224u] = 0u;
    pal4::ThrowPropertyActionTask::Destroy(
        kTask, action, events, throwing);
    Check(action.words[kTask] ==
              pal4::ThrowPropertyActionTask::kVtable &&
          trace == std::vector<std::string>{
              "effect:16784", "property_base", "throw_component",
              "throw_base", "effect:16616", "action_base"},
          "destructor releases effect, property, throw and base in order");
}

void UpdateAndStatus() {
    ActionRuntime action;
    ThrowRuntime throwing;
    PropertyRuntime property;
    action.pending = true;
    Check(pal4::ThrowPropertyActionTask::Update(
              kTask, 0.25f, action, throwing, property) == 1u &&
          action.calls.back() == "actor_state" &&
          action.elapsed_calls == 0 && property.uses == 0,
          "pending action sets source state before launch");
    action.bytes[kTask + 409u] = 1u;
    Check(pal4::ThrowPropertyActionTask::Update(
              kTask, 0.25f, action, throwing, property) == 1u &&
          action.elapsed_calls == 1 &&
          action.completion_checks == 1 && property.uses == 0 &&
          action.bytes[kTask + 408u] == 0u,
          "active throw updates position and elapsed until complete");
    action.throw_complete = true;
    Check(pal4::ThrowPropertyActionTask::Update(
              kTask, 0.25f, action, throwing, property) == 1u &&
          property.uses == 1 && property.applies == 1 &&
          action.bytes[kTask + 408u] == 1u,
          "completed throw applies embedded property once");
    Check(pal4::ThrowPropertyActionTask::Update(
              kTask, 0.25f, action, throwing, property) == 1u &&
          property.uses == 1,
          "completed action skips repeated property use");
    action.bytes[kTask + 408u] = 0u;
    Check(!pal4::ThrowPropertyActionTask::CheckStatus(
              kTask, action, property) && property.status_calls == 0,
          "status short-circuits before embedded effect when not applied");
    action.bytes[kTask + 408u] = 1u;
    property.needs_update = true;
    Check(!pal4::ThrowPropertyActionTask::CheckStatus(
              kTask, action, property) && property.status_calls == 1,
          "pending embedded effect blocks completion");
    property.needs_update = false;
    action.bytes[kTask + 410u] = 1u;
    Check(pal4::ThrowPropertyActionTask::CheckStatus(
              kTask, action, property),
          "applied property and completed event report ready");
}

void EffectCallbacks() {
    EffectRuntime effect;
    effect.name = "eFf13";
    Check(pal4::ThrowPropertyActionTask::HandleEffectEvent(
              kTask, kEvent, effect) == 41u &&
          effect.copies == 1 && effect.resets == 1,
          "EFF13 is filtered before owner lookup");
    effect = EffectRuntime{};
    effect.name = "EFF14";
    Check(pal4::ThrowPropertyActionTask::HandleEffectEvent(
              kTask, kEvent, effect) == 41u &&
          effect.copies == 2 && effect.resets == 2,
          "EFF14 is filtered after second name lookup");
    effect = EffectRuntime{};
    effect.owner = 0x9999u;
    Check(pal4::ThrowPropertyActionTask::HandleEffectEvent(
              kTask, kEvent, effect) == 0x9999u &&
          effect.words[kTask + 232u] == 0u,
          "foreign owner does not transfer effect");
    effect = EffectRuntime{};
    effect.words[kTask + 410u] = 1u;
    Check(pal4::ThrowPropertyActionTask::HandleEffectEvent(
              kTask, kEvent, effect) == 41u &&
          effect.words[kTask + 232u] == 31u &&
          effect.words[kTask + 236u] == 32u &&
          effect.words[kTask + 410u] == 0u &&
          effect.copies == 3,
          "owned effect transfers pair and clears +410 completion");
    Check(pal4::ThrowPropertyActionTask::HandleCompletionEvent(
              kTask, kEvent, effect) == 43u &&
          effect.words[kTask + 410u] == 1u &&
          effect.completions == 1,
          "matching completion event sets +410");
    effect.words[kTask + 410u] = 0u;
    Check(pal4::ThrowPropertyActionTask::HandleAnimationEvent(
              kTask, {31u, 32u}, "ENDAct", effect) == 41 &&
          effect.words[kTask + 410u] == 1u &&
          effect.completions == 2,
          "owned endact completes and resets effect");
}

}  // namespace

int main() {
    try {
        LifecycleAndCommands();
        UpdateAndStatus();
        EffectCallbacks();
        std::cout << "throw_property_action_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
