#include "pal4/combat_reward_record.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kSource = 0x1000u;
constexpr std::uint32_t kDestination = 0x2000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatRewardRecordRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x4000u;
    std::uint32_t requested = ~0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        requested = bytes;
        calls.emplace_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
};

void FillTail(Runtime& runtime) {
    runtime.bytes[kSource] = 0x12u;
    runtime.bytes[kSource + 36u] = 0x34u;
    for (std::uint32_t offset = 16u; offset <= 32u; offset += 4u)
        runtime.words[kSource + offset] = 0x100u + offset;
}

void CheckTail(const Runtime& runtime) {
    Check(runtime.bytes.at(kDestination) == 0x12u &&
              runtime.bytes.at(kDestination + 36u) == 0x34u,
          "boundary bytes copied");
    for (std::uint32_t offset = 16u; offset <= 32u; offset += 4u)
        Check(runtime.words.at(kDestination + offset) == 0x100u + offset,
              "five trailing words copied");
}

void NonemptyCopy() {
    Runtime runtime;
    FillTail(runtime);
    runtime.words[kSource + 4u] = 0x3000u;
    runtime.words[kSource + 8u] = 0x300cu;
    runtime.words[0x3000u] = 7u;
    runtime.words[0x3004u] = 8u;
    runtime.words[0x3008u] = 9u;
    Check(pal4::CombatRewardRecord::Copy(kDestination, kSource, runtime) ==
              kDestination,
          "returns destination");
    Check(runtime.requested == 12u &&
              runtime.words.at(kDestination + 4u) == 0x4000u &&
              runtime.words.at(kDestination + 8u) == 0x400cu &&
              runtime.words.at(kDestination + 12u) == 0x400cu &&
              runtime.words.at(0x4000u) == 7u &&
              runtime.words.at(0x4004u) == 8u &&
              runtime.words.at(0x4008u) == 9u,
          "three words deep-copied with exact end/capacity pointers");
    CheckTail(runtime);
    Check(runtime.calls.front() == "byte:8192" &&
              runtime.calls[1] == "allocate:12" &&
              runtime.calls[2] == "write:8196" &&
              runtime.calls.back() == "byte:8228",
          "IDA copy order preserved");
}

void EmptyAndNullAllocation() {
    Runtime empty;
    FillTail(empty);
    empty.words[kSource + 4u] = 0u;
    empty.words[kSource + 8u] = 0u;
    empty.allocation = 0u;
    (void)pal4::CombatRewardRecord::Copy(kDestination, kSource, empty);
    Check(empty.requested == 0u &&
              empty.words.at(kDestination + 4u) == 0u &&
              empty.words.at(kDestination + 8u) == 0u &&
              empty.words.at(kDestination + 12u) == 0u,
          "empty source still allocates zero bytes");
    CheckTail(empty);

    Runtime failed;
    FillTail(failed);
    failed.words[kSource + 4u] = 0x3000u;
    failed.words[kSource + 8u] = 0x3008u;
    failed.words[0x3000u] = 1u;
    failed.words[0x3004u] = 2u;
    failed.allocation = 0u;
    (void)pal4::CombatRewardRecord::Copy(kDestination, kSource, failed);
    Check(failed.requested == 8u &&
              failed.words.at(kDestination + 4u) == 0u &&
              failed.words.at(kDestination + 8u) == 8u &&
              failed.words.at(kDestination + 12u) == 8u,
          "native null allocation skips writes yet advances end pointer");
    CheckTail(failed);
}

}  // namespace

int main() {
    try {
        NonemptyCopy();
        EmptyAndNullAllocation();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
