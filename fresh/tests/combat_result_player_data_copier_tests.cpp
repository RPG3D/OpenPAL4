#include "pal4/combat_result_player_data_copier.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kDestination = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;
constexpr std::uint32_t kSourceBuffer = 0x3000u;
constexpr std::uint32_t kTargetBuffer = 0x4000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatResultPlayerDataRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t next_allocation = 0x9000u;

    Runtime() {
        words[kSource + 4u] = kSourceBuffer;
        words[kSource + 8u] = kSourceBuffer + 12u;
        words[kSourceBuffer] = 11u;
        words[kSourceBuffer + 4u] = 22u;
        words[kSourceBuffer + 8u] = 33u;
        for (std::uint32_t offset = 16u; offset <= 32u; offset += 4u)
            words[kSource + offset] = 100u + offset;
        bytes[kSource + 36u] = 0xabu;
        words[kDestination + 4u] = kTargetBuffer;
        words[kDestination + 8u] = kTargetBuffer + 4u;
        words[kDestination + 12u] = kTargetBuffer + 16u;
        words[kTargetBuffer] = 99u;
        words[kTargetBuffer + 4u] = 98u;
        words[kTargetBuffer + 8u] = 97u;
        words[kTargetBuffer + 12u] = 96u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte");
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate:" + std::to_string(size));
        return next_allocation;
    }
};

void CheckTail(const Runtime& runtime) {
    for (std::uint32_t offset = 16u; offset <= 32u; offset += 4u)
        Check(runtime.words.at(kDestination + offset) ==
                  runtime.words.at(kSource + offset), "tail word copy");
    Check(runtime.bytes.at(kDestination + 36u) == 0xabu,
          "tail byte copy");
}

void ShorterSourceUsesExistingSize() {
    Runtime runtime;
    runtime.words[kSource + 8u] = kSourceBuffer + 8u;
    runtime.words[kDestination + 8u] = kTargetBuffer + 12u;
    const auto result = pal4::CombatResultPlayerDataCopier::Copy(
        kDestination, kSource, runtime);
    Check(result == kDestination && runtime.words[kTargetBuffer] == 11u &&
              runtime.words[kTargetBuffer + 4u] == 22u &&
              runtime.words[kTargetBuffer + 8u] == 97u &&
              runtime.words[kDestination + 8u] == kTargetBuffer + 8u &&
              runtime.words[kDestination + 12u] == kTargetBuffer + 16u &&
              runtime.calls.front() ==
                  "write:" + std::to_string(kTargetBuffer),
          "shorter source overwrites only live prefix");
    CheckTail(runtime);
}

void SpareCapacityAppends() {
    Runtime runtime;
    const auto result = pal4::CombatResultPlayerDataCopier::Copy(
        kDestination, kSource, runtime);
    Check(result == kDestination && runtime.words[kTargetBuffer] == 11u &&
              runtime.words[kTargetBuffer + 4u] == 22u &&
              runtime.words[kTargetBuffer + 8u] == 33u &&
              runtime.words[kTargetBuffer + 12u] == 96u &&
              runtime.words[kDestination + 8u] == kTargetBuffer + 12u &&
              runtime.words[kDestination + 12u] == kTargetBuffer + 16u,
          "spare capacity preserves allocation and copies suffix");
    CheckTail(runtime);
}

void ExactReallocationAndNullAllocation() {
    for (const auto allocation : {0x9000u, 0u}) {
        Runtime runtime;
        runtime.next_allocation = allocation;
        runtime.words[kDestination + 12u] = kTargetBuffer + 8u;
        const auto result = pal4::CombatResultPlayerDataCopier::Copy(
            kDestination, kSource, runtime);
        Check(result == kDestination && runtime.calls[0] ==
                  "free:" + std::to_string(kTargetBuffer) &&
                  runtime.calls[1] == "allocate:12" &&
                  runtime.words[kDestination + 4u] == allocation &&
                  runtime.words[kDestination + 8u] == allocation + 12u &&
                  runtime.words[kDestination + 12u] == allocation + 12u,
              "reallocation order and exact capacity");
        if (allocation != 0u)
            Check(runtime.words[allocation] == 11u &&
                      runtime.words[allocation + 4u] == 22u &&
                      runtime.words[allocation + 8u] == 33u,
                  "reallocated contents");
        CheckTail(runtime);
    }
}

void EmptyAndSelfAssignment() {
    Runtime empty;
    empty.words[kSource + 4u] = 0u;
    empty.words[kSource + 8u] = 0u;
    (void)pal4::CombatResultPlayerDataCopier::Copy(
        kDestination, kSource, empty);
    Check(empty.words[kDestination + 8u] == kTargetBuffer &&
              empty.words[kTargetBuffer] == 99u,
          "empty source truncates logical size without deleting allocation");
    CheckTail(empty);

    Runtime self;
    (void)pal4::CombatResultPlayerDataCopier::Copy(
        kDestination, kDestination, self);
    Check(self.words[kDestination + 4u] == kTargetBuffer &&
              self.words[kDestination + 8u] == kTargetBuffer + 4u &&
              self.words[kDestination + 12u] == kTargetBuffer + 16u &&
              self.calls.size() == 6u && self.calls.back() == "byte",
          "self assignment skips array but copies five words and byte");
}

}  // namespace

int main() {
    try {
        ShorterSourceUsesExistingSize();
        SpareCapacityAppends();
        ExactReallocationAndNullAllocation();
        EmptyAndSelfAssignment();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
