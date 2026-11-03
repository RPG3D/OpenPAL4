#include "pal4/physical_attack_task.h"

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

struct Runtime final : pal4::IPhysicalAttackTransitionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget}};
    std::map<std::uint32_t, std::uint8_t> bytes{
        {kTask + 52u, 7u}, {kTask + 53u, 8u},
        {kTask + 54u, 9u}, {kNext + 54u, 0xaau}};
    std::vector<std::string> calls;
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
    bool ActorStateThree(std::uint32_t actor) override {
        Check(actor == kSource, "state check receives source actor");
        calls.push_back("state_three");
        return state_three;
    }
    void NotifyStateThree(std::uint32_t actor) override {
        Check(actor == kSource, "state notification receives source");
        calls.push_back("notify_state_three");
    }
    std::uint32_t ProcessWounded(std::uint32_t actor,
                                 std::uint32_t target) override {
        Check(actor == kSource && target == kTarget,
              "wounded processing receives actor pair");
        calls.push_back("wounded");
        return 0x88u;
    }
    std::int32_t ActorCounter(std::uint32_t actor,
                              std::uint32_t index) override {
        Check(actor == kSource && index == 3u,
              "counter getter receives source and index three");
        calls.push_back("counter");
        return counter;
    }
    void SetActorCounter(std::uint32_t actor,
                         std::uint32_t index,
                         std::int32_t value) override {
        Check(actor == kSource && index == 3u,
              "counter setter receives source and index three");
        calls.push_back("set_counter");
        counter = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 56u, "transition allocates 56-byte task");
        calls.push_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == kNext, "free receives new task");
        calls.push_back("free");
    }
    void InitializeContainer(std::uint32_t container) override {
        Check(container == kNext + 4u,
              "new GOM container begins at task+4");
        calls.push_back("init_container");
        if (throw_on_init) throw std::runtime_error("init");
    }
    void DestroyContainer(std::uint32_t container) override {
        Check(container == kNext + 4u,
              "GOM cleanup begins at task+4");
        calls.push_back("destroy_container");
    }
    void SetNameErrorFlag(std::uint32_t name,
                          std::uint8_t flag) override {
        Check(name == kNext + 20u && flag == 0u,
              "repeat branch clears name error flag");
        calls.push_back("set_error_flag");
        words[kNext + 24u] = 0u;
        words[kNext + 28u] = 0u;
        words[kNext + 32u] = 0u;
    }
    bool AllocateNameBuffer(std::uint32_t name,
                            std::uint32_t length,
                            std::uint8_t flag) override {
        Check(name == kNext + 20u &&
                  (length == 14u || length == 9u) && flag == 1u,
              "name allocator receives task header and exact length");
        calls.push_back("allocate_name");
        return name_allocation;
    }
    void CopyName(std::uint32_t name,
                  std::string_view value) override {
        Check(name == kNext + 20u &&
                  (value == "PhysicalAttack" || value == "EndAttack"),
              "name copier receives selected task name");
        calls.push_back("copy:" + std::string(value));
        if (throw_on_copy) throw std::runtime_error("copy");
    }
    void CleanupName(std::uint32_t name) override {
        Check(name == kNext + 20u, "name cleanup begins at +20");
        calls.push_back("cleanup_name");
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected runtime call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void WoundedPath() {
    Runtime runtime;
    runtime.state_three = true;
    Check(pal4::PhysicalAttackTask::CreateNext(kTask, runtime) == 0x88u &&
          runtime.calls == std::vector<std::string>{
              "read:4140", "state_three", "notify_state_three",
              "read:4144", "wounded"},
          "state-three actor notifies then forwards wounded result");
}

void RepeatAttack() {
    Runtime runtime;
    Check(pal4::PhysicalAttackTask::CreateNext(kTask, runtime) == kNext &&
          runtime.counter == 1 &&
          runtime.words[kNext] == pal4::PhysicalAttackTask::kVtable &&
          runtime.words[kNext + 28u] == 14u &&
          runtime.words[kNext + 36u] == 0u &&
          runtime.words[kNext + 44u] == kSource &&
          runtime.words[kNext + 48u] == kTarget &&
          runtime.bytes[kNext + 20u] == 9u &&
          runtime.bytes[kNext + 40u] == 1u &&
          runtime.bytes[kNext + 52u] == 7u &&
          runtime.bytes[kNext + 53u] == 8u &&
          runtime.bytes[kNext + 54u] == 9u,
          "positive counter decrements and rebuilds PhysicalAttack");
    Check(Index(runtime, "set_counter") < Index(runtime, "allocate") &&
          Index(runtime, "read_byte:4149") <
              Index(runtime, "read_byte:4150") &&
          Index(runtime, "set_error_flag") <
              Index(runtime, "copy:PhysicalAttack") &&
          Index(runtime, "copy:PhysicalAttack") <
              Index(runtime, "write:8192"),
          "repeat branch preserves counter and construction order");
}

void EndAttack() {
    for (const auto count : {0, -3}) {
        Runtime runtime;
        runtime.counter = count;
        Check(pal4::PhysicalAttackTask::CreateNext(kTask, runtime) ==
                  kNext &&
              runtime.counter == count &&
              runtime.words[kNext] == pal4::EndAttackTask::kVtable &&
              runtime.words[kNext + 24u] == 0u &&
              runtime.words[kNext + 28u] == 9u &&
              runtime.words[kNext + 32u] == 0u &&
              runtime.words[kNext + 36u] == 0u &&
              runtime.words[kNext + 44u] == kSource &&
              runtime.words[kNext + 48u] == kTarget &&
              runtime.bytes[kNext + 20u] == 7u &&
              runtime.bytes[kNext + 40u] == 1u &&
              runtime.bytes[kNext + 52u] == 7u &&
              runtime.bytes[kNext + 53u] == 8u &&
              runtime.bytes[kNext + 54u] == 0xaau,
              "nonpositive counter creates EndAttack without writing +54");
        Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                        "set_counter") == runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "set_error_flag") == runtime.calls.end(),
              "end branch skips counter write and name-error helper");
    }
}

void FailuresAndUnwind() {
    Runtime runtime;
    runtime.allocation = 0u;
    Check(pal4::PhysicalAttackTask::CreateNext(kTask, runtime) == 0u &&
          runtime.counter == 1 && runtime.calls.back() == "allocate",
          "counter decrement persists when task allocation fails");
    runtime = Runtime{};
    runtime.counter = 0;
    runtime.name_allocation = false;
    Check(pal4::PhysicalAttackTask::CreateNext(kTask, runtime) == kNext &&
          runtime.words[kNext + 28u] == 0u &&
          runtime.words[kNext] == pal4::EndAttackTask::kVtable,
          "text allocation failure still returns constructed EndAttack");
    runtime = Runtime{};
    runtime.throw_on_copy = true;
    bool caught = false;
    try {
        (void)pal4::PhysicalAttackTask::CreateNext(kTask, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught &&
          Index(runtime, "cleanup_name") <
              Index(runtime, "destroy_container") &&
          runtime.calls.back() == "free",
          "name copy exception releases name, container and task");
    runtime = Runtime{};
    runtime.throw_on_init = true;
    caught = false;
    try {
        (void)pal4::PhysicalAttackTask::CreateNext(kTask, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "free" &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "destroy_container") == runtime.calls.end(),
          "container init exception frees raw task");
}

}  // namespace

int main() {
    WoundedPath();
    RepeatAttack();
    EndAttack();
    FailuresAndUnwind();
    std::cout << "PhysicalAttack transition tests passed\n";
}
