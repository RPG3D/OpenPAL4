#include "pal4/start_attack_task.h"

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

struct Runtime final : pal4::IStartAttackTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kParent + 44u, 0x3000u}, {kParent + 48u, 0x4000u},
        {kParent + 52u, 0x5000u}};
    std::map<std::uint32_t, std::uint8_t> bytes{{kParent + 56u, 7u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = kTask;
    bool name_allocation = true;
    bool throw_on_copy = false;
    bool throw_on_init = false;

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
        Check(size == pal4::StartAttackTask::kSize,
              "StartAttack allocation is 56 bytes");
        calls.push_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == kTask, "free receives task");
        calls.push_back("free");
    }
    void InitializeContainer(std::uint32_t container) override {
        Check(container == kTask + 4u, "GOM container starts at +4");
        calls.push_back("init_container");
        if (throw_on_init) throw std::runtime_error("init");
    }
    void DestroyContainer(std::uint32_t container) override {
        Check(container == kTask + 4u, "GOM destroy starts at +4");
        calls.push_back("destroy_container");
    }
    void SetNameErrorFlag(std::uint32_t name,
                          std::uint8_t flag) override {
        Check(name == kTask + 20u && flag == 0u,
              "selected branch clears text error flag");
        calls.push_back("set_error_flag");
        words[kTask + 24u] = 0u;
        words[kTask + 28u] = 0u;
        words[kTask + 32u] = 0u;
    }
    bool AllocateNameBuffer(std::uint32_t name,
                            std::uint32_t length,
                            std::uint8_t flag) override {
        Check(name == kTask + 20u && length == 11u && flag == 1u,
              "text allocator receives StartAttack length and flag");
        calls.push_back("allocate_name");
        return name_allocation;
    }
    void CopyName(std::uint32_t name,
                  std::string_view value) override {
        Check(name == kTask + 20u && value == "StartAttack",
              "text copier receives StartAttack name");
        calls.push_back("copy_name");
        if (throw_on_copy) throw std::runtime_error("copy");
    }
    void CleanupName(std::uint32_t name) override {
        Check(name == kTask + 20u, "text cleanup receives name header");
        calls.push_back("cleanup_name");
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call occurred");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void SelectedTargetBranch() {
    Runtime runtime;
    Check(pal4::StartAttackTask::Create(kParent, runtime) == kTask &&
          runtime.words[kTask] == pal4::StartAttackTask::kVtable &&
          runtime.words[kTask + 28u] == 11u &&
          runtime.words[kTask + 36u] == 0u &&
          runtime.words[kTask + 44u] == 0x3000u &&
          runtime.words[kTask + 48u] == 0x5000u &&
          runtime.bytes[kTask + 20u] == 7u &&
          runtime.bytes[kTask + 40u] == 1u &&
          runtime.bytes[kTask + 52u] == 0u &&
          runtime.bytes[kTask + 53u] == 1u &&
          runtime.bytes[kTask + 54u] == 7u,
          "selected branch writes task name, actors and mode flags");
    Check(Index(runtime, "read_byte:4152") <
              Index(runtime, "read:4140") &&
          Index(runtime, "init_container") <
              Index(runtime, "set_error_flag") &&
          Index(runtime, "set_error_flag") <
              Index(runtime, "allocate_name") &&
          Index(runtime, "copy_name") < Index(runtime, "write:8192"),
          "selected branch preserves IDA read and construction order");
}

void UnselectedBranch() {
    Runtime runtime;
    runtime.words[kParent + 52u] = 0u;
    Check(pal4::StartAttackTask::Create(kParent, runtime) == kTask &&
          runtime.words[kTask + 44u] == 0x3000u &&
          runtime.words[kTask + 48u] == 0x4000u &&
          runtime.bytes[kTask + 52u] == 1u &&
          runtime.bytes[kTask + 53u] == 0u &&
          Index(runtime, "read:4140") <
              Index(runtime, "read_byte:4152") &&
          Index(runtime, "read_byte:4152") <
              Index(runtime, "read:4144"),
          "unselected branch uses fallback protector and reversed flags");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "set_error_flag") == runtime.calls.end() &&
          runtime.words[kTask + 24u] == 0u &&
          runtime.words[kTask + 32u] == 0u,
          "unselected branch initializes name header directly");
}

void AllocationAndTextFailure() {
    Runtime runtime;
    runtime.allocation = 0u;
    Check(pal4::StartAttackTask::Create(kParent, runtime) == 0u &&
          runtime.calls.back() == "allocate",
          "task allocation failure returns before reading parent fields");
    runtime = Runtime{};
    runtime.name_allocation = false;
    Check(pal4::StartAttackTask::Create(kParent, runtime) == kTask &&
          runtime.words[kTask] == pal4::StartAttackTask::kVtable &&
          runtime.words[kTask + 28u] == 0u &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "copy_name") == runtime.calls.end(),
          "text allocation failure still constructs task without copying");
}

void ExceptionCleanup() {
    Runtime runtime;
    runtime.throw_on_copy = true;
    bool caught = false;
    try {
        (void)pal4::StartAttackTask::Create(kParent, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught &&
          Index(runtime, "cleanup_name") <
              Index(runtime, "destroy_container") &&
          runtime.calls.back() == "free",
          "text copy exception releases name, container and task");
    runtime = Runtime{};
    runtime.throw_on_init = true;
    caught = false;
    try {
        (void)pal4::StartAttackTask::Create(kParent, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "free" &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "destroy_container") == runtime.calls.end(),
          "container init exception frees raw task");
}

}  // namespace

int main() {
    SelectedTargetBranch();
    UnselectedBranch();
    AllocationAndTextFailure();
    ExceptionCleanup();
    std::cout << "StartAttack task tests passed\n";
}
