#include "pal4/actor_manager_property_state.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t old_property = 0x2000u;
constexpr std::uint32_t new_property = 0x3000u;

struct Runtime final : pal4::IActorManagerPropertyStateRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> ids;
    std::vector<std::string> trace;
    pal4::ActorPropertyHandle notified_previous;
    pal4::ActorPropertyHandle notified_incoming;
    bool restore_owner_during_notification = false;
    bool blocked = false;
    std::uint32_t action_result = 42u;
    std::uint32_t actor_mode = 4u;
    std::uint32_t made_variant = 99u;
    bool revive_status = false;

    Runtime() {
        bytes[actor + 796u] = 1u;
        words[actor + 800u] = old_property;
        words[actor + 828u] = 0u;
        words[actor + 808u] = 0u;
        ids[old_property] = 7u;
        ids[new_property] = 8u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        trace.push_back("byte:" + std::to_string(address) + ":" +
                        std::to_string(value));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        trace.push_back("word:" + std::to_string(address) + ":" +
                        std::to_string(value));
    }
    void NotifyPropertyChange(std::uint32_t subject,
                              pal4::ActorPropertyHandle previous,
                              pal4::ActorPropertyHandle incoming) override {
        Check(subject == actor, "property notification actor");
        notified_previous = previous;
        notified_incoming = incoming;
        trace.emplace_back("notify");
        if (restore_owner_during_notification)
            bytes[actor + 796u] = 1u;
    }
    void RefreshPropertyCount(std::uint32_t property,
                              std::uint32_t subject) override {
        Check(subject == actor, "refresh actor");
        trace.push_back("refresh:" + std::to_string(property));
    }
    void BindProperty(std::uint32_t property,
                      std::uint32_t subject) override {
        Check(subject == actor, "bind actor");
        trace.push_back("bind:" + std::to_string(property));
    }
    std::uint8_t ReleaseProperty(std::uint32_t property) override {
        trace.push_back("release:" + std::to_string(property));
        return 0x5au;
    }
    std::uint32_t PropertyId(std::uint32_t property) override {
        trace.push_back("id:" + std::to_string(property));
        return ids.at(property);
    }
    void FinishProperty(std::uint32_t property,
                        std::uint32_t subject) override {
        Check(subject == actor, "finish actor");
        trace.push_back("finish:" + std::to_string(property));
    }
    void DispatchProperty(std::uint32_t property,
                          std::uint32_t subject,
                          std::uint32_t event_id) override {
        Check(subject == actor && event_id <= 8u,
              "event dispatch args");
        trace.push_back("dispatch:" + std::to_string(property));
    }
    bool HasBlockingObjectState(std::uint32_t subject) override {
        Check(subject == actor, "state check actor");
        trace.emplace_back("blocked");
        return blocked;
    }
    std::uint32_t InvokePropertySlot(
        std::uint32_t property, std::uint32_t subject,
        std::uint32_t slot, std::uint32_t argument) override {
        Check(property == old_property && subject == actor &&
              ((slot == 236u || slot == 244u || slot == 252u ||
                slot == 260u || slot == 268u || slot == 276u ||
                slot == 284u || slot == 292u || slot == 300u)
                   ? argument == 0u
                   : (slot == 240u || slot == 248u || slot == 256u ||
                      slot == 264u || slot == 272u || slot == 280u ||
                      slot == 288u || slot == 296u || slot == 304u) &&
                         (argument == 0x99u ||
                          (slot == 272u && argument == 3u))),
              "property slot arguments");
        trace.push_back("slot" + std::to_string(slot));
        return slot == 236u || slot == 244u || slot == 252u ||
                       slot == 260u || slot == 268u || slot == 276u ||
                       slot == 284u || slot == 292u || slot == 300u
                   ? 0x55u : action_result;
    }
    void CleanupConditionalList(std::uint32_t subject,
                                std::uint32_t id,
                                std::uint32_t value) override {
        Check(subject == actor && id >= 39u && id <= 47u && value == 0u,
              "conditional list cleanup arguments");
        trace.push_back("cleanup:" + std::to_string(id));
    }
    pal4::ActorPropertyHandle MakePropertyHandle(
        std::uint32_t first, std::uint32_t second) override {
        Check(first <= 8u && second == action_result,
              "property factory arguments");
        made_variant = first;
        trace.emplace_back("make");
        return second == 0u ? pal4::ActorPropertyHandle{}
                            : pal4::ActorPropertyHandle{true, new_property};
    }
    std::uint32_t ActorMode(std::uint32_t subject) override {
        Check(subject == actor, "actor mode argument");
        trace.emplace_back("mode");
        return actor_mode;
    }
    std::uint32_t CombatActorData() override {
        trace.emplace_back("data");
        return 0x7000u;
    }
    void SetClumpAlpha(float alpha,
                       std::uint32_t actor_data) override {
        Check(actor_data == 0x7000u &&
              (alpha == 0.05f || alpha == 1.0f),
              "clump alpha arguments");
        trace.push_back("alpha:" + std::to_string(alpha));
    }
    bool CheckReviveStatus(std::uint32_t subject) override {
        Check(subject == actor, "revive status actor");
        trace.emplace_back("check-status");
        return revive_status;
    }
    void ClearReviveStatus(std::uint32_t subject) override {
        Check(subject == actor, "clear revive actor");
        trace.emplace_back("clear-status");
    }
    std::uint32_t InvokeActorSlot48(
        std::uint32_t subject, std::uint32_t argument) override {
        Check(subject == actor && argument == 0u, "actor slot 48 args");
        trace.emplace_back("actor48");
        return 123u;
    }
    void InvokeActorSlot68(std::uint32_t subject,
                           std::uint32_t kind,
                           std::uint32_t value) override {
        Check(subject == actor && kind == 7u && value == 123u,
              "actor slot 68 args");
        trace.emplace_back("actor68");
    }
    void DispatchRevive(std::uint32_t subject) override {
        Check(subject == actor, "revive event actor");
        trace.emplace_back("ae_revive");
    }
    void InvokeActorSlot272(std::uint32_t subject,
                            std::uint32_t argument) override {
        Check(subject == actor && argument == 7432u,
              "actor slot 272 args");
        trace.emplace_back("actor272");
    }
    std::uint32_t InvokeActorSlot168(
        std::uint32_t subject) override {
        Check(subject == actor, "actor slot 168 args");
        trace.emplace_back("actor168");
        return 0x66u;
    }
};

void SetNullAndTransfer() {
    Runtime runtime;
    Check(pal4::ActorManagerPropertyState::Set(actor, {true, 0u}, runtime) == 1u &&
          runtime.trace.empty(), "null property leaves actor unchanged");

    Check(pal4::ActorManagerPropertyState::Set(
              actor, {true, new_property}, runtime) == 0u &&
          runtime.notified_previous.owned &&
          runtime.notified_previous.property == old_property &&
          runtime.notified_incoming.owned &&
          runtime.notified_incoming.property == new_property &&
          runtime.bytes[actor + 796u] == 1u &&
          runtime.words[actor + 800u] == new_property &&
          runtime.trace == std::vector<std::string>{
              "byte:4892:0", "notify", "byte:4892:1",
              "word:4896:12288", "refresh:12288", "bind:12288"},
          "incoming property ownership transfers after notification");
}

void ReplaceAndMerge() {
    Runtime runtime;
    runtime.restore_owner_during_notification = true;
    (void)pal4::ActorManagerPropertyState::Set(actor, {false, new_property},
                                               runtime);
    Check(runtime.trace == std::vector<std::string>{
              "byte:4892:0", "notify", "release:8192",
              "byte:4892:0", "word:4896:12288", "refresh:12288",
              "bind:12288"},
          "different property releases current owner after callback");

    runtime.trace.clear();
    runtime.restore_owner_during_notification = false;
    (void)pal4::ActorManagerPropertyState::Set(actor, {true, new_property},
                                               runtime);
    Check(runtime.bytes[actor + 796u] == 1u &&
          runtime.trace == std::vector<std::string>{
              "byte:4892:0", "notify", "byte:4892:1",
              "word:4896:12288", "refresh:12288", "bind:12288"},
          "same property merges incoming ownership without release");
}

void EmbeddedHandleAndEvent() {
    Runtime runtime;
    runtime.bytes[old_property + 4u] = 1u;
    runtime.words[old_property + 8u] = new_property;
    pal4::ActorManagerPropertyState::HandleEvent(actor, 7u, runtime);
    Check(runtime.bytes[old_property + 4u] == 0u &&
          runtime.bytes[actor + 796u] == 1u &&
          runtime.words[actor + 800u] == new_property &&
          runtime.trace == std::vector<std::string>{
              "id:8192", "byte:4892:0", "byte:8196:0",
              "byte:4892:1", "word:4896:12288", "finish:8192",
              "id:12288", "dispatch:12288", "release:8192"},
          "event moves embedded handle then continues on replacement");

    Runtime other;
    pal4::ActorManagerPropertyState::HandleEvent(actor, 8u, other);
    Check(other.trace == std::vector<std::string>{"id:8192", "dispatch:8192"},
          "different event ID dispatches to current property");
}

void StatePredicates() {
    Runtime runtime;
    for (std::uint32_t state = 0u; state <= 8u; ++state) {
        runtime.words[actor + 828u] = state;
        runtime.trace.clear();
        const bool ready = pal4::ActorManagerPropertyState::IsReady(actor, runtime);
        Check(pal4::ActorManagerPropertyState::IsIdle(actor, runtime) ==
                  (state == 0u) &&
              ready == (state == 1u || state == 2u || state == 5u) &&
              pal4::ActorManagerPropertyState::IsUnavailable(actor, runtime) ==
                  (state == 3u || state == 4u || state == 6u || state == 7u) &&
              runtime.trace.size() == (ready ? 1u : 0u),
              "all actor state classes and short circuit");
    }
    runtime.words[actor + 828u] = 5u;
    runtime.blocked = true;
    Check(!pal4::ActorManagerPropertyState::IsReady(actor, runtime),
          "blocking object state prevents readiness");
}

void PropertyAction() {
    Runtime runtime;
    Check(pal4::ActorManagerPropertyState::ForwardSlot236(actor, runtime) ==
              0x55u &&
          runtime.trace == std::vector<std::string>{"slot236"},
          "slot 236 forwards current property and actor");

    runtime.trace.clear();
    Check(pal4::ActorManagerPropertyState::ProcessPropertyAction(
              actor, 0x99u, runtime) == 42u &&
          runtime.made_variant == 0u &&
          runtime.words[actor + 800u] == new_property &&
          runtime.bytes[actor + 796u] == 1u &&
          runtime.trace == std::vector<std::string>{
              "slot240", "make", "byte:4892:0", "notify",
              "byte:4892:1", "word:4896:12288", "refresh:12288",
              "bind:12288"},
          "slot 240 result creates and transfers a property handle");

    Runtime empty;
    empty.action_result = 0u;
    Check(pal4::ActorManagerPropertyState::ProcessPropertyAction(
              actor, 0x99u, empty) == 0u &&
          empty.made_variant == 0u &&
          empty.trace == std::vector<std::string>{
              "slot240", "cleanup:39", "make", "id:8192",
              "dispatch:8192"},
          "zero result cleans list and handles property event zero");
}

void PropertyActionVariants() {
    Runtime forward;
    Check(pal4::ActorManagerPropertyState::ForwardSlot244(actor, forward) ==
              0x55u &&
          pal4::ActorManagerPropertyState::ForwardSlot252(actor, forward) ==
              0x55u &&
          forward.trace ==
              std::vector<std::string>{"slot244", "slot252"},
          "adjacent property forwarding slots");

    Runtime one;
    Check(pal4::ActorManagerPropertyState::ProcessPropertyAction1(
              actor, 0x99u, one) == 42u && one.made_variant == 1u &&
          one.trace == std::vector<std::string>{
              "slot248", "make", "byte:4892:0", "notify",
              "byte:4892:1", "word:4896:12288", "refresh:12288",
              "bind:12288"},
          "variant one uses slot 248 and factory argument one");
    Runtime one_empty;
    one_empty.action_result = 0u;
    Check(pal4::ActorManagerPropertyState::ProcessPropertyAction1(
              actor, 0x99u, one_empty) == 0u &&
          one_empty.trace == std::vector<std::string>{
              "slot248", "cleanup:40", "make", "id:8192",
              "dispatch:8192"},
          "variant one zero result uses cleanup id 40 and event one");

    Runtime two;
    two.words[actor + 808u] = 0x4000u;
    Check(pal4::ActorManagerPropertyState::ProcessPropertyAction2(
              actor, 0x99u, two) == 42u && two.made_variant == 2u &&
          two.trace == std::vector<std::string>{
              "slot256", "make", "byte:4892:0", "notify",
              "byte:4892:1", "word:4896:12288", "refresh:12288",
              "bind:12288", "mode", "data", "alpha:0.050000"},
          "variant two fades combat actor data in mode four");

    Runtime two_other;
    two_other.actor_mode = 3u;
    two_other.words[actor + 808u] = 0x4000u;
    (void)pal4::ActorManagerPropertyState::ProcessPropertyAction2(
        actor, 0x99u, two_other);
    Check(two_other.trace.back() == "alpha:1.000000",
          "variant two restores full alpha outside mode four");

    Runtime two_empty;
    two_empty.action_result = 0u;
    Check(pal4::ActorManagerPropertyState::ProcessPropertyAction2(
              actor, 0x99u, two_empty) == 0u &&
          two_empty.trace == std::vector<std::string>{
              "slot256", "cleanup:41", "make", "id:8192",
              "dispatch:8192", "mode"},
          "variant two without child skips alpha but still checks mode");
}

void FurtherPropertyActions() {
    Runtime forward;
    Check(pal4::ActorManagerPropertyState::ForwardSlot260(actor, forward) ==
              0x55u &&
          pal4::ActorManagerPropertyState::ForwardSlot268(actor, forward) ==
              0x55u &&
          forward.trace ==
              std::vector<std::string>{"slot260", "slot268"},
          "property slots 260 and 268 forward current property");

    Runtime three;
    Check(pal4::ActorManagerPropertyState::ProcessPropertyAction3(
              actor, 0x99u, three) == 42u && three.made_variant == 3u &&
          three.trace == std::vector<std::string>{
              "slot264", "make", "byte:4892:0", "notify",
              "byte:4892:1", "word:4896:12288", "refresh:12288",
              "bind:12288"},
          "variant three uses property slot 264 and factory argument three");
    Runtime three_empty;
    three_empty.action_result = 0u;
    Check(pal4::ActorManagerPropertyState::ProcessPropertyAction3(
              actor, 0x99u, three_empty) == 0u &&
          three_empty.trace == std::vector<std::string>{
              "slot264", "cleanup:42", "make", "id:8192",
              "dispatch:8192"},
          "variant three zero result uses cleanup id 42 and event three");

    Runtime four;
    Check(pal4::ActorManagerPropertyState::ProcessPropertyAction4(
              actor, 0x99u, four) == 42u && four.made_variant == 4u &&
          four.trace == std::vector<std::string>{
              "slot272", "make", "byte:4892:0", "notify",
              "byte:4892:1", "word:4896:12288", "refresh:12288",
              "bind:12288"},
          "variant four ordinary argument skips revive status check");
    Runtime four_empty;
    four_empty.action_result = 0u;
    Check(pal4::ActorManagerPropertyState::ProcessPropertyAction4(
              actor, 0x99u, four_empty) == 0u &&
          four_empty.trace == std::vector<std::string>{
              "slot272", "cleanup:43", "make", "id:8192",
              "dispatch:8192"},
          "variant four zero result uses cleanup id 43 and event four");

    Runtime inactive;
    (void)pal4::ActorManagerPropertyState::ProcessPropertyAction4(
        actor, 3u, inactive);
    Check(inactive.trace.front() == "check-status" &&
          inactive.trace[1] == "slot272",
          "revive-status miss continues into property slot 272");

    Runtime revive;
    revive.revive_status = true;
    Check(pal4::ActorManagerPropertyState::ProcessPropertyAction4(
              actor, 3u, revive) == 0x66u &&
          revive.trace == std::vector<std::string>{
              "check-status", "clear-status", "actor48", "actor68",
              "ae_revive", "actor272", "actor168"},
          "revive path bypasses property factory and preserves call order");
}

void FinalPropertyActionVariants() {
    using Forward = std::uint32_t (*)(
        std::uint32_t, pal4::IActorManagerPropertyStateRuntime&);
    using Action = std::uint32_t (*)(
        std::uint32_t, std::uint32_t,
        pal4::IActorManagerPropertyStateRuntime&);
    struct Case {
        std::uint32_t forward_slot;
        std::uint32_t action_slot;
        std::uint32_t variant;
        Forward forward;
        Action action;
    };
    constexpr std::array cases{
        Case{276u, 280u, 5u,
             &pal4::ActorManagerPropertyState::ForwardSlot276,
             &pal4::ActorManagerPropertyState::ProcessPropertyAction5},
        Case{284u, 288u, 6u,
             &pal4::ActorManagerPropertyState::ForwardSlot284,
             &pal4::ActorManagerPropertyState::ProcessPropertyAction6},
        Case{292u, 296u, 7u,
             &pal4::ActorManagerPropertyState::ForwardSlot292,
             &pal4::ActorManagerPropertyState::ProcessPropertyAction7},
        Case{300u, 304u, 8u,
             &pal4::ActorManagerPropertyState::ForwardSlot300,
             &pal4::ActorManagerPropertyState::ProcessPropertyAction8}};
    for (const auto& entry : cases) {
        Runtime forward;
        Check(entry.forward(actor, forward) == 0x55u &&
              forward.trace == std::vector<std::string>{
                  "slot" + std::to_string(entry.forward_slot)},
              "property forward slot and result");

        Runtime created;
        Check(entry.action(actor, 0x99u, created) == 42u &&
              created.made_variant == entry.variant &&
              created.trace == std::vector<std::string>{
                  "slot" + std::to_string(entry.action_slot), "make",
                  "byte:4892:0", "notify", "byte:4892:1",
                  "word:4896:12288", "refresh:12288", "bind:12288"},
              "property action slot, factory and ownership transfer");

        Runtime empty;
        empty.action_result = 0u;
        empty.ids[old_property] = 9u;
        Check(entry.action(actor, 0x99u, empty) == 0u &&
              empty.made_variant == entry.variant &&
              empty.trace == std::vector<std::string>{
                  "slot" + std::to_string(entry.action_slot),
                  "cleanup:" + std::to_string(39u + entry.variant),
                  "make", "id:8192", "dispatch:8192"},
              "property action cleanup ID and fallback event");
    }
}

}  // namespace

int main() {
    try {
        SetNullAndTransfer();
        ReplaceAndMerge();
        EmbeddedHandleAndEvent();
        StatePredicates();
        PropertyAction();
        PropertyActionVariants();
        FurtherPropertyActions();
        FinalPropertyActionVariants();
        std::cout << "actor_manager_property_state_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
