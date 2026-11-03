#include "pal4/lock_control_task.h"

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

struct Runtime final : pal4::ILockControlTaskRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t actor = 0x5000u;
    std::uint32_t expected_mode = 0u;

    Runtime() { words[object + 0x40u] = 0x4000u; }
    void BaseConstruct(std::uint32_t address) override {
        Check(address == object, "lockCtrl constructor address");
        calls.emplace_back("base construct");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write vtable");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("write completion");
    }
    void ResetActorAndSync(std::uint32_t mode) override {
        Check(mode == expected_mode, "control reset mode");
        calls.emplace_back("reset actor");
    }
    void ProcessUiEvents(std::uint32_t mode) override {
        Check(mode == expected_mode, "control UI mode");
        calls.emplace_back("process UI");
    }
    std::uint32_t CurrentActor() override {
        calls.emplace_back("current actor");
        return actor;
    }
    void SetActorState(std::uint32_t address,
                       std::string_view state) override {
        Check(address == actor && state == "C01",
              "lockCtrl actor state");
        calls.emplace_back("actor state");
    }
    void SetPlayerState(std::uint32_t value) override {
        Check(value == expected_mode, "control player state");
        calls.emplace_back("player state");
    }
};

void CheckConstruct() {
    Runtime runtime;
    Check(pal4::LockControlTask::Construct(Runtime::object, runtime) ==
              Runtime::object &&
              runtime.words.at(Runtime::object) == 0x848834u &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"base construct",
                                            "write vtable",
                                            "write completion"}),
          "lockCtrl constructor layout and order");
}

void CheckEnter() {
    Runtime missing;
    missing.words[Runtime::object + 0x40u] = 0u;
    Check(pal4::LockControlTask::Enter(Runtime::object, missing) ==
              0u && missing.calls.empty(),
          "lockCtrl requires owner");
    Runtime runtime;
    Check(pal4::LockControlTask::Enter(Runtime::object, runtime) ==
              1u && runtime.bytes.at(Runtime::object + 0x3cu) ==
                        1u &&
              runtime.calls ==
                  (std::vector<std::string>{"reset actor",
                                            "process UI",
                                            "current actor",
                                            "actor state",
                                            "player state",
                                            "write completion"}),
          "lockCtrl applies control lock in IDA order");
    Runtime no_actor;
    no_actor.actor = 0u;
    Check(pal4::LockControlTask::Enter(Runtime::object, no_actor) ==
              1u &&
              no_actor.calls ==
                  (std::vector<std::string>{"reset actor",
                                            "process UI",
                                            "current actor",
                                            "player state",
                                            "write completion"}),
          "lockCtrl skips absent actor state");
}

void CheckUnlock() {
    Runtime missing;
    missing.expected_mode = 1u;
    missing.words[Runtime::object + 0x40u] = 0u;
    Check(pal4::UnlockControlTask::Enter(
              Runtime::object, missing) == 0u &&
              missing.calls.empty(), "unlockCtrl owner gate");
    Runtime runtime;
    runtime.expected_mode = 1u;
    Check(pal4::UnlockControlTask::Construct(
              Runtime::object, runtime) == Runtime::object &&
              runtime.words.at(Runtime::object) == 0x8489d8u &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 0u,
          "unlockCtrl constructor layout");
    runtime.calls.clear();
    Check(pal4::UnlockControlTask::Enter(
              Runtime::object, runtime) == 1u &&
              runtime.bytes.at(Runtime::object + 0x3cu) == 1u &&
              runtime.calls ==
                  (std::vector<std::string>{"reset actor", "process UI",
                                            "player state",
                                            "write completion"}),
          "unlockCtrl applies mode-one state in IDA order");
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckEnter();
        CheckUnlock();
        std::cout << "lock_control_task_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
