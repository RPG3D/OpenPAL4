#include "pal4/combat_result_data_copier.h"

#include <cstdint>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kSource = 0x1000u;
constexpr std::uint32_t kDestination = 0x2000u;
constexpr std::uint32_t kRef =
    pal4::CombatResultDataCopier::kSharedRefCountAddress;
constexpr std::uint32_t kNil =
    pal4::CombatResultDataCopier::kSharedNilAddress;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatResultDataCopyRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> cleaned;
    std::uint32_t nodes_created = 0u;
    std::uint32_t arrays_allocated = 0u;
    std::uint32_t fail_node = 0u;
    std::uint32_t fail_array = 0u;

    Runtime() {
        bytes[kSource] = 0x11u;
        for (std::uint32_t offset = 0u; offset <= 16u; offset += 4u)
            words[kSource + offset] = 10u + offset;
        for (std::uint32_t tree : {20u, 36u}) {
            bytes[kSource + tree] = 0x31u;
            bytes[kSource + tree + 1u] = 0x32u;
            bytes[kSource + tree + 8u] = 0x33u;
            words[kSource + tree + 12u] = 2u;
        }
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            const auto record = kSource + 52u + index * 40u;
            const auto array = 0x6000u + index * 0x100u;
            bytes[record] = static_cast<std::uint8_t>(0x40u + index);
            bytes[record + 36u] = static_cast<std::uint8_t>(0x50u + index);
            words[record + 4u] = array;
            words[record + 8u] = array + 8u;
            words[array] = 100u + index;
            words[array + 4u] = 200u + index;
            for (std::uint32_t offset = 16u; offset <= 32u; offset += 4u)
                words[record + offset] = index * 100u + offset;
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 8u, "each reward array has two words");
        ++arrays_allocated;
        if (arrays_allocated == fail_array) throw std::bad_alloc();
        const auto address = 0xb000u + (arrays_allocated - 1u) * 0x100u;
        calls.emplace_back("allocate-array");
        return address;
    }
    std::uint32_t CreateNode(std::uint32_t parent,
                             std::uint32_t color) override {
        ++nodes_created;
        if (nodes_created == fail_node) return 0u;
        const auto address = 0x8000u + (nodes_created - 1u) * 0x100u;
        words[address + 4u] = parent;
        words[address + 20u] = color;
        calls.emplace_back("node:" + std::to_string(parent) + ":" +
                           std::to_string(color));
        return address;
    }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free:" + std::to_string(address));
    }
    void CopyAssignTree(std::uint32_t destination,
                        std::uint32_t source) override {
        Check(destination - kDestination == source - kSource,
              "matching tree offsets");
        calls.emplace_back("tree-copy:" + std::to_string(destination));
        words[destination + 12u] = words[source + 12u];
    }
    void DestroyTree(std::uint32_t tree) override {
        calls.emplace_back("tree-destroy:" + std::to_string(tree));
        Free(words[tree + 4u]);
        words[tree + 4u] = 0u;
        words[tree + 12u] = 0u;
        const auto remaining = --words[kRef];
        if (remaining == 0u) {
            const auto nil = words[kNil];
            words[kNil] = 0u;
            Free(nil);
        }
    }
    void CleanupRecord(std::uint32_t record) override {
        cleaned.push_back(record);
        calls.emplace_back("record-cleanup:" + std::to_string(record));
        Free(words[record + 4u]);
    }
};

void FreshCopy() {
    Runtime runtime;
    Check(pal4::CombatResultDataCopier::Copy(
              kDestination, kSource, runtime) == kDestination,
          "copy returns destination");
    Check(runtime.nodes_created == 4u &&
              runtime.arrays_allocated == 4u &&
              runtime.words[kNil] == 0x8000u &&
              runtime.words[kRef] == 2u &&
              runtime.freed == std::vector<std::uint32_t>{0x8200u},
          "two trees share one newly created sentinel");
    Check(runtime.words[0x8000u] == 0u &&
              runtime.words[0x8008u] == 0u &&
              runtime.words[0x8014u] == 1u,
          "shared sentinel fields");
    for (std::uint32_t index = 0u; index < 2u; ++index) {
        const auto tree = kDestination + 20u + index * 16u;
        const auto head = index == 0u ? 0x8100u : 0x8300u;
        Check(runtime.bytes[tree] == 0x31u &&
                  runtime.bytes[tree + 1u] == 0x32u &&
                  runtime.bytes[tree + 8u] == 0x33u &&
                  runtime.words[tree + 4u] == head &&
                  runtime.words[tree + 12u] == 2u &&
                  runtime.words[head] == head &&
                  runtime.words[head + 4u] == 0x8000u &&
                  runtime.words[head + 8u] == head,
              "tree flags, circular head and assigned contents");
    }
    for (std::uint32_t offset = 0u; offset <= 16u; offset += 4u)
        Check(runtime.words[kDestination + offset] == 10u + offset,
              "five report header words copied");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto record = kDestination + 52u + index * 40u;
        const auto array = 0xb000u + index * 0x100u;
        Check(runtime.bytes[record] == 0x40u + index &&
                  runtime.bytes[record + 36u] == 0x50u + index &&
                  runtime.words[record + 4u] == array &&
                  runtime.words[record + 8u] == array + 8u &&
                  runtime.words[record + 12u] == array + 8u &&
                  runtime.words[array] == 100u + index &&
                  runtime.words[array + 4u] == 200u + index,
              "four reward records deep copied");
    }
    Check(runtime.calls.front() == "byte:8192" &&
              runtime.calls[1] == "word:8192" &&
              runtime.calls.back() == "byte:8400",
          "header byte precedes word; record tail ends copy");
}

void ReusedSentinelAndFailureCleanup() {
    Runtime reused;
    reused.words[kNil] = 0x9000u;
    reused.words[kRef] = 5u;
    (void)pal4::CombatResultDataCopier::Copy(
        kDestination, kSource, reused);
    Check(reused.words[kNil] == 0x9000u && reused.words[kRef] == 7u &&
              reused.freed == std::vector<std::uint32_t>{0x8000u, 0x8200u} &&
              reused.words[0x8104u] == 0x9000u &&
              reused.words[0x8304u] == 0x9000u,
          "existing sentinel candidates released after increments");

    for (const std::uint32_t failed_node : {2u, 4u}) {
        Runtime failed;
        failed.fail_node = failed_node;
        bool threw = false;
        try {
            (void)pal4::CombatResultDataCopier::Copy(
                kDestination, kSource, failed);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        Check(threw && failed.words[kRef] == 0u &&
                  failed.words[kNil] == 0u &&
                  failed.freed.back() == 0x8000u,
              "failed head allocation releases shared sentinel");
        if (failed_node == 4u)
            Check(failed.words[kDestination + 24u] == 0u,
                  "completed first tree destroyed on second head failure");
    }

    Runtime failed_record;
    failed_record.fail_array = 3u;
    bool threw = false;
    try {
        (void)pal4::CombatResultDataCopier::Copy(
            kDestination, kSource, failed_record);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && failed_record.cleaned ==
                       std::vector<std::uint32_t>{kDestination + 92u,
                                                   kDestination + 52u} &&
              failed_record.words[kRef] == 0u &&
              failed_record.words[kNil] == 0u,
          "record failure unwinds completed records then both trees");
}

}  // namespace

int main() {
    try {
        FreshCopy();
        ReusedSentinelAndFailureCleanup();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
