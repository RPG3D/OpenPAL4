#include "pal4/gob_obstacle.h"

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

struct Runtime final : pal4::IGobObstacleRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation = object;
    std::uint32_t cleanup_result = 0x77u;
    bool fail_machine = false;
    bool fail_helper = false;
    bool needs_dialog_update = false;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::GobObstacle::kSize, "obstacle allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == object, "free address");
        calls.emplace_back("free");
    }
    void ConstructMachine(std::uint32_t address) override {
        Check(address == object, "machine constructor address");
        calls.emplace_back("construct machine");
        if (fail_machine) throw std::runtime_error("machine failed");
    }
    void DestructMachine(std::uint32_t address) override {
        Check(address == object, "machine destructor address");
        calls.emplace_back("destruct machine");
    }
    std::uint32_t CleanupMachineResources(std::uint32_t address) override {
        Check(address == object, "machine cleanup address");
        calls.emplace_back("cleanup machine");
        return cleanup_result;
    }
    void InitializeObstacleHelper(std::uint32_t address) override {
        Check(address == object + 0x2ccu, "helper init offset");
        calls.emplace_back("initialize helper");
        if (fail_helper) throw std::runtime_error("helper failed");
    }
    void DestructObstacleHelper(std::uint32_t address) override {
        Check(address == object + 0x2ccu, "helper destructor offset");
        calls.emplace_back("destruct helper");
    }
    void InitializeStringRecord(std::uint32_t address) override {
        Check(address == object + 0x308u || address == object + 0x320u,
              "string init offset");
        calls.emplace_back(address == object + 0x308u ?
            "initialize first string" : "initialize second string");
    }
    void ReleaseStringRecord(std::uint32_t address) override {
        Check(address == object + 0x308u || address == object + 0x320u,
              "string release offset");
        calls.emplace_back(address == object + 0x308u ?
            "release first string" : "release second string");
    }
    void InitializeEffectHandle(std::uint32_t address) override {
        Check(address == object + 0x32cu, "effect init offset");
        calls.emplace_back("initialize effect handle");
    }
    void ResetEffectHandle(std::uint32_t address) override {
        Check(address == object + 0x32cu, "effect reset offset");
        calls.emplace_back("reset effect handle");
    }
    bool DialogNeedsUpdate(std::uint32_t address) override {
        Check(address == object + 0x32cu, "dialog check offset");
        calls.emplace_back("dialog needs update");
        return needs_dialog_update;
    }
    void UpdateDialogContent(std::uint32_t address) override {
        Check(address == object + 0x32cu, "dialog update offset");
        calls.emplace_back("update dialog");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write " + std::to_string(address - object));
    }
};

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::GobObstacle::Create(runtime) == Runtime::object &&
          runtime.calls == (std::vector<std::string>{
              "allocate", "construct machine", "initialize helper",
              "initialize first string", "initialize second string",
              "initialize effect handle", "write 0", "write 476",
              "write 372", "write 756"}) &&
          runtime.words[Runtime::object] == pal4::GobObstacle::kVtable &&
          runtime.words[Runtime::object + 0x1dcu] ==
              pal4::GobObstacle::kStateVtable &&
          runtime.words[Runtime::object + 0x174u] == 1u &&
          runtime.words[Runtime::object + 0x2f4u] == 0u,
          "obstacle constructor layout and order");
    Check(pal4::GobObstacle::GetObstacleRuntimeHelper(Runtime::object) ==
              Runtime::object + 0x2ccu &&
          pal4::GobObstacle::TypeKey() == 10u,
          "obstacle helper offset and type key");
    runtime.calls.clear();
    pal4::GobObstacle::Destruct(Runtime::object, runtime);
    Check(runtime.calls == (std::vector<std::string>{
              "write 0", "write 476", "reset effect handle",
              "release second string", "release first string",
              "destruct helper", "destruct machine"}),
          "obstacle destructor reverse order");
    Runtime null;
    null.allocation = 0u;
    Check(pal4::GobObstacle::Create(null) == 0u &&
          null.calls == (std::vector<std::string>{"allocate"}),
          "obstacle null allocation");
    Runtime failed_machine;
    failed_machine.fail_machine = true;
    try {
        (void)pal4::GobObstacle::Create(failed_machine);
        throw std::runtime_error("missing constructor exception");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "machine failed" &&
              failed_machine.calls == (std::vector<std::string>{
                  "allocate", "construct machine", "free"}),
              "machine constructor failure releases allocation");
    }
    Runtime failed_helper;
    failed_helper.fail_helper = true;
    try {
        (void)pal4::GobObstacle::Create(failed_helper);
        throw std::runtime_error("missing helper exception");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "helper failed" &&
              failed_helper.calls == (std::vector<std::string>{
                  "allocate", "construct machine", "initialize helper", "free"}),
              "helper constructor failure releases allocation");
    }
}

void CheckDialogCleanup() {
    Runtime quiet;
    Check(pal4::GobObstacle::UpdateDialogAndCleanup(
              Runtime::object, quiet) == 0x77u &&
          quiet.calls == (std::vector<std::string>{
              "dialog needs update", "cleanup machine"}),
          "quiet dialog delegates machine cleanup");
    Runtime active;
    active.needs_dialog_update = true;
    active.cleanup_result = 0u;
    Check(pal4::GobObstacle::UpdateDialogAndCleanup(
              Runtime::object, active) == 0u &&
          active.calls == (std::vector<std::string>{
              "dialog needs update", "update dialog", "cleanup machine"}),
          "dialog update precedes machine cleanup and forwards zero");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckDialogCleanup();
        std::cout << "gob_obstacle_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
