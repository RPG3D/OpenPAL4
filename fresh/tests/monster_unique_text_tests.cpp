#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_unique_text.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
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

struct Memory final : pal4::IMonsterUniqueTextRuntime {
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t next_record{0x10000u};
    std::uint32_t next_text{0x20000u};
    std::vector<std::uint32_t> freed_text;
    std::vector<std::uint32_t> freed_records;
    std::vector<std::string> calls;
    bool allocation_succeeds{true};
    bool throw_on_records{};
    std::uint8_t initial_refcount{};

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        std::uint32_t value = 0;
        for (std::uint32_t i = 0; i < 4u; ++i)
            value |= static_cast<std::uint32_t>(ReadByte(address + i)) <<
                     (i * 8u);
        return value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        for (std::uint32_t i = 0; i < 4u; ++i)
            WriteByte(address + i,
                      static_cast<std::uint8_t>(value >> (i * 8u)));
    }
    std::uint32_t EmptyTextAddress() override { return 0x90000u; }
    void SetErrorFlag(std::uint32_t record, std::uint8_t flag) override {
        calls.emplace_back("flag");
        WriteByte(record, flag);
    }
    bool AllocateText(std::uint32_t record, std::uint32_t size,
                      std::uint8_t mode) override {
        calls.emplace_back("allocate-text");
        Check(mode == 1u, "text allocation mode");
        if (!allocation_succeeds)
            return false;
        const auto text = next_text + 1u;
        next_text += size + 0x20u;
        WriteByte(text - 1u, initial_refcount);
        WriteDword(record + 4u, text);
        return true;
    }
    void DetachAndReset(std::uint32_t) override {
        calls.emplace_back("detach");
    }
    void SetTextSize(std::uint32_t record, std::uint32_t size) override {
        calls.emplace_back("set-size");
        WriteDword(record + 8u, size);
        WriteByte(ReadDword(record + 4u) + size, 0u);
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t size) override {
        calls.emplace_back("copy-bytes");
        for (std::uint32_t i = 0; i < size; ++i)
            WriteByte(destination + i, ReadByte(source + i));
    }
    [[noreturn]] void ThrowRange() override {
        throw std::out_of_range("range");
    }
    std::uint32_t AllocateRecords(std::uint32_t size) override {
        calls.emplace_back("allocate-records");
        if (throw_on_records)
            throw std::bad_alloc();
        const auto address = next_record;
        next_record += size + 0x20u;
        return address;
    }
    void FreeRecords(std::uint32_t address) override {
        calls.emplace_back("free-records");
        freed_records.push_back(address);
    }
    void AssignRecord(std::uint32_t destination,
                      std::uint32_t source) override {
        calls.emplace_back("assign-record");
        for (std::uint32_t i = 0u; i < 16u; ++i)
            WriteByte(destination + i, ReadByte(source + i));
    }
    std::uint32_t TemporaryRecordAddress() override { return 0x8000u; }
    void FreeTextHeader(std::uint32_t address) override {
        calls.emplace_back("free-text");
        freed_text.push_back(address);
    }

    void PutString(std::uint32_t address, const std::string& value) {
        for (std::size_t i = 0; i < value.size(); ++i)
            WriteByte(address + static_cast<std::uint32_t>(i),
                      static_cast<std::uint8_t>(value[i]));
        WriteByte(address + static_cast<std::uint32_t>(value.size()), 0u);
    }
};

void EmptyArrayAndDuplicate() {
    pal4::MonsterBehaviorManagerState state{};
    Memory memory;
    memory.PutString(0x3000u, "combat");
    state.words[0x720u / 4u] = 0x1234u;
    pal4::MonsterUniqueText::Add(state, 0x3000u, memory);
    const auto first = state.words[0x724u / 4u];
    Check(state.words[0x720u / 4u] == 0x1234u &&
              first == 0x10000u &&
              state.words[0x728u / 4u] == first + 16u &&
              state.words[0x72cu / 4u] == first + 16u &&
              memory.ReadDword(first + 8u) == 6u &&
              memory.ReadByte(memory.ReadDword(first + 4u) - 1u) == 0u &&
              memory.freed_text.empty(),
          "first insert transfers one shared text reference");

    const auto call_count = memory.calls.size();
    pal4::MonsterUniqueText::Add(state, 0x3000u, memory);
    Check(memory.calls.size() == call_count &&
              state.words[0x728u / 4u] == first + 16u,
          "exact duplicate causes no allocation or insertion");
}

void PrefixAndGrowth() {
    pal4::MonsterBehaviorManagerState state{};
    Memory memory;
    memory.PutString(0x3000u, "combat");
    memory.PutString(0x4000u, "com");
    memory.PutString(0x5000u, "combat!");
    pal4::MonsterUniqueText::Add(state, 0x3000u, memory);
    const auto old_begin = state.words[0x724u / 4u];
    pal4::MonsterUniqueText::Add(state, 0x4000u, memory);
    pal4::MonsterUniqueText::Add(state, 0x5000u, memory);
    Check(state.words[0x728u / 4u] - state.words[0x724u / 4u] == 48u &&
              memory.ReadDword(state.words[0x724u / 4u] + 8u) == 6u &&
              memory.ReadDword(state.words[0x724u / 4u] + 24u) == 3u &&
              memory.ReadDword(state.words[0x724u / 4u] + 40u) == 7u &&
              memory.freed_records.size() == 3u &&
              memory.freed_records[1] == old_begin,
          "prefix variants stay distinct across record-array growth");
}

void FailedAllocationAndEmptyText() {
    pal4::MonsterBehaviorManagerState state{};
    Memory memory;
    memory.PutString(0x3000u, "value");
    memory.allocation_succeeds = false;
    pal4::MonsterUniqueText::Add(state, 0x3000u, memory);
    Check(memory.ReadDword(state.words[0x724u / 4u] + 8u) == 0u &&
              memory.freed_text.empty() &&
              state.words[0x728u / 4u] - state.words[0x724u / 4u] == 16u,
          "failed text allocation still inserts an empty record");

    pal4::MonsterBehaviorManagerState empty_state{};
    Memory empty_memory;
    empty_memory.PutString(0x4000u, "");
    pal4::MonsterUniqueText::Add(empty_state, 0x4000u, empty_memory);
    Check(empty_memory.freed_text.size() == 1u &&
              empty_memory.ReadDword(
                  empty_state.words[0x724u / 4u] + 8u) == 0u,
          "zero-length temporary allocation is released after insertion");
}

void ReferenceCountRelease() {
    for (const auto initial : {std::uint8_t{0}, std::uint8_t{7},
                               std::uint8_t{0xff}}) {
        pal4::MonsterBehaviorManagerState state{};
        Memory memory;
        memory.initial_refcount = initial;
        memory.PutString(0x3000u, "x");
        pal4::MonsterUniqueText::Add(state, 0x3000u, memory);
        const auto text = memory.ReadDword(0x8004u);
        if (initial == 0xffu) {
            Check(memory.freed_text.size() == 1u,
                  "0xff reference header uses free path");
        } else {
            Check(memory.freed_text.empty() &&
                      memory.ReadByte(text - 1u) == initial,
                  "ordinary shared header returns to initial count");
        }
    }
}

void InsertExceptionReleasesTemporary() {
    pal4::MonsterBehaviorManagerState state{};
    Memory memory;
    memory.PutString(0x3000u, "combat");
    memory.throw_on_records = true;
    bool threw = false;
    try {
        pal4::MonsterUniqueText::Add(state, 0x3000u, memory);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && memory.freed_text.size() == 1u &&
              state.words[0x724u / 4u] == 0u &&
              state.words[0x728u / 4u] == 0u,
          "failed record allocation unwinds temporary text ownership");
}

void LinkableStub() {
    pal4::MonsterBehaviorManagerState state{};
    pal4::MonsterUniqueTextRuntimeStub runtime;
    pal4::MonsterUniqueText::Add(state, 0x3000u, runtime);
    Check(state.words[0x728u / 4u] == state.words[0x724u / 4u] + 16u,
          "runtime stub links and records an insertion");
}

}  // namespace

int main() {
    EmptyArrayAndDuplicate();
    PrefixAndGrowth();
    FailedAllocationAndEmptyText();
    ReferenceCountRelease();
    InsertExceptionReleasesTemporary();
    LinkableStub();
    std::cout << "monster unique text cases passed\n";
}
