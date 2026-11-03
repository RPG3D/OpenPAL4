#include "pal4/combat_action_factory.h"
#include "pal4/check_protect_task.h"
#include "pal4/quick_attack_task.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;
constexpr std::uint32_t kTarget = 0x3000u;

struct Runtime final : pal4::ICombatActionFactoryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool inactive = true;
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
        requested_size = size;
        calls.push_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "free original allocation");
        calls.push_back("free");
    }
    void InitializeContainer(std::uint32_t container) override {
        Check(container == kObject + 4u, "GOM container at object +4");
        calls.push_back("init_container");
        if (fail_container) throw std::runtime_error("GOM init failed");
    }
    void DestroyContainer(std::uint32_t container) override {
        Check(container == kObject + 4u, "GOM cleanup at object +4");
        calls.push_back("destroy_container");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("write_byte:" + std::to_string(address));
    }
    void SetNameErrorFlag(std::uint32_t header,
                          std::uint8_t flag) override {
        Check(inactive && header == kObject + 20u && flag == 0u,
              "QuickAttack clears name error flag");
        calls.push_back("set_name_flag");
        words[header + 4u] = 0u;
        words[header + 8u] = 0u;
        words[header + 12u] = 0u;
    }
    bool AllocateName(std::uint32_t header,
                      std::uint32_t length,
                      std::uint8_t flag) override {
        Check(header == kObject + 20u && flag == 1u &&
                  length == (inactive ? 11u : 12u),
              "name allocation length and flag");
        calls.push_back("allocate_name");
        return name_allocation;
    }
    void CopyName(std::uint32_t header,
                  std::string_view name) override {
        Check(header == kObject + 20u &&
                  name == (inactive ? "QuickAttack" : "CheckProtect"),
              "action name chosen by combat branch");
        calls.push_back("copy_name");
        if (fail_name_copy) throw std::runtime_error("name copy failed");
        words[header + 8u] = static_cast<std::uint32_t>(name.size());
    }
    void CleanupName(std::uint32_t header) override {
        Check(header == kObject + 20u, "name cleanup address");
        calls.push_back("cleanup_name");
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

bool Called(const Runtime& runtime, const std::string& call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

void InputGatesAndAllocation() {
    Runtime r;
    Check(pal4::CombatActionFactory::Create(0u, kTarget, 7u, r) == 0u &&
              r.calls.empty(),
          "null source exits before singleton lookup");
    Check(pal4::CombatActionFactory::Create(kSource, 0u, 7u, r) == 0u &&
              r.calls.empty(),
          "null target exits before singleton lookup");

    r.allocation = 0u;
    Check(pal4::CombatActionFactory::Create(kSource, kTarget, 7u, r) ==
                  0u &&
              r.calls == std::vector<std::string>{
                  "combat_inactive", "allocate"} &&
              r.requested_size == 56u,
          "QuickAttack allocation failure stops before GOM init");
    r = Runtime{};
    r.inactive = false;
    r.allocation = 0u;
    Check(pal4::CombatActionFactory::Create(kSource, kTarget, 7u, r) ==
                  0u && r.requested_size == 60u,
          "CheckProtect allocation failure requests 60 bytes");
}

void QuickAttackLayout() {
    Runtime r;
    Check(pal4::CombatActionFactory::Create(kSource, kTarget, 7u, r) ==
                  kObject &&
              r.requested_size == pal4::QuickAttackTask::kSize &&
              r.words[kObject] == pal4::QuickAttackTask::kVtable &&
              r.bytes[kObject + 20u] == 7u &&
              r.words[kObject + 28u] == 11u &&
              r.words[kObject + 36u] == 0u &&
              r.bytes[kObject + 40u] == 1u &&
              r.words[kObject + 44u] == kSource &&
              r.words[kObject + 48u] == kTarget &&
              r.bytes[kObject + 52u] == 1u &&
              r.bytes[kObject + 53u] == 1u &&
              Index(r, "init_container") < Index(r, "set_name_flag") &&
              Index(r, "copy_name") < Index(r, "write:4096"),
          "QuickAttack size, name, actor fields, flags and order");
}

void CheckProtectLayout() {
    Runtime r;
    r.inactive = false;
    Check(pal4::CombatActionFactory::Create(kSource, kTarget, 9u, r) ==
                  kObject &&
              r.requested_size == pal4::CheckProtectTask::kSize &&
              r.words[kObject] == pal4::CheckProtectTask::kVtable &&
              r.bytes[kObject + 20u] == 9u &&
              r.words[kObject + 24u] == 0u &&
              r.words[kObject + 28u] == 12u &&
              r.words[kObject + 32u] == 0u &&
              r.words[kObject + 36u] == 0u &&
              r.bytes[kObject + 40u] == 1u &&
              r.words[kObject + 44u] == kSource &&
              r.words[kObject + 48u] == kTarget &&
              r.words[kObject + 52u] == 0u &&
              r.bytes[kObject + 56u] == 9u &&
              !Called(r, "set_name_flag") &&
              Index(r, "copy_name") < Index(r, "write:4096"),
          "CheckProtect size, name, actor fields, mode and order");
}

void FailureAndExceptionCleanup() {
    for (bool inactive : {true, false}) {
        Runtime r;
        r.inactive = inactive;
        r.name_allocation = false;
        Check(pal4::CombatActionFactory::Create(kSource, kTarget, 7u, r) ==
                  kObject && !Called(r, "copy_name") &&
                  r.words[kObject] == (inactive
                      ? pal4::QuickAttackTask::kVtable
                      : pal4::CheckProtectTask::kVtable),
              "name allocation failure still yields action object");

        r = Runtime{};
        r.inactive = inactive;
        r.fail_container = true;
        bool caught = false;
        try {
            (void)pal4::CombatActionFactory::Create(
                kSource, kTarget, 7u, r);
        } catch (const std::runtime_error&) { caught = true; }
        Check(caught && r.calls.back() == "free" &&
                  !Called(r, "destroy_container"),
              "container init failure releases raw allocation");

        r = Runtime{};
        r.inactive = inactive;
        r.fail_name_copy = true;
        caught = false;
        try {
            (void)pal4::CombatActionFactory::Create(
                kSource, kTarget, 7u, r);
        } catch (const std::runtime_error&) { caught = true; }
        Check(caught && Index(r, "cleanup_name") <
                            Index(r, "destroy_container") &&
                  r.calls.back() == "free",
              "name copy failure cleans text then GOM then storage");
    }
}

}  // namespace

int main() {
    try {
        InputGatesAndAllocation();
        QuickAttackLayout();
        CheckProtectLayout();
        FailureAndExceptionCleanup();
        std::cout << "Combat action factory tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
