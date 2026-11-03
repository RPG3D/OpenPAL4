#include "pal4/combat_action_task_names.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatActionTaskNameRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> callbacks;
    std::vector<std::string> calls;
    std::uint32_t next_data = 0x7001u;

    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    void SetErrorFlag(const std::uint32_t buffer,
                      const std::uint8_t value) override {
        Check(value == 1u, "task name sets owned-string flag");
        bytes[buffer] = static_cast<std::uint8_t>(bytes[buffer] | value);
        calls.emplace_back("set_flag");
    }
    std::uint32_t ReallocateString(const std::uint32_t buffer,
                                   const std::uint32_t length) override {
        Check(length > 0u && words[buffer + 4u] == 0u,
              "task name reallocates an empty buffer");
        const auto data = next_data;
        next_data += 0x100u;
        words[buffer + 4u] = data;
        words[buffer + 12u] = length;
        calls.emplace_back("reallocate");
        return data;
    }
    void CopyBytes(const std::uint32_t destination,
                   const std::string_view text) override {
        strings[destination] = text;
        calls.emplace_back("copy");
    }
    void Free(const std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free");
    }
    int RegisterAtExit(const std::uint32_t callback) override {
        callbacks.push_back(callback);
        calls.emplace_back("atexit");
        return -7;
    }
};

void AllTaskNamesAndCallbacks() {
    Runtime runtime;
    for (const auto& binding : pal4::CombatActionTaskNames::kBindings) {
        const auto result =
            pal4::CombatActionTaskNames::InitializeAndRegister(
                binding.task, 0x80u, runtime);
        const auto data = runtime.words[binding.buffer + 4u];
        Check(result == -7 && data != 0u &&
              runtime.bytes[binding.buffer] == 0x81u &&
              runtime.words[binding.buffer + 8u] == binding.text.size() &&
              runtime.words[binding.buffer + 12u] == binding.text.size() &&
              runtime.strings[data] == binding.text &&
              runtime.bytes[data + binding.text.size()] == 0u &&
              runtime.callbacks.back() == binding.destructor,
              "each task name initializes owned text and its destructor");
    }
    Check(runtime.callbacks.size() == 32u,
          "thirty-two task names register matching cleanup callbacks");
}

void ReferenceCountAndGuard() {
    Runtime runtime;
    const auto& owned = pal4::CombatActionTaskNames::kBindings[0];
    Check(pal4::CombatActionTaskNames::Create(
              owned.task, 0u, runtime) == 1u,
          "creator returns low byte of allocated data pointer");
    const auto data = runtime.words[owned.buffer + 4u];
    runtime.bytes[data - 1u] = 2u;
    pal4::CombatActionTaskNames::Cleanup(owned.task, runtime);
    Check(runtime.bytes[owned.guard] == 1u &&
          runtime.bytes[data - 1u] == 1u && runtime.freed.empty() &&
          runtime.words[owned.buffer + 4u] == 0u &&
          runtime.words[owned.buffer + 8u] == 0u &&
          runtime.words[owned.buffer + 12u] == 0u,
          "shared string decrements reference count and clears three fields");
    pal4::CombatActionTaskNames::Cleanup(owned.task, runtime);
    Check(runtime.freed.empty(), "cleanup guard prevents second release");

    const auto& sole = pal4::CombatActionTaskNames::kBindings[1];
    (void)pal4::CombatActionTaskNames::Create(sole.task, 0u, runtime);
    const auto sole_data = runtime.words[sole.buffer + 4u];
    runtime.bytes[sole_data - 1u] = 0u;
    pal4::CombatActionTaskNames::Cleanup(sole.task, runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{sole_data - 1u},
          "zero reference header releases allocation");

    const auto& saturated = pal4::CombatActionTaskNames::kBindings[2];
    (void)pal4::CombatActionTaskNames::Create(saturated.task, 0u, runtime);
    const auto saturated_data = runtime.words[saturated.buffer + 4u];
    runtime.bytes[saturated_data - 1u] = 0xffu;
    pal4::CombatActionTaskNames::Cleanup(saturated.task, runtime);
    Check(runtime.freed.back() == saturated_data - 1u,
          "saturated reference header releases allocation");
}

void NewTaskNameCleanup() {
    for (std::size_t index = 5u; index < 9u; ++index) {
        Runtime runtime;
        const auto& binding = pal4::CombatActionTaskNames::kBindings[index];
        (void)pal4::CombatActionTaskNames::Create(binding.task, 0u, runtime);
        const auto data = runtime.words[binding.buffer + 4u];
        const bool shared = (index % 2u) == 0u;
        runtime.bytes[data - 1u] = shared ? 2u : 0u;
        pal4::CombatActionTaskNames::Cleanup(binding.task, runtime);
        Check(runtime.bytes[binding.guard] == 1u &&
              runtime.words[binding.buffer + 4u] == 0u &&
              runtime.words[binding.buffer + 8u] == 0u &&
              runtime.words[binding.buffer + 12u] == 0u &&
              (shared ? runtime.bytes[data - 1u] == 1u &&
                            runtime.freed.empty()
                      : runtime.freed ==
                            std::vector<std::uint32_t>{data - 1u}),
              "new task name cleanup uses its own guard and buffer");
        const auto freed_count = runtime.freed.size();
        pal4::CombatActionTaskNames::Cleanup(binding.task, runtime);
        Check(runtime.freed.size() == freed_count,
              "new task name cleanup guard is idempotent");
    }
}

void SharedGuardBits() {
    Runtime runtime;
    const auto& attack = pal4::CombatActionTaskNames::kBindings[9];
    const auto& joint = pal4::CombatActionTaskNames::kBindings[10];
    Check(attack.guard == joint.guard && attack.guard_mask == 1u &&
          joint.guard_mask == 2u,
          "attack and joint-attack names share distinct guard bits");
    (void)pal4::CombatActionTaskNames::Create(attack.task, 0u, runtime);
    (void)pal4::CombatActionTaskNames::Create(joint.task, 0u, runtime);
    const auto attack_data = runtime.words[attack.buffer + 4u];
    const auto joint_data = runtime.words[joint.buffer + 4u];
    runtime.bytes[attack_data - 1u] = 0u;
    runtime.bytes[joint_data - 1u] = 0u;
    pal4::CombatActionTaskNames::Cleanup(attack.task, runtime);
    Check(runtime.bytes[attack.guard] == 1u &&
          runtime.freed == std::vector<std::uint32_t>{attack_data - 1u} &&
          runtime.words[joint.buffer + 4u] == joint_data,
          "first guard bit leaves joint task live");
    pal4::CombatActionTaskNames::Cleanup(joint.task, runtime);
    Check(runtime.bytes[attack.guard] == 3u &&
          runtime.freed == std::vector<std::uint32_t>{
              attack_data - 1u, joint_data - 1u},
          "second guard bit independently releases joint task");
    pal4::CombatActionTaskNames::Cleanup(attack.task, runtime);
    pal4::CombatActionTaskNames::Cleanup(joint.task, runtime);
    Check(runtime.freed.size() == 2u,
          "both shared guard bits are idempotent");
}

void LifecycleNameCleanup() {
    for (std::size_t index = 11u;
         index < pal4::CombatActionTaskNames::kBindings.size(); ++index) {
        Runtime runtime;
        const auto& binding = pal4::CombatActionTaskNames::kBindings[index];
        (void)pal4::CombatActionTaskNames::Create(binding.task, 0u, runtime);
        const auto data = runtime.words[binding.buffer + 4u];
        runtime.bytes[data - 1u] = 0u;
        pal4::CombatActionTaskNames::Cleanup(binding.task, runtime);
        Check(runtime.bytes[binding.guard] == 1u &&
              runtime.freed == std::vector<std::uint32_t>{data - 1u} &&
              runtime.words[binding.buffer + 4u] == 0u &&
              runtime.words[binding.buffer + 8u] == 0u &&
              runtime.words[binding.buffer + 12u] == 0u,
              "lifecycle task cleanup uses its own buffer and guard");
        pal4::CombatActionTaskNames::Cleanup(binding.task, runtime);
        Check(runtime.freed.size() == 1u,
              "lifecycle task guard prevents duplicate free");
    }
}

}  // namespace

int main() {
    try {
        AllTaskNamesAndCallbacks();
        ReferenceCountAndGuard();
        NewTaskNameCleanup();
        SharedGuardBits();
        LifecycleNameCleanup();
        std::cout << "combat_action_task_names_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
