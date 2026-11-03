#include "pal4/combat_defend_status_child_task.h"

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

constexpr std::uint32_t kObject = 0x1000u;

struct Runtime final : pal4::IDefendStatusChildRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation = kObject;
    bool fail_base = false;
    bool fail_vtable = false;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 64u, "Defend child allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "Defend child free address");
        calls.emplace_back("free");
    }
    void ConstructBase(std::uint32_t object) override {
        Check(object == kObject, "Defend child base constructor address");
        calls.emplace_back("construct_base");
        if (fail_base) throw std::runtime_error("base construction");
    }
    void DestroyBase(std::uint32_t object) override {
        Check(object == kObject, "Defend child base destructor address");
        calls.emplace_back("destroy_base");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == kObject &&
                  value == pal4::DefendStatusChildTask::kVtable,
              "Defend child final vtable");
        calls.emplace_back("write_vtable");
        if (fail_vtable) throw std::runtime_error("vtable write");
        words[address] = value;
    }
};

void SuccessAndAllocationFailure() {
    Runtime runtime;
    Check(pal4::DefendStatusChildTask::Create(runtime) == kObject &&
              runtime.words.at(kObject) == 0x845cf4u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "construct_base", "write_vtable"},
          "Defend child constructor order and layout");
    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(pal4::DefendStatusChildTask::Create(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "null allocation exits before base construction");
}

void FailureCleanup() {
    Runtime runtime;
    runtime.fail_base = true;
    bool caught = false;
    try { (void)pal4::DefendStatusChildTask::Create(runtime); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate", "construct_base", "free"},
          "base construction failure frees raw storage");

    runtime = Runtime{};
    runtime.fail_vtable = true;
    caught = false;
    try { (void)pal4::DefendStatusChildTask::Create(runtime); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate", "construct_base", "write_vtable",
              "destroy_base", "free"},
          "vtable failure destroys base before freeing storage");
}

}  // namespace

int main() {
    SuccessAndAllocationFailure();
    FailureCleanup();
    std::cout << "Defend status child task tests passed\n";
}
