#include "pal4/combat_action_factory.h"
#include "pal4/combat_defend_sequence_task.h"
#include "pal4/combat_flee_sequence_task.h"

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

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kActor = 0x1234u;

struct Runtime final : pal4::ICombatDefendActionInitiatorRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool inactive = true;
    bool actor_ready = false;
    bool flee = false;
    bool name_allocation = true;
    bool fail_container = false;
    bool fail_name_copy = false;
    bool fail_name_size = false;
    std::uint32_t allocation = kObject;
    std::uint32_t requested_size = 0u;

    bool CombatInactive() override {
        calls.push_back("combat_inactive");
        return inactive;
    }
    bool IsActorReady(std::uint32_t actor) override {
        Check(actor == kActor, "Defend initiator actor pointer");
        calls.push_back("actor_ready");
        return actor_ready;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate");
        requested_size = bytes;
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "free original object");
        calls.push_back("free");
    }
    void InitializeContainer(std::uint32_t container) override {
        Check(container == kObject + 4u, "container address");
        calls.push_back("init_container");
        if (fail_container) throw std::runtime_error("container init failed");
    }
    void DestroyContainer(std::uint32_t container) override {
        Check(container == kObject + 4u, "container cleanup address");
        calls.push_back("destroy_container");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("word:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void SetNameErrorFlag(std::uint32_t address, std::uint8_t flag) override {
        Check(inactive && address == kObject + 20u && flag == 0u,
              "inactive branch resets name error flag");
        calls.push_back("set_name_flag");
        words[address + 4u] = 0u;
        words[address + 8u] = 0u;
        words[address + 12u] = 0u;
    }
    bool AllocateName(std::uint32_t address, std::uint32_t length,
                      std::uint8_t flag) override {
        Check(address == kObject + 20u &&
                  length == (flee ? 4u : 6u) && flag == 1u,
              "single-actor name allocation arguments");
        calls.push_back("allocate_name");
        return name_allocation;
    }
    void CopyName(std::uint32_t address, std::string_view name) override {
        Check(address == kObject + 20u &&
                  name == (flee ? "Flee" : "Defend"),
              "single-actor name text");
        calls.push_back("copy_name");
        if (fail_name_copy) throw std::runtime_error("name copy failed");
        words[address + 8u] = static_cast<std::uint32_t>(name.size());
    }
    void SetNameSize(std::uint32_t address,
                     std::uint32_t size) override {
        Check(flee && inactive && address == kObject + 20u && size == 4u,
              "QuickFlee size setter arguments");
        calls.push_back("set_name_size");
        if (fail_name_size) throw std::runtime_error("name size failed");
        words[address + 8u] = size;
    }
    void CleanupName(std::uint32_t address) override {
        Check(address == kObject + 20u, "name cleanup address");
        calls.push_back("cleanup_name");
    }
};

bool Called(const Runtime& runtime, const std::string& call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "missing expected call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void InputAndAllocationGates() {
    Runtime runtime;
    Check(pal4::CombatDefendActionFactory::Create(0u, runtime) == 0u &&
              runtime.calls.empty(), "zero actor exits before combat lookup");
    for (bool inactive : {true, false}) {
        runtime = Runtime{};
        runtime.inactive = inactive;
        runtime.allocation = 0u;
        Check(pal4::CombatDefendActionFactory::Create(kActor, runtime) == 0u &&
                  runtime.requested_size == pal4::DefendSequenceTask::kSize &&
                  runtime.calls == std::vector<std::string>{
                      "combat_inactive", "allocate"},
              "allocation failure exits before container initialization");
    }
}

void BranchLayout(bool inactive) {
    Runtime runtime;
    runtime.inactive = inactive;
    Check(pal4::CombatDefendActionFactory::Create(kActor, runtime) ==
              kObject, "factory returns task");
    Check(runtime.requested_size == 48u &&
              runtime.words.at(kObject) ==
                  pal4::DefendSequenceTask::kVtable &&
              runtime.bytes.at(kObject + 20u) == 0x34u &&
              runtime.words.at(kObject + 24u) == 0u &&
              runtime.words.at(kObject + 28u) == 6u &&
              runtime.words.at(kObject + 32u) == 0u &&
              runtime.words.at(kObject + 36u) == 0u &&
              runtime.bytes.at(kObject + 40u) == 1u &&
              runtime.words.at(kObject + 44u) == kActor,
          "Defend size, vtable, name, state and actor layout");
    Check(Called(runtime, "set_name_flag") == inactive &&
              Index(runtime, "combat_inactive") < Index(runtime, "allocate") &&
              Index(runtime, "init_container") <
                  Index(runtime, "allocate_name") &&
              Index(runtime, "copy_name") < Index(runtime, "word:4096"),
          "branch-specific name initialization and construction order");
    if (!inactive) {
        Check(Index(runtime, "word:4120") <
                  Index(runtime, "allocate_name") &&
                  Index(runtime, "word:4124") <
                      Index(runtime, "allocate_name") &&
                  Index(runtime, "word:4128") <
                      Index(runtime, "allocate_name"),
              "active branch clears all three name header words");
    }
}

void FailurePaths(bool inactive) {
    Runtime runtime;
    runtime.inactive = inactive;
    runtime.name_allocation = false;
    Check(pal4::CombatDefendActionFactory::Create(kActor, runtime) ==
              kObject && !Called(runtime, "copy_name") &&
              runtime.words.at(kObject + 44u) == kActor,
          "failed name allocation still constructs task");

    runtime = Runtime{};
    runtime.inactive = inactive;
    runtime.fail_container = true;
    bool caught = false;
    try {
        (void)pal4::CombatDefendActionFactory::Create(kActor, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls.back() == "free" &&
              !Called(runtime, "cleanup_name") &&
              !Called(runtime, "destroy_container"),
          "container failure frees raw storage");

    runtime = Runtime{};
    runtime.inactive = inactive;
    runtime.fail_name_copy = true;
    caught = false;
    try {
        (void)pal4::CombatDefendActionFactory::Create(kActor, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && Index(runtime, "cleanup_name") <
                        Index(runtime, "destroy_container") &&
              runtime.calls.back() == "free",
          "name failure releases name, container and object");
}

void FleeInputAndAllocationGates() {
    Runtime runtime;
    runtime.flee = true;
    Check(pal4::CombatFleeActionFactory::Create(0u, runtime) == 0u &&
              runtime.calls.empty(), "zero Flee actor exits early");
    for (bool inactive : {true, false}) {
        runtime = Runtime{};
        runtime.inactive = inactive;
        runtime.flee = true;
        runtime.allocation = 0u;
        Check(pal4::CombatFleeActionFactory::Create(kActor, runtime) == 0u &&
                  runtime.requested_size == 48u &&
                  runtime.calls == std::vector<std::string>{
                      "combat_inactive", "allocate"},
              "Flee allocation failure exits before initialization");
    }
}

void FleeBranchLayout(bool inactive) {
    Runtime runtime;
    runtime.inactive = inactive;
    runtime.flee = true;
    Check(pal4::CombatFleeActionFactory::Create(kActor, runtime) ==
              kObject, "Flee factory returns task");
    Check(runtime.requested_size == 48u &&
              runtime.words.at(kObject) == (inactive
                  ? pal4::QuickFleeSequenceTask::kVtable
                  : pal4::FleeSequenceTask::kVtable) &&
              runtime.bytes.at(kObject + 20u) == 0x34u &&
              runtime.words.at(kObject + 24u) == 0u &&
              runtime.words.at(kObject + 28u) == 4u &&
              runtime.words.at(kObject + 32u) == 0u &&
              runtime.words.at(kObject + 36u) == 0u &&
              runtime.bytes.at(kObject + 40u) == 1u &&
              runtime.words.at(kObject + 44u) == kActor,
          "Flee size, vtable, name, state and actor layout");
    Check(Called(runtime, "set_name_flag") == inactive &&
              Called(runtime, "set_name_size") == inactive &&
              Index(runtime, "combat_inactive") < Index(runtime, "allocate") &&
              Index(runtime, "init_container") <
                  Index(runtime, "allocate_name") &&
              Index(runtime, "copy_name") < Index(runtime, "word:4096"),
          "Flee branch-specific name handling and construction order");
    if (inactive) {
        Check(Index(runtime, "copy_name") <
                  Index(runtime, "set_name_size"),
              "QuickFlee finalizes name after copying bytes");
    } else {
        Check(Index(runtime, "word:4120") <
                  Index(runtime, "allocate_name") &&
                  Index(runtime, "word:4124") <
                      Index(runtime, "allocate_name") &&
                  Index(runtime, "word:4128") <
                      Index(runtime, "allocate_name"),
              "active Flee branch clears name header words");
    }
}

void FleeFailurePaths(bool inactive) {
    Runtime runtime;
    runtime.inactive = inactive;
    runtime.flee = true;
    runtime.name_allocation = false;
    Check(pal4::CombatFleeActionFactory::Create(kActor, runtime) ==
              kObject && !Called(runtime, "copy_name") &&
              !Called(runtime, "set_name_size") &&
              runtime.words.at(kObject + 44u) == kActor,
          "failed Flee name allocation still constructs task");

    runtime = Runtime{};
    runtime.inactive = inactive;
    runtime.flee = true;
    runtime.fail_container = true;
    bool caught = false;
    try {
        (void)pal4::CombatFleeActionFactory::Create(kActor, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls.back() == "free" &&
              !Called(runtime, "cleanup_name") &&
              !Called(runtime, "destroy_container"),
          "Flee container failure frees raw storage");

    runtime = Runtime{};
    runtime.inactive = inactive;
    runtime.flee = true;
    runtime.fail_name_copy = true;
    caught = false;
    try {
        (void)pal4::CombatFleeActionFactory::Create(kActor, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && Index(runtime, "cleanup_name") <
                        Index(runtime, "destroy_container") &&
              runtime.calls.back() == "free" &&
              !Called(runtime, "set_name_size"),
          "Flee copy failure releases name, container and object");

    if (inactive) {
        runtime = Runtime{};
        runtime.flee = true;
        runtime.fail_name_size = true;
        caught = false;
        try {
            (void)pal4::CombatFleeActionFactory::Create(kActor, runtime);
        } catch (const std::runtime_error&) { caught = true; }
        Check(caught && Index(runtime, "set_name_size") <
                            Index(runtime, "cleanup_name") &&
                  Index(runtime, "cleanup_name") <
                      Index(runtime, "destroy_container") &&
                  runtime.calls.back() == "free",
              "QuickFlee size failure releases all initialized resources");
    }
}

void DefendInitiatorGate() {
    Runtime runtime;
    Check(pal4::CombatDefendActionInitiator::Create(kActor, runtime) ==
              0u &&
              runtime.calls == std::vector<std::string>{"actor_ready"},
          "unready actor exits before Defend factory");
    runtime = Runtime{};
    runtime.actor_ready = true;
    Check(pal4::CombatDefendActionInitiator::Create(kActor, runtime) ==
              kObject &&
              runtime.words.at(kObject) ==
                  pal4::DefendSequenceTask::kVtable &&
              runtime.words.at(kObject + 44u) == kActor &&
              runtime.calls.front() == "actor_ready" &&
              Index(runtime, "actor_ready") < Index(runtime, "combat_inactive"),
          "ready actor delegates to Defend factory");
}

void FleeInitiatorGate() {
    Runtime runtime;
    runtime.flee = true;
    Check(pal4::CombatFleeActionInitiator::Create(kActor, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"actor_ready"},
          "unready actor exits before Flee factory");
    runtime = Runtime{};
    runtime.flee = true;
    runtime.actor_ready = true;
    Check(pal4::CombatFleeActionInitiator::Create(kActor, runtime) ==
              kObject &&
              runtime.words.at(kObject) ==
                  pal4::QuickFleeSequenceTask::kVtable &&
              runtime.words.at(kObject + 44u) == kActor &&
              Index(runtime, "actor_ready") < Index(runtime, "combat_inactive"),
          "ready actor delegates to Flee factory");
}

}  // namespace

int main() {
    try {
        InputAndAllocationGates();
        DefendInitiatorGate();
        FleeInitiatorGate();
        FleeInputAndAllocationGates();
        for (bool inactive : {true, false}) {
            BranchLayout(inactive);
            FailurePaths(inactive);
            FleeBranchLayout(inactive);
            FleeFailurePaths(inactive);
        }
        std::cout << "Combat single-actor factory tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
