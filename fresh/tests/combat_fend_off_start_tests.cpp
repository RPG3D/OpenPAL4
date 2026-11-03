#include "pal4/combat_fend_off_sequence_task.h"
#include "pal4/win_action_task.h"

#include <algorithm>
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

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kSource = 0x6000u;
constexpr std::uint32_t kTarget = 0x5000u;
constexpr std::array<std::uint32_t, 3> kChildren{
    0x2000u, 0x3000u, 0x4000u};
constexpr std::array<std::uint32_t, 3> kFactories{
    pal4::FendOffImpactChildFactory::kVtable,
    pal4::WinActionThirdChildFactory::kVtable,
    pal4::WinActionSecondChildFactory::kVtable};

struct Runtime final : pal4::IFendOffStartRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget},
        {0x8f96a8u, 0x9000u}, {0x8f95d0u, 0x9100u},
        {0x8f9770u, 0x9200u}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> texts;
    std::vector<std::uint32_t> target_reads;
    bool set_state = true;
    bool mutate_on_add = false;
    std::size_t fail_child = 3u;
    std::size_t created = 0u;
    std::size_t added = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        const auto value = words.at(address);
        if (address == kTask + 48u) target_reads.push_back(value);
        return value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address >= kChildren.front() + 60u,
              "no write through a null child");
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    bool ActorCheckStateThree(std::uint32_t actor) override {
        Check(actor == kTarget, "state probe uses initial target");
        calls.emplace_back("check_state");
        return set_state;
    }
    void SetActorState(std::uint32_t actor, std::uint32_t state,
                       std::uint32_t enabled) override {
        Check(actor == kTarget && state == 3u && enabled == 1u,
              "state transition arguments");
        calls.emplace_back("set_state");
    }
    void ActorVirtual396(std::uint32_t actor,
                         std::uint32_t argument) override {
        Check(argument == 0u && actor ==
                  kTarget + (mutate_on_add ? 1u : 0u),
              "actor virtual +396 after impact task");
        calls.emplace_back("virtual396");
    }
    std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) override {
        Check(created < kFactories.size() &&
                  factory_vtable == kFactories[created],
              "child factories follow FendOff, third, second order");
        calls.push_back("create:" + std::to_string(created));
        const auto index = created++;
        return index == fail_child ? 0u : kChildren[index];
    }
    std::uint32_t AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u && added < kChildren.size() &&
                  child == kChildren[added] &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "child add address, order and timing");
        texts.push_back(text);
        calls.push_back("add:" + std::to_string(added));
        ++added;
        if (mutate_on_add) ++words[kTask + 48u];
        return static_cast<std::uint32_t>(0x7000u + added);
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source,
                    std::uint32_t start,
                    std::uint32_t count) override {
        Check(destination == kChildren[1] + 64u &&
                  source == 0x8fa32cu && start == 0u &&
                  count == 0xffffffffu,
              "message text assignment arguments");
        calls.emplace_back("assign_text");
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(),
                              runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void FactorySlot() {
    pal4::FendOffImpactChildFactorySlot slot;
    Check(pal4::FendOffImpactChildFactory::Initialize(&slot) == &slot &&
              slot.vtable == 0x845d10u,
          "FendOff impact factory slot identity and vtable");
}

void CompleteSequence(bool configured_text, bool state_change,
                      bool mutate_target) {
    Runtime runtime;
    runtime.set_state = state_change;
    runtime.mutate_on_add = mutate_target;
    if (!configured_text) {
        runtime.words[0x8f96a8u] = 0u;
        runtime.words[0x8f95d0u] = 0u;
        runtime.words[0x8f9770u] = 0u;
    }
    Check(pal4::FendOffSequenceTask::Start(kTask, runtime) == 0x7003u &&
              runtime.created == 3u && runtime.added == 3u &&
              runtime.texts == (configured_text
                  ? std::vector<std::uint32_t>{0x9000u, 0x9100u, 0x9200u}
                  : std::vector<std::uint32_t>{
                        0x841964u, 0x841964u, 0x841964u}),
          "three children, text sources and final add result");
    Check(runtime.words.at(kChildren[0] + 60u) == kSource &&
              runtime.words.at(kChildren[0] + 64u) ==
                  kTarget + (mutate_target ? 1u : 0u) &&
              runtime.words.at(kChildren[1] + 60u) ==
                  kTarget + (mutate_target ? 2u : 0u) &&
              runtime.words.at(kChildren[2] + 60u) ==
                  kTarget + (mutate_target ? 3u : 0u),
          "child actor fields follow callback-sensitive target reads");
    const auto state_call = std::find(runtime.calls.begin(),
                                      runtime.calls.end(), "set_state");
    Check(runtime.target_reads.size() == (state_change ? 6u : 5u) &&
              (state_call != runtime.calls.end()) == state_change &&
              (!state_change ||
                  Index(runtime, "set_state") < Index(runtime, "create:0")),
          "state gate precedes child creation");
    Check(Index(runtime, "add:0") < Index(runtime, "virtual396") &&
              Index(runtime, "add:1") < Index(runtime, "assign_text") &&
              Index(runtime, "assign_text") < Index(runtime, "create:2") &&
              Index(runtime, "add:2") <
                  Index(runtime, "write:16444"),
          "FendOff child and actor operation order");
}

void NullChildAtEachStage() {
    for (std::size_t stage = 0; stage < 3u; ++stage) {
        Runtime runtime;
        runtime.fail_child = stage;
        Check(pal4::FendOffSequenceTask::Start(kTask, runtime) == 0u &&
                  runtime.created == stage + 1u &&
                  runtime.added == stage &&
                  runtime.calls.back() ==
                      "create:" + std::to_string(stage),
              "null child stops before invalid native offset writes");
    }
}

}  // namespace

int main() {
    FactorySlot();
    CompleteSequence(true, true, false);
    CompleteSequence(false, false, true);
    NullChildAtEachStage();
    std::cout << "FendOff start tests passed\n";
}
