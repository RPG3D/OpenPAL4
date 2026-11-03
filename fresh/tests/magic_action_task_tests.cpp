#include "pal4/magic_action_task.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;

struct Events final : pal4::IEventSubscriptionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string>& order;
    struct Registration { std::uint32_t router, mode, key, owner, callback; };
    std::vector<Registration> registrations;
    std::vector<std::uint32_t> released;
    std::vector<std::uint32_t> freed;
    std::uint32_t group = 0;
    int routers = 0;
    bool native_clear = false;
    bool TryClearOwnedSlots(std::uint32_t owner, std::uint32_t offset, std::uint32_t count) override {
        if (!native_clear) return false;
        Check(owner == kTask && offset == 216u && count == 5u, "native token cleanup range");
        order.push_back("native_clear");
        return true;
    }

    explicit Events(std::vector<std::string>& shared) : order(shared) {
        for (std::size_t i = 0; i <
             pal4::CombatActionHandlers::kBindings.size(); ++i) {
            const auto address =
                pal4::CombatActionHandlers::kBindings[i].key_address;
            words[address] = 0x6000u + static_cast<std::uint32_t>(i);
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t EventKey(std::string_view) override {
        throw std::runtime_error("global event keys must be read directly");
    }
    std::uint32_t EventRouter() override {
        ++routers;
        return 0x7000u;
    }
    std::uint32_t Register(std::uint32_t router, std::uint32_t mode,
                           std::uint32_t key, std::uint32_t owner,
                           std::uint32_t callback) override {
        registrations.push_back({router, mode, key, owner, callback});
        order.emplace_back("register");
        return 0xa000u + static_cast<std::uint32_t>(registrations.size());
    }
    std::uint32_t FindEventGroup(std::uint32_t router,
                                 std::uint32_t key) override {
        Check(router == 0x7000u && key == 0x6000u, "old event group key");
        return group;
    }
    void ReleaseOwnedSlot(std::uint32_t slot) override {
        released.push_back(slot);
        order.emplace_back("release");
    }
    void FreeNode(std::uint32_t node) override {
        freed.push_back(node);
        order.emplace_back("free");
    }
};

struct Cast final : pal4::ICastMagicTaskRuntime {
    std::vector<std::string>& order;
    std::uint32_t embedded = 0;
    std::uint32_t target = 0;
    explicit Cast(std::vector<std::string>& shared) : order(shared) {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == kTask + 256u + 60u) return 0x5000u;
        if (address == kTask + 256u + 64u) return 0x5100u;
        if (address == kTask + 256u + 68u) return 0x5200u;
        if (address == 0x5200u + 76u) return 11u;
        throw std::runtime_error("unexpected cast read");
    }
    void WriteScale(std::uint32_t, float) override {}
    void SetActorTarget(std::uint32_t actor, std::uint32_t magic,
                        std::uint32_t selected) override {
        Check(actor == 0x5000u && magic == 0x5200u,
              "embedded cast actor and magic");
        target = selected;
        order.emplace_back("cast_target");
    }
    int ChangeActorValue(std::uint32_t actor, std::uint32_t field,
                         std::int32_t delta) override {
        Check(actor == 0x5000u && field == 9u && delta == -11,
              "embedded cast cost");
        order.emplace_back("cast_cost");
        return 0;
    }
    std::uint32_t MagicMode(std::uint32_t) override { return 0u; }
    float DamageScale(std::uint32_t) override { return 0.0f; }
    std::uint32_t TargetIterator(std::uint32_t) override { return 0u; }
    bool IteratorAtEnd(std::uint32_t) override { return true; }
    std::uint32_t IteratorCurrent(std::uint32_t) override { return 0u; }
    void IteratorNext(std::uint32_t) override {}
    int ReleaseIterator(std::uint32_t) override { return 0; }
    bool ActorEligible(std::uint32_t) override { return false; }
    int ActorProperty3(std::uint32_t) override { return 0; }
    int MagicRejectsSelf(std::uint32_t) override { return 0; }
    float ReadFloat(std::uint32_t) override { return 0.0f; }
    bool CheckDodge(std::uint32_t, std::uint32_t, float) override {
        return false;
    }
    int CallActor(std::uint32_t, std::uint32_t, std::uint32_t,
                  std::uint32_t) override { return 0; }
};

struct Actions final : pal4::IMagicActionTaskRuntime {
    std::vector<std::string>& order;
    std::uint32_t written_address = 0, written_value = 0;
    std::uint32_t frame = 0;
    std::uint32_t attack_task = 0;
    std::string animated;
    std::string cleaned;
    explicit Actions(std::vector<std::string>& shared) : order(shared) {}
    void StartAction(std::uint32_t task) override {
        Check(task == kTask, "start task");
        order.emplace_back("start");
    }
    void StopAction(std::uint32_t task,
                    std::uint32_t context) override {
        Check(task == kTask, "stop task");
        frame = context;
        order.emplace_back("stop");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        written_address = address; written_value = value;
        order.emplace_back("vtable");
    }
    void DestroyCastTask(std::uint32_t task) override {
        Check(task == kTask + 256u, "embedded cast offset");
        order.emplace_back("destroy_cast");
    }
    void DestroyBase(std::uint32_t task) override {
        Check(task == kTask, "base destructor owner");
        order.emplace_back("destroy_base");
    }
    void ProcessAttack(std::uint32_t task) override {
        attack_task = task;
        order.emplace_back("attack");
    }
    void ProcessAnimationCommand(std::uint32_t task,
                                 std::string_view name) override {
        Check(task == kTask, "animation command task");
        animated = name;
        order.emplace_back("animation");
    }
    int CleanupCommand(std::string_view lower) override {
        cleaned = lower;
        order.emplace_back("cleanup_command");
        return -9;
    }
};

void SubscriptionAndCleanup() {
    std::vector<std::string> order;
    Events events(order);
    for (std::size_t index = 0; index < 5u; ++index) {
        const auto& binding = pal4::CombatActionHandlers::kBindings[index];
        Check(events.words[binding.key_address] == 0x6000u + index,
              "global key binding");
    }
    Check(pal4::CombatActionHandlers::Subscribe(kTask, events) == 0xa005u &&
          events.registrations.size() == 5u && events.routers == 5,
          "five registrations and last handle");
    for (std::size_t i = 0; i < 5u; ++i) {
        const auto& record = events.registrations[i];
        Check(record.router == 0x7000u && record.mode == 2u &&
              record.key == 0x6000u + i && record.owner == kTask &&
              record.callback ==
                  pal4::CombatActionHandlers::kBindings[i].callback &&
              events.words[kTask + 216u + 8u * i] == 0xa001u + i &&
              events.words[kTask + 220u + 8u * i] == 0x6000u + i,
              "registered handle/key and callback ordering");
    }

    // One live callback node, one missing group, and three empty slots.
    events.words[kTask + 216u] = 0xaaaa;
    events.words[kTask + 220u] = 0x6000u;
    events.words[kTask + 224u] = 0u;
    events.words[kTask + 232u] = 0u;
    events.words[kTask + 240u] = 0u;
    events.words[kTask + 248u] = 0u;
    events.group = 0x8000u;
    events.words[0x8004u] = 0x9000u;
    events.words[0x9000u] = 0x9100u;
    events.words[0x9100u] = 0x9000u;
    events.words[0x9104u] = 0x9000u;
    events.words[0x910cu] = 0xaaaau;
    events.words[0x9004u] = 0x9100u;
    events.words[0x8008u] = 1u;
    pal4::CombatActionHandlers::Clear(kTask, events);
    Check(events.released == std::vector<std::uint32_t>{0x9108u} &&
          events.freed == std::vector<std::uint32_t>{0x9100u} &&
          events.words[0x8008u] == 0u &&
          events.words[0x9000u] == 0x9000u &&
          events.words[kTask + 216u] == 0u &&
          events.words[kTask + 220u] == 0u,
          "clear unlinks and releases matching callback");
}

void LifecycleAndCommands() {
    std::vector<std::string> order;
    Events events(order);
    Cast cast(order);
    Actions actions(order);
    Check(pal4::MagicActionTask::Process(kTask, actions, events, cast) == 1u &&
          cast.target == 0x5100u &&
          order.front() == "start" &&
          order[1] == "register" &&
          order[5] == "register" &&
          order[6] == "cast_target" &&
          order[7] == "cast_cost",
          "process starts, subscribes and sets embedded cast target");
    order.clear();
    // Avoid a registered-node lookup when testing task-level stop order.
    for (std::size_t i = 0; i < 5u; ++i)
        events.words[kTask + 216u + 8u * i] = 0u;
    Check(pal4::MagicActionTask::Stop(kTask, 0x1234u, actions, events) == 1u &&
          actions.frame == 0x1234u && order == std::vector<std::string>{"stop"},
          "stop clears handlers before forwarding frame context");
    order.clear();
    pal4::MagicActionTask::Destroy(kTask, actions, events);
    Check(actions.written_address == kTask &&
          actions.written_value == pal4::MagicActionTask::kVtable &&
          order == std::vector<std::string>{"vtable", "destroy_cast",
              "destroy_base"},
          "destructor restores vtable, clears handlers and destroys bases");

    order.clear();
    Check(pal4::MagicActionTask::ProcessCommand(
              kTask, "AtKMore", actions) == -9 &&
          actions.attack_task == kTask && actions.cleaned == "atkmore" &&
          order == std::vector<std::string>{"attack", "cleanup_command"},
          "case-insensitive atk prefix calls task virtual action");
    order.clear();
    Check(pal4::MagicActionTask::ProcessCommand(
              kTask, "At", actions) == -9 &&
          actions.animated == "At" && actions.cleaned == "at" &&
          order == std::vector<std::string>{"animation", "cleanup_command"},
          "short command uses original event name");
}

}  // namespace

int main() {
    try {
        SubscriptionAndCleanup();
        LifecycleAndCommands();
        {
            std::vector<std::string> order; Events events(order); Actions actions(order);
            events.native_clear = true;
            Check(pal4::MagicActionTask::Stop(kTask, 0x1234u, actions, events) == 1u &&
                  events.routers == 0 && order == std::vector<std::string>{"native_clear", "stop"},
                  "native token cleanup precedes base stop without legacy tree traversal");
        }
        std::cout << "magic_action_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
