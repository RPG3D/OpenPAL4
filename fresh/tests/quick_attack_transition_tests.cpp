#include "pal4/quick_attack_task.h"

#include <algorithm>
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
constexpr std::uint32_t kNext = 0x2000u;
constexpr std::uint32_t kSource = 0x3000u;
constexpr std::uint32_t kTarget = 0x4000u;

struct Runtime final : pal4::IQuickAttackTransitionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget}};
    std::map<std::uint32_t, std::uint8_t> bytes{
        {kTask + 52u, 7u}, {kTask + 53u, 8u}};
    std::vector<std::string> calls;
    bool active = true;
    bool state_three = false;
    std::int32_t counter = 2;
    std::uint32_t allocation = kNext;
    bool name_allocation = true;
    bool throw_on_copy = false;
    bool throw_on_init = false;

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
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("write_byte:" + std::to_string(address));
        bytes[address] = value;
    }
    bool CombatActive() override {
        calls.push_back("active");
        return active;
    }
    bool SourceStateThree(std::uint32_t actor) override {
        Check(actor == kSource, "state check receives source");
        calls.push_back("state_three");
        return state_three;
    }
    std::uint32_t ProcessDead(std::uint32_t source,
                              std::uint32_t target) override {
        Check(source == kSource && target == kTarget,
              "dead processing receives actor pair");
        calls.push_back("process_dead");
        return allocation;
    }
    std::uint32_t EndAction(std::uint32_t source) override {
        Check(source == kSource, "end action receives source");
        calls.push_back("end_action");
        return 0x99u;
    }
    std::int32_t ActorCounter(std::uint32_t actor,
                              std::uint32_t index) override {
        Check(actor == kSource && index == 3u,
              "counter reads source slot three");
        calls.push_back("counter");
        return counter;
    }
    void SetActorCounter(std::uint32_t actor,
                         std::uint32_t index,
                         std::int32_t value) override {
        Check(actor == kSource && index == 3u,
              "counter writes source slot three");
        calls.push_back("set_counter");
        counter = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::QuickAttackTask::kSize,
              "successor uses 56 bytes");
        calls.push_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t task) override {
        Check(task == kNext, "free receives successor");
        calls.push_back("free");
    }
    void InitializeContainer(std::uint32_t container) override {
        Check(container == kNext + 4u, "GOM starts at +4");
        calls.push_back("init_container");
        if (throw_on_init) throw std::runtime_error("init");
    }
    void DestroyContainer(std::uint32_t container) override {
        Check(container == kNext + 4u, "GOM cleanup starts at +4");
        calls.push_back("destroy_container");
    }
    void SetNameErrorFlag(std::uint32_t name,
                          std::uint8_t flag) override {
        Check(name == kNext + 20u && flag == 0u,
              "repeat clears name error flag");
        calls.push_back("clear_name_flag");
    }
    bool AllocateNameBuffer(std::uint32_t name,
                            std::uint32_t length,
                            std::uint8_t flag) override {
        Check(name == kNext + 20u && flag == 1u &&
                  (length == 11u || length == 10u),
              "name buffer receives branch-specific length");
        calls.push_back("allocate_name");
        return name_allocation;
    }
    void CopyName(std::uint32_t name,
                  std::string_view value) override {
        Check(name == kNext + 20u &&
                  (value == "QuickAttack" || value == "cbQuickEnd"),
              "successor receives branch-specific name");
        calls.push_back("copy_name:" + std::string(value));
        if (throw_on_copy) throw std::runtime_error("copy");
    }
    void CleanupName(std::uint32_t name) override {
        Check(name == kNext + 20u, "name cleanup starts at +20");
        calls.push_back("cleanup_name");
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void InactiveAndDead() {
    Runtime runtime;
    runtime.active = false;
    Check(pal4::QuickAttackTask::CreateNext(kTask, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"active"},
          "inactive combat returns before reading actor");
    runtime = Runtime{};
    runtime.state_three = true;
    Check(pal4::QuickAttackTask::CreateNext(kTask, runtime) == kNext &&
              runtime.words.at(kNext + 36u) == 0x99u &&
              Index(runtime, "process_dead") <
                  Index(runtime, "end_action") &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "counter") == runtime.calls.end(),
          "dead branch stores end action in task +36");
    runtime = Runtime{};
    runtime.state_three = true;
    runtime.allocation = 0u;
    Check(pal4::QuickAttackTask::CreateNext(kTask, runtime) == 0u &&
              runtime.calls.back() == "end_action",
          "null dead task returns safely after original side effects");
}

void RepeatAndEnd() {
    Runtime runtime;
    Check(pal4::QuickAttackTask::CreateNext(kTask, runtime) == kNext &&
              runtime.counter == 1 &&
              runtime.words.at(kNext) == pal4::QuickAttackTask::kVtable &&
              runtime.words.at(kNext + 28u) == 11u &&
              runtime.words.at(kNext + 36u) == 0u &&
              runtime.words.at(kNext + 44u) == kSource &&
              runtime.words.at(kNext + 48u) == kTarget &&
              runtime.bytes.at(kNext + 20u) == 0u &&
              runtime.bytes.at(kNext + 40u) == 1u &&
              runtime.bytes.at(kNext + 52u) == 7u &&
              runtime.bytes.at(kNext + 53u) == 0u,
          "positive counter creates deterministic QuickAttack successor");
    Check(Index(runtime, "set_counter") < Index(runtime, "allocate") &&
              Index(runtime, "init_container") <
                  Index(runtime, "copy_name:QuickAttack") &&
              runtime.calls.back() == "write:" + std::to_string(kNext) &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "read_byte:" + std::to_string(kTask + 53u)) ==
                  runtime.calls.end(),
          "repeat never reads parent +53 and installs vtable last");
    for (const auto count : {0, -1}) {
        runtime = Runtime{};
        runtime.counter = count;
        Check(pal4::QuickAttackTask::CreateNext(kTask, runtime) == kNext &&
                  runtime.words.at(kNext) ==
                      pal4::QuickAttackTask::kEndVtable &&
                  runtime.words.at(kNext + 28u) == 10u &&
                  runtime.bytes.at(kNext + 20u) == 8u &&
                  runtime.bytes.at(kNext + 52u) == 7u &&
                  runtime.bytes.at(kNext + 53u) == 8u &&
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            "set_counter") == runtime.calls.end(),
              "nonpositive counter creates cbQuickEnd without decrement");
    }
}

void FailureAndUnwind() {
    Runtime runtime;
    runtime.allocation = 0u;
    Check(pal4::QuickAttackTask::CreateNext(kTask, runtime) == 0u &&
              runtime.counter == 1 && runtime.calls.back() == "allocate",
          "counter decrements before failed allocation");
    runtime = Runtime{};
    runtime.name_allocation = false;
    Check(pal4::QuickAttackTask::CreateNext(kTask, runtime) == kNext &&
              runtime.words.at(kNext) == pal4::QuickAttackTask::kVtable &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "copy_name:QuickAttack") == runtime.calls.end(),
          "name allocation failure still returns task");
    runtime = Runtime{};
    runtime.throw_on_copy = true;
    bool caught = false;
    try {
        (void)pal4::QuickAttackTask::CreateNext(kTask, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && Index(runtime, "cleanup_name") <
                        Index(runtime, "destroy_container") &&
              runtime.calls.back() == "free",
          "name exception releases name, GOM and task");
    runtime = Runtime{};
    runtime.throw_on_init = true;
    caught = false;
    try {
        (void)pal4::QuickAttackTask::CreateNext(kTask, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "free" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "destroy_container") == runtime.calls.end(),
          "GOM initialization exception frees raw task");
}

}  // namespace

int main() {
    InactiveAndDead();
    RepeatAndEnd();
    FailureAndUnwind();
    std::cout << "QuickAttack transition tests passed\n";
}
