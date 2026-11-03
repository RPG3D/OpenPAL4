#include "pal4/end_attack_task.h"

#include "pal4/check_protect_task.h"
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
constexpr std::uint32_t kTarget = 0x3000u;
constexpr std::uint32_t kBase = 0x4000u;
constexpr std::uint32_t kPosition = 0x5000u;
constexpr std::uint32_t kRun1 = 0x6000u;
constexpr std::uint32_t kRun2 = 0x7000u;

struct Runtime final : pal4::IEndAttackStartRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget},
        {kSource + 804u, 1u}, {kTarget + 804u, 2u}};
    std::map<std::uint32_t, std::uint8_t> bytes{{kTask + 53u, 1u}};
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> factories;
    std::vector<std::string> texts;
    std::vector<std::array<float, 3>> run_targets;
    std::vector<std::uint32_t> children{kBase, kPosition, kRun1, kRun2};
    std::size_t created = 0u;
    std::uint32_t completion = 0u;
    bool match_states_on_completion = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read_byte:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        calls.push_back("float:" + std::to_string(address));
    }
    std::string_view GlobalText(std::uint32_t address) override {
        calls.push_back("text:" + std::to_string(address));
        if (address == 0x8f966cu) return "base";
        if (address == 0x8f9798u) return "position";
        if (address == 0x8f9784u) return "run-one";
        Check(address == 0x8f97acu, "second run text address");
        return {};
    }
    std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) override {
        factories.push_back(factory_vtable);
        calls.push_back("create:" + std::to_string(factory_vtable));
        return children.at(created++);
    }
    void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::string_view text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u && child != 0u &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "EndAttack child uses original GOM container and timing");
        texts.emplace_back(text);
        calls.push_back("add:" + std::to_string(child));
    }
    std::array<float, 3> ActorTargetPosition(
        std::uint32_t actor) override {
        calls.push_back("position:" + std::to_string(actor));
        if (actor == kTarget) return {1.5f, 2.5f, 3.5f};
        Check(actor == kSource, "run position comes from source");
        return {4.0f, 5.0f, 6.0f};
    }
    std::uint32_t SourceEndsSequence(std::uint32_t actor) override {
        Check(actor == kSource, "completion receives source actor");
        calls.push_back("completion");
        if (match_states_on_completion)
            words[kSource + 804u] = words[kTarget + 804u];
        return completion;
    }
    std::uint32_t SetApproachTaskTarget(
        std::uint32_t child, std::uint32_t actor,
        const std::array<float, 3>& position) override {
        Check(actor == kSource, "RunTask target receives source actor");
        run_targets.push_back(position);
        calls.push_back("set_target:" + std::to_string(child));
        return child == kRun1 ? 0x111u : 0x222u;
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected runtime call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void FactoryAndAllChildren() {
    pal4::EndAttackBaseFactorySlot slot;
    Check(pal4::EndAttackBaseFactory::Initialize(&slot) == &slot &&
          slot.vtable == pal4::EndAttackBaseFactory::kVtable,
          "base factory writes vtable and returns slot");
    Runtime runtime;
    Check(pal4::EndAttackTask::Start(kTask, runtime) == 0x222u &&
          runtime.factories == std::vector<std::uint32_t>{
              pal4::EndAttackBaseFactory::kVtable,
              pal4::CheckProtectSubtaskFactory::kPositionVtable,
              pal4::StartAttackApproachFactory::kPrimaryVtable,
              pal4::StartAttackApproachFactory::kSecondaryVtable} &&
          runtime.texts == std::vector<std::string>{
              "base", "position", "run-one", ""} &&
          runtime.words[kBase + 60u] == kSource &&
          runtime.words[kBase + 64u] == kTarget &&
          runtime.words[kPosition + 72u] == kTarget &&
          runtime.floats[kPosition + 60u] == 1.5f &&
          runtime.floats[kPosition + 64u] == 2.5f &&
          runtime.floats[kPosition + 68u] == 3.5f &&
          runtime.run_targets ==
              std::vector<std::array<float, 3>>{
                  {4.0f, 5.0f, 6.0f}, {4.0f, 5.0f, 6.0f}},
          "EndAttack schedules base, position and both RunTask children");
    Check(Index(runtime, "add:16384") <
              Index(runtime, "read_byte:4149") &&
          Index(runtime, "position:12288") <
              Index(runtime, "add:20480") &&
          Index(runtime, "add:20480") <
              Index(runtime, "completion") &&
          Index(runtime, "add:24576") <
              Index(runtime, "position:8192") &&
          Index(runtime, "set_target:24576") <
              Index(runtime, "add:28672"),
          "child and callback order matches IDA");
}

void GatesAndStateChanges() {
    Runtime runtime;
    runtime.bytes[kTask + 53u] = 0u;
    runtime.children = {kBase, kRun1, kRun2};
    Check(pal4::EndAttackTask::Start(kTask, runtime) == 0x222u &&
          runtime.factories.size() == 3u &&
          runtime.factories[1] ==
              pal4::StartAttackApproachFactory::kPrimaryVtable,
          "clear +53 skips the position child");
    runtime = Runtime{};
    runtime.completion = 77u;
    Check(pal4::EndAttackTask::Start(kTask, runtime) == 77u &&
          runtime.factories.size() == 2u,
          "completion result is forwarded after optional position child");
    runtime = Runtime{};
    runtime.words[kTarget + 804u] = 1u;
    Check(pal4::EndAttackTask::Start(kTask, runtime) == 1u &&
          runtime.factories.size() == 2u,
          "equal states omit both run tasks");
    runtime = Runtime{};
    runtime.match_states_on_completion = true;
    Check(pal4::EndAttackTask::Start(kTask, runtime) == 2u &&
          runtime.factories.size() == 2u,
          "state read after completion callback sees changed state");
}

void ChildFailures() {
    for (std::size_t failed = 0u; failed < 4u; ++failed) {
        Runtime runtime;
        runtime.children[failed] = 0u;
        Check(pal4::EndAttackTask::Start(kTask, runtime) == 0u &&
              runtime.created == failed + 1u &&
              runtime.texts.size() == failed,
              "null child exits before writing its fields");
    }
}

}  // namespace

int main() {
    FactoryAndAllChildren();
    GatesAndStateChanges();
    ChildFailures();
    std::cout << "EndAttack task tests passed\n";
}
