#include "pal4/physical_attack_task.h"

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

constexpr std::uint32_t kParent = 0x1000u;
constexpr std::uint32_t kTask = 0x2000u;

struct Runtime final : pal4::IPhysicalAttackTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kParent + 44u, 0x3000u}, {kParent + 48u, 0x4000u}};
    std::map<std::uint32_t, std::uint8_t> bytes{
        {kParent + 52u, 1u}, {kParent + 53u, 0u},
        {kParent + 54u, 9u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = kTask;
    bool throw_on_init = false;
    bool throw_on_name = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read_byte:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("write_byte:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::PhysicalAttackTask::kSize,
              "PhysicalAttack allocation is 56 bytes");
        calls.push_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == kTask, "free receives task");
        calls.push_back("free");
    }
    void InitializeContainer(std::uint32_t container) override {
        Check(container == kTask + 4u, "container begins at task+4");
        calls.push_back("init_container");
        if (throw_on_init) throw std::runtime_error("init");
    }
    void DestroyContainer(std::uint32_t container) override {
        Check(container == kTask + 4u, "destroy begins at task+4");
        calls.push_back("destroy_container");
    }
    void AssignName(std::uint32_t name,
                    std::string_view value) override {
        Check(name == kTask + 20u && value == "PhysicalAttack",
              "PhysicalAttack name is assigned to task+20");
        calls.push_back("assign_name");
        if (throw_on_name) throw std::runtime_error("name");
        words[kTask + 28u] = static_cast<std::uint32_t>(value.size());
    }
    void CleanupName(std::uint32_t name) override {
        Check(name == kTask + 20u, "name cleanup begins at task+20");
        calls.push_back("cleanup_name");
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call occurred");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void ConstructorLayoutAndOrder() {
    Runtime runtime;
    Check(pal4::PhysicalAttackTask::Create(kParent, runtime) == kTask &&
          runtime.words[kTask] == pal4::PhysicalAttackTask::kVtable &&
          runtime.words[kTask + 24u] == 0u &&
          runtime.words[kTask + 28u] == 14u &&
          runtime.words[kTask + 32u] == 0u &&
          runtime.words[kTask + 36u] == 0u &&
          runtime.words[kTask + 44u] == 0x3000u &&
          runtime.words[kTask + 48u] == 0x4000u &&
          runtime.bytes[kTask + 20u] == 9u &&
          runtime.bytes[kTask + 40u] == 1u &&
          runtime.bytes[kTask + 52u] == 1u &&
          runtime.bytes[kTask + 53u] == 0u &&
          runtime.bytes[kTask + 54u] == 9u,
          "constructor writes name, actors, flags and vtable at IDA offsets");
    Check(Index(runtime, "read_byte:4149") <
              Index(runtime, "read_byte:4150") &&
          Index(runtime, "read_byte:4150") <
              Index(runtime, "read:4140") &&
          Index(runtime, "read:4140") <
              Index(runtime, "read_byte:4148") &&
          Index(runtime, "read_byte:4148") <
              Index(runtime, "read:4144") &&
          Index(runtime, "init_container") <
              Index(runtime, "assign_name") &&
          Index(runtime, "assign_name") < Index(runtime, "write:8192"),
          "parent reads and task construction follow IDA order");
}

void AllocationAndExceptionPaths() {
    Runtime runtime;
    runtime.allocation = 0u;
    Check(pal4::PhysicalAttackTask::Create(kParent, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"allocate"},
          "allocation failure returns before reading parent");

    runtime = Runtime{};
    runtime.throw_on_name = true;
    bool caught = false;
    try {
        (void)pal4::PhysicalAttackTask::Create(kParent, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught &&
          Index(runtime, "cleanup_name") <
              Index(runtime, "destroy_container") &&
          runtime.calls.back() == "free",
          "name failure releases name, container and storage");

    runtime = Runtime{};
    runtime.throw_on_init = true;
    caught = false;
    try {
        (void)pal4::PhysicalAttackTask::Create(kParent, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "free" &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "destroy_container") == runtime.calls.end(),
          "container initialization failure frees raw allocation only");
}

}  // namespace

int main() {
    ConstructorLayoutAndOrder();
    AllocationAndExceptionPaths();
    std::cout << "PhysicalAttack task tests passed\n";
}
