#include "pal4/combat_action_factory.h"
#include "pal4/cast_magic_sequence_task.h"
#include "pal4/combat_stunt_sequence_task.h"
#include "pal4/combat_use_property_sequence_task.h"
#include "pal4/quick_cast_magic_sequence_task.h"

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
constexpr std::uint32_t kMagic = 0x4321u;
constexpr std::uint32_t kSource = 0x1234u;
constexpr std::uint32_t kTarget = 0x5678u;

enum class Kind { Magic, Stunt, Property };

struct Runtime final : pal4::ICombatActionFactoryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool inactive = true;
    Kind kind = Kind::Magic;
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
        Check(object == kObject, "free the allocated object");
        calls.push_back("free");
    }
    void InitializeContainer(std::uint32_t address) override {
        Check(address == kObject + 4u, "container address");
        calls.push_back("init_container");
        if (fail_container) throw std::runtime_error("container init failed");
    }
    void DestroyContainer(std::uint32_t address) override {
        Check(address == kObject + 4u, "container cleanup address");
        calls.push_back("destroy_container");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte:" + std::to_string(address));
    }
    void SetNameErrorFlag(std::uint32_t address, std::uint8_t flag) override {
        Check(inactive && address == kObject + 20u && flag == 0u,
              "quick name header reset");
        calls.push_back("set_name_flag");
        words[address + 4u] = 0u;
        words[address + 8u] = 0u;
        words[address + 12u] = 0u;
    }
    bool AllocateName(std::uint32_t address, std::uint32_t length,
                      std::uint8_t flag) override {
        Check(address == kObject + 20u && flag == 1u &&
                  length == (kind == Kind::Stunt ? (inactive ? 10u : 5u)
                      : kind == Kind::Property ? (inactive ? 16u : 11u)
                                               : (inactive ? 14u : 9u)),
              "action name length");
        calls.push_back("allocate_name");
        return name_allocation;
    }
    void CopyName(std::uint32_t address, std::string_view name) override {
        Check(address == kObject + 20u &&
                  name == (kind == Kind::Stunt
                      ? (inactive ? "QuickStunt" : "Stunt")
                      : kind == Kind::Property
                          ? (inactive ? "QuickUseProperty" : "UseProperty")
                          : (inactive ? "QuickCastMagic" : "CastMagic")),
              "action name");
        calls.push_back("copy_name");
        if (fail_name_copy) throw std::runtime_error("name copy failed");
        words[address + 8u] = static_cast<std::uint32_t>(name.size());
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

std::uint32_t CreateAction(std::uint32_t action, std::uint32_t source,
                           std::uint32_t target, Runtime& runtime) {
    switch (runtime.kind) {
    case Kind::Magic:
        return pal4::CombatMagicActionFactory::Create(
            action, source, target, runtime);
    case Kind::Stunt:
        return pal4::CombatStuntActionFactory::Create(
            action, source, target, runtime);
    case Kind::Property:
        return pal4::CombatUsePropertyActionFactory::Create(
            action, source, target, runtime);
    }
    throw std::runtime_error("unknown action kind");
}

void InputGatesAndAllocation(Kind kind) {
    Runtime runtime;
    runtime.kind = kind;
    Check(CreateAction(0u, kSource, kTarget, runtime) == 0u &&
              runtime.calls.empty(), "zero magic exits early");
    Check(CreateAction(kMagic, 0u, kTarget, runtime) == 0u &&
              runtime.calls.empty(), "zero source exits early");
    Check(CreateAction(kMagic, kSource, 0u, runtime) == 0u &&
              runtime.calls.empty(), "zero target exits early");
    for (bool inactive : {true, false}) {
        runtime = Runtime{};
        runtime.inactive = inactive;
        runtime.kind = kind;
        runtime.allocation = 0u;
        Check(CreateAction(kMagic, kSource, kTarget, runtime) == 0u &&
                  runtime.requested_size == 56u &&
                  runtime.calls == std::vector<std::string>{
                      "combat_inactive", "allocate"},
              "allocation failure exits before initialization");
    }
}

void BranchLayout(bool inactive, Kind kind) {
    Runtime runtime;
    runtime.inactive = inactive;
    runtime.kind = kind;
    Check(CreateAction(kMagic, kSource, kTarget, runtime) == kObject,
          "factory returns object");
    Check(runtime.requested_size == 56u &&
              runtime.words[kObject] == (kind == Kind::Stunt
                  ? (inactive ? pal4::QuickStuntSequenceTask::kVtable
                              : pal4::StuntSequenceTask::kVtable)
                  : kind == Kind::Property
                      ? (inactive
                          ? pal4::QuickUsePropertySequenceTask::kVtable
                          : pal4::UsePropertySequenceTask::kVtable)
                      : (inactive ? pal4::QuickCastMagicSequenceTask::kVtable
                                  : pal4::CastMagicSequenceTask::kVtable)) &&
              runtime.bytes[kObject + 20u] == 0x34u &&
              runtime.words[kObject + 36u] == 0u &&
              runtime.bytes[kObject + 40u] == 1u &&
              runtime.words[kObject + 44u] == kSource &&
              runtime.words[kObject + 48u] == kTarget &&
              runtime.words[kObject + 52u] == kMagic,
          "magic object layout and source low byte");
    Check(runtime.words[kObject + 28u] ==
              (kind == Kind::Stunt ? (inactive ? 10u : 5u)
                  : kind == Kind::Property ? (inactive ? 16u : 11u)
                                           : (inactive ? 14u : 9u)) &&
              Called(runtime, "set_name_flag") == inactive &&
              Index(runtime, "combat_inactive") < Index(runtime, "allocate") &&
              Index(runtime, "init_container") < Index(runtime, "allocate_name") &&
              Index(runtime, "copy_name") < Index(runtime, "word:4096"),
          "name header and construction order");
    if (!inactive) {
        Check(runtime.words[kObject + 24u] == 0u &&
                  runtime.words[kObject + 32u] == 0u,
              "active branch clears the name header");
    }
}

void FailureCleanup(Kind kind) {
    for (bool inactive : {true, false}) {
        Runtime runtime;
        runtime.inactive = inactive;
        runtime.kind = kind;
        runtime.name_allocation = false;
        Check(CreateAction(kMagic, kSource, kTarget, runtime) == kObject &&
                  !Called(runtime, "copy_name") &&
                  runtime.words[kObject + 52u] == kMagic,
              "name allocation failure still constructs object");

        runtime = Runtime{};
        runtime.inactive = inactive;
        runtime.kind = kind;
        runtime.fail_container = true;
        bool caught = false;
        try {
            (void)CreateAction(kMagic, kSource, kTarget, runtime);
        } catch (const std::runtime_error&) { caught = true; }
        Check(caught && runtime.calls.back() == "free" &&
                  !Called(runtime, "destroy_container") &&
                  !Called(runtime, "cleanup_name"),
              "failed container initialization frees raw object");

        runtime = Runtime{};
        runtime.inactive = inactive;
        runtime.kind = kind;
        runtime.fail_name_copy = true;
        caught = false;
        try {
            (void)CreateAction(kMagic, kSource, kTarget, runtime);
        } catch (const std::runtime_error&) { caught = true; }
        Check(caught && Index(runtime, "cleanup_name") <
                            Index(runtime, "destroy_container") &&
                  runtime.calls.back() == "free",
              "name failure releases name, container and object");
    }
}

}  // namespace

int main() {
    try {
        for (Kind kind : {Kind::Magic, Kind::Stunt, Kind::Property}) {
            InputGatesAndAllocation(kind);
            BranchLayout(true, kind);
            BranchLayout(false, kind);
            FailureCleanup(kind);
        }
        std::cout << "Combat named action factory tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
