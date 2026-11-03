#include "pal4/combat_wounded_action_factory.h"
#include "pal4/combat_wounded_sequence_task.h"

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

struct Runtime final : pal4::IWoundedActionFactoryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool terminal = false;
    bool inactive = true;
    std::uint32_t action_kind = 3u;
    std::uint32_t stance = 2u;
    std::uint32_t marker = 1u;
    bool fail_container = false;
    bool fail_mode_name = false;
    bool fail_message = false;
    std::uint32_t allocation = kObject;
    std::uint32_t requested_size = 0u;

    bool ActorInTerminalState(std::uint32_t actor) override {
        Check(actor == kSource, "terminal-state actor");
        calls.push_back("terminal_state");
        return terminal;
    }
    bool CombatInactive() override {
        calls.push_back("combat_inactive");
        return inactive;
    }
    std::uint32_t ActorActionKind(std::uint32_t actor) override {
        Check(actor == kSource, "actor action kind argument");
        calls.push_back("action_kind");
        return action_kind;
    }
    std::uint32_t ActorWord(std::uint32_t actor,
                            std::uint32_t offset) override {
        Check(actor == kSource, "actor word argument");
        calls.push_back("actor_word:" + std::to_string(offset));
        if (offset == 804u) return stance;
        if (offset == 1056u) return marker;
        throw std::runtime_error("unexpected actor offset");
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate");
        requested_size = size;
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "free original task");
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
        Check(!inactive && header == kObject + 20u && flag == 0u,
              "FinalHit name flag arguments");
        calls.push_back("set_name_flag");
        words[header + 4u] = 0u;
        words[header + 8u] = 0u;
        words[header + 12u] = 0u;
    }
    bool AllocateName(std::uint32_t, std::uint32_t,
                      std::uint8_t) override {
        throw std::runtime_error("unexpected DialogTextBufferAllocate");
    }
    void CopyName(std::uint32_t, std::string_view) override {
        throw std::runtime_error("unexpected direct name copy");
    }
    void InitializeModeName(std::uint32_t header,
                            std::string_view name,
                            std::uint8_t mode) override {
        Check(inactive && header == kObject + 20u &&
                  name == "QuickWounded" && mode == 0x34u,
              "QuickWounded mode-string arguments");
        calls.push_back("init_mode_name");
        bytes[header] = mode;
        words[header + 4u] = 0u;
        words[header + 8u] = 0u;
        words[header + 12u] = 0u;
        if (fail_mode_name) throw std::runtime_error("mode name failed");
        words[header + 8u] = static_cast<std::uint32_t>(name.size());
    }
    void SetExceptionMessage(std::uint32_t header,
                             std::string_view name) override {
        Check(!inactive && header == kObject + 20u &&
                  name == (action_kind == 3u && stance == 2u && marker != 0u
                      ? "FinalHit" : "WoundedTask"),
              "active Wounded message arguments");
        calls.push_back("set_message");
        if (fail_message) throw std::runtime_error("message failed");
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

void EarlyGates() {
    Runtime runtime;
    Check(pal4::CombatWoundedActionFactory::Create(0u, kTarget,
                                                    runtime) == 0u &&
              runtime.calls.empty(), "zero source exits before actor lookup");
    runtime.terminal = true;
    Check(pal4::CombatWoundedActionFactory::Create(kSource, kTarget,
                                                    runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"terminal_state"},
          "terminal actor exits before combat lookup");
    for (bool inactive : {true, false}) {
        runtime = Runtime{};
        runtime.inactive = inactive;
        Check(pal4::CombatWoundedActionFactory::Create(kSource, 0u,
                                                        runtime) == kObject &&
                  runtime.words.at(kObject + (inactive ? 44u : 48u)) == 0u,
              "zero target is accepted and stored in branch-specific slot");
    }
}

void BranchLayout(bool inactive, bool final_hit) {
    Runtime runtime;
    runtime.inactive = inactive;
    runtime.marker = final_hit ? 1u : 0u;
    Check(pal4::CombatWoundedActionFactory::Create(
              kSource, kTarget, runtime) == kObject,
          "Wounded factory returns task");
    Check(runtime.requested_size == 52u &&
              runtime.words.at(kObject) == (inactive
                  ? pal4::QuickWoundedSequenceTask::kVtable
                  : final_hit ? pal4::FinalHitSequenceTask::kVtable
                              : pal4::WoundedSequenceTask::kVtable) &&
              runtime.bytes.at(kObject + 20u) ==
                  static_cast<std::uint8_t>(inactive ? kSource : kTarget) &&
              runtime.words.at(kObject + 28u) == (inactive
                  ? 12u : final_hit ? 8u : 11u) &&
              runtime.words.at(kObject + 36u) == 0u &&
              runtime.bytes.at(kObject + 40u) == 1u &&
              runtime.words.at(kObject + 44u) ==
                  (inactive ? kTarget : kSource) &&
              runtime.words.at(kObject + 48u) ==
                  (inactive ? kSource : kTarget),
          "three Wounded layouts, text modes and actor direction");
    Check(runtime.calls.front() == "terminal_state" &&
              Index(runtime, "combat_inactive") < Index(runtime, "allocate") &&
              Index(runtime, "init_container") < Index(runtime, "word:4096"),
          "actor/combat gates precede allocation and final vtable");
    if (inactive) {
        Check(!Called(runtime, "action_kind") &&
                  !Called(runtime, "set_message") &&
                  !Called(runtime, "set_name_flag") &&
                  Index(runtime, "init_mode_name") <
                      Index(runtime, "word:4132") &&
                  Index(runtime, "word:4140") <
                      Index(runtime, "word:4144"),
              "QuickWounded uses mode constructor and reversed actor slots");
    } else if (final_hit) {
        Check(Index(runtime, "action_kind") <
                  Index(runtime, "actor_word:804") &&
                  Index(runtime, "actor_word:804") <
                      Index(runtime, "actor_word:1056") &&
                  Index(runtime, "set_name_flag") <
                      Index(runtime, "set_message") &&
                  !Called(runtime, "init_mode_name"),
              "FinalHit gate and name sequence");
    } else {
        Check(!Called(runtime, "set_name_flag") &&
                  Index(runtime, "word:4120") <
                      Index(runtime, "set_message") &&
                  Index(runtime, "word:4124") <
                      Index(runtime, "set_message") &&
                  Index(runtime, "word:4128") <
                      Index(runtime, "set_message"),
              "WoundedTask clears header before message assignment");
    }
}

void FinalHitShortCircuit() {
    Runtime runtime;
    runtime.inactive = false;
    runtime.action_kind = 2u;
    Check(pal4::CombatWoundedActionFactory::Create(kSource, kTarget,
                                                    runtime) == kObject &&
              runtime.words.at(kObject) ==
                  pal4::WoundedSequenceTask::kVtable &&
              !Called(runtime, "actor_word:804") &&
              !Called(runtime, "actor_word:1056"),
          "action kind mismatch skips actor fields");

    runtime = Runtime{};
    runtime.inactive = false;
    runtime.stance = 1u;
    Check(pal4::CombatWoundedActionFactory::Create(kSource, kTarget,
                                                    runtime) == kObject &&
              Called(runtime, "actor_word:804") &&
              !Called(runtime, "actor_word:1056") &&
              runtime.words.at(kObject) ==
                  pal4::WoundedSequenceTask::kVtable,
          "stance mismatch skips final marker");
}

void FailurePaths(bool inactive, bool final_hit) {
    Runtime runtime;
    runtime.inactive = inactive;
    runtime.marker = final_hit ? 1u : 0u;
    runtime.allocation = 0u;
    Check(pal4::CombatWoundedActionFactory::Create(
              kSource, kTarget, runtime) == 0u &&
              runtime.requested_size == 52u &&
              !Called(runtime, "init_container"),
          "allocation failure exits before container initialization");

    runtime = Runtime{};
    runtime.inactive = inactive;
    runtime.marker = final_hit ? 1u : 0u;
    runtime.fail_container = true;
    bool caught = false;
    try {
        (void)pal4::CombatWoundedActionFactory::Create(
            kSource, kTarget, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls.back() == "free" &&
              !Called(runtime, "cleanup_name") &&
              !Called(runtime, "destroy_container"),
          "container failure frees raw allocation");

    runtime = Runtime{};
    runtime.inactive = inactive;
    runtime.marker = final_hit ? 1u : 0u;
    runtime.fail_mode_name = inactive;
    runtime.fail_message = !inactive;
    caught = false;
    try {
        (void)pal4::CombatWoundedActionFactory::Create(
            kSource, kTarget, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && Index(runtime, "cleanup_name") <
                        Index(runtime, "destroy_container") &&
              runtime.calls.back() == "free" &&
              Called(runtime, inactive ? "init_mode_name" : "set_message"),
          "text failure releases name, container and task");
}

}  // namespace

int main() {
    try {
        EarlyGates();
        BranchLayout(true, false);
        BranchLayout(false, false);
        BranchLayout(false, true);
        FinalHitShortCircuit();
        FailurePaths(true, false);
        FailurePaths(false, false);
        FailurePaths(false, true);
        std::cout << "Combat wounded action factory tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
