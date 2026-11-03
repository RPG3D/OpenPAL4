#include "pal4/combat_fatal_sequence_runtime.h"
#include "pal4/combat_dead_sequence_task.h"
#include "pal4/combat_wounded_sequence_task.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kActor = 0x2000u;
constexpr std::uint32_t kTarget = 0x3000u;

struct Addition {
    std::uint32_t container;
    std::uint32_t child;
    std::uint32_t text;
    std::array<std::uint32_t, 3> timing;
};

struct Runtime final : pal4::IFatalSequenceRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kActor}, {kTask + 48u, kTarget},
        {kActor + 804u, 1u}, {kActor + 1052u, 0u},
        {kActor + 1056u, 0u},
        {0x8f965cu, 0x9100u}, {0x8f9620u, 0x9200u},
        {0x8f9748u, 0x9300u}, {0x8f975cu, 0x9400u},
        {0x8f96d0u, 0x9500u}, {0x8f9734u, 0x9600u}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> factories;
    std::vector<Addition> additions;
    std::vector<std::uint32_t> children;
    std::map<std::size_t, std::uint32_t> actor_after_add;
    std::size_t next_child = 0u;
    std::uint32_t assigned_header = 0u;
    std::uint32_t assigned_source = 0u;
    std::uint32_t message_header = 0u;
    std::uint32_t message_source = 0u;
    std::uint32_t actor_state = 0xffffffffu;
    std::uint32_t state_mode = 0xffffffffu;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void SetActorState(std::uint32_t actor,
                       std::uint32_t state,
                       std::uint32_t mode) override {
        Check(actor == kActor, "state actor");
        actor_state = state;
        state_mode = mode;
        calls.emplace_back("state");
    }
    std::uint32_t CreateChildTask(std::uint32_t factory) override {
        factories.push_back(factory);
        calls.emplace_back("create");
        Check(next_child < children.size(), "unexpected child");
        return children[next_child++];
    }
    void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) override {
        additions.push_back({container, child, text, timing});
        if (const auto it = actor_after_add.find(additions.size());
            it != actor_after_add.end())
            words[kTask + 44u] = it->second;
        calls.emplace_back("add");
    }
    std::uint32_t EventDataByIndex(std::uint32_t index) override {
        Check(index == 43u, "event index");
        calls.emplace_back("event");
        return 0xbeefu;
    }
    void AssignText(std::uint32_t header,
                    std::uint32_t source) override {
        assigned_header = header;
        assigned_source = source;
        calls.emplace_back("text");
    }
    void SetMessage(std::uint32_t header,
                    std::uint32_t source) override {
        message_header = header;
        message_source = source;
        calls.emplace_back("message");
    }
};

void CheckAddition(const Addition& addition,
                   std::uint32_t child, std::uint32_t text,
                   std::array<std::uint32_t, 3> timing) {
    Check(addition.container == kTask + 4u &&
              addition.child == child && addition.text == text &&
              addition.timing == timing, "child addition");
}

void Factories() {
    pal4::FatalChildFactorySlot slot;
    Check(pal4::FatalChildFactories::InitializeEvent(&slot) == &slot &&
              slot.vtable == 0x845de0u, "event factory");
    Check(pal4::FatalChildFactories::InitializeFirstFinal(&slot) == &slot &&
              slot.vtable == 0x845de4u, "first final factory");
    Check(pal4::FatalChildFactories::InitializeFinish(&slot) == &slot &&
              slot.vtable == 0x845de8u, "finish factory");
    Check(pal4::FatalChildFactories::InitializeEnd(&slot) == &slot &&
              slot.vtable == 0x845decu, "end factory");
    Check(pal4::FatalChildFactories::InitializeMessage(&slot) == &slot &&
              slot.vtable == 0x845df0u, "message factory");
}

void DeadNormal() {
    Runtime r;
    r.children = {0x4000u, 0x4100u, 0x4200u};
    pal4::DeadSequenceTask::Start(kTask, r);
    Check(r.actor_state == 3u && r.state_mode == 1u,
          "death actor state");
    Check(r.factories == std::vector<std::uint32_t>{
              0x845de0u, 0x845d34u, 0x845decu},
          "death normal factories");
    Check(r.additions.size() == 3u, "death normal count");
    CheckAddition(r.additions[0], 0x4000u, 0x9100u, {1u, 0u, 0u});
    CheckAddition(r.additions[1], 0x4100u, 0x9200u, {1u, 0u, 0u});
    CheckAddition(r.additions[2], 0x4200u, 0x9500u, {1u, 0u, 0u});
    Check(r.words[0x4000u + 60u] == kActor &&
              r.words[0x4000u + 64u] == 0xbeefu &&
              r.words[0x4000u + 68u] == 0x40400000u,
          "event child fields");
    Check(r.words[0x4100u + 60u] == kActor &&
              r.assigned_header == 0x4100u + 64u &&
              r.assigned_source == 0x8fa35cu &&
              r.bytes[0x4100u + 80u] == 0u,
          "death collapse fields");
    Check(r.words[0x4200u + 60u] == kActor &&
              r.words[0x4200u + 64u] == kTarget,
          "death end fields");
    Check(r.calls[2] == "state" &&
              r.calls[4] == "create" &&
              r.calls[5] == "add" &&
              r.calls[6] == "event",
          "death state, child and event order");
}

void DeadSpecialAndFallback() {
    Runtime r;
    r.words[kActor + 804u] = 2u;
    r.words[0x8f965cu] = 0u;
    r.words[0x8f975cu] = 0u;
    r.children = {0x4000u, 0x4100u, 0x4200u, 0x4300u, 0x4400u};
    pal4::DeadSequenceTask::Start(kTask, r);
    Check(r.factories == std::vector<std::uint32_t>{
              0x845de0u, 0x845d34u, 0x845de4u,
              0x845de8u, 0x845decu}, "death special factories");
    Check(r.additions.size() == 5u, "death special count");
    CheckAddition(r.additions[0], 0x4000u, 0x841964u, {1u, 0u, 0u});
    CheckAddition(r.additions[2], 0x4200u, 0x9300u, {1u, 0u, 0u});
    CheckAddition(r.additions[3], 0x4300u, 0x841964u,
                  {1u, 0x3fc00000u, 0x3f800000u});
    Check(r.words[0x4200u + 60u] == kActor &&
              r.words[0x4300u + 60u] == kActor,
          "special actor fields");
}

void FinalHitNormal() {
    Runtime r;
    r.children = {0x4000u};
    pal4::FinalHitSequenceTask::Start(kTask, r);
    Check(r.actor_state == 2u && r.state_mode == 0u,
          "final-hit actor state");
    Check(r.factories == std::vector<std::uint32_t>{0x845d34u},
          "final-hit normal factory");
    CheckAddition(r.additions[0], 0x4000u, 0x9200u, {1u, 0u, 0u});
    Check(r.assigned_source == 0x8fa3a0u &&
              r.words[0x4000u + 60u] == kActor,
          "final-hit collapse fields");
}

void FinalHitSpecial(bool message_present) {
    Runtime r;
    r.words[kActor + 804u] = 2u;
    r.words[kActor + 1056u] = message_present ? 1u : 0u;
    r.words[kActor + 1052u] = message_present ? 0x9700u : 0u;
    r.children = {0x4000u, 0x4100u, 0x4200u, 0x4300u};
    pal4::FinalHitSequenceTask::Start(kTask, r);
    Check(r.factories == (message_present
              ? std::vector<std::uint32_t>{
                    0x845d34u, 0x845df0u, 0x845de4u, 0x845de8u}
              : std::vector<std::uint32_t>{
                    0x845d34u, 0x845de4u, 0x845de8u}),
          "final-hit special factories");
    if (message_present) {
        CheckAddition(r.additions[1], 0x4100u, 0x9600u,
                      {1u, 0u, 0u});
        Check(r.words[0x4100u + 76u] == kActor &&
                  r.bytes[0x4100u + 88u] == 0u &&
                  r.message_header == 0x4100u + 60u &&
                  r.message_source == 0x9700u,
              "message child fields");
    } else {
        Check(r.message_header == 0u, "message skipped");
    }
    const auto first = message_present ? 0x4200u : 0x4100u;
    const auto last = message_present ? 0x4300u : 0x4200u;
    Check(r.words[first + 60u] == kActor &&
              r.words[last + 60u] == kActor,
          "final-hit pair actors");
    CheckAddition(r.additions.back(), last, 0x9400u,
                  {1u, 0x3fc00000u, 0x3f800000u});
}

void GatesAndNullChildren() {
    Runtime r;
    r.words[kTask + 44u] = 0u;
    pal4::DeadSequenceTask::Start(kTask, r);
    Check(r.factories.empty() && r.actor_state == 0xffffffffu,
          "zero source gate");
    r.words[kTask + 44u] = kActor;
    r.words[kTask + 48u] = 0u;
    pal4::FinalHitSequenceTask::Start(kTask, r);
    Check(r.factories.empty() && r.actor_state == 0xffffffffu,
          "zero target gate");
    r.words[kTask + 48u] = kTarget;
    r.children = {0u};
    pal4::DeadSequenceTask::Start(kTask, r);
    Check(r.factories.size() == 1u && r.additions.empty(),
          "zero event child");
    Runtime final_hit;
    final_hit.children = {0u};
    pal4::FinalHitSequenceTask::Start(kTask, final_hit);
    Check(final_hit.factories.size() == 1u &&
              final_hit.additions.empty(), "zero collapse child");
}

void OptionalMessageFallbackAndNull() {
    Runtime r;
    r.words[kActor + 804u] = 2u;
    r.words[kActor + 1056u] = 1u;
    r.words[kActor + 1052u] = 0u;
    r.words[0x8f9734u] = 0u;
    r.children = {0x4000u, 0x4100u, 0x4200u, 0x4300u};
    pal4::FinalHitSequenceTask::Start(kTask, r);
    Check(r.message_source == 0x841964u &&
              r.additions[1].text == 0x841964u,
          "optional message empty fallbacks");

    Runtime null_message;
    null_message.words[kActor + 804u] = 2u;
    null_message.words[kActor + 1056u] = 1u;
    null_message.children = {0x4000u, 0u};
    pal4::FinalHitSequenceTask::Start(kTask, null_message);
    Check(null_message.factories.size() == 2u &&
              null_message.message_header == 0u,
          "null optional message stops safely");

    Runtime null_final;
    null_final.words[kActor + 804u] = 2u;
    null_final.children = {0x4000u, 0x4100u, 0u};
    pal4::DeadSequenceTask::Start(kTask, null_final);
    Check(null_final.factories.size() == 3u &&
              null_final.additions.size() == 2u,
          "null first final child stops safely");
}

void ActorRereadsAfterChildCallbacks() {
    constexpr std::uint32_t second_actor = 0x2100u;
    constexpr std::uint32_t third_actor = 0x2200u;
    Runtime r;
    r.words[second_actor + 804u] = 1u;
    r.words[third_actor + 804u] = 1u;
    r.actor_after_add = {{1u, second_actor}, {2u, third_actor}};
    r.children = {0x4000u, 0x4100u, 0x4200u};
    pal4::DeadSequenceTask::Start(kTask, r);
    Check(r.words[0x4000u + 60u] == second_actor &&
              r.words[0x4100u + 60u] == third_actor &&
              r.words[0x4200u + 60u] == third_actor,
          "actor is reread after each child callback");
}

void QuickDeadStart() {
    Runtime r;
    r.children = {0x4000u, 0x4100u};
    r.actor_after_add = {{1u, 0x2200u}};
    pal4::QuickDeadSequenceTask::Start(kTask, r);
    Check(r.actor_state == 3u && r.state_mode == 1u,
          "quick death state");
    Check(r.factories == std::vector<std::uint32_t>{
              0x845de0u, 0x845decu},
          "quick death child factories");
    CheckAddition(r.additions[0], 0x4000u, 0x9100u, {1u, 0u, 0u});
    CheckAddition(r.additions[1], 0x4100u, 0x9500u, {1u, 0u, 0u});
    Check(r.words[0x4000u + 60u] == 0x2200u &&
              r.words[0x4000u + 64u] == 0xbeefu &&
              r.words[0x4000u + 68u] == 0x40400000u &&
              r.words[0x4100u + 60u] == 0x2200u &&
              r.words[0x4100u + 64u] == kTarget,
          "quick death child layout and reread");

    Runtime null_child;
    null_child.children = {0u};
    pal4::QuickDeadSequenceTask::Start(kTask, null_child);
    Check(null_child.factories.size() == 1u &&
              null_child.additions.empty(), "quick death null child");
    Runtime no_actor;
    no_actor.words[kTask + 44u] = 0u;
    pal4::QuickDeadSequenceTask::Start(kTask, no_actor);
    Check(no_actor.factories.empty() &&
              no_actor.actor_state == 0xffffffffu,
          "quick death actor gate");
}

struct CompletionRuntime final : pal4::IFinalHitCompletionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kActor}, {kTask + 48u, kTarget}};
    std::vector<std::string> calls;
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void ActorVirtual172(std::uint32_t actor,
                         std::uint32_t value) override {
        Check(actor == kActor && value == 3u, "completion +172");
        words[kTask + 44u] = 0x2200u;
        calls.emplace_back("virtual172");
    }
    std::uint32_t ActorVirtual544(std::uint32_t actor,
                                  std::uint32_t target) override {
        Check(actor == 0x2200u && target == kTarget,
              "completion +544 reread");
        calls.emplace_back("virtual544");
        return 0x1234u;
    }
};

void FinalHitCompletion() {
    CompletionRuntime r;
    Check(pal4::FinalHitSequenceTask::CreateNext(kTask, r) ==
              0x1234u, "completion return");
    Check(r.calls == std::vector<std::string>{
              "read:4140", "virtual172", "read:4140",
              "read:4144", "virtual544"},
          "completion order");
}

}  // namespace

int main() {
    try {
        Factories();
        DeadNormal();
        DeadSpecialAndFallback();
        FinalHitNormal();
        FinalHitSpecial(false);
        FinalHitSpecial(true);
        GatesAndNullChildren();
        OptionalMessageFallbackAndNull();
        ActorRereadsAfterChildCallbacks();
        QuickDeadStart();
        FinalHitCompletion();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
