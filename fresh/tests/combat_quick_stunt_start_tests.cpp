#include "pal4/combat_stunt_sequence_task.h"

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
constexpr std::uint32_t kSource = 0x2000u;
constexpr std::uint32_t kTarget = 0x3000u;
constexpr std::uint32_t kStunt = 0x4000u;
constexpr std::uint32_t kChild = 0x5000u;

struct Runtime final : pal4::IQuickStuntStartRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget},
        {kTask + 52u, kStunt}, {0x8f97d4u, 0x9000u}};
    std::vector<std::string> calls;
    std::uint32_t child = kChild;
    std::uint32_t added_text = 0u;
    bool mutate_after_add = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void NotifySource(std::uint32_t source) override {
        Check(source == kSource, "QuickStunt notify source actor");
        calls.emplace_back("notify");
    }
    std::uint32_t CreateChildTask(std::uint32_t factory) override {
        Check(factory == pal4::QuickStuntChildFactory::kVtable,
              "QuickStunt exact child factory vtable");
        calls.emplace_back("create");
        return child;
    }
    void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t task_child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u && task_child == kChild &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "QuickStunt GOM timing and addresses");
        added_text = text;
        calls.emplace_back("add");
        if (mutate_after_add) {
            words[kTask + 44u] = 0x2100u;
            words[kTask + 48u] = 0x3100u;
            words[kTask + 52u] = 0x4100u;
        }
    }
};

void FactoryAndFields() {
    pal4::QuickStuntChildFactorySlot slot;
    Check(pal4::QuickStuntChildFactory::Initialize(&slot) == &slot &&
              slot.vtable == 0x845d94u,
          "QuickStunt factory slot identity and vtable");
    Runtime runtime;
    Check(pal4::QuickStuntSequenceTask::Start(kTask, runtime) ==
              kStunt &&
              runtime.added_text == 0x9000u &&
              runtime.words.at(kChild + 60u) == kSource &&
              runtime.words.at(kChild + 64u) == kTarget &&
              runtime.words.at(kChild + 68u) == kStunt,
          "QuickStunt child actor, target, stunt and return");
    runtime = Runtime{};
    runtime.words[0x8f97d4u] = 0u;
    runtime.mutate_after_add = true;
    Check(pal4::QuickStuntSequenceTask::Start(kTask, runtime) ==
              0x4100u &&
              runtime.added_text == 0x841964u &&
              runtime.words.at(kChild + 60u) == 0x2100u &&
              runtime.words.at(kChild + 64u) == 0x3100u &&
              runtime.words.at(kChild + 68u) == 0x4100u,
          "QuickStunt uses fallback text and rereads fields after add");
    runtime = Runtime{};
    runtime.child = 0u;
    Check(pal4::QuickStuntSequenceTask::Start(kTask, runtime) == 0u &&
              runtime.added_text == 0u &&
              runtime.calls.back() == "create",
          "null QuickStunt child exits before native invalid writes");
}

}  // namespace

int main() {
    try {
        FactoryAndFields();
        std::cout << "Combat QuickStunt start tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
