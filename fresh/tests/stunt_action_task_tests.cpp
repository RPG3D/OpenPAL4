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
constexpr std::uint32_t kActor = 0x5000u;
constexpr std::uint32_t kTarget = 0x6000u;
constexpr std::uint32_t kStunt = 0x7000u;

struct Events final : pal4::IEventSubscriptionRuntime {
    struct Registration {
        std::uint32_t router, mode, key, owner, callback;
    };
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Registration> registered;
    std::vector<std::string>& order;
    explicit Events(std::vector<std::string>& shared) : order(shared) {
        for (std::size_t i = 0;
             i < pal4::StuntActionHandlers::kBindings.size(); ++i)
            words[pal4::StuntActionHandlers::kBindings[i].key_address] =
                0x8000u + static_cast<std::uint32_t>(i);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t EventKey(std::string_view) override {
        throw std::runtime_error("IDA uses key globals");
    }
    std::uint32_t EventRouter() override { return 0x9000u; }
    std::uint32_t Register(std::uint32_t router, std::uint32_t mode,
                           std::uint32_t key, std::uint32_t owner,
                           std::uint32_t callback) override {
        registered.push_back({router, mode, key, owner, callback});
        order.emplace_back("register");
        return 0xa000u + static_cast<std::uint32_t>(registered.size());
    }
    std::uint32_t FindEventGroup(std::uint32_t,
                                 std::uint32_t) override {
        return 0u;
    }
    void ReleaseOwnedSlot(std::uint32_t) override {}
    void FreeNode(std::uint32_t) override {}
};

struct ActionRuntime final : pal4::IStuntActionTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 60u, kActor}, {kTask + 72u, 4u},
        {kTask + 320u, 0x1234u}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string>& order;
    std::string task_animation = "cast";
    std::string actor_animation = "cast";
    bool animation_flag = true;
    bool pending = true;
    std::uint32_t state_actor = 0u;
    std::uint32_t state_value = 0xffffffffu;
    std::uint32_t reset_actor = 0u;
    std::uint32_t destroyed_embedded = 0u;
    std::uint32_t stop_context = 0u;
    std::string cleaned_command;
    std::string forwarded_name;
    int animations = 0;
    bool throw_on_animation = false;

    explicit ActionRuntime(std::vector<std::string>& shared) : order(shared) {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        order.emplace_back("write_word");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        order.emplace_back("read_byte");
        return bytes[address];
    }
    std::string_view TaskAnimationName(std::uint32_t address,
                                       std::uint32_t length) override {
        Check(address == kTask + 68u && length == task_animation.size(),
              "animation text reads stored length");
        return task_animation;
    }
    std::string_view ActorAnimationName(std::uint32_t actor) override {
        Check(actor == kActor, "animation name uses task actor");
        return actor_animation;
    }
    bool ActorAnimationFlag(std::uint32_t actor) override {
        Check(actor == kActor, "animation flag uses task actor");
        order.emplace_back("flag");
        return animation_flag;
    }
    void ActorSetState(std::uint32_t actor,
                       std::uint32_t state) override {
        state_actor = actor;
        state_value = state;
        order.emplace_back("state");
    }
    void ActorReset(std::uint32_t actor) override {
        reset_actor = actor;
        order.emplace_back("actor_reset");
    }
    void StartAction(std::uint32_t task) override {
        Check(task == kTask, "start receives task");
        order.emplace_back("start");
    }
    std::uint8_t StopAction(std::uint32_t task,
                            std::uint32_t context) override {
        Check(task == kTask, "stop receives task");
        stop_context = context;
        order.emplace_back("stop");
        return 0xabu;
    }
    bool IsActionPending(std::uint32_t task) override {
        Check(task == kTask, "status receives task");
        order.emplace_back("pending");
        return pending;
    }
    void DestroyEmbedded(std::uint32_t task) override {
        destroyed_embedded = task;
        order.emplace_back("destroy_embedded");
    }
    void DestroyBase(std::uint32_t task) override {
        Check(task == kTask, "base destructor receives task");
        order.emplace_back("destroy_base");
    }
    void ProcessAnimationCommand(std::uint32_t task,
                                 std::string_view name) override {
        Check(task == kTask, "animation command receives task");
        forwarded_name = name;
        ++animations;
        if (throw_on_animation)
            throw std::runtime_error("animation failure");
    }
    int CleanupCommand(std::string_view lowercase) override {
        cleaned_command = lowercase;
        return -7;
    }
};

struct UnusedProcess final : pal4::IStuntActionProcessRuntime {
    int reads = 0;
    int immediate = 0;
    std::uint8_t completion = 255u;
    bool throw_on_prepare = false;
    std::uint32_t ReadWord(std::uint32_t) override {
        ++reads;
        return 0u;
    }
    void WriteByte(std::uint32_t, std::uint8_t value) override {
        completion = value;
    }
    void WriteFloat(std::uint32_t, float) override {}
    std::string_view StuntName(std::uint32_t) override { return {}; }
    std::uint32_t StuntTargetKind(std::uint32_t) override { return 0u; }
    bool ActorReady(std::uint32_t) override { return false; }
    std::uint32_t EmptyEffect() override { return 0u; }
    std::uint32_t ExecuteEffect(std::uint32_t, std::uint32_t,
                                std::uint32_t) override { return 0u; }
    bool NeedsUpdate(std::uint32_t) override { return false; }
    void BindEffect(std::uint32_t, std::string_view,
                    std::uint32_t) override {}
    void ReleaseEffect(std::uint32_t) override {}
    std::uint32_t ConfigValue(std::uint32_t,
                              std::string_view) override { return 0u; }
    int ResetAnimationEffect(std::uint32_t) override { return 0; }
    std::uint32_t EventActor(std::uint32_t) override { return 0u; }
    std::uint32_t CallActor(std::uint32_t, std::uint32_t,
                            std::uint32_t) override { return 0u; }
    void PrepareImmediateStunt() override {
        ++immediate;
        if (throw_on_prepare)
            throw std::runtime_error("stunt failure");
    }
    std::uint32_t TargetIterator(std::uint32_t) override { return 0u; }
    bool IteratorEmpty(std::uint32_t) override { return true; }
    bool IteratorAtEnd(std::uint32_t) override { return true; }
    std::uint32_t IteratorCurrent(std::uint32_t) override { return 0u; }
    void IteratorNext(std::uint32_t) override {}
    int ReleaseIterator(std::uint32_t) override { return 0; }
};

struct StuntRuntime final : pal4::IStuntTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 256u + 60u, kActor},
        {kTask + 256u + 64u, kTarget},
        {kTask + 256u + 68u, kStunt},
        {kStunt + 116u, 5u}};
    std::vector<std::string>& order;
    std::uint32_t source = 0u;
    std::int32_t cost = 0;
    explicit StuntRuntime(std::vector<std::string>& shared) : order(shared) {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteFloat(std::uint32_t, float) override {}
    void SetStuntTarget(std::uint32_t actor, std::uint32_t stunt,
                        std::uint32_t target) override {
        Check(stunt == kStunt && target == kTarget,
              "embedded stunt receives target and stunt");
        source = actor;
        order.emplace_back("stunt_target");
    }
    void AdjustStuntCost(std::uint32_t actor, std::uint32_t mode,
                         std::int32_t amount) override {
        Check(actor == kActor && mode == 8u,
              "embedded stunt applies source cost");
        cost = amount;
        order.emplace_back("stunt_cost");
    }
    void AdjustActorValue(std::uint32_t, std::int32_t) override {}
    std::uint32_t ApplyActorEffect(std::uint32_t, std::uint32_t,
                                   std::int32_t) override { return 0u; }
    std::uint32_t StuntTargetKind(std::uint32_t) override { return 0u; }
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
    std::uint32_t ActorProperty(std::uint32_t,
                                std::uint32_t) override { return 0u; }
    std::uint32_t ActorReady(std::uint32_t) override { return 0u; }
    std::uint32_t StuntIncludesSelf(std::uint32_t) override { return 0u; }
    void ApplyTargetHit(std::uint32_t, std::uint32_t,
                        std::uint32_t) override {}
    void NotifySource(std::uint32_t, std::uint32_t,
                      std::uint32_t) override {}
    std::uint32_t SetTargetState12(std::uint32_t,
                                   std::uint32_t) override { return 0u; }
    std::uint32_t SetTargetStateOne(std::uint32_t) override { return 0u; }
};

void SubscriptionAndProcess() {
    std::vector<std::string> order;
    Events events(order);
    ActionRuntime runtime(order);
    StuntRuntime stunt(order);
    Check(pal4::StuntActionTask::Process(kTask, runtime, events, stunt) ==
              1u && events.registered.size() == 5u &&
          stunt.source == kActor && stunt.cost == -5,
          "process subscribes five events and sets embedded stunt target");
    for (std::size_t i = 0; i < events.registered.size(); ++i) {
        const auto& binding = pal4::StuntActionHandlers::kBindings[i];
        const auto& registered = events.registered[i];
        Check(registered.router == 0x9000u && registered.mode == 2u &&
              registered.key == 0x8000u + i &&
              registered.owner == kTask &&
              registered.callback == binding.callback &&
              events.words[kTask + 216u + 8u * i] == 0xa001u + i &&
              events.words[kTask + 220u + 8u * i] == 0x8000u + i,
              "all event callbacks and slots match IDA order");
    }
    Check(order[4] == "register" && order[5] == "start" &&
          order[6] == "stunt_target",
          "subscriptions precede action start and embedded target setup");
    Check(pal4::StuntActionTask::Stop(kTask, 0xdeadbeefu,
                                      runtime, events) == 0xabu &&
          runtime.reset_actor == kActor &&
          runtime.stop_context == 0xdeadbeefu &&
          order[order.size() - 2] == "actor_reset" &&
          order.back() == "stop",
          "stop clears handlers, resets actor and forwards frame context");
    for (std::size_t i = 0; i < 5u; ++i)
        Check(events.words[kTask + 216u + 8u * i] == 0u &&
              events.words[kTask + 220u + 8u * i] == 0u,
              "stop clears every event slot");
    pal4::StuntActionTask::Destroy(kTask, runtime, events);
    Check(runtime.words[kTask] == pal4::StuntActionTask::kVtable &&
          runtime.destroyed_embedded == kTask + 256u &&
          order[order.size() - 2] == "destroy_embedded" &&
          order.back() == "destroy_base",
          "destructor installs vtable and destroys embedded task then base");
}

void StateAndCommands() {
    std::vector<std::string> order;
    ActionRuntime runtime(order);
    UnusedProcess process;
    StuntRuntime stunt(order);
    runtime.bytes[kTask + 332u] = 1u;
    Check(pal4::StuntActionTask::CheckStatus(kTask, runtime),
          "pending action and byte one report ready");
    runtime.pending = false;
    order.clear();
    Check(!pal4::StuntActionTask::CheckStatus(kTask, runtime) &&
          order == std::vector<std::string>{"pending"},
          "pending false short circuits completion byte");
    runtime.pending = true;
    runtime.bytes[kTask + 332u] = 0u;
    Check(!pal4::StuntActionTask::CheckStatus(kTask, runtime),
          "zero completion byte reports false");
    Check(pal4::StuntActionTask::GetActorProperty(kTask, runtime) ==
              0x1234u,
          "actor property getter returns task +320");

    Check(pal4::StuntActionTask::UpdateActorState(kTask, runtime) == 1u &&
          runtime.state_actor == kActor && runtime.state_value == 0u,
          "matching animation with flag clears actor state");
    runtime.state_actor = 0u;
    runtime.actor_animation = "cast_more";
    Check(pal4::StuntActionTask::UpdateActorState(kTask, runtime) == 1u &&
          runtime.state_actor == 0u,
          "matching prefix with different length does not clear state");
    runtime.actor_animation = "cast";
    runtime.animation_flag = false;
    Check(pal4::StuntActionTask::UpdateActorState(kTask, runtime) == 1u &&
          runtime.state_actor == 0u,
          "matching animation without flag does not clear state");

    Check(pal4::StuntActionTask::ProcessCommand(kTask, "At",
                                                runtime, process, stunt) == -7 &&
          runtime.animations == 1 &&
          runtime.forwarded_name == "At" &&
          runtime.cleaned_command == "at",
          "short command falls back with original name");
    Check(process.reads == 0,
          "non-attack command does not enter stunt processing");
    Check(pal4::StuntActionTask::ProcessCommand(
              kTask, "ATK_hit", runtime, process, stunt) == -7 &&
          process.reads > 0 && process.immediate == 1 &&
          process.completion == 1u &&
          runtime.cleaned_command == "atk_hit",
          "attack prefix composes reconstructed stunt processing");

    runtime.throw_on_animation = true;
    runtime.cleaned_command.clear();
    bool animation_threw = false;
    try {
        (void)pal4::StuntActionTask::ProcessCommand(
            kTask, "Walk", runtime, process, stunt);
    } catch (const std::runtime_error&) {
        animation_threw = true;
    }
    Check(animation_threw && runtime.cleaned_command == "walk",
          "animation exception cleans lowercase command");
    runtime.throw_on_animation = false;
    process.throw_on_prepare = true;
    runtime.cleaned_command.clear();
    bool stunt_threw = false;
    try {
        (void)pal4::StuntActionTask::ProcessCommand(
            kTask, "ATK", runtime, process, stunt);
    } catch (const std::runtime_error&) {
        stunt_threw = true;
    }
    Check(stunt_threw && runtime.cleaned_command == "atk",
          "stunt exception cleans lowercase command");
}

}  // namespace

int main() {
    try {
        SubscriptionAndProcess();
        StateAndCommands();
        std::cout << "stunt_action_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
