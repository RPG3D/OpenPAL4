#include "pal4/cast_magic_task.h"

#include <algorithm>
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

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kActor = 0x5000u;
constexpr std::uint32_t kTarget = 0x6000u;
constexpr std::uint32_t kMagic = 0x7000u;

struct Runtime final : pal4::ICastMagicTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{kTask + 60u, kActor},
        {kTask + 64u, kTarget}, {kTask + 68u, kMagic}, {kMagic + 76u, 13u}};
    std::vector<std::string> order;
    std::vector<std::uint32_t> handled;
    std::vector<std::uint32_t> candidates{0x8000u, 0x8100u};
    std::size_t index = 0;
    std::uint32_t magic_mode = 1u;
    std::uint32_t set_actor = 0, set_magic = 0, set_target = 0;
    std::uint32_t changed_actor = 0, changed_field = 0;
    std::int32_t changed_delta = 0;
    std::uint32_t scale_address = 0;
    float scale = 0;
    int release_count = 0;
    bool throw_in_internal = false;
    int property3 = 1;
    int rejects_self = 0;
    bool dodge = true;
    bool replace_on_target_setup = false;
    bool replace_actor_after_hit = false;
    struct ActorCall {
        std::uint32_t actor, slot, first, second;
    };
    std::vector<ActorCall> actor_calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        order.emplace_back("read");
        return words[address];
    }
    void WriteScale(std::uint32_t address, float value) override {
        scale_address = address; scale = value;
        order.emplace_back("scale");
    }
    void SetActorTarget(std::uint32_t actor, std::uint32_t magic,
                        std::uint32_t target) override {
        set_actor = actor; set_magic = magic; set_target = target;
        order.emplace_back("set_target");
        if (replace_on_target_setup) {
            words[kTask + 60u] = 0x5100u;
            words[kTask + 68u] = 0x7100u;
            words[0x7100u + 76u] = 29u;
        }
    }
    int ChangeActorValue(std::uint32_t actor, std::uint32_t field,
                         std::int32_t delta) override {
        changed_actor = actor; changed_field = field; changed_delta = delta;
        order.emplace_back("change_value");
        return -19;
    }
    std::uint32_t MagicMode(std::uint32_t magic) override {
        Check(magic == kMagic, "magic mode source");
        order.emplace_back("mode");
        return magic_mode;
    }
    float DamageScale(std::uint32_t index_value) override {
        Check(index_value == 26u, "group magic scale index");
        order.emplace_back("db_scale");
        return 0.625f;
    }
    std::uint32_t TargetIterator(std::uint32_t target) override {
        Check(target == kTarget, "group magic iterator target");
        order.emplace_back("iterator");
        return 0x9000u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "iterator identity");
        return index == candidates.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t) override {
        return candidates.at(index);
    }
    void IteratorNext(std::uint32_t) override { ++index; }
    int ReleaseIterator(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "released iterator identity");
        ++release_count;
        order.emplace_back("release");
        return 42;
    }
    bool ActorEligible(std::uint32_t actor) override {
        return actor != 0x8100u;
    }
    int ActorProperty3(std::uint32_t) override { return property3; }
    int MagicRejectsSelf(std::uint32_t magic) override {
        Check(magic == kMagic, "self rule source");
        return rejects_self;
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address == kMagic + 84u, "dodge bonus offset");
        return 0.25f;
    }
    bool CheckDodge(std::uint32_t attacker_data,
                    std::uint32_t target_data, float bonus) override {
        Check(attacker_data == kActor + 340u && bonus == 0.25f &&
              target_data != 0u, "dodge data and bonus");
        return dodge;
    }
    int CallActor(std::uint32_t actor, std::uint32_t slot,
                  std::uint32_t first, std::uint32_t second) override {
        actor_calls.push_back({actor, slot, first, second});
        if (slot == 488u) {
            handled.push_back(actor);
            order.emplace_back("handle");
            if (throw_in_internal)
                throw std::runtime_error("injected failure");
            if (replace_actor_after_hit)
                words[kTask + 60u] = 0x5200u;
        }
        return static_cast<int>(slot);
    }
};

void TargetAndProcess() {
    Runtime runtime;
    Check(pal4::CastMagicTask::SetTarget(kTask, runtime) == -19 &&
          runtime.set_actor == kActor && runtime.set_magic == kMagic &&
          runtime.set_target == kTarget && runtime.changed_actor == kActor &&
          runtime.changed_field == 9u && runtime.changed_delta == -13,
          "set target and subtract magic cost");
    Check(std::find(runtime.order.begin(), runtime.order.end(), "set_target") <
          std::find(runtime.order.begin(), runtime.order.end(), "change_value"),
          "target is set before cost change");
    Runtime changed;
    changed.replace_on_target_setup = true;
    Check(pal4::CastMagicTask::SetTarget(kTask, changed) == -19 &&
          changed.set_actor == kActor && changed.set_magic == kMagic &&
          changed.changed_actor == 0x5100u && changed.changed_delta == -29,
          "cost and actor are reread after target callback");
    runtime.order.clear();
    Check(pal4::CastMagicTask::Process(kTask, runtime) == 1u &&
          runtime.handled == std::vector<std::uint32_t>{kTarget} &&
          runtime.scale_address == kTask + 72u &&
          runtime.scale == 1.0f, "process handles direct magic");
    Check(std::find(runtime.order.begin(), runtime.order.end(), "set_target") <
          std::find(runtime.order.begin(), runtime.order.end(), "handle"),
          "process orders target before handling");
}

void GuardsAndGroup() {
    Runtime runtime;
    runtime.words[kTask + 60u] = 0u;
    Check(pal4::CastMagicTask::HandleActor(kTask, 7, runtime) == 0 &&
          runtime.handled.empty(), "actor gate");
    runtime.words[kTask + 60u] = kActor;
    runtime.words[kTask + 64u] = 0u;
    Check(pal4::CastMagicTask::HandleActor(kTask, 7, runtime) == 0,
          "target gate");
    runtime.words[kTask + 64u] = kTarget;
    runtime.words[kTask + 68u] = 0u;
    Check(pal4::CastMagicTask::HandleActor(kTask, 7, runtime) == 0,
          "magic gate");
    runtime.words[kTask + 68u] = kMagic;
    for (auto mode : {3u, 4u}) {
        runtime.magic_mode = mode;
        runtime.index = 0;
        runtime.handled.clear();
        Check(pal4::CastMagicTask::HandleActor(kTask, 7, runtime) == 42 &&
              runtime.scale == 0.625f &&
              runtime.handled == std::vector<std::uint32_t>{0x8000u},
              "group modes use GameDB scale and eligible candidates");
    }
    Check(runtime.release_count == 2, "each group iterator releases once");
    runtime.index = 0;
    runtime.throw_in_internal = true;
    try {
        (void)pal4::CastMagicTask::HandleActor(kTask, 0, runtime);
        throw std::runtime_error("missing exception");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "injected failure",
              "handler exception propagated");
    }
    Check(runtime.release_count == 3, "exception releases iterator");
}

void EffectBranches() {
    Runtime runtime;
    runtime.property3 = 0;
    Check(pal4::CastMagicTask::HandleActorInternal(
              kTask, kTarget, 0, runtime) == 0 &&
          runtime.actor_calls.empty(), "property-three gate");
    runtime.property3 = 1;
    runtime.rejects_self = 1;
    Check(pal4::CastMagicTask::HandleActorInternal(
              kTask, kActor, 0, runtime) == 1 &&
          runtime.actor_calls.empty(), "self cast blocked by magic rule");
    runtime.rejects_self = 0;
    runtime.words[kActor + 804u] = 1u;
    runtime.words[kTarget + 804u] = 2u;
    runtime.dodge = false;
    Check(pal4::CastMagicTask::HandleActorInternal(
              kTask, kTarget, 0, runtime) == 512 &&
          runtime.actor_calls.back().actor == kActor &&
          runtime.actor_calls.back().slot == 512u &&
          runtime.actor_calls.back().first == kTarget,
          "opposing faction failed dodge calls attacker slot 512");

    runtime.actor_calls.clear();
    runtime.dodge = true;
    runtime.words[kMagic + 72u] = 0x1234u;
    runtime.words[kMagic + 4u] = 2u;
    runtime.magic_mode = 1u;
    Check(pal4::CastMagicTask::HandleActorInternal(
              kTask, kTarget, 0, runtime) == 396 &&
          runtime.actor_calls.size() == 5u &&
          runtime.actor_calls[0].slot == 488u &&
          runtime.actor_calls[0].first == kActor &&
          runtime.actor_calls[0].second == kMagic &&
          runtime.actor_calls[1].slot == 264u &&
          runtime.actor_calls[1].first == 0x1234u &&
          runtime.actor_calls[1].second == kTarget &&
          runtime.actor_calls[2].slot == 260u &&
          runtime.actor_calls[2].first == 0u &&
          runtime.actor_calls[3].slot == 260u &&
          runtime.actor_calls[3].first == 9u &&
          runtime.actor_calls[4].slot == 396u,
          "hit notifies target, attacker, category and final state");
    runtime.actor_calls.clear();
    runtime.magic_mode = 2u;
    runtime.words[kMagic + 4u] = 5u;
    Check(pal4::CastMagicTask::NotifyCastProperties(
              kTask, kTarget, runtime) == 5 &&
          runtime.actor_calls.size() == 1u &&
          runtime.actor_calls[0].first == 0u,
          "invalid category returns category after event zero");
    runtime.actor_calls.clear();
    Check(pal4::CastMagicTask::HandleActorInternal(
              kTask, kTarget, 0, runtime) == 260 &&
          runtime.actor_calls.size() == 4u &&
          runtime.actor_calls[2].first == 0u &&
          runtime.actor_calls[3].first == 12u &&
          runtime.actor_calls[3].second == kActor,
          "mode two emits event twelve and invalid category skips mapping");
    runtime.actor_calls.clear();
    runtime.magic_mode = 4u;
    Check(pal4::CastMagicTask::HandleActorInternal(
              kTask, kTarget, 0, runtime) == 260 &&
          runtime.actor_calls.back().first == 12u,
          "mode four emits event twelve");
    runtime.actor_calls.clear();
    runtime.magic_mode = 1u;
    runtime.words[kActor + 804u] = 1u;
    Check(pal4::CastMagicTask::HandleActorInternal(
              kTask, kActor, 0, runtime) == 1 &&
          runtime.actor_calls.size() == 3u &&
          runtime.actor_calls.back().slot == 260u,
          "allowed self cast skips final state callback");
    Runtime changed;
    changed.words[kMagic + 72u] = 0x1234u;
    changed.replace_actor_after_hit = true;
    Check(pal4::CastMagicTask::HandleActorInternal(
              kTask, kTarget, 0, changed) == 396 &&
          changed.actor_calls[0].first == kActor &&
          changed.actor_calls[1].actor == 0x5200u,
          "attacker is reread after target hit callback");
}

}  // namespace

int main() {
    try {
        TargetAndProcess();
        GuardsAndGroup();
        EffectBranches();
        std::cout << "cast_magic_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
