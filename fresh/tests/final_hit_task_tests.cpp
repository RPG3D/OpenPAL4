#include "pal4/final_hit_task.h"

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
constexpr std::uint32_t kActor = 0x5000u;

struct Events final : pal4::IEventSubscriptionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {pal4::FinalHitTaskEvents::kEventKey, 0x9000u}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::uint32_t group = 0u;
    std::uint32_t registered_owner = 0u;
    std::uint32_t registered_callback = 0u;
    std::uint32_t registered_key = 0u;
    int registrations = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t EventKey(std::string_view) override {
        throw std::runtime_error("IDA reads fixed key global");
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return 0x9100u;
    }
    std::uint32_t Register(std::uint32_t router, std::uint32_t mode,
                           std::uint32_t key, std::uint32_t owner,
                           std::uint32_t callback) override {
        Check(router == 0x9100u && mode == 2u,
              "final-hit registration uses router mode two");
        registered_owner = owner;
        registered_callback = callback;
        registered_key = key;
        ++registrations;
        calls.emplace_back("register");
        return 0xa000u;
    }
    std::uint32_t FindEventGroup(std::uint32_t router,
                                 std::uint32_t key) override {
        Check(router == 0x9100u && key == 0x9000u,
              "final-hit removal locates event key");
        calls.emplace_back("find_group");
        return group;
    }
    void ReleaseOwnedSlot(std::uint32_t slot) override {
        Check(slot == 0xb208u, "release node payload slot");
        calls.emplace_back("release");
    }
    void FreeNode(std::uint32_t node) override {
        freed.push_back(node);
        calls.emplace_back("free");
    }

    void InstallRegisteredNode() {
        group = 0xb000u;
        words[group + 4u] = 0xb100u;
        words[group + 8u] = 1u;
        words[0xb100u] = 0xb200u;
        words[0xb100u + 4u] = 0xb200u;
        words[0xb200u] = 0xb100u;
        words[0xb200u + 4u] = 0xb100u;
        words[0xb200u + 12u] = 0xa000u;
    }
};

struct Runtime final : pal4::IFinalHitTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{kTask + 76u, kActor}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::string name = "FinalHitEffect";
    bool needs_update = false;
    bool throw_on_properties = false;
    std::uint32_t text = 0xc000u;
    std::uint32_t property_source = 0u;
    std::uint32_t property_text = 0u;
    std::uint32_t updated_position = 0u;
    int temporary_resets = 0;
    int owned_resets = 0;
    int text_releases = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write_word");
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("write_byte");
        bytes[address] = value;
    }
    std::string_view ReadText(std::uint32_t address) override {
        Check(address == kTask + 64u, "effect name uses task text");
        calls.emplace_back("name");
        return name;
    }
    std::uint32_t CombatSystem() override {
        calls.emplace_back("combat");
        return 0xd000u;
    }
    std::uint32_t CombatCamera(std::uint32_t combat) override {
        Check(combat == 0xd000u, "camera receives combat singleton");
        calls.emplace_back("camera");
        return 0xd100u;
    }
    pal4::DeadEffectHandle CreateEffect(
        std::uint32_t camera, std::string_view effect_name,
        std::uint32_t position, std::uint32_t mode,
        std::uint32_t flags) override {
        Check(camera == 0xd100u && effect_name == name &&
              position == pal4::FinalHitTask::kEffectPosition &&
              mode == 1u && flags == 0u,
              "final-hit effect creation arguments");
        calls.emplace_back("create_effect");
        return {0xe000u, 0xe100u};
    }
    void ResetTemporaryEffect(pal4::DeadEffectHandle& effect) override {
        Check(effect.first == 0xe000u && effect.second == 0xe100u,
              "temporary handle retains returned pair");
        calls.emplace_back("reset_temporary");
        ++temporary_resets;
    }
    void ResetOwnedEffect(std::uint32_t address) override {
        Check(address == kTask + 80u, "destructor resets embedded effect");
        calls.emplace_back("reset_owned");
        ++owned_resets;
    }
    std::uint32_t CreateText(std::string_view value) override {
        Check(value == "T1", "effect source label is T1");
        calls.emplace_back("create_text");
        return text;
    }
    void ReleaseText(std::uint32_t value) override {
        Check(value == text, "release temporary T1 text");
        calls.emplace_back("release_text");
        ++text_releases;
    }
    void SetProperties(std::uint32_t effect,
                       std::uint32_t value,
                       std::uint32_t source) override {
        Check(effect == kTask + 80u, "properties use embedded handle");
        calls.emplace_back("properties");
        property_text = value;
        property_source = source;
        if (throw_on_properties)
            throw std::runtime_error("property failure");
    }
    std::uint32_t ChildPosition(std::uint32_t actor) override {
        Check(actor == kActor, "position uses task actor");
        calls.emplace_back("child_position");
        return 0xf000u;
    }
    void UpdatePosition(std::uint32_t effect,
                        std::uint32_t position,
                        std::uint8_t mode) override {
        Check(effect == kTask + 80u && mode == 0u,
              "position update uses embedded handle and mode zero");
        calls.emplace_back("position");
        updated_position = position;
    }
    void FinalizeEffect(std::uint32_t effect) override {
        Check(effect == kTask + 80u, "finalize uses embedded handle");
        calls.emplace_back("finalize");
    }
    bool NeedsUpdate(std::uint32_t effect) override {
        Check(effect == kTask + 80u, "poll uses embedded handle");
        calls.emplace_back("needs_update");
        return needs_update;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed.push_back(address);
    }
    std::uint32_t DestroyBase(std::uint32_t task) override {
        Check(task == kTask, "base destructor receives task");
        calls.emplace_back("destroy_base");
        return 0x12345678u;
    }
};

void EventSubscription() {
    Events events;
    Check(pal4::FinalHitTaskEvents::Subscribe(kTask, events) == 0xa000u &&
          events.words[kTask + 92u] == 0xa000u &&
          events.words[kTask + 96u] == 0x9000u &&
          events.registered_owner == kTask &&
          events.registered_callback == pal4::FinalHitTaskEvents::kCallback &&
          events.registered_key == 0x9000u,
          "subscription stores owner, callback, handle and key");
    events.InstallRegisteredNode();
    pal4::FinalHitTaskEvents::Clear(kTask, events);
    Check(events.words[0xb100u] == 0xb100u &&
          events.words[0xb100u + 4u] == 0xb100u &&
          events.words[0xb000u + 8u] == 0u &&
          events.words[kTask + 92u] == 0u &&
          events.words[kTask + 96u] == 0u &&
          events.freed == std::vector<std::uint32_t>{0xb200u},
          "unsubscribe unlinks node and clears task slots");
    pal4::FinalHitTaskEvents::Clear(kTask, events);
    Check(events.freed.size() == 1u, "unsubscribe is idempotent");
}

void ProcessAndCompletion() {
    for (const bool pending : {false, true}) {
        Events events;
        Runtime runtime;
        runtime.needs_update = pending;
        Check(pal4::FinalHitTask::Process(kTask, runtime, events) == 1u &&
              events.registrations == 1 &&
              runtime.words[kTask + 80u] == 0xe000u &&
              runtime.words[kTask + 84u] == 0xe100u &&
              runtime.temporary_resets == 1 &&
              runtime.property_text == runtime.text &&
              runtime.property_source == kActor + 724u &&
              runtime.text_releases == 1 &&
              runtime.updated_position == 0xf000u &&
              runtime.bytes[kTask + 88u] == (pending ? 0u : 1u),
              "process subscribes, creates effect, binds actor and checks completion");
        Check(runtime.calls[0] == "name" &&
              runtime.calls[1] == "combat" &&
              runtime.calls.back() == (pending ? "needs_update" : "write_byte"),
              "effect processing keeps native call order");
        Check(pal4::FinalHitTask::CheckStatus(kTask, runtime) ==
                  (pending ? 0u : 1u),
              "status returns completion byte");
        runtime.words[0x6000u] = 0xe000u;
        runtime.words[0x6004u] = 0xe100u;
        Check(pal4::FinalHitTask::HandleEvent(kTask, 0x6000u, runtime) ==
                  0x6000u && runtime.bytes[kTask + 88u] == 1u,
              "matching event completes effect");
        runtime.bytes[kTask + 88u] = 0u;
        runtime.words[0x6004u] = 0xe101u;
        Check(pal4::FinalHitTask::HandleEvent(kTask, 0x6000u, runtime) ==
                  0x6000u && runtime.bytes[kTask + 88u] == 0u,
              "nonmatching effect does not complete");
        Check(pal4::FinalHitTask::Stop(kTask, events) == 1u &&
              events.words[kTask + 92u] == 0u,
              "stop clears event handle");
    }
    Runtime standalone;
    standalone.needs_update = true;
    Check(pal4::FinalHitTask::ProcessEffect(kTask, standalone) == 1u &&
          standalone.words[kTask + 80u] == 0xe000u &&
          standalone.words[kTask + 84u] == 0xe100u &&
          standalone.temporary_resets == 1 && standalone.text_releases == 1 &&
          standalone.bytes[kTask + 88u] == 0u,
          "effect sequence supports externally owned subscription");
}

void TextFailureAndDestroy() {
    Events events;
    Runtime failing;
    failing.throw_on_properties = true;
    bool threw = false;
    try {
        (void)pal4::FinalHitTask::Process(kTask, failing, events);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing.temporary_resets == 1 &&
          failing.text_releases == 1 &&
          failing.calls.back() == "release_text",
          "property failure releases temporary text");

    Runtime runtime;
    runtime.words[kTask + 64u] = 0x7001u;
    runtime.words[kTask + 68u] = 7u;
    runtime.words[kTask + 72u] = 8u;
    runtime.bytes[0x7000u] = 2u;
    Check(pal4::FinalHitTask::Destroy(kTask, runtime, events) ==
              0x12345678u &&
          runtime.words[kTask] == pal4::FinalHitTask::kVtable &&
          runtime.owned_resets == 1 &&
          runtime.bytes[0x7000u] == 1u && runtime.freed.empty() &&
          runtime.words[kTask + 64u] == 0u &&
          runtime.words[kTask + 68u] == 0u &&
          runtime.words[kTask + 72u] == 0u &&
          runtime.calls.back() == "destroy_base",
          "destructor clears owned effect and decrements shared name");

    Runtime sole;
    sole.words[kTask + 64u] = 0x7101u;
    sole.bytes[0x7100u] = 0u;
    (void)pal4::FinalHitTask::Destroy(kTask, sole, events);
    Check(sole.freed == std::vector<std::uint32_t>{0x7100u},
          "destructor frees uniquely owned name");
}

}  // namespace

int main() {
    try {
        EventSubscription();
        ProcessAndCompletion();
        TextFailureAndDestroy();
        std::cout << "final_hit_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
