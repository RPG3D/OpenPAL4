#include "pal4/combat_miss_sequence_task.h"
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
constexpr std::array<std::uint32_t, 2> kChildren{0x2000u, 0x3000u};
constexpr std::array<std::uint32_t, 2> kFactories{
    pal4::MissMessageChildFactory::kVtable,
    pal4::WinActionSecondChildFactory::kVtable};

struct Runtime final : pal4::IMissStartRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget},
        {0x8f960cu, 0x9000u}, {0x8f9770u, 0x9100u}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> texts;
    std::size_t created = 0u;
    std::size_t added = 0u;
    std::size_t fail_child = 2u;
    bool mutate_target_on_add = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == kChildren[0] + 60u ||
                  address == kChildren[1] + 60u,
              "Miss child actor field address");
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void ActorVirtual512(std::uint32_t source,
                         std::uint32_t target) override {
        Check(source == kSource && target == kTarget,
              "Miss dispatch source and target");
        calls.emplace_back("virtual512");
    }
    std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) override {
        Check(created < kFactories.size() &&
                  factory_vtable == kFactories[created],
              "Miss message and finish factory order");
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
              "Miss child container, order and timing");
        texts.push_back(text);
        calls.push_back("add:" + std::to_string(added));
        ++added;
        if (mutate_target_on_add) ++words[kTask + 48u];
        return static_cast<std::uint32_t>(0x7000u + added);
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source,
                    std::uint32_t start,
                    std::uint32_t count) override {
        Check(destination == kChildren[0] + 64u &&
                  source == 0x8fa308u && start == 0u &&
                  count == 0xffffffffu,
              "Miss message assignment arguments");
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
    pal4::MissMessageChildFactorySlot slot;
    Check(pal4::MissMessageChildFactory::Initialize(&slot) == &slot &&
              slot.vtable == 0x845d14u,
          "Miss message child factory slot identity and vtable");
}

void CompleteSequence(bool configured_text, bool mutate_target) {
    Runtime runtime;
    runtime.mutate_target_on_add = mutate_target;
    if (!configured_text) {
        runtime.words[0x8f960cu] = 0u;
        runtime.words[0x8f9770u] = 0u;
    }
    Check(pal4::MissSequenceTask::Start(kTask, runtime) == 0x7002u &&
              runtime.created == 2u && runtime.added == 2u &&
              runtime.texts == (configured_text
                  ? std::vector<std::uint32_t>{0x9000u, 0x9100u}
                  : std::vector<std::uint32_t>{
                        0x841964u, 0x841964u}),
          "Miss schedules both children and forwards final add result");
    Check(runtime.words.at(kChildren[0] + 60u) ==
              kTarget + (mutate_target ? 1u : 0u) &&
              runtime.words.at(kChildren[1] + 60u) ==
                  kTarget + (mutate_target ? 2u : 0u),
          "Miss reads target after each child-add callback");
    Check(Index(runtime, "read:4140") <
              Index(runtime, "virtual512") &&
              Index(runtime, "virtual512") <
                  Index(runtime, "create:0") &&
              Index(runtime, "add:0") <
                  Index(runtime, "write:8252") &&
              Index(runtime, "write:8252") <
                  Index(runtime, "assign_text") &&
              Index(runtime, "assign_text") <
                  Index(runtime, "create:1") &&
              Index(runtime, "add:1") <
                  Index(runtime, "write:12348"),
          "Miss callback, child and message assignment order");
}

void NullChildStages() {
    for (std::size_t stage = 0u; stage < 2u; ++stage) {
        Runtime runtime;
        runtime.fail_child = stage;
        Check(pal4::MissSequenceTask::Start(kTask, runtime) == 0u &&
                  runtime.created == stage + 1u &&
                  runtime.added == stage &&
                  runtime.calls.back() ==
                      "create:" + std::to_string(stage),
              "null Miss child exits before invalid native write");
    }
}

}  // namespace

int main() {
    FactorySlot();
    CompleteSequence(true, false);
    CompleteSequence(false, true);
    NullChildStages();
    std::cout << "Miss start tests passed\n";
}
