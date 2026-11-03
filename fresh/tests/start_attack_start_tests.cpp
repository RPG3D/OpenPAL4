#include "pal4/start_attack_task.h"

#include <algorithm>
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
constexpr std::uint32_t kProtector = 0x3000u;
constexpr std::uint32_t kFirst = 0x4000u;
constexpr std::uint32_t kSecond = 0x5000u;

struct Runtime final : pal4::IStartAttackExecutionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kProtector},
        {kSource + 808u, 1u}, {kSource + 804u, 1u},
        {kProtector + 804u, 2u}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> factories;
    std::vector<std::string> texts;
    std::vector<std::array<float, 3>> targets;
    bool blocked = false;
    std::uint32_t completion = 0u;
    bool match_states_on_completion = false;
    std::uint32_t first = kFirst;
    std::uint32_t second = kSecond;
    std::size_t created = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::array<float, 3> ActorChildPosition(
        std::uint32_t actor) override {
        calls.push_back("position:" + std::to_string(actor));
        if (actor == kProtector) return {2.0f, 3.0f, 4.0f};
        Check(actor == kSource, "child position belongs to source");
        return {5.0f, 3.0f, 4.0f};
    }
    void NormalizeVector(std::array<float, 3>& vector) override {
        Check(vector == std::array<float, 3>{3.0f, 0.0f, 0.0f},
              "RenderWare input is source minus protector");
        calls.push_back("normalize");
        vector = {1.0f, 0.0f, 0.0f};
    }
    float ActorClampedChildSize(std::uint32_t actor) override {
        Check(actor == kProtector, "clamped size belongs to protector");
        calls.push_back("child_size");
        return 10.0f;
    }
    void NotifySource(std::uint32_t actor) override {
        Check(actor == kSource, "notify receives source");
        calls.push_back("notify");
    }
    bool SourceBlocksEvent(std::uint32_t actor) override {
        Check(actor == kSource, "block check receives source");
        calls.push_back("blocked");
        return blocked;
    }
    std::uint32_t SourceEndsSequence(std::uint32_t actor) override {
        Check(actor == kSource, "completion receives source");
        calls.push_back("completion");
        if (match_states_on_completion)
            words[kSource + 804u] = words[kProtector + 804u];
        return completion;
    }
    std::array<float, 3> ActorTargetPosition(
        std::uint32_t actor) override {
        Check(actor == kProtector, "target position belongs to protector");
        calls.push_back("target_position");
        return {7.0f, 8.0f, 9.0f};
    }
    std::string_view GlobalText(std::uint32_t address) override {
        calls.push_back("text:" + std::to_string(address));
        if (address == 0x8f9784u) return "run-primary";
        Check(address == 0x8f97acu, "second text global");
        return {};
    }
    std::uint32_t CreateApproachTask(
        const pal4::StartAttackApproachFactorySlot& factory) override {
        factories.push_back(factory.vtable);
        calls.push_back("create:" + std::to_string(factory.vtable));
        return created++ == 0u ? first : second;
    }
    void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::string_view text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u &&
                  (child == kFirst || child == kSecond) &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "run task uses original container and timing");
        texts.emplace_back(text);
        calls.push_back("add:" + std::to_string(child));
    }
    std::uint32_t SetApproachTaskTarget(
        std::uint32_t child, std::uint32_t source,
        const std::array<float, 3>& position) override {
        Check(source == kSource, "RunTask receives source actor");
        targets.push_back(position);
        calls.push_back("set_target:" + std::to_string(child));
        return child == kFirst ? 0x111u : 0x222u;
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call occurred");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void VectorAndFactoryHelpers() {
    std::array<float, 3> lhs{2.0f, 3.0f, 4.0f};
    std::array<float, 3> rhs{1.0f, -2.0f, 5.0f};
    Check(pal4::ScriptVector3Operations::Add(lhs, rhs) == &rhs &&
          lhs == std::array<float, 3>{3.0f, 1.0f, 9.0f},
          "vector add updates destination and returns operand pointer");
    Check(pal4::ScriptVector3Operations::Subtract(lhs, rhs) == &rhs &&
          lhs == std::array<float, 3>{2.0f, 3.0f, 4.0f},
          "vector subtract updates destination and returns operand pointer");
    pal4::StartAttackApproachFactorySlot slot;
    Check(pal4::StartAttackApproachFactory::InitializePrimary(&slot) == &slot &&
          slot.vtable == pal4::StartAttackApproachFactory::kPrimaryVtable,
          "primary factory writes vtable and returns slot");
    Check(pal4::StartAttackApproachFactory::InitializeSecondary(&slot) == &slot &&
          slot.vtable == pal4::StartAttackApproachFactory::kSecondaryVtable,
          "secondary factory reuses slot and returns it");
}

void InactiveSource() {
    Runtime runtime;
    runtime.words[kSource + 808u] = 0u;
    Check(pal4::StartAttackTask::Start(kTask, runtime) == kSource &&
          runtime.calls.size() == 2u && runtime.created == 0u,
          "inactive source returns actor pointer before geometry work");
}

void DifferentStates() {
    Runtime runtime;
    Check(pal4::StartAttackTask::Start(kTask, runtime) == 0x222u &&
          runtime.factories == std::vector<std::uint32_t>{
              pal4::StartAttackApproachFactory::kPrimaryVtable,
              pal4::StartAttackApproachFactory::kSecondaryVtable} &&
          runtime.texts ==
              std::vector<std::string>{"run-primary", ""} &&
          runtime.targets ==
              std::vector<std::array<float, 3>>{
                  {12.0f, 3.0f, 4.0f}, {12.0f, 3.0f, 4.0f}},
          "different states schedule two run tasks at displaced position");
    Check(Index(runtime, "position:12288") <
              Index(runtime, "position:8192") &&
          Index(runtime, "normalize") < Index(runtime, "child_size") &&
          Index(runtime, "notify") < Index(runtime, "blocked") &&
          Index(runtime, "text:" + std::to_string(0x8f9784u)) <
              Index(runtime, "create:" + std::to_string(
                  pal4::StartAttackApproachFactory::kPrimaryVtable)) &&
          Index(runtime, "add:16384") <
              Index(runtime, "set_target:16384") &&
          Index(runtime, "completion") < Index(runtime, "add:20480"),
          "geometry, actor callbacks, text and child tasks follow IDA order");
}

void BlockedAndSameState() {
    Runtime runtime;
    runtime.blocked = true;
    Check(pal4::StartAttackTask::Start(kTask, runtime) == 0x222u &&
          runtime.targets ==
              std::vector<std::array<float, 3>>{
                  {7.0f, 8.0f, 9.0f}, {7.0f, 8.0f, 9.0f}} &&
          Index(runtime, "add:16384") <
              Index(runtime, "target_position"),
          "blocked branch reads actor target after first child is added");
    runtime = Runtime{};
    runtime.words[kProtector + 804u] = 1u;
    Check(pal4::StartAttackTask::Start(kTask, runtime) == 1u &&
          runtime.created == 1u &&
          runtime.targets ==
              std::vector<std::array<float, 3>>{{2.0f, 3.0f, 4.0f}},
          "same state keeps protector position and omits second run task");
}

void CompletionAndStateChange() {
    Runtime runtime;
    runtime.completion = 77u;
    Check(pal4::StartAttackTask::Start(kTask, runtime) == 77u &&
          runtime.created == 1u,
          "nonzero completion result is forwarded without second task");
    runtime = Runtime{};
    runtime.match_states_on_completion = true;
    Check(pal4::StartAttackTask::Start(kTask, runtime) == 2u &&
          runtime.created == 1u &&
          runtime.targets.front() ==
              std::array<float, 3>{12.0f, 3.0f, 4.0f},
          "post-callback state is read again before second-task gate");
}

void NullChildren() {
    Runtime runtime;
    runtime.first = 0u;
    Check(pal4::StartAttackTask::Start(kTask, runtime) == 0u &&
          runtime.created == 1u && runtime.targets.empty(),
          "null first child safely exits before target write");
    runtime = Runtime{};
    runtime.second = 0u;
    Check(pal4::StartAttackTask::Start(kTask, runtime) == 0u &&
          runtime.created == 2u && runtime.targets.size() == 1u,
          "null second child keeps first task without invalid target write");
}

}  // namespace

int main() {
    VectorAndFactoryHelpers();
    InactiveSource();
    DifferentStates();
    BlockedAndSameState();
    CompletionAndStateChange();
    NullChildren();
    std::cout << "StartAttack start tests passed\n";
}
