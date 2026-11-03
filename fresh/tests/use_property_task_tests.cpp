#include "pal4/use_property_task.h"

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
constexpr std::uint32_t kOther = 0x6100u;
constexpr std::uint32_t kProperty = 0x7000u;

struct Runtime final : pal4::IUsePropertyTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 60u, kSource}, {kTask + 64u, kTarget},
        {kTask + 68u, kProperty}, {kProperty, 0x31u},
        {kProperty + 148u, 0x55u}, {kSource + 804u, 1u},
        {kTarget + 804u, 2u}, {kOther + 804u, 3u}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> candidates;
    std::vector<std::uint32_t> applied;
    std::vector<std::uint32_t> notified_targets;
    std::size_t index = 0u;
    std::uint32_t kind = 1u;
    std::uint32_t property_result = 1u;
    bool needs_update = false;
    bool throw_on_apply = false;
    int effect_count = 0;
    int resets = 0;
    int iterator_destroys = 0;
    std::uint32_t consumed_type = 0u;
    std::int32_t consumed_amount = 0;
    std::uint32_t source_value = 0u;
    std::array<float, 3> effect_scale{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    void UseProperty(std::uint32_t source, std::uint32_t property,
                     std::uint32_t target) override {
        Check(source == kSource && property == kProperty &&
              target == kTarget,
              "source slot 524 receives property and target");
        calls.emplace_back("use");
    }
    void ConsumeProperty(std::uint32_t source, std::uint32_t type,
                         std::int32_t amount) override {
        Check(source == kSource, "source slot 360 receives task source");
        consumed_type = type;
        consumed_amount = amount;
        calls.emplace_back("consume");
    }
    std::uint32_t TargetProperty(std::uint32_t target,
                                 std::uint32_t mode) override {
        Check(target == kTarget && mode == 4u,
              "target property gate uses mode four");
        calls.emplace_back("target_property");
        return property_result;
    }
    std::uint32_t PropertyKind(std::uint32_t property) override {
        Check(property == kProperty, "kind uses task property");
        calls.emplace_back("kind");
        return kind;
    }
    std::uint32_t TargetIterator(std::uint32_t target) override {
        Check(target == kTarget, "group iterator uses target");
        index = 0u;
        calls.emplace_back("iterator");
        return 0x8000u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x8000u, "end probe uses created iterator");
        return index == candidates.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(iterator == 0x8000u && index < candidates.size(),
              "current uses valid iterator position");
        return candidates[index];
    }
    void IteratorNext(std::uint32_t iterator) override {
        Check(iterator == 0x8000u, "next uses created iterator");
        ++index;
    }
    void DestroyIterator(std::uint32_t iterator) override {
        Check(iterator == 0x8000u, "destroy uses created iterator");
        ++iterator_destroys;
        calls.emplace_back("destroy_iterator");
    }
    bool CandidateEligible(std::uint32_t actor) override {
        return actor != kOther;
    }
    void ApplyToTarget(std::uint32_t target, std::uint32_t source,
                       std::uint32_t property) override {
        Check(source == kSource && property == kProperty,
              "target slot 496 receives source and property");
        applied.push_back(target);
        calls.emplace_back("apply");
        if (throw_on_apply) throw std::runtime_error("apply failure");
    }
    void NotifySource(std::uint32_t source, std::uint32_t value,
                      std::uint32_t target) override {
        Check(source == kSource && target != 0u,
              "source slot 264 receives candidate");
        source_value = value;
        calls.emplace_back("notify_source");
    }
    std::array<float, 3> ActorScale(std::uint32_t actor) override {
        Check(actor != 0u, "scale uses candidate actor");
        calls.emplace_back("scale");
        return {1.0f, 1.5f, 2.0f};
    }
    pal4::PropertyTaskEffectHandle CreateChestEffect(
        std::uint32_t actor, std::string_view effect_name,
        std::string_view hook_name) override {
        Check(actor != 0u && effect_name == "EFF14" &&
              hook_name == "hook_ef_chest",
              "chest effect uses full IDA literals");
        ++effect_count;
        calls.emplace_back("create_effect");
        return {0xa000u + static_cast<std::uint32_t>(effect_count),
                0xb000u + static_cast<std::uint32_t>(effect_count)};
    }
    void ResetTemporaryEffect(
        pal4::PropertyTaskEffectHandle& effect) override {
        Check(effect.first != 0u && effect.second != 0u,
              "reset receives created pair");
        ++resets;
        calls.emplace_back("reset");
    }
    void SetEffectScale(std::uint32_t address,
                        const std::array<float, 3>& scale,
                        std::uint8_t mode) override {
        Check(address == kTask + 72u && mode == 0u,
              "effect scale uses embedded handle and mode zero");
        effect_scale = scale;
        calls.emplace_back("effect_scale");
    }
    void NotifyTarget(std::uint32_t target,
                      std::uint32_t source) override {
        Check(source == kSource, "target slot 340 receives source");
        notified_targets.push_back(target);
        calls.emplace_back("notify_target");
    }
    bool NeedsUpdate(std::uint32_t address) override {
        Check(address == kTask + 72u, "status polls embedded effect");
        return needs_update;
    }
};

void GateAndStatus() {
    for (const std::uint32_t missing : {60u, 64u, 68u}) {
        Runtime runtime;
        runtime.words[kTask + missing] = 0u;
        Check(pal4::UsePropertyTask::Process(kTask, runtime) == 1u &&
              runtime.effect_count == 0 &&
              runtime.consumed_type == 0u,
              "missing source, target or property exits before use");
    }
    Runtime runtime;
    runtime.property_result = 0x100u;
    pal4::UsePropertyTask::ProcessInternal(kTask, runtime);
    Check(runtime.consumed_type == 0x31u &&
          runtime.consumed_amount == -1 &&
          runtime.applied.empty(),
          "target property gate checks low byte after consumption");
    Check(pal4::UsePropertyTask::CheckStatus(kTask, runtime),
          "effect without pending update is complete");
    runtime.needs_update = true;
    Check(!pal4::UsePropertyTask::CheckStatus(kTask, runtime),
          "pending effect reports incomplete");
}

void DirectEffectAndStateGate() {
    Runtime runtime;
    Check(pal4::UsePropertyTask::Process(kTask, runtime) == 1u &&
          runtime.applied == std::vector<std::uint32_t>{kTarget} &&
          runtime.source_value == 0x55u &&
          runtime.effect_count == 1 && runtime.resets == 1 &&
          runtime.words[kTask + 72u] == 0xa001u &&
          runtime.words[kTask + 76u] == 0xb001u &&
          runtime.effect_scale ==
              std::array<float, 3>{1.0f, 1.5f, 2.0f} &&
          runtime.notified_targets ==
              std::vector<std::uint32_t>{kTarget},
          "direct property updates target and creates scaled chest effect");
    runtime.words[kTarget + 804u] = 1u;
    runtime.effect_count = 0;
    runtime.notified_targets.clear();
    pal4::UsePropertyTask::ProcessInternal(kTask, runtime);
    Check(runtime.effect_count == 0 &&
          runtime.notified_targets.empty() &&
          runtime.applied.size() == 2u,
          "matching actor states skip visual effect after property application");
}

void GroupTraversalAndException() {
    Runtime runtime;
    runtime.kind = 3u;
    runtime.candidates = {kOther, kTarget};
    pal4::UsePropertyTask::ProcessInternal(kTask, runtime);
    Check(runtime.applied == std::vector<std::uint32_t>{kTarget} &&
          runtime.effect_count == 1 && runtime.iterator_destroys == 1 &&
          runtime.calls.back() == "destroy_iterator",
          "group branch filters candidates and releases iterator");

    Runtime failing;
    failing.kind = 4u;
    failing.candidates = {kTarget};
    failing.throw_on_apply = true;
    bool threw = false;
    try {
        pal4::UsePropertyTask::ProcessInternal(kTask, failing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing.iterator_destroys == 1 &&
          failing.calls.back() == "destroy_iterator",
          "group iterator releases when candidate processing throws");
}

}  // namespace

int main() {
    try {
        GateAndStatus();
        DirectEffectAndStateGate();
        GroupTraversalAndException();
        std::cout << "use_property_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
