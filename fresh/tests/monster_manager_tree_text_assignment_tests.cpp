#include "pal4/monster_manager_tree_text_assignment.h"
#include "pal4/npc_manager_tree.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMonsterManagerTreeTextRuntime {
    static constexpr std::uint32_t kDestination = 0x1000u;
    static constexpr std::uint32_t kSource = 0x2000u;
    static constexpr std::uint32_t kEmpty = 0x5001u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool allocation_succeeds = true;
    unsigned detach_calls = 0u;

    Runtime() {
        bytes[kSource] = 0x7au;
        bytes[kDestination] = 0xaau;
        bytes[kDestination + 1u] = 0xbbu;
        bytes[kEmpty - 1u] = 0xfeu;
        bytes[kEmpty] = 0u;
        bytes[kEmpty + 1u] = 0u;
        words[kDestination + 4u] = 0xdeadbeefu;
        words[kDestination + 8u] = 7u;
        words[kDestination + 0x0cu] = 9u;
        words[kDestination + 0x10u] = 0x11111111u;
        words[kSource + 4u] = 0x6001u;
        words[kSource + 8u] = 3u;
        words[kSource + 0x0cu] = 8u;
        words[kSource + 0x10u] = 0x12345678u;
        bytes[0x6000u] = 0u;
        bytes[0x6001u] = 'a';
        bytes[0x6002u] = 'b';
        bytes[0x6003u] = 'c';
    }
    std::uint32_t ReadDword(std::uint32_t address) const override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) const override {
        return bytes.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t EmptyTextData() const override { return kEmpty; }
    void SetErrorFlag(std::uint32_t record, std::uint8_t flag) override {
        Check(record == kDestination && flag == 1u, "share flag arguments");
        calls.emplace_back("flag");
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        Check(record == kDestination && mode == 1u, "allocation arguments");
        calls.push_back("allocate:" + std::to_string(length));
        if (!allocation_succeeds) return false;
        words[record + 4u] = 0x8001u;
        words[record + 0x0cu] = length + 8u;
        return true;
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t length) override {
        calls.push_back("copy:" + std::to_string(length));
        for (std::uint32_t i = 0u; i < length; ++i)
            bytes[destination + i] = ReadByte(source + i);
    }
    void DetachAndReset(std::uint32_t record) override {
        Check(record == kDestination && words.at(record + 4u) == 0u &&
              words.at(record + 8u) == 0u &&
              words.at(record + 0x0cu) == 0u,
              "self-assignment clears buffer before detach");
        ++detach_calls;
        calls.emplace_back("detach");
    }
};

void TestNullAndSelf() {
    Runtime runtime;
    pal4::MonsterManagerTreeTextAssignment::Assign(
        0u, Runtime::kSource, runtime);
    Check(runtime.calls.empty() && runtime.words[Runtime::kDestination + 4u]
              == 0xdeadbeefu, "null destination is untouched");

    runtime.bytes[Runtime::kDestination] = 0x31u;
    pal4::MonsterManagerTreeTextAssignment::Assign(
        Runtime::kDestination, Runtime::kDestination, runtime);
    Check(runtime.detach_calls == 2u &&
          runtime.calls == std::vector<std::string>{"detach", "detach"},
          "self-assignment uses two detach calls");
    Check(runtime.words[Runtime::kDestination + 4u] == 0u &&
          runtime.words[Runtime::kDestination + 8u] == 0u &&
          runtime.words[Runtime::kDestination + 0x0cu] == 0u &&
          runtime.words[Runtime::kDestination + 0x10u] == 0x11111111u &&
          runtime.bytes[Runtime::kDestination] == 0x31u &&
          runtime.bytes[Runtime::kDestination + 1u] == 0xbbu,
          "self-assignment preserves metadata and upper mode bytes");
}

void TestSharedCopy() {
    for (const auto initial_count : {std::uint8_t{0u},
                                     std::uint8_t{0xfdu}}) {
        Runtime runtime;
        runtime.bytes[0x6000u] = initial_count;
        pal4::MonsterManagerTreeTextAssignment::Assign(
            Runtime::kDestination, Runtime::kSource, runtime);
        Check(runtime.calls == std::vector<std::string>{"flag"},
              "share does not allocate or copy bytes");
        Check(runtime.words[Runtime::kDestination + 4u] == 0x6001u &&
              runtime.words[Runtime::kDestination + 8u] == 3u &&
              runtime.words[Runtime::kDestination + 0x0cu] == 8u &&
              runtime.words[Runtime::kDestination + 0x10u] == 0x12345678u &&
              runtime.bytes[0x6000u] == initial_count + 1u,
              "shared data, capacity, metadata and reference increment");
        Check(runtime.bytes[Runtime::kDestination] == 0x7au &&
              runtime.bytes[Runtime::kDestination + 1u] == 0xbbu,
              "only low mode byte copied");
    }
}

void TestDeepCopy() {
    for (const auto count : {std::uint8_t{0xfeu}, std::uint8_t{0xffu}}) {
        Runtime runtime;
        runtime.bytes[0x6000u] = count;
        pal4::MonsterManagerTreeTextAssignment::Assign(
            Runtime::kDestination, Runtime::kSource, runtime);
        Check(runtime.calls ==
                  (std::vector<std::string>{"allocate:3", "copy:3"}),
              "unshareable data is allocated and copied");
        Check(runtime.bytes[0x8001u] == 'a' &&
              runtime.bytes[0x8002u] == 'b' &&
              runtime.bytes[0x8003u] == 'c' &&
              runtime.bytes[0x8004u] == 0u &&
              runtime.words[Runtime::kDestination + 8u] == 3u &&
              runtime.words[Runtime::kDestination + 0x0cu] == 11u &&
              runtime.words[Runtime::kDestination + 0x10u] == 0x12345678u &&
              runtime.bytes[0x6000u] == count,
              "deep copy content, terminator and metadata");
    }
}

void TestEmptyAndAllocationFailure() {
    Runtime empty;
    empty.words[Runtime::kSource + 4u] = 0u;
    empty.words[Runtime::kSource + 8u] = 0u;
    pal4::MonsterManagerTreeTextAssignment::Assign(
        Runtime::kDestination, Runtime::kSource, empty);
    Check(empty.calls ==
              (std::vector<std::string>{"allocate:0", "copy:0"}) &&
          empty.words[Runtime::kDestination + 8u] == 0u &&
          empty.bytes[0x8001u] == 0u,
          "empty source still requests allocation and terminator");

    Runtime failed;
    failed.bytes[0x6000u] = 0xfeu;
    failed.allocation_succeeds = false;
    pal4::MonsterManagerTreeTextAssignment::Assign(
        Runtime::kDestination, Runtime::kSource, failed);
    Check(failed.calls == std::vector<std::string>{"allocate:3"} &&
          failed.words[Runtime::kDestination + 4u] == 0u &&
          failed.words[Runtime::kDestination + 8u] == 0u &&
          failed.words[Runtime::kDestination + 0x0cu] == 0u &&
          failed.words[Runtime::kDestination + 0x10u] == 0x12345678u,
          "allocation failure still copies final metadata");
}

void TestNpcTextAssignmentEntry() {
    Runtime shared;
    shared.bytes[0x6000u] = 0u;
    pal4::NpcManagerTree::AssignText(
        Runtime::kDestination, Runtime::kSource, shared);
    Check(shared.words[Runtime::kDestination + 4u] == 0x6001u &&
          shared.bytes[0x6000u] == 1u &&
          shared.words[Runtime::kDestination + 0x10u] == 0x12345678u,
          "NPC tree text shares data and copies metadata");

    Runtime copied;
    copied.bytes[0x6000u] = 0xfeu;
    pal4::NpcManagerTree::AssignText(
        Runtime::kDestination, Runtime::kSource, copied);
    Check(copied.calls ==
              std::vector<std::string>{"allocate:3", "copy:3"} &&
          copied.bytes[0x8004u] == 0u,
          "NPC tree text deep-copies an unshareable buffer");
}

}  // namespace

int main() {
    TestNullAndSelf();
    TestSharedCopy();
    TestDeepCopy();
    TestEmptyAndAllocationFailure();
    TestNpcTextAssignmentEntry();
}
