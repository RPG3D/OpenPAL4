#include "pal4/physical_attack_child_task.h"

#include <algorithm>
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

struct Runtime final : pal4::IPhysicalAttackChildRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation = kObject;
    bool throw_on_base = false;
    bool throw_on_embedded = false;
    int throw_effect = -1;

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
        Check(task == kObject || task == kObject + 216u,
              "constructs outer or embedded base");
        calls.push_back(task == kObject ? "base" : "embedded_base");
        if ((task == kObject && throw_on_base) ||
            (task != kObject && throw_on_embedded))
            throw std::runtime_error("construct");
    }
    std::uint32_t Allocate(std::uint32_t bytes_requested) override {
        Check(bytes_requested == pal4::PhysicalAttackChildTask::kSize,
              "factory allocates 292 bytes");
        calls.push_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "free receives allocation");
        calls.push_back("free");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("write_byte:" + std::to_string(address));
    }
    void ConstructNameNode(std::uint32_t slot) override {
        Check(slot >= kObject + 96u &&
                  slot <= kObject + 96u + 4u * 24u &&
                  (slot - kObject - 96u) % 24u == 0u,
              "effect slot uses five 24-byte records");
        const auto index = (slot - kObject - 96u) / 24u;
        calls.push_back("effect:" + std::to_string(index));
        if (static_cast<int>(index) == throw_effect)
            throw std::runtime_error("effect");
    }
    void DestroyEffectSlot(std::uint32_t slot) override {
        calls.push_back("destroy_effect:" +
                        std::to_string((slot - kObject - 96u) / 24u));
    }
    void DestroyCombatBase(std::uint32_t object) override {
        Check(object == kObject, "partial cleanup receives object");
        calls.push_back("destroy_combat_base");
    }
    void DestroyMessageBase(std::uint32_t object) override {
        Check(object == kObject, "message cleanup receives object");
        calls.push_back("destroy_message_base");
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void ConstructedLayout() {
    Runtime runtime;
    Check(pal4::PhysicalAttackChildTask::Create(runtime) == kObject &&
              runtime.words.at(kObject) ==
                  pal4::PhysicalAttackChildTask::kVtable &&
              runtime.words.at(kObject + 60u) == 0u &&
              runtime.bytes.at(kObject + 64u) == 0u &&
              runtime.words.at(kObject + 68u) == 0u &&
              runtime.words.at(kObject + 72u) == 0u &&
              runtime.words.at(kObject + 76u) == 0u,
          "outer task fields and final vtable match IDA");
    for (std::uint32_t offset = 80u; offset < 96u; offset += 4u)
        Check(runtime.words.at(kObject + offset) == 0u,
              "two eight-byte records are zeroed");
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto slot = kObject + 96u + 24u * index;
        Check(runtime.bytes.at(slot + 8u) == 0u &&
                  runtime.words.at(slot + 12u) == 0u &&
                  runtime.words.at(slot + 16u) == 0u &&
                  runtime.words.at(slot + 20u) == 0u,
              "effect slot constructor clears byte and three words");
    }
    Check(runtime.words.at(kObject + 216u) ==
              pal4::NormalAttackTask::kVtable &&
              runtime.words.at(kObject + 216u + 60u) == 0u &&
              runtime.words.at(kObject + 216u + 64u) == 0u,
          "embedded NormalAttack task composes existing constructor");
    Check(Index(runtime, "base") < Index(runtime, "effect:0") &&
              Index(runtime, "effect:0") < Index(runtime, "effect:4") &&
              Index(runtime, "effect:4") <
                  Index(runtime, "embedded_base") &&
              runtime.calls.back() == "write:" +
                                      std::to_string(kObject),
          "outer, effects, embedded and final vtable follow IDA order");
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "write:" + std::to_string(kObject)) == 3,
          "outer object receives all three construction vtables");
}

void AllocationAndBaseFailure() {
    Runtime runtime;
    runtime.allocation = 0u;
    Check(pal4::PhysicalAttackChildTask::Create(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "null allocation stops before base construction");
    runtime = Runtime{};
    runtime.throw_on_base = true;
    bool caught = false;
    try {
        (void)pal4::PhysicalAttackChildTask::Create(runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls ==
              std::vector<std::string>{"allocate", "base", "free"},
          "base constructor failure frees allocation");
}

void EffectAndEmbeddedFailure() {
    Runtime runtime;
    runtime.throw_effect = 2;
    bool caught = false;
    try {
        (void)pal4::PhysicalAttackChildTask::Create(runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught &&
              Index(runtime, "effect:2") <
                  Index(runtime, "destroy_effect:1") &&
              Index(runtime, "destroy_effect:1") <
                  Index(runtime, "destroy_effect:0") &&
              Index(runtime, "destroy_effect:0") <
                  Index(runtime, "destroy_combat_base") &&
              runtime.calls.back() == "free" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "embedded_base") == runtime.calls.end(),
          "effect constructor failure unwinds completed slots in reverse");
    runtime = Runtime{};
    runtime.throw_on_embedded = true;
    caught = false;
    try {
        (void)pal4::PhysicalAttackChildTask::Create(runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && Index(runtime, "effect:4") <
                        Index(runtime, "embedded_base") &&
              Index(runtime, "embedded_base") <
                  Index(runtime, "destroy_message_base") &&
              runtime.calls.back() == "free",
          "embedded failure delegates message-base unwind");
}

struct DeleteRuntime final : pal4::ICombatMessageBaseDeletingRuntime {
    std::vector<std::string> calls;
    void DestroyBase(std::uint32_t object) override {
        Check(object == kObject, "base deletion receives object");
        calls.push_back("base");
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "deletion frees object");
        calls.push_back("free");
    }
};

void DeleteFlags() {
    for (const auto flags : {0u, 1u, 2u, 3u}) {
        DeleteRuntime runtime;
        Check(pal4::CombatMessageTaskBase::Destroy(
                  kObject, static_cast<std::uint8_t>(flags), runtime) ==
                  kObject &&
                  runtime.calls == ((flags & 1u) != 0u
                      ? std::vector<std::string>{"base", "free"}
                      : std::vector<std::string>{"base"}),
              "deleting destructor tests low flag bit and returns object");
    }
}

}  // namespace

int main() {
    ConstructedLayout();
    AllocationAndBaseFailure();
    EffectAndEmbeddedFailure();
    DeleteFlags();
    std::cout << "PhysicalAttack child task tests passed\n";
}
