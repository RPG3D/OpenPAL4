#include "pal4/combat_system_text_properties.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatSystemTextPropertyRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocated_data = 0xa000u;
    bool allocation_succeeds = true;
    std::uint32_t detach_count = 0u;
    bool modify_first_detach = false;
    std::uint32_t detached_length = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    [[noreturn]] void ThrowRange() override {
        calls.emplace_back("range");
        throw std::out_of_range("text range");
    }
    std::uint8_t DetachAndReset(std::uint32_t record) override {
        calls.emplace_back("detach:" + std::to_string(record));
        if (++detach_count == 1u && modify_first_detach)
            words[record + 8u] = detached_length;
        return 0x71u;
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        calls.emplace_back("allocate:" + std::to_string(record) + ":" +
                           std::to_string(length) + ":" +
                           std::to_string(mode));
        if (!allocation_succeeds) return false;
        words[record + 4u] = allocated_data;
        words[record + 12u] = length;
        return true;
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t length) override {
        calls.emplace_back("set-size:" + std::to_string(length));
        words[record + 8u] = length;
        bytes[words[record + 4u] + length] = 0u;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t count) override {
        calls.emplace_back("copy:" + std::to_string(destination) + ":" +
                           std::to_string(source) + ":" +
                           std::to_string(count));
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
};

using Assignment = std::uint8_t (*)(
    std::uint32_t, std::uint32_t,
    pal4::ICombatSystemTextPropertyRuntime&);

constexpr std::array<Assignment, 3> kAssign{
    &pal4::CombatSystemTextProperties::AssignFirst,
    &pal4::CombatSystemTextProperties::AssignSecond,
    &pal4::CombatSystemTextProperties::AssignThird};
constexpr std::array<std::uint32_t, 3> kOffsets{452u, 468u, 484u};

void SharedAndCopyPaths() {
    for (std::size_t index = 0; index < kAssign.size(); ++index) {
        Runtime shared;
        const auto dest = kObject + kOffsets[index];
        shared.words[kSource + 4u] = 0x9000u;
        shared.words[kSource + 8u] = 5u;
        shared.words[kSource + 12u] = 8u;
        shared.bytes[0x8fffu] = 2u;
        shared.words[dest + 4u] = 0x8000u;
        shared.bytes[0x7fffu] = 0u;
        Check(kAssign[index](kObject, kSource, shared) == 0u &&
                  shared.words[dest + 4u] == 0x9000u &&
                  shared.words[dest + 8u] == 5u &&
                  shared.words[dest + 12u] == 8u &&
                  shared.bytes[0x8fffu] == 3u &&
                  shared.calls[0] == "free:32767",
              "shareable source frees old unique text and increments source");

        Runtime copied;
        copied.words[kSource + 4u] = 0x9000u;
        copied.words[kSource + 8u] = 5u;
        copied.bytes[0x8fffu] = 0xfeu;
        Check(kAssign[index](kObject, kSource, copied) == 5u &&
                  copied.words[dest + 4u] == 0xa000u &&
                  copied.words[dest + 8u] == 5u &&
                  copied.bytes[0xa005u] == 0u &&
                  copied.calls == std::vector<std::string>{
                      "allocate:" + std::to_string(dest) + ":5:1",
                      "copy:40960:36864:5",
                      "word:" + std::to_string(dest + 8u),
                      "byte:40965"},
              "saturated source allocates and copies every byte");
    }
}

void ReferenceAndFailureBranches() {
    Runtime shared_old;
    const auto dest = kObject + 452u;
    shared_old.words[kSource + 4u] = 0x9000u;
    shared_old.words[kSource + 8u] = 1u;
    shared_old.bytes[0x8fffu] = 0u;
    shared_old.words[dest + 4u] = 0x8000u;
    shared_old.bytes[0x7fffu] = 3u;
    static_cast<void>(kAssign[0](kObject, kSource, shared_old));
    Check(shared_old.bytes[0x7fffu] == 2u &&
              shared_old.bytes[0x8fffu] == 1u &&
              shared_old.calls[0] == "byte:32767",
          "shared destination decrements old reference");

    Runtime empty;
    empty.words[kSource + 8u] = 0u;
    Check(kAssign[1](kObject, kSource, empty) == 0u &&
              empty.calls[0] == "allocate:4564:0:1" &&
              empty.calls[1] == "copy:40960:8657252:0",
          "empty source allocates zero bytes and uses empty-text fallback");

    Runtime failed;
    failed.allocation_succeeds = false;
    failed.words[kSource + 8u] = 4u;
    failed.bytes[pal4::CombatSystemTextProperties::kEmptyText - 1u] = 0xfeu;
    Check(kAssign[2](kObject, kSource, failed) == 0u &&
              failed.calls == std::vector<std::string>{
                  "allocate:4580:4:1"},
          "failed allocation leaves destination unchanged");
}

void SelfAssignment() {
    const auto dest = kObject + 452u;
    Runtime unchanged;
    unchanged.words[dest + 4u] = 0x9000u;
    unchanged.words[dest + 8u] = 5u;
    Check(kAssign[0](kObject, dest, unchanged) == 0x71u &&
              unchanged.calls == std::vector<std::string>{
                  "detach:4548", "detach:4548"},
          "ordinary self assignment detaches twice");

    Runtime shortened;
    shortened.words[dest + 4u] = 0x9000u;
    shortened.words[dest + 8u] = 5u;
    shortened.modify_first_detach = true;
    shortened.detached_length = 4u;
    Check(kAssign[0](kObject, dest, shortened) == 0x71u &&
              shortened.calls == std::vector<std::string>{
                  "detach:4548", "copy:36869:36868:0",
                  "allocate:4548:5:0", "set-size:5", "detach:4548"},
          "changed self length follows minus-one allocation branch");

    Runtime failed_reallocate;
    failed_reallocate.words[dest + 4u] = 0x9000u;
    failed_reallocate.words[dest + 8u] = 5u;
    failed_reallocate.modify_first_detach = true;
    failed_reallocate.detached_length = 7u;
    failed_reallocate.allocation_succeeds = false;
    Check(kAssign[0](kObject, dest, failed_reallocate) == 0x71u &&
              failed_reallocate.calls == std::vector<std::string>{
                  "detach:4548", "copy:36869:36871:0",
                  "allocate:4548:5:0", "detach:4548"},
          "failed self reallocation still performs final detach");
}

}  // namespace

int main() {
    try {
        SharedAndCopyPaths();
        ReferenceAndFailureBranches();
        SelfAssignment();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
