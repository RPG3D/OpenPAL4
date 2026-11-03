#include "pal4/combat_script_task_operations.h"

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

struct Runtime final : pal4::INormalAttackCreationRuntime {
    std::uint32_t allocation = kTask;
    std::uint32_t size = 0u;
    bool throw_on_base = false;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    void FireEvent(std::uint32_t, std::uint32_t,
                   std::uint32_t) override {}
    void SetProtectTarget(std::uint32_t, std::uint32_t,
                          std::uint32_t) override {}
    void ConstructBase(std::uint32_t task) override {
        Check(task == kTask, "NormalAttack base receives allocation");
        calls.push_back("base");
        if (throw_on_base) throw std::runtime_error("base");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate");
        size = bytes;
        return allocation;
    }
    void Free(std::uint32_t task) override {
        Check(task == kTask, "free receives task allocation");
        calls.push_back("free");
    }
};

void Construction() {
    Runtime runtime;
    Check(pal4::NormalAttackTask::Create(runtime) == kTask &&
              runtime.size == 76u &&
              runtime.words.at(kTask) == pal4::NormalAttackTask::kVtable &&
              runtime.words.at(kTask + 64u) == 0u &&
              runtime.words.at(kTask + 60u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "base", "write:4096",
                  "write:4160", "write:4156"},
          "factory composes NormalAttack constructor in IDA order");
}

void Failure() {
    Runtime runtime;
    runtime.allocation = 0u;
    Check(pal4::NormalAttackTask::Create(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "null allocation skips construction");
    runtime = Runtime{};
    runtime.throw_on_base = true;
    bool caught = false;
    try {
        (void)pal4::NormalAttackTask::Create(runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate", "base", "free"},
          "base exception releases allocation");
}

}  // namespace

int main() {
    Construction();
    Failure();
    std::cout << "NormalAttack factory tests passed\n";
}
