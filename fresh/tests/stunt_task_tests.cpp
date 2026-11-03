#include "pal4/stunt_task.h"

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
constexpr std::uint32_t kSource = 0x5000u;
constexpr std::uint32_t kTarget = 0x6000u;
constexpr std::uint32_t kStunt = 0x7000u;

struct Runtime final : pal4::IStuntTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 60u, kSource}, {kTask + 64u, kTarget},
        {kTask + 68u, kStunt}};
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> candidates;
    std::vector<std::uint32_t> hit_targets;
    std::vector<std::uint32_t> effect_types;
    std::vector<std::int32_t> effect_amounts;
    std::size_t index = 0u;
    std::uint32_t kind = 1u;
    std::uint32_t property_result = 1u;
    std::uint32_t ready_result = 1u;
    std::uint32_t includes_self = 0u;
    std::uint32_t state12_result = 0x43u;
    std::uint32_t state_one_result = 0x34u;
    std::uint32_t last_target = 0u;
    std::uint32_t last_stunt = 0u;
    std::int32_t stunt_cost = 0;
    std::int32_t actor_value = 0;
    int iterator_destroys = 0;
    bool throw_on_hit = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words[address];
    }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        calls.emplace_back("write_float");
    }
    void SetStuntTarget(std::uint32_t source, std::uint32_t stunt,
                        std::uint32_t target) override {
        Check(source == kSource && stunt == kStunt,
              "source slot 520 receives source and stunt");
        last_target = target;
        last_stunt = stunt;
        calls.emplace_back("set_target");
    }
    void AdjustStuntCost(std::uint32_t source, std::uint32_t type,
                         std::int32_t amount) override {
        Check(source == kSource && type == 8u,
              "source slot 84 uses type eight");
        stunt_cost = amount;
        calls.emplace_back("cost");
    }
    void AdjustActorValue(std::uint32_t source,
                          std::int32_t amount) override {
        Check(source == kSource, "source slot 368 uses task source");
        actor_value = amount;
        calls.emplace_back("actor_value");
    }
    std::uint32_t ApplyActorEffect(std::uint32_t source,
                                   std::uint32_t type,
                                   std::int32_t amount) override {
        Check(source == kSource, "source slot 360 uses task source");
        effect_types.push_back(type);
        effect_amounts.push_back(amount);
        calls.emplace_back("effect");
        return 0x12345678u;
    }
    std::uint32_t StuntTargetKind(std::uint32_t stunt) override {
        Check(stunt == kStunt, "kind uses task stunt");
        calls.emplace_back("kind");
        return kind;
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        return 0x8000u;
    }
    float FloatArrayElement(std::uint32_t array,
                            std::uint32_t element) override {
        Check(array == 0x8000u + 184u && element == 26u,
              "group scale reads database float array index 26");
        calls.emplace_back("group_scale");
        return 2.5f;
    }
    std::uint32_t TargetIterator(std::uint32_t target) override {
        Check(target == kTarget, "iterator uses target collection");
        index = 0u;
        calls.emplace_back("iterator");
        return 0x9000u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "iterator end uses created object");
        calls.emplace_back("at_end");
        return index == candidates.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(iterator == 0x9000u && index < candidates.size(),
              "iterator current uses valid position");
        calls.emplace_back("current");
        return candidates[index];
    }
    void IteratorNext(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "iterator next uses created object");
        ++index;
        calls.emplace_back("next");
    }
    std::uint32_t DestroyIterator(std::uint32_t iterator) override {
        Check(iterator == 0x9000u, "destroy uses created iterator");
        ++iterator_destroys;
        calls.emplace_back("destroy_iterator");
        return 0x12345678u;
    }
    bool CandidateEligible(std::uint32_t actor) override {
        calls.emplace_back("eligible");
        return actor == kTarget;
    }
    std::uint32_t ActorProperty(std::uint32_t,
                                std::uint32_t mode) override {
        Check(mode == 2u, "property slot 12 uses mode two");
        calls.emplace_back("property");
        return property_result;
    }
    std::uint32_t ActorReady(std::uint32_t) override {
        calls.emplace_back("ready");
        return ready_result;
    }
    std::uint32_t StuntIncludesSelf(std::uint32_t stunt) override {
        Check(stunt == kStunt, "self predicate uses task stunt");
        calls.emplace_back("includes_self");
        return includes_self;
    }
    void ApplyTargetHit(std::uint32_t target, std::uint32_t source,
                        std::uint32_t stunt) override {
        Check(source == kSource && stunt == kStunt,
              "target slot 492 receives source and stunt");
        calls.emplace_back("hit");
        hit_targets.push_back(target);
        if (throw_on_hit) throw std::runtime_error("hit failed");
    }
    void NotifySource(std::uint32_t source, std::uint32_t value,
                      std::uint32_t target) override {
        Check(source == kSource && value == words[kStunt + 112u] &&
              target != 0u,
              "source slot 264 receives stunt field and target");
        calls.emplace_back("notify");
    }
    std::uint32_t SetTargetState12(std::uint32_t target,
                                   std::uint32_t source) override {
        Check(target != 0u && source == kSource,
              "target slot 260 receives mode twelve and source");
        calls.emplace_back("state12");
        return state12_result;
    }
    std::uint32_t SetTargetStateOne(std::uint32_t target) override {
        Check(target != kSource, "target slot 396 excludes source");
        calls.emplace_back("state_one");
        return state_one_result;
    }
};

void TargetAndEffects() {
    Runtime runtime;
    runtime.words[kStunt + 116u] = 7u;
    runtime.words[kStunt + 120u] = 9u;
    runtime.words[kStunt + 72u] = 3u;
    runtime.words[kStunt + 92u] = 20u;
    runtime.words[kStunt + 88u] = 4u;
    runtime.words[kStunt + 108u] = 8u;
    Check(pal4::StuntTask::SetTarget(kTask, runtime) == 0x12345678u &&
          runtime.last_target == kTarget && runtime.last_stunt == kStunt &&
          runtime.stunt_cost == -7 && runtime.actor_value == -9 &&
          runtime.effect_types == std::vector<std::uint32_t>{3u, 4u} &&
          runtime.effect_amounts == std::vector<std::int32_t>{-20, -8},
          "set target applies cost, actor value and five-field effects");
    const auto set_target = std::find(runtime.calls.begin(),
                                      runtime.calls.end(), "set_target");
    const auto cost = std::find(runtime.calls.begin(),
                                runtime.calls.end(), "cost");
    const auto value = std::find(runtime.calls.begin(),
                                 runtime.calls.end(), "actor_value");
    Check(set_target < cost && cost < value,
          "stunt target, cost and actor value retain order");

    runtime.words[kStunt + 88u] = 0u;
    Check(pal4::StuntTask::ApplyEffects(kTask, runtime) == kStunt,
          "last empty field returns stunt pointer after earlier effects");
}

void SingleTargetGatesAndModes() {
    Runtime runtime;
    Check(pal4::StuntTask::HandleActor(kTask, runtime) == 0x34u &&
          runtime.floats[kTask + 72u] == 1.0f &&
          runtime.hit_targets == std::vector<std::uint32_t>{kTarget} &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "state_one") != runtime.calls.end(),
          "single target uses scale one and state-one branch");
    runtime.kind = 2u;
    runtime.calls.clear();
    Check(pal4::StuntTask::HandleActorInternal(kTask, kTarget, runtime) ==
              0x43u && runtime.calls.back() == "state12",
          "kind two uses state-twelve branch");
    runtime.property_result = 0x100u;
    runtime.calls.clear();
    Check(pal4::StuntTask::HandleActorInternal(kTask, kTarget, runtime) ==
              0u && runtime.calls == std::vector<std::string>{"property"},
          "property gate checks only low byte");
    runtime.property_result = 1u;
    runtime.ready_result = 0u;
    runtime.calls.clear();
    Check(pal4::StuntTask::HandleActorInternal(kTask, kTarget, runtime) ==
              0u && runtime.calls ==
                  std::vector<std::string>{"property", "ready"},
          "unready actor exits before stunt checks");
    runtime.ready_result = 1u;
    runtime.includes_self = 1u;
    runtime.calls.clear();
    Check(pal4::StuntTask::HandleActorInternal(kTask, kSource, runtime) ==
              1u &&
          std::find(runtime.calls.begin(), runtime.calls.end(), "hit") ==
              runtime.calls.end(),
          "self-target predicate suppresses hit");
    runtime.words[kTask + 60u] = 0u;
    runtime.calls.clear();
    Check(pal4::StuntTask::HandleActor(kTask, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"read"},
          "missing source exits before target or stunt reads");
}

void GroupTraversalAndCleanup() {
    Runtime runtime;
    runtime.kind = 4u;
    runtime.candidates = {kTarget, 0x6100u};
    Check(pal4::StuntTask::HandleActor(kTask, runtime) == 0x78u &&
          runtime.floats[kTask + 72u] == 2.5f &&
          runtime.hit_targets == std::vector<std::uint32_t>{kTarget} &&
          runtime.iterator_destroys == 1 &&
          runtime.calls.back() == "destroy_iterator",
          "group traversal filters candidates and returns destructor byte");
    Runtime failing;
    failing.kind = 3u;
    failing.candidates = {kTarget};
    failing.throw_on_hit = true;
    bool threw = false;
    try {
        (void)pal4::StuntTask::HandleActor(kTask, failing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing.iterator_destroys == 1 &&
          failing.calls.back() == "destroy_iterator",
          "group iterator releases when target processing throws");
}

void ProcessOrder() {
    Runtime runtime;
    runtime.words[kStunt + 116u] = 1u;
    Check(pal4::StuntTask::Process(kTask, runtime) == 1u &&
          runtime.last_target == kTarget &&
          runtime.hit_targets == std::vector<std::uint32_t>{kTarget} &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "set_target") <
              std::find(runtime.calls.begin(), runtime.calls.end(), "hit"),
          "process runs target setup before actor handling");
}

}  // namespace

int main() {
    try {
        TargetAndEffects();
        SingleTargetGatesAndModes();
        GroupTraversalAndCleanup();
        ProcessOrder();
        std::cout << "stunt_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
