#include "pal4/combat_defend_sequence_task.h"

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
constexpr std::uint32_t kActor = 0x2000u;
constexpr std::uint32_t kChild = 0x3000u;

struct Runtime final : pal4::IDefendSequenceRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kActor}, {0x8f96f8u, 0x4000u}};
    std::vector<std::string> calls;
    std::uint32_t child = kChild;
    std::uint32_t add_result = 0x87654321u;
    std::uint32_t text_seen = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void NotifyActor(std::uint32_t actor) override {
        Check(actor == kActor, "notify actor via virtual +476");
        calls.emplace_back("notify");
    }
    std::uint32_t CreateChildTask(std::uint32_t factory_vtable) override {
        Check(factory_vtable == pal4::DefendStatusTaskFactory::kVtable,
              "Defend child factory vtable");
        calls.emplace_back("create");
        return child;
    }
    std::uint32_t AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t task, std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u && task == kChild &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "Defend child container and timing");
        text_seen = text;
        calls.emplace_back("add");
        return add_result;
    }
};

void CheckFactorySlot() {
    pal4::DefendStatusTaskFactorySlot slot;
    Check(pal4::DefendStatusTaskFactory::Initialize(&slot) == &slot &&
              slot.vtable == 0x845cf0u,
          "Defend Defend child factory slot identity and vtable");
}

void CheckStart(std::uint32_t configured_text) {
    Runtime runtime;
    runtime.words[0x8f96f8u] = configured_text;
    Check(pal4::DefendSequenceTask::Start(kTask, runtime) ==
              runtime.add_result &&
              runtime.text_seen ==
                  (configured_text == 0u ? 0x841964u : configured_text) &&
              runtime.words.at(kChild + 60u) == kActor &&
              runtime.calls == std::vector<std::string>{
                  "read:4140", "notify", "read:9410296", "create",
                  "add", "read:4140", "write:12348"},
          "Defend update call order, fallback and forwarded add result");
}

void CheckNullChild() {
    Runtime runtime;
    runtime.child = 0u;
    Check(pal4::DefendSequenceTask::Start(kTask, runtime) == 0u &&
              runtime.words.find(60u) == runtime.words.end() &&
              runtime.calls.back() == "create",
          "null child exits without writing the original null+60 address");
}

}  // namespace

int main() {
    CheckFactorySlot();
    CheckStart(0u);
    CheckStart(0x4000u);
    CheckNullChild();
    std::cout << "Defend sequence start tests passed\n";
}
