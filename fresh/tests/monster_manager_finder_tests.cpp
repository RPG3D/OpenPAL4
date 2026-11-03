#include "pal4/monster_manager_finder.h"

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

struct Runtime final : pal4::MonsterManagerFindRuntimeStub {
    std::uint32_t CStringLength(std::uint32_t address) override {
        events.emplace_back("length");
        Check(address == 0x12344u, "finder uses source name address");
        return 6u;
    }
    void SetErrorFlag(pal4::ModeStringBufferRecord& text,
                      std::uint8_t flag) override {
        events.emplace_back("set");
        Check(flag == 0u && (text.mode_word & 0xffu) == 0x44u,
              "finder initializes mode byte from name pointer");
        text.buffer = {};
    }
    bool AllocateText(pal4::ModeStringBufferRecord& text,
                      std::uint32_t length,
                      std::uint8_t mode) override {
        events.emplace_back("allocate-text");
        Check(length == 6u && mode == 1u,
              "finder requests six bytes in mode one");
        if (!allocate_text) return false;
        text.buffer.data = 0x8001u;
        header_count[0x8000u] = initial_reference_count;
        return true;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        events.emplace_back("copy");
        Check(destination == 0x8001u && source == 0x12344u &&
                  length == 6u, "finder copies name bytes exactly");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        events.emplace_back("write-byte");
        bytes[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.emplace_back("read-header");
        return header_count.at(address);
    }
    void FreeTextHeader(std::uint32_t address) override {
        events.emplace_back("free-header");
        freed.push_back(address);
    }
    std::uint32_t SearchNodeAlternative(
        std::uint32_t tree,
        const pal4::ModeStringBufferRecord& name) override {
        events.emplace_back("search");
        Check(tree == 0x5028u &&
                  name.buffer.data == (allocate_text ? 0x8001u : 0u) &&
                  name.buffer.length == (allocate_text ? 6u : 0u),
              "search sees prepared temporary string");
        return candidate;
    }
    std::int32_t CompareName(
        const pal4::ModeStringBufferRecord& name,
        std::uint32_t node_text) override {
        events.emplace_back("compare");
        Check(node_text == 0x710cu &&
                  name.buffer.data == (allocate_text ? 0x8001u : 0u),
              "comparison targets candidate key at +0x0c");
        return compare_result;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read-dword");
        if (address == 0x502cu) return 0x7000u;
        Check(address == 0x711cu,
              "found node returns object at +0x1c");
        return 0xa000u;
    }

    bool allocate_text{true};
    std::uint8_t initial_reference_count{};
    std::uint32_t candidate{0x7100u};
    std::int32_t compare_result{};
    std::unordered_map<std::uint32_t, std::uint8_t> header_count;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;
    std::vector<std::string> events;
};

void TestFoundAndStringTermination() {
    Runtime runtime;
    Check(pal4::MonsterManagerFinder::FindByName(
              0x5000u, 0x12344u, runtime) == 0xa000u &&
              runtime.bytes.at(0x8007u) == 0u &&
              runtime.freed == std::vector<std::uint32_t>{0x8000u},
          "matching candidate returns object and frees name");
    Check(runtime.events == std::vector<std::string>{
              "set", "length", "allocate-text", "copy",
              "write-byte", "search", "read-dword", "compare",
              "read-dword", "read-header", "free-header"},
          "finder preserves copy, compare and cleanup order");
}

void TestHeaderAndNegativeComparisonMiss() {
    Runtime runtime;
    runtime.candidate = 0x7000u;
    Check(pal4::MonsterManagerFinder::FindByName(
              0x5000u, 0x12344u, runtime) == 0u &&
              runtime.freed == std::vector<std::uint32_t>{0x8000u},
          "header candidate returns zero without comparing");
    Check(runtime.events == std::vector<std::string>{
              "set", "length", "allocate-text", "copy",
              "write-byte", "search", "read-dword",
              "read-header", "free-header"},
          "header path skips comparison and object read");

    Runtime negative;
    negative.compare_result = -1;
    Check(pal4::MonsterManagerFinder::FindByName(
              0x5000u, 0x12344u, negative) == 0u &&
              negative.events == std::vector<std::string>{
                  "set", "length", "allocate-text", "copy",
                  "write-byte", "search", "read-dword", "compare",
                  "read-header", "free-header"},
          "negative key comparison rejects candidate");
}

void TestPositiveComparisonAndReferenceDecrement() {
    Runtime runtime;
    runtime.compare_result = 1;
    runtime.initial_reference_count = 2u;
    Check(pal4::MonsterManagerFinder::FindByName(
              0x5000u, 0x12344u, runtime) == 0xa000u &&
              runtime.bytes.at(0x8000u) == 1u && runtime.freed.empty(),
          "positive comparison accepts candidate and decrements reference");
}

void TestTextAllocationFailureStillSearches() {
    Runtime runtime;
    runtime.allocate_text = false;
    runtime.candidate = 0x7000u;
    Check(pal4::MonsterManagerFinder::FindByName(
              0x5000u, 0x12344u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "set", "length", "allocate-text", "search",
                  "read-dword"},
          "text allocation failure skips copy but still searches");
}

}  // namespace

int main() {
    TestFoundAndStringTermination();
    TestHeaderAndNegativeComparisonMiss();
    TestPositiveComparisonAndReferenceDecrement();
    TestTextAllocationFailureStillSearches();
}
