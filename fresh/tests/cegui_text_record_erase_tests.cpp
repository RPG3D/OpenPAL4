#include "pal4/cegui_text_record_array.h"

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

struct Memory final : pal4::ICeguiTextRecordArrayRuntime {
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;
    std::vector<std::string> calls;
    std::uint32_t next_text{0x9000u};
    bool allocation_succeeds{true};

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        std::uint32_t value = 0u;
        for (std::uint32_t index = 0u; index < 4u; ++index)
            value |= static_cast<std::uint32_t>(ReadByte(address + index))
                     << (index * 8u);
        return value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        for (std::uint32_t index = 0u; index < 4u; ++index)
            WriteByte(address + index,
                      static_cast<std::uint8_t>(value >> (index * 8u)));
    }
    std::uint32_t EmptyTextAddress() override { return 0x8001u; }
    void SetErrorFlag(std::uint32_t record, std::uint8_t flag) override {
        calls.emplace_back("flag");
        const auto old = ReadDword(record + 4u);
        if (old != 0u)
            freed.push_back(old - 1u);
        WriteByte(record, flag);
    }
    bool AllocateText(std::uint32_t record, std::uint32_t,
                      std::uint8_t mode) override {
        Check(mode == 1u, "deep copy text allocation mode");
        calls.emplace_back("allocate-text");
        if (!allocation_succeeds)
            return false;
        WriteDword(record + 4u, next_text + 1u);
        WriteByte(next_text, 0u);
        next_text += 0x100u;
        return true;
    }
    void DetachAndReset(std::uint32_t) override {}
    void SetTextSize(std::uint32_t record, std::uint32_t size) override {
        calls.emplace_back("set-size");
        WriteDword(record + 8u, size);
        WriteByte(ReadDword(record + 4u) + size, 0u);
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t size) override {
        calls.emplace_back("copy");
        for (std::uint32_t index = 0u; index < size; ++index)
            WriteByte(destination + index, ReadByte(source + index));
    }
    [[noreturn]] void ThrowRange() override {
        throw std::out_of_range("range");
    }
    std::uint32_t AllocateRecords(std::uint32_t) override { return 0u; }
    void FreeRecords(std::uint32_t address) override {
        calls.emplace_back("free");
        freed.push_back(address);
    }
    void AssignRecord(std::uint32_t, std::uint32_t) override {}
    void EraseText(std::uint32_t record, std::uint32_t offset,
                   std::uint32_t count) override {
        Check(record == 0x1000u, "self erase destination");
        calls.emplace_back(std::string("erase:") +
                           std::to_string(offset) + ":" +
                           std::to_string(count));
    }

    void Record(std::uint32_t address, std::uint32_t text,
                std::uint32_t length, std::uint8_t count) {
        WriteDword(address, 0x55u);
        WriteDword(address + 4u, text);
        WriteDword(address + 8u, length);
        WriteDword(address + 12u, length + 1u);
        if (text != 0u)
            WriteByte(text - 1u, count);
    }
};

void ClearAll() {
    Memory memory;
    memory.Record(0x1000u, 0x4001u, 2u, 0u);
    memory.Record(0x1010u, 0x5001u, 3u, 3u);
    memory.Record(0x1020u, 0x6001u, 4u, 0xffu);
    pal4::CeguiTextRecordArrayState array{77u, 0x1000u, 0x1030u,
                                          0x1040u};
    Check(pal4::CeguiTextRecordArray::EraseRange(
              array, 0x1000u, 0x1030u, memory) == 0x1000u &&
              array.end == 0x1000u && array.begin == 0x1000u &&
              array.capacity == 0x1040u && array.unused == 77u &&
              memory.freed == std::vector<std::uint32_t>{
                  0x4000u, 0x6000u} &&
              memory.ReadByte(0x5000u) == 2u,
          "clear all releases zero and ff headers and decrements shared one");
    for (auto record = 0x1000u; record != 0x1030u; record += 16u)
        Check(memory.ReadDword(record) == 0x55u &&
                  memory.ReadDword(record + 4u) == 0u &&
                  memory.ReadDword(record + 8u) == 0u &&
                  memory.ReadDword(record + 12u) == 0u,
              "tail cleanup clears only pointer, size and capacity");
}

void MoveAndShareSuffix() {
    Memory memory;
    memory.Record(0x1000u, 0x4001u, 1u, 0u);
    memory.Record(0x1010u, 0x5001u, 3u, 1u);
    memory.WriteByte(0x5001u, 'a');
    memory.WriteByte(0x5002u, 'b');
    memory.WriteByte(0x5003u, 'c');
    pal4::CeguiTextRecordArrayState array{0u, 0x1000u, 0x1020u,
                                          0x1030u};
    Check(pal4::CeguiTextRecordArray::EraseRange(
              array, 0x1000u, 0x1010u, memory) == 0x1000u &&
              array.end == 0x1010u &&
              memory.ReadDword(0x1004u) == 0x5001u &&
              memory.ReadDword(0x1008u) == 3u &&
              memory.ReadDword(0x1014u) == 0u &&
              memory.ReadByte(0x5000u) == 1u &&
              memory.calls == std::vector<std::string>{"flag"},
          "remaining suffix shares text then releases the trailing owner");
}

void DeepCopyAndFailure() {
    for (const auto succeeds : {true, false}) {
        Memory memory;
        memory.allocation_succeeds = succeeds;
        memory.Record(0x1000u, 0u, 0u, 0u);
        memory.Record(0x1010u, 0x5001u, 2u, 0xfeu);
        memory.WriteByte(0x5001u, 'x');
        memory.WriteByte(0x5002u, 'y');
        pal4::CeguiTextRecordArrayState array{0u, 0x1000u, 0x1020u,
                                              0x1030u};
        (void)pal4::CeguiTextRecordArray::EraseRange(
            array, 0x1000u, 0x1010u, memory);
        Check(array.end == 0x1010u &&
                  memory.ReadByte(0x5000u) == 0xfdu &&
                  memory.ReadDword(0x1014u) == 0u &&
                  memory.ReadDword(0x1008u) == (succeeds ? 2u : 0u),
              "high-refcount source takes deep-copy or allocation-failure path");
        if (succeeds)
            Check(memory.ReadByte(memory.ReadDword(0x1004u)) == 'x' &&
                      memory.ReadByte(memory.ReadDword(0x1004u) + 1u) == 'y',
                  "deep copy retains source bytes");
    }
}

void SelfAndEmptyRange() {
    Memory memory;
    memory.Record(0x1000u, 0x4001u, 2u, 1u);
    pal4::CeguiTextRecordArrayState array{0u, 0x1000u, 0x1010u,
                                          0x1020u};
    Check(pal4::CeguiTextRecordArray::EraseRange(
              array, 0x1000u, 0x1000u, memory) == 0x1000u &&
              array.end == 0x1010u &&
              memory.calls == std::vector<std::string>{
                  "erase:2:4294967295", "erase:0:0"},
          "same source and destination invokes both string erases");
    memory.calls.clear();
    Check(pal4::CeguiTextRecordArray::EraseRange(
              array, 0x1010u, 0x1010u, memory) == 0x1010u &&
              memory.calls.empty() && array.end == 0x1010u,
          "empty range at end leaves array intact");
}

}  // namespace

int main() {
    ClearAll();
    MoveAndShareSuffix();
    DeepCopyAndFailure();
    SelfAndEmptyRange();
    std::cout << "CEGUI text record erase cases passed\n";
}
