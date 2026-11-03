#include "pal4/end_attack_child_task.h"

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

constexpr std::uint32_t kChild = 0x1000u;

struct Runtime final : pal4::IEndAttackChildRuntime {
    std::uint32_t allocation = kChild;
    std::uint32_t size = 0u;
    bool throw_on_base = false;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate");
        size = bytes;
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kChild, "free receives child allocation");
        calls.push_back("free");
    }
    void ConstructBase(std::uint32_t object) override {
        Check(object == kChild, "base constructor receives child");
        calls.push_back("base");
        if (throw_on_base) throw std::runtime_error("base");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write");
        words[address] = value;
    }
};

void Construction() {
    Runtime runtime;
    Check(pal4::EndAttackBaseChildTask::Create(runtime) == kChild &&
              runtime.size == 68u &&
              runtime.words.at(kChild) ==
                  pal4::EndAttackBaseChildTask::kVtable &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "base", "write"},
          "EndAttack child installs derived vtable after base");
}

void Failure() {
    Runtime runtime;
    runtime.allocation = 0u;
    Check(pal4::EndAttackBaseChildTask::Create(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "null allocation skips base constructor");
    runtime = Runtime{};
    runtime.throw_on_base = true;
    bool caught = false;
    try {
        (void)pal4::EndAttackBaseChildTask::Create(runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate", "base", "free"},
          "base exception frees allocated child");
}

}  // namespace

int main() {
    Construction();
    Failure();
    std::cout << "EndAttack child task tests passed\n";
}
