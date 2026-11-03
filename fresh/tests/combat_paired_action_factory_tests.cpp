#include "pal4/combat_action_factory.h"
#include "pal4/combat_dead_sequence_task.h"
#include "pal4/combat_fend_off_sequence_task.h"
#include "pal4/combat_miss_sequence_task.h"

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
constexpr std::uint32_t kSource = 0x1234u;
constexpr std::uint32_t kTarget = 0x5678u;

enum class Kind { Dead, FendOff, Miss };

struct Runtime final : pal4::ICombatActionFactoryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool inactive = true;
    Kind kind = Kind::Dead;
    bool name_allocation = true;
    bool fail_container = false;
    bool fail_name_copy = false;
    std::uint32_t allocation = kObject;
    std::uint32_t requested_size = 0u;

    bool CombatInactive() override {
        calls.push_back("combat_inactive");
        return inactive;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate");
        requested_size = size;
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
    void SetNameErrorFlag(std::uint32_t header,
                          std::uint8_t flag) override {
        Check(inactive && header == kObject + 20u && flag == 0u,
              "quick paired action resets name error flag");
        calls.push_back("set_name_flag");
        words[header + 4u] = 0u;
        words[header + 8u] = 0u;
        words[header + 12u] = 0u;
    }
    bool AllocateName(std::uint32_t header, std::uint32_t length,
                      std::uint8_t flag) override {
        Check(header == kObject + 20u && flag == 1u &&
                  length == (kind == Kind::Dead
                      ? (inactive ? 9u : 4u)
                      : kind == Kind::FendOff ? (inactive ? 12u : 7u)
                                               : (inactive ? 9u : 8u)),
              "paired action name allocation length");
        calls.push_back("allocate_name");
        return name_allocation;
    }
    void CopyName(std::uint32_t header, std::string_view name) override {
        Check(header == kObject + 20u &&
                  name == (kind == Kind::Dead
                      ? (inactive ? "QuickDead" : "Dead")
                      : kind == Kind::FendOff
                          ? (inactive ? "QuickFendOff" : "FendOff")
                          : (inactive ? "quickMiss" : "MissTask")),
              "paired action name");
        calls.push_back("copy_name");
        if (fail_name_copy) throw std::runtime_error("name copy failed");
        words[header + 8u] = static_cast<std::uint32_t>(name.size());
    }
    void CleanupName(std::uint32_t header) override {
        Check(header == kObject + 20u, "name cleanup address");
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

std::uint32_t CreateAction(std::uint32_t source,
                           std::uint32_t target, Runtime& runtime) {
    switch (runtime.kind) {
    case Kind::Dead:
        return pal4::CombatDeadActionFactory::Create(source, target, runtime);
    case Kind::FendOff:
        return pal4::CombatFendOffActionFactory::Create(
            source, target, runtime);
    case Kind::Miss:
        return pal4::CombatMissActionFactory::Create(source, target, runtime);
    }
    throw std::runtime_error("unknown action kind");
}

void InputAndAllocationGates(Kind kind) {
    Runtime runtime;
    runtime.kind = kind;
    Check(CreateAction(0u, kTarget, runtime) == 0u &&
              runtime.calls.empty(), "zero source exits early");
    Check(CreateAction(kSource, 0u, runtime) == 0u &&
              runtime.calls.empty(), "zero target exits early");
    for (bool inactive : {true, false}) {
        runtime = Runtime{};
        runtime.inactive = inactive;
        runtime.kind = kind;
        runtime.allocation = 0u;
        Check(CreateAction(kSource, kTarget, runtime) == 0u &&
                  runtime.requested_size == 52u &&
                  runtime.calls == std::vector<std::string>{
                      "combat_inactive", "allocate"},
              "allocation failure exits before container initialization");
    }
}

void BranchLayout(bool inactive, Kind kind) {
    Runtime runtime;
    runtime.inactive = inactive;
    runtime.kind = kind;
    Check(CreateAction(kSource, kTarget, runtime) == kObject,
          "paired factory returns task");
    Check(runtime.requested_size == 52u &&
              runtime.words.at(kObject) == (kind == Kind::Dead
                  ? (inactive ? pal4::QuickDeadSequenceTask::kVtable
                              : pal4::DeadSequenceTask::kVtable)
                  : kind == Kind::FendOff
                      ? (inactive ? pal4::QuickFendOffSequenceTask::kVtable
                                  : pal4::FendOffSequenceTask::kVtable)
                      : (inactive ? pal4::QuickMissSequenceTask::kVtable
                                  : pal4::MissSequenceTask::kVtable)) &&
              runtime.bytes.at(kObject + 20u) == 0x34u &&
              runtime.words.at(kObject + 24u) == 0u &&
              runtime.words.at(kObject + 28u) == (kind == Kind::Dead
                  ? (inactive ? 9u : 4u)
                  : kind == Kind::FendOff ? (inactive ? 12u : 7u)
                                           : (inactive ? 9u : 8u)) &&
              runtime.words.at(kObject + 32u) == 0u &&
              runtime.words.at(kObject + 36u) == 0u &&
              runtime.bytes.at(kObject + 40u) == 1u &&
              runtime.words.at(kObject + 44u) == kSource &&
              runtime.words.at(kObject + 48u) == kTarget,
          "paired action size, vtable, name, state and actor layout");
    Check(Called(runtime, "set_name_flag") == inactive &&
              Index(runtime, "combat_inactive") < Index(runtime, "allocate") &&
              Index(runtime, "init_container") <
                  Index(runtime, "allocate_name") &&
              Index(runtime, "copy_name") < Index(runtime, "word:4096"),
          "branch-specific name initialization and final vtable order");
    if (inactive) {
        Check(Index(runtime, "word:4144") < Index(runtime, "word:4132"),
              "quick paired action stores target before state");
    } else {
        Check(Index(runtime, "word:4120") <
                  Index(runtime, "allocate_name") &&
                  Index(runtime, "word:4124") <
                      Index(runtime, "allocate_name") &&
                  Index(runtime, "word:4128") <
                      Index(runtime, "allocate_name") &&
                  Index(runtime, "word:4132") < Index(runtime, "word:4140") &&
                  Index(runtime, "word:4140") < Index(runtime, "word:4144"),
              "active paired action clears header and stores state before actors");
    }
}

void FailurePaths(bool inactive, Kind kind) {
    Runtime runtime;
    runtime.inactive = inactive;
    runtime.kind = kind;
    runtime.name_allocation = false;
    Check(CreateAction(kSource, kTarget, runtime) == kObject &&
              !Called(runtime, "copy_name") &&
              runtime.words.at(kObject + 48u) == kTarget,
          "name allocation failure still constructs paired task");

    runtime = Runtime{};
    runtime.inactive = inactive;
    runtime.kind = kind;
    runtime.fail_container = true;
    bool caught = false;
    try {
        (void)CreateAction(kSource, kTarget, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls.back() == "free" &&
              !Called(runtime, "cleanup_name") &&
              !Called(runtime, "destroy_container"),
          "container failure frees raw task storage");

    runtime = Runtime{};
    runtime.inactive = inactive;
    runtime.kind = kind;
    runtime.fail_name_copy = true;
    caught = false;
    try {
        (void)CreateAction(kSource, kTarget, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && Index(runtime, "cleanup_name") <
                        Index(runtime, "destroy_container") &&
              runtime.calls.back() == "free",
          "name failure releases name, container and task");
}

}  // namespace

int main() {
    try {
        for (Kind kind : {Kind::Dead, Kind::FendOff, Kind::Miss}) {
            InputAndAllocationGates(kind);
            for (bool inactive : {true, false}) {
                BranchLayout(inactive, kind);
                FailurePaths(inactive, kind);
            }
        }
        std::cout << "Combat paired action factory tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
