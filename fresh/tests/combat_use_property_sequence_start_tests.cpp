#include "pal4/combat_use_property_sequence_task.h"
#include "pal4/combat_dead_sequence_task.h"
#include "pal4/combat_end_action_task.h"

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

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;
constexpr std::uint32_t kTarget = 0x3000u;
constexpr std::uint32_t kProperty = 0x4000u;
constexpr std::array<std::uint32_t, 3> kChildPosition{
    0x3f800000u, 0x40000000u, 0x40400000u};
constexpr std::array<std::uint32_t, 3> kTargetPosition{
    0x40800000u, 0x40a00000u, 0x40c00000u};

struct Runtime final : pal4::IUsePropertySequenceStartRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget},
        {kTask + 52u, kProperty}, {kProperty + 156u, 0u},
        {0x8f9784u, 0x9010u}, {0x8f97e8u, 0x9020u},
        {0x8f97fcu, 0x9030u}, {0x8f9824u, 0x9040u}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> modes{1u};
    std::vector<std::uint32_t> children{0x5000u};
    std::size_t mode_index = 0u;
    std::size_t child_index = 0u;
    std::uint32_t added_text = 0u;
    std::uint32_t assigned_text = 0u;
    std::uint32_t assigned_source = 0u;
    std::uint32_t throw_component = 0u;
    std::uint32_t run_source = 0u;
    std::array<std::uint32_t, 3> run_position{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::array<std::uint32_t, 3> ActorChildPosition(
        std::uint32_t actor) override {
        Check(actor == words.at(kTask + 48u), "child position target");
        calls.emplace_back("child_position");
        return kChildPosition;
    }
    std::array<std::uint32_t, 3> ActorTargetPosition(
        std::uint32_t actor) override {
        Check(actor == kTarget, "virtual +448 target");
        calls.emplace_back("target_position");
        return kTargetPosition;
    }
    std::uint32_t PropertyMovementMode(std::uint32_t property) override {
        Check(property == kProperty, "property mode argument");
        calls.emplace_back("mode");
        const auto index = mode_index++;
        return modes[index < modes.size() ? index : modes.size() - 1u];
    }
    void NotifySource(std::uint32_t actor) override {
        Check(actor == kSource, "source virtual +476");
        calls.emplace_back("notify");
    }
    std::uint32_t CreateChildTask(std::uint32_t factory) override {
        calls.push_back("factory:" + std::to_string(factory));
        Check(child_index < children.size(), "child allocation count");
        return children[child_index++];
    }
    void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u && child != 0u &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "GOM container and timing");
        added_text = text;
        calls.emplace_back("add");
    }
    void SetApproachTaskTarget(
        std::uint32_t child, std::uint32_t source,
        const std::array<std::uint32_t, 3>& position) override {
        Check(child == 0x5000u, "run child");
        run_source = source;
        run_position = position;
        calls.emplace_back("run_target");
    }
    std::uint32_t AssignChildText(std::uint32_t header,
                                   std::uint32_t source) override {
        assigned_text = header;
        assigned_source = source;
        calls.emplace_back("assign_text");
        return 0xbeefu;
    }
    std::uint32_t SetThrowPropertyTargets(
        std::uint32_t embedded, std::uint32_t source,
        std::uint32_t target, std::uint32_t property) override {
        Check(source == kSource && target == words.at(kTask + 48u) &&
                  property == kProperty, "throw component arguments");
        throw_component = embedded;
        calls.emplace_back("throw_targets");
        return property;
    }
};

void FactoriesAndTargetFields() {
    pal4::UsePropertyChildFactorySlot slot;
    Check(pal4::UsePropertyChildFactories::InitializeCast(&slot) == &slot &&
              slot.vtable == 0x845db4u, "cast factory slot");
    Check(pal4::UsePropertyChildFactories::InitializeThrow(&slot) == &slot &&
              slot.vtable == 0x845db8u, "throw factory slot");
    Check(pal4::UsePropertyChildFactories::InitializeDirect(&slot) == &slot &&
              slot.vtable == 0x845dbcu, "direct factory slot");
    Runtime r;
    Check(pal4::UsePropertyTask::SetActorsAndProperty(
              0x6000u, kSource, kTarget, kProperty, r) == kProperty &&
              r.words.at(0x6000u + 60u) == kSource &&
              r.words.at(0x6000u + 64u) == kTarget &&
              r.words.at(0x6000u + 68u) == kProperty,
          "embedded use-property target assignment");
}

void DirectAction() {
    Runtime r;
    Check(pal4::UsePropertySequenceTask::Start(kTask, r) == 0xbeefu,
          "direct action returns text assignment result");
    Check(r.calls.front() == "read:4144" &&
              r.calls[1] == "child_position" &&
              r.calls[5] == "notify", "position and notify order");
    Check(r.words.at(0x5000u + 60u) == kSource &&
              r.words.at(0x5000u + 300u) == kSource &&
              r.words.at(0x5000u + 304u) == kTarget &&
              r.words.at(0x5000u + 308u) == kProperty &&
              r.bytes.at(0x5000u + 320u) == 1u,
          "direct child fields");
    Check(r.assigned_text == 0x5000u + 64u &&
              r.assigned_source == 0x8fa2f8u &&
              r.added_text == 0x9040u,
          "direct text sources");
    Check(r.calls.back() != "run_target", "direct mode skips run");
    r = Runtime{};
    r.words[0x8f9824u] = 0u;
    r.children = {0u};
    Check(pal4::UsePropertySequenceTask::Start(kTask, r) == 0u &&
              r.added_text == 0u && r.assigned_text == 0u,
          "null direct child exits without field writes");
}

void CastActionWithRun() {
    Runtime r;
    r.modes = {4u};
    r.words[kProperty + 156u] = 1u;
    r.children = {0x5000u, 0x6000u};
    r.words[0x8f97e8u] = 0u;
    Check(pal4::UsePropertySequenceTask::Start(kTask, r) == kProperty,
          "cast action returns property");
    Check(r.run_source == kSource && r.run_position == kTargetPosition,
          "mode four uses actor virtual +448 position");
    Check(r.words.at(0x6000u + 60u) == kSource &&
              r.words.at(0x6000u + 292u) == kSource &&
              r.words.at(0x6000u + 296u) == kTarget &&
              r.words.at(0x6000u + 300u) == kProperty &&
              r.bytes.at(0x6000u + 326u) == 1u,
          "cast child and embedded target fields");
    Check(r.assigned_source == 0x8fa2e8u &&
              r.added_text == 0x841964u,
          "cast text assignment and fallback GOM text");
    r = Runtime{};
    r.modes = {2u};
    r.children = {0u};
    Check(pal4::UsePropertySequenceTask::Start(kTask, r) == 0u &&
              r.assigned_text == 0u, "null run child stops sequence");
}

void ThrowActionWithoutRun() {
    Runtime r;
    r.modes = {2u};
    r.words[kTask + 48u] = kSource;
    r.words[kProperty + 156u] = 0u;
    r.children = {0x6000u};
    Check(pal4::UsePropertySequenceTask::Start(kTask, r) == kProperty,
          "throw action returns property");
    Check(r.run_source == 0u &&
              r.throw_component == 0x6000u + 240u &&
              r.words.at(0x6000u + 60u) == kSource &&
              r.words.at(0x6000u + 388u) == kSource &&
              r.words.at(0x6000u + 392u) == kSource &&
              r.words.at(0x6000u + 396u) == kProperty &&
              r.bytes.at(0x6000u + 410u) == 1u,
          "same-actor throw skips run and sets all fields");
    Check(r.assigned_source == 0x8fa2e8u &&
              r.added_text == 0x9030u, "throw text sources");

    r = Runtime{};
    r.modes = {1u, 1u, 4u};
    r.words[kTask + 48u] = kSource;
    r.children = {0x6000u};
    Check(pal4::UsePropertySequenceTask::Start(kTask, r) == kProperty &&
              r.mode_index == 3u && r.throw_component != 0u,
          "post-notify movement mode is reread");
}

struct TransitionRuntime final : pal4::IQuickUsePropertyTransitionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations{0x7000u, 0x8000u};
    std::size_t allocation_index = 0u;
    bool state_three = false;
    bool mutate_after_state = false;
    bool active = true;
    bool null_iterator = false;
    bool throw_on_current = false;
    std::vector<std::uint32_t> actors;
    std::size_t cursor = 0u;
    std::map<std::uint32_t, bool> ready;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    bool SourceStateThree(std::uint32_t source) override {
        Check(source == kSource, "state check source");
        calls.emplace_back("state");
        if (mutate_after_state) words[kTask + 44u] = 0x2100u;
        return state_three;
    }
    bool CombatActive() override {
        calls.emplace_back("combat_active");
        return active;
    }
    bool IsActorReady(std::uint32_t actor) override {
        calls.push_back("ready:" + std::to_string(actor));
        return ready[actor];
    }
    std::uint32_t OpenIterator(std::uint32_t actor,
                               std::uint32_t offset) override {
        Check(actor == kTarget && offset == 464u,
              "QuickUseProperty target iterator +464");
        calls.emplace_back("open");
        return null_iterator ? 0u : 0x9000u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "iterator identity");
        return cursor == actors.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "iterator current identity");
        if (throw_on_current) throw std::runtime_error("iterator failure");
        return actors.at(cursor);
    }
    void IteratorNext(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "iterator next identity");
        ++cursor;
    }
    void DestroyIterator(std::uint32_t iterator,
                         std::uint32_t flags) override {
        Check(iterator == 0x9000u && flags == 1u,
              "iterator deleting destructor");
        calls.emplace_back("destroy_iterator");
    }
    void NotifyUnreadyActor(std::uint32_t actor,
                            std::uint32_t source) override {
        Check(source == kSource, "unready actor source argument");
        calls.push_back("notify:" + std::to_string(actor));
    }
    bool CombatInactive() override { return false; }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == (allocation_index == 0u && state_three ? 52u : 48u),
              "action allocation size");
        calls.push_back("allocate:" + std::to_string(size));
        return allocations[allocation_index++];
    }
    void Free(std::uint32_t object) override {
        calls.push_back("free:" + std::to_string(object));
    }
    void InitializeContainer(std::uint32_t container) override {
        calls.push_back("init:" + std::to_string(container));
    }
    void DestroyContainer(std::uint32_t container) override {
        calls.push_back("destroy:" + std::to_string(container));
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void SetNameErrorFlag(std::uint32_t, std::uint8_t) override {}
    bool AllocateName(std::uint32_t, std::uint32_t,
                      std::uint8_t) override { return true; }
    void CopyName(std::uint32_t, std::string_view name) override {
        calls.push_back("name:" + std::string{name});
    }
    void CleanupName(std::uint32_t) override {}
};

void FollowUpAction() {
    TransitionRuntime r;
    Check(pal4::UsePropertySequenceTask::CreateNext(kTask, r) == 0x7000u &&
              r.words.at(0x7000u) ==
                  pal4::CombatEndActionTask::kVtable &&
              r.words.at(0x7000u + 44u) == kSource &&
              r.allocation_index == 1u,
          "living source creates EndAction only");
    r = TransitionRuntime{};
    r.state_three = true;
    Check(pal4::UsePropertySequenceTask::CreateNext(kTask, r) == 0x7000u &&
              r.words.at(0x7000u) == pal4::DeadSequenceTask::kVtable &&
              r.words.at(0x7000u + 44u) == kSource &&
              r.words.at(0x7000u + 48u) == kTarget &&
              r.words.at(0x7000u + 36u) == 0x8000u &&
              r.words.at(0x8000u) ==
                  pal4::CombatEndActionTask::kVtable,
          "dead source links Dead and EndAction");
    r = TransitionRuntime{};
    r.state_three = true;
    r.allocations[0] = 0u;
    Check(pal4::UsePropertySequenceTask::CreateNext(kTask, r) == 0u &&
              r.allocation_index == 1u,
          "failed Dead allocation stops before EndAction");
    r = TransitionRuntime{};
    r.state_three = true;
    r.allocations[1] = 0u;
    Check(pal4::UsePropertySequenceTask::CreateNext(kTask, r) == 0x7000u &&
              r.words.at(0x7000u + 36u) == 0u,
          "failed EndAction leaves Dead without linked child");
    r = TransitionRuntime{};
    r.mutate_after_state = true;
    Check(pal4::UsePropertySequenceTask::CreateNext(kTask, r) == 0x7000u &&
              r.words.at(0x7000u + 44u) == 0x2100u,
          "EndAction uses source reread after state check");
}

struct QuickStartRuntime final : pal4::IQuickUsePropertyStartRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget},
        {kTask + 52u, kProperty}, {0x8f9838u, 0x9050u}};
    std::vector<std::string> calls;
    std::uint32_t child = 0x5000u;
    std::uint32_t added_text = 0u;
    bool mutate_after_add = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void NotifySource(std::uint32_t source) override {
        Check(source == kSource, "QuickUseProperty source notify");
        calls.emplace_back("notify");
    }
    std::uint32_t CreateChildTask(std::uint32_t factory) override {
        Check(factory == pal4::QuickUsePropertyChildFactory::kVtable,
              "QuickUseProperty factory vtable");
        calls.emplace_back("create");
        return child;
    }
    void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t task_child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u && task_child == child &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "QuickUseProperty container and timing");
        added_text = text;
        calls.emplace_back("add");
        if (mutate_after_add) {
            words[kTask + 44u] = 0x2100u;
            words[kTask + 48u] = 0x3100u;
            words[kTask + 52u] = 0x4100u;
        }
    }
};

void QuickStartAndFollowUp() {
    pal4::QuickUsePropertyChildFactorySlot slot;
    Check(pal4::QuickUsePropertyChildFactory::Initialize(&slot) == &slot &&
              slot.vtable == 0x845dc0u, "QuickUseProperty factory slot");
    QuickStartRuntime start;
    Check(pal4::QuickUsePropertySequenceTask::Start(kTask, start) ==
              kProperty && start.added_text == 0x9050u &&
              start.words.at(0x5000u + 60u) == kSource &&
              start.words.at(0x5000u + 64u) == kTarget &&
              start.words.at(0x5000u + 68u) == kProperty,
          "QuickUseProperty child fields and return");
    start = QuickStartRuntime{};
    start.words[0x8f9838u] = 0u;
    start.mutate_after_add = true;
    Check(pal4::QuickUsePropertySequenceTask::Start(kTask, start) ==
              0x4100u && start.added_text == 0x841964u &&
              start.words.at(0x5000u + 60u) == 0x2100u &&
              start.words.at(0x5000u + 64u) == 0x3100u &&
              start.words.at(0x5000u + 68u) == 0x4100u,
          "QuickUseProperty fallback text and field rereads");
    start = QuickStartRuntime{};
    start.child = 0u;
    Check(pal4::QuickUsePropertySequenceTask::Start(kTask, start) == 0u &&
              start.added_text == 0u && start.calls.back() == "create",
          "null QuickUseProperty child exits before writes");

    TransitionRuntime r;
    r.active = false;
    Check(pal4::QuickUsePropertySequenceTask::CreateNext(kTask, r) == 0u &&
              r.calls == std::vector<std::string>{"combat_active"},
          "inactive combat exits before task reads");
    r = TransitionRuntime{};
    r.state_three = true;
    Check(pal4::QuickUsePropertySequenceTask::CreateNext(kTask, r) ==
              0x7000u && r.words.at(0x7000u + 36u) == 0x8000u &&
              r.words.at(0x7000u) == pal4::DeadSequenceTask::kVtable,
          "dead source links Dead and EndAction");
    r = TransitionRuntime{};
    r.actors = {0x5000u, 0x6000u};
    r.ready[0x5000u] = false;
    r.ready[0x6000u] = true;
    Check(pal4::QuickUsePropertySequenceTask::CreateNext(kTask, r) ==
              0x7000u && r.words.at(0x7000u) ==
                  pal4::CombatEndActionTask::kVtable &&
              r.cursor == 2u &&
              std::find(r.calls.begin(), r.calls.end(), "notify:20480") !=
                  r.calls.end() &&
              std::find(r.calls.begin(), r.calls.end(), "notify:24576") ==
                  r.calls.end() && r.calls.back() == "destroy_iterator",
          "unready actor notification and iterator cleanup");
    r = TransitionRuntime{};
    r.null_iterator = true;
    Check(pal4::QuickUsePropertySequenceTask::CreateNext(kTask, r) == 0u &&
              r.allocation_index == 0u,
          "null iterator blocks follow-up action");
    r = TransitionRuntime{};
    r.actors = {0x5000u};
    r.throw_on_current = true;
    bool caught = false;
    try {
        (void)pal4::QuickUsePropertySequenceTask::CreateNext(kTask, r);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && r.calls.back() == "destroy_iterator",
          "iterator cleanup on callback exception");
}

}  // namespace

int main() {
    try {
        FactoriesAndTargetFields();
        DirectAction();
        CastActionWithRun();
        ThrowActionWithoutRun();
        FollowUpAction();
        QuickStartAndFollowUp();
        std::cout << "Combat UseProperty sequence start tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
