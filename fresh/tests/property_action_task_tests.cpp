#include "pal4/property_action_task.h"

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

struct Events final : pal4::IEventSubscriptionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x8b8b64u, 11u}, {0x8b5940u, 22u}};
    std::vector<std::array<std::uint32_t, 5>> registrations;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    std::uint32_t EventKey(std::string_view) override { return 0u; }
    std::uint32_t EventRouter() override { return 0x9000u; }
    std::uint32_t Register(std::uint32_t router, std::uint32_t mode,
                           std::uint32_t key, std::uint32_t owner,
                           std::uint32_t callback) override {
        registrations.push_back({router, mode, key, owner, callback});
        calls.emplace_back("register");
        return 0xa000u + static_cast<std::uint32_t>(registrations.size());
    }
    std::uint32_t FindEventGroup(std::uint32_t,
                                 std::uint32_t) override {
        calls.emplace_back("find");
        return 0u;
    }
    void ReleaseOwnedSlot(std::uint32_t) override {}
    void FreeNode(std::uint32_t) override {}
};

struct Runtime final : pal4::IPropertyActionTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{kTask + 60u, kSource},
                                                 {kTask + 304u, kTarget}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> candidates;
    std::vector<std::uint32_t> changed;
    std::size_t index = 0u;
    bool pending = true;
    bool throw_on_ready = false;
    int iterator_destroys = 0;
    std::string cleaned;
    std::string forwarded;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write_vtable");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kTask + 320u, "status reads completion byte");
        calls.emplace_back("read_status");
        return static_cast<std::uint8_t>(words[address]);
    }
    void ResetEffect(std::uint32_t address) override {
        calls.push_back("reset:" + std::to_string(address));
    }
    void DestroyEmbedded(std::uint32_t address) override {
        calls.push_back("destroy_embedded:" + std::to_string(address));
    }
    void DestroyBase(std::uint32_t address) override {
        Check(address == kTask, "base destructor receives task");
        calls.emplace_back("destroy_base");
    }
    void StartAction(std::uint32_t address) override {
        Check(address == kTask, "start receives task");
        calls.emplace_back("start");
    }
    void StopAction(std::uint32_t address,
                    std::uint32_t frame) override {
        Check(address == kTask && frame == 123u,
              "stop receives task and frame");
        calls.emplace_back("stop");
    }
    bool IsActionPending(std::uint32_t address) override {
        Check(address == kTask, "pending receives task");
        return pending;
    }
    void ResetEmbedded(std::uint32_t address) override {
        Check(address == kTask + 240u,
              "stop resets embedded property task");
        calls.emplace_back("reset_embedded");
    }
    void ProcessAnimationCommand(std::uint32_t address,
                                 std::string_view name) override {
        Check(address == kTask, "fallback receives task");
        forwarded = name;
    }
    int CleanupCommand(std::string_view name) override {
        cleaned = name;
        calls.emplace_back("cleanup_command");
        return 17;
    }
    std::uint32_t TargetIterator(std::uint32_t target) override {
        Check(target == kTarget, "atk scans action target");
        index = 0u;
        return 0x8000u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x8000u, "iterator end handle");
        return index == candidates.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(iterator == 0x8000u, "iterator current handle");
        return candidates.at(index);
    }
    void IteratorNext(std::uint32_t iterator) override {
        Check(iterator == 0x8000u, "iterator next handle");
        ++index;
    }
    void DestroyIterator(std::uint32_t iterator) override {
        Check(iterator == 0x8000u, "iterator destroy handle");
        ++iterator_destroys;
    }
    bool ActorReady(std::uint32_t actor) override {
        if (throw_on_ready) throw std::runtime_error("ready failure");
        return actor != 0x6100u;
    }
    void ActorSetState(std::uint32_t actor,
                       std::uint32_t state) override {
        Check(state == 1u, "atk sets state one");
        changed.push_back(actor);
    }
};

struct Property final : pal4::IUsePropertyTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 240u + 60u, kSource},
        {kTask + 240u + 64u, kTarget},
        {kTask + 240u + 68u, kProperty},
        {kProperty, 42u}, {kProperty + 148u, 6u},
        {kSource + 804u, 1u}, {kTarget + 804u, 1u}};
    int uses = 0;
    int applies = 0;
    std::uint32_t ReadWord(std::uint32_t a) override { return words[a]; }
    void WriteWord(std::uint32_t a, std::uint32_t v) override {
        words[a] = v;
    }
    void UseProperty(std::uint32_t s, std::uint32_t p,
                     std::uint32_t t) override {
        Check(s == kSource && p == kProperty && t == kTarget,
              "atk composes embedded property task");
        ++uses;
    }
    void ConsumeProperty(std::uint32_t, std::uint32_t type,
                         std::int32_t amount) override {
        Check(type == 42u && amount == -1,
              "embedded task consumes one property");
    }
    std::uint32_t TargetProperty(std::uint32_t, std::uint32_t) override {
        return 1u;
    }
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
    void NotifyTarget(std::uint32_t,
                      std::uint32_t) override {}
    bool NeedsUpdate(std::uint32_t) override { return false; }
};

void SubscriptionsAndLifecycle() {
    Events events;
    Runtime runtime;
    Check(pal4::PropertyActionTask::Process(kTask, runtime, events) == 1u &&
          events.registrations ==
              std::vector<std::array<std::uint32_t, 5>>{
                  {0x9000u, 2u, 11u, kTask, 0x55da30u},
                  {0x9000u, 2u, 22u, kTask, 0x55d880u}} &&
          events.words[kTask + 216u] == 0xa001u &&
          events.words[kTask + 220u] == 11u &&
          events.words[kTask + 224u] == 0xa002u &&
          events.words[kTask + 228u] == 22u &&
          runtime.calls.back() == "start",
          "process subscribes two callbacks before starting");
    runtime.words[kTask + 320u] = 1u;
    Check(pal4::PropertyActionTask::CheckStatus(kTask, runtime),
          "pending action with completion byte reports ready");
    runtime.pending = false;
    const auto before = runtime.calls.size();
    Check(!pal4::PropertyActionTask::CheckStatus(kTask, runtime) &&
          runtime.calls.size() == before,
          "status short-circuits completion byte when not pending");
    events.words[kTask + 216u] = 0u;
    events.words[kTask + 224u] = 0u;
    Check(pal4::PropertyActionTask::Stop(kTask, 123u, runtime, events) == 1u &&
          runtime.calls[runtime.calls.size() - 2u] == "stop" &&
          runtime.calls.back() == "reset_embedded",
          "stop clears, forwards frame and resets embedded task");
    runtime.calls.clear();
    pal4::PropertyActionTask::Destroy(kTask, runtime, events);
    Check(runtime.words[kTask] == pal4::PropertyActionTask::kVtable &&
          runtime.calls == std::vector<std::string>{
              "write_vtable", "reset:16696", "destroy_embedded:16624",
              "reset:16616", "destroy_base"},
          "destructor resets both effect pairs around embedded destruction");
    runtime.calls.clear();
    pal4::PropertyActionTask::CleanupEmbedded(kTask + 240u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "reset:16696", "destroy_embedded:16624"},
          "embedded cleanup resets handle before base destruction");
}

void CommandRoutingAndCleanup() {
    Runtime runtime;
    Property property;
    runtime.candidates = {kSource, 0x6100u, kTarget};
    Check(pal4::PropertyActionTask::ProcessCommand(
              kTask, "ATK_extra", runtime, property) == 17 &&
          property.uses == 1 && property.applies == 1 &&
          runtime.changed == std::vector<std::uint32_t>{kTarget} &&
          runtime.iterator_destroys == 1 &&
          runtime.cleaned == "atk_extra",
          "atk composes property use and sets eligible non-source actor state");
    Check(pal4::PropertyActionTask::ProcessCommand(
              kTask, "At", runtime, property) == 17 &&
          runtime.forwarded == "At" && runtime.cleaned == "at" &&
          property.uses == 1,
          "short command uses animation fallback and cleans lowercase copy");
    Runtime failing;
    failing.candidates = {kTarget};
    failing.throw_on_ready = true;
    bool threw = false;
    try {
        (void)pal4::PropertyActionTask::ProcessCommand(
            kTask, "atk", failing, property);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing.iterator_destroys == 1 &&
          failing.cleaned == "atk",
          "exception releases iterator and temporary command string");
}

struct EventRuntime final : pal4::IPropertyActionEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 304u, kTarget}, {0x8000u, 31u},
        {0x8004u, 32u}};
    std::string name = "other";
    std::uint32_t owner = kTarget;
    int copies = 0;
    int resets = 0;
    int preparations = 0;
    bool throw_on_name = false;
    bool throw_on_prepare = false;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write_word");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == kTask + 320u,
              "callback writes task completion byte");
        words[address] = value;
        calls.emplace_back("write_byte");
    }
    pal4::PropertyTaskEffectHandle CopyEffectPair(
        std::uint32_t address) override {
        Check(address == 0x8004u, "event helper copies handle at +4");
        ++copies;
        calls.emplace_back("copy");
        return {31u, 32u};
    }
    std::string_view EffectName(
        const pal4::PropertyTaskEffectHandle& effect) override {
        Check(effect.first == 31u && effect.second == 32u,
              "name query uses copied event effect");
        if (throw_on_name) throw std::runtime_error("name failure");
        calls.emplace_back("name");
        return name;
    }
    int ResetEffectPair(
        pal4::PropertyTaskEffectHandle& effect) override {
        Check(effect.first == 31u && effect.second == 32u,
              "reset receives copied effect");
        ++resets;
        calls.emplace_back("reset");
        return 41;
    }
    std::uint32_t EffectOwner(std::uint32_t event) override {
        Check(event == 0x8000u, "owner query uses original event");
        calls.emplace_back("owner");
        return owner;
    }
    std::uint32_t PrepareEffectCompletion(
        const pal4::PropertyTaskEffectHandle& effect) override {
        Check(effect.first == 31u && effect.second == 32u,
              "completion receives matching effect");
        if (throw_on_prepare)
            throw std::runtime_error("completion failure");
        ++preparations;
        calls.emplace_back("prepare");
        return 43u;
    }
};

void EffectCallbacks() {
    EventRuntime runtime;
    auto copied = pal4::PropertyActionTask::CopyEventEffect(0x8000u,
                                                            runtime);
    Check(copied.first == 31u && copied.second == 32u &&
          runtime.copies == 1, "copy helper reads event effect pair");

    runtime.name = "eff13";
    Check(pal4::PropertyActionTask::HandleEffectEvent(
              kTask, 0x8000u, runtime) == 41u &&
          runtime.copies == 2 && runtime.resets == 1,
          "EFF13 returns after first effect reset");
    runtime = EventRuntime{};
    runtime.name = "EfF14";
    Check(pal4::PropertyActionTask::HandleEffectEvent(
              kTask, 0x8000u, runtime) == 41u &&
          runtime.copies == 2 && runtime.resets == 2,
          "EFF14 returns after second name query");
    runtime = EventRuntime{};
    runtime.owner = 0x9999u;
    Check(pal4::PropertyActionTask::HandleEffectEvent(
              kTask, 0x8000u, runtime) == 0x9999u &&
          runtime.copies == 2 && runtime.resets == 2 &&
          runtime.words[kTask + 232u] == 0u,
          "foreign effect does not become pending action effect");
    runtime = EventRuntime{};
    runtime.words[kTask + 320u] = 1u;
    Check(pal4::PropertyActionTask::HandleEffectEvent(
              kTask, 0x8000u, runtime) == 41u &&
          runtime.copies == 3 && runtime.resets == 3 &&
          runtime.words[kTask + 232u] == 31u &&
          runtime.words[kTask + 236u] == 32u &&
          runtime.words[kTask + 320u] == 0u,
          "owned non-filtered effect becomes pending pair");

    runtime.words[0x8000u] = 30u;
    Check(pal4::PropertyActionTask::HandleCompletionEvent(
              kTask, 0x8000u, runtime) == 30u &&
          runtime.preparations == 0,
          "completion ignores first-word mismatch");
    runtime.words[0x8000u] = 31u;
    runtime.words[0x8004u] = 33u;
    Check(pal4::PropertyActionTask::HandleCompletionEvent(
              kTask, 0x8000u, runtime) == 32u &&
          runtime.preparations == 0,
          "completion ignores second-word mismatch");
    runtime.words[0x8004u] = 32u;
    Check(pal4::PropertyActionTask::HandleCompletionEvent(
              kTask, 0x8000u, runtime) == 43u &&
          runtime.preparations == 1 &&
          runtime.words[kTask + 320u] == 1u,
          "matching effect completion sets task flag");

    runtime.words[kTask + 320u] = 0u;
    Check(pal4::PropertyActionTask::HandleAnimationEvent(
              kTask, {31u, 32u}, "ENDAct", runtime) == 41 &&
          runtime.preparations == 2 && runtime.resets == 4 &&
          runtime.words[kTask + 320u] == 1u,
          "owned endact completes and resets animation effect");
    runtime.words[kTask + 320u] = 0u;
    Check(pal4::PropertyActionTask::HandleAnimationEvent(
              kTask, {31u, 32u}, "other", runtime) == 41 &&
          runtime.preparations == 2 &&
          runtime.words[kTask + 320u] == 0u,
          "other animation event only resets effect");
    runtime.throw_on_prepare = true;
    bool threw = false;
    const auto before = runtime.resets;
    try {
        (void)pal4::PropertyActionTask::HandleAnimationEvent(
            kTask, {31u, 32u}, "endact", runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.resets == before + 1,
          "animation callback releases effect on preparation failure");

    runtime = EventRuntime{};
    runtime.throw_on_name = true;
    threw = false;
    try {
        (void)pal4::PropertyActionTask::HandleEffectEvent(
            kTask, 0x8000u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.resets == 1,
          "effect callback releases copied handle on name failure");
}

}  // namespace

int main() {
    try {
        SubscriptionsAndLifecycle();
        CommandRoutingAndCleanup();
        EffectCallbacks();
        std::cout << "property_action_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
