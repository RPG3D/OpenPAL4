#include "pal4/combat_action_factory.h"
#include "pal4/combat_end_action_task.h"
#include "pal4/level_up_action_task.h"
#include "pal4/win_action_task.h"

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

struct Runtime final : pal4::ICombatLevelUpActionFactoryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool inactive = false;
    bool name_allocation = true;
    bool fail_container = false;
    bool fail_copy = false;
    bool fail_reallocate = false;
    std::uint32_t allocation = kObject;
    std::uint32_t requested_size = 0u;
    std::uint32_t actor_state = 0u;
    std::string copied_name;

    bool CombatInactive() override {
        calls.emplace_back("combat");
        return inactive;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kActor + 828u, "actor state read offset +828");
        calls.emplace_back("actor_state");
        return actor_state;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.emplace_back("allocate");
        requested_size = size;
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "free original object");
        calls.emplace_back("free");
    }
    void InitializeContainer(std::uint32_t container) override {
        Check(container == kObject + 4u, "container address");
        calls.emplace_back("init_container");
        if (fail_container) throw std::runtime_error("container init");
    }
    void DestroyContainer(std::uint32_t container) override {
        Check(container == kObject + 4u, "destroy container address");
        calls.emplace_back("destroy_container");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("word:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void SetNameErrorFlag(std::uint32_t header, std::uint8_t flag) override {
        Check(header == kObject + 20u, "name header");
        calls.emplace_back("flag:" + std::to_string(flag));
        words[header + 4u] = 0u;
        words[header + 8u] = 0u;
        words[header + 12u] = 0u;
    }
    bool AllocateName(std::uint32_t header, std::uint32_t length,
                      std::uint8_t flag) override {
        Check(header == kObject + 20u && flag == 1u && length == 9u,
              "name allocation arguments");
        calls.emplace_back("allocate_name");
        return name_allocation;
    }
    void ReallocateName(std::uint32_t header,
                        std::uint32_t length) override {
        Check(header == kObject + 20u &&
                  (length == 9u || length == 7u),
              "active action reallocation arguments");
        calls.emplace_back("reallocate_name");
        if (fail_reallocate) throw std::runtime_error("name reallocation");
    }
    void CopyName(std::uint32_t header, std::string_view name) override {
        Check(header == kObject + 20u, "name copy header");
        calls.emplace_back("copy_name");
        if (fail_copy) throw std::runtime_error("name copy");
        copied_name = name;
    }
    void SetNameSize(std::uint32_t header,
                     std::uint32_t size) override {
        Check(header == kObject + 20u &&
                  (size == 9u || size == 7u),
              "name size arguments");
        calls.emplace_back("set_name_size");
        words[header + 8u] = size;
    }
    void CleanupName(std::uint32_t header) override {
        Check(header == kObject + 20u, "cleanup name header");
        calls.emplace_back("cleanup_name");
    }
};

bool Called(const Runtime& runtime, const std::string& call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void GatesAndLayouts() {
    Runtime runtime;
    Check(pal4::CombatEndActionFactory::Create(0u, runtime) == 0u &&
              pal4::CombatWinActionFactory::Create(0u, runtime) == 0u &&
              runtime.calls.empty(), "zero actor exits before combat lookup");
    runtime.inactive = true;
    Check(pal4::CombatWinActionFactory::Create(kActor, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"combat"},
          "inactive combat rejects WinAction before allocation");

    for (bool inactive : {false, true}) {
        runtime = Runtime{};
        runtime.inactive = inactive;
        Check(pal4::CombatEndActionFactory::Create(kActor, runtime) ==
                  kObject &&
                  runtime.requested_size == pal4::CombatEndActionTask::kSize &&
                  runtime.copied_name == "EndAction" &&
                  runtime.words.at(kObject) ==
                      pal4::CombatEndActionTask::kVtable &&
                  runtime.words.at(kObject + 24u) == 0u &&
                  runtime.words.at(kObject + 28u) == 9u &&
                  runtime.words.at(kObject + 32u) == 0u &&
                  runtime.words.at(kObject + 36u) == 0u &&
                  runtime.bytes.at(kObject + 20u) == 0x34u &&
                  runtime.bytes.at(kObject + 40u) == 1u &&
                  runtime.words.at(kObject + 44u) == kActor,
              "EndAction name and 48-byte task layout");
        Check(Called(runtime, "flag:0") == inactive &&
                  Index(runtime, "init_container") <
                      Index(runtime, "allocate_name") &&
                  Index(runtime, "copy_name") <
                      Index(runtime, "set_name_size") &&
                  Index(runtime, "set_name_size") <
                      Index(runtime, "word:4096"),
              "EndAction branch and construction order");
    }

    runtime = Runtime{};
    Check(pal4::CombatWinActionFactory::Create(kActor, runtime) == kObject &&
              runtime.requested_size == pal4::WinActionTask::kSize &&
              runtime.copied_name == "WinAction" &&
              runtime.words.at(kObject) == pal4::WinActionTask::kVtable &&
              runtime.words.at(kObject + 28u) == 9u &&
              runtime.words.at(kObject + 36u) == 0u &&
              runtime.bytes.at(kObject + 20u) == 0x34u &&
              runtime.bytes.at(kObject + 40u) == 1u &&
              runtime.words.at(kObject + 44u) == kActor,
          "WinAction name and task layout");
    Check(Called(runtime, "flag:1") &&
              !Called(runtime, "allocate_name") &&
              Index(runtime, "flag:1") <
                  Index(runtime, "reallocate_name") &&
              Index(runtime, "reallocate_name") <
                  Index(runtime, "copy_name"),
          "WinAction uses string reallocation path");
}

void AllocationAndNameFailures() {
    for (bool win : {false, true}) {
        Runtime runtime;
        runtime.allocation = 0u;
        Check((win ? pal4::CombatWinActionFactory::Create(kActor, runtime)
                   : pal4::CombatEndActionFactory::Create(kActor, runtime)) ==
                  0u &&
                  runtime.calls ==
                      std::vector<std::string>{"combat", "allocate"},
              "allocation failure skips construction");

        runtime = Runtime{};
        runtime.fail_container = true;
        bool caught = false;
        try {
            (void)(win ? pal4::CombatWinActionFactory::Create(kActor, runtime)
                       : pal4::CombatEndActionFactory::Create(kActor, runtime));
        } catch (const std::runtime_error&) { caught = true; }
        Check(caught && runtime.calls.back() == "free" &&
                  !Called(runtime, "cleanup_name") &&
                  !Called(runtime, "destroy_container"),
              "container failure frees raw task");

        runtime = Runtime{};
        runtime.fail_copy = true;
        caught = false;
        try {
            (void)(win ? pal4::CombatWinActionFactory::Create(kActor, runtime)
                       : pal4::CombatEndActionFactory::Create(kActor, runtime));
        } catch (const std::runtime_error&) { caught = true; }
        Check(caught && Index(runtime, "cleanup_name") <
                            Index(runtime, "destroy_container") &&
                  runtime.calls.back() == "free",
              "name failure cleans task in reverse construction order");
    }

    Runtime runtime;
    runtime.name_allocation = false;
    Check(pal4::CombatEndActionFactory::Create(kActor, runtime) == kObject &&
              !Called(runtime, "copy_name") &&
              !Called(runtime, "set_name_size"),
          "EndAction survives false name allocation result");

    runtime = Runtime{};
    runtime.fail_reallocate = true;
    bool caught = false;
    try {
        (void)pal4::CombatWinActionFactory::Create(kActor, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && Index(runtime, "cleanup_name") <
                        Index(runtime, "destroy_container") &&
              runtime.calls.back() == "free",
          "WinAction reallocation failure cleans name and container");
}

void LevelUpGatesAndLayout() {
    Runtime runtime;
    Check(pal4::CombatLevelUpActionFactory::Create(0u, runtime) == 0u &&
              runtime.calls.empty(),
          "null LevelUp actor exits before state read");
    runtime.actor_state = 7u;
    Check(pal4::CombatLevelUpActionFactory::Create(kActor, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"actor_state"},
          "state 7 rejects LevelUp before combat lookup");
    runtime = Runtime{};
    runtime.inactive = true;
    Check(pal4::CombatLevelUpActionFactory::Create(kActor, runtime) == 0u &&
              runtime.calls ==
                  std::vector<std::string>{"actor_state", "combat"},
          "inactive combat rejects LevelUp before allocation");

    runtime = Runtime{};
    Check(pal4::CombatLevelUpActionFactory::Create(kActor, runtime) ==
              kObject &&
              runtime.requested_size == pal4::LevelUpActionTask::kSize &&
              runtime.copied_name == "LevelUp" &&
              runtime.words.at(kObject) ==
                  pal4::LevelUpActionTask::kVtable &&
              runtime.words.at(kObject + 24u) == 0u &&
              runtime.words.at(kObject + 28u) == 7u &&
              runtime.words.at(kObject + 32u) == 0u &&
              runtime.words.at(kObject + 36u) == 0u &&
              runtime.bytes.at(kObject + 20u) == 0x34u &&
              runtime.bytes.at(kObject + 40u) == 1u &&
              runtime.words.at(kObject + 44u) == kActor,
          "LevelUp name and 48-byte task layout");
    Check(Index(runtime, "actor_state") < Index(runtime, "combat") &&
              Index(runtime, "flag:1") <
                  Index(runtime, "reallocate_name") &&
              Index(runtime, "reallocate_name") <
                  Index(runtime, "copy_name") &&
              Index(runtime, "copy_name") <
                  Index(runtime, "set_name_size"),
          "LevelUp state gate and string reallocation order");

    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(pal4::CombatLevelUpActionFactory::Create(kActor, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "actor_state", "combat", "allocate"},
          "LevelUp allocation failure exits before container init");
    runtime = Runtime{};
    runtime.fail_reallocate = true;
    bool caught = false;
    try {
        (void)pal4::CombatLevelUpActionFactory::Create(kActor, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && Index(runtime, "cleanup_name") <
                        Index(runtime, "destroy_container") &&
              runtime.calls.back() == "free",
          "LevelUp reallocation failure cleans partial task");
}

}  // namespace

int main() {
    GatesAndLayouts();
    AllocationAndNameFailures();
    LevelUpGatesAndLayout();
    std::cout << "Combat End/Win action factory tests passed\n";
}
