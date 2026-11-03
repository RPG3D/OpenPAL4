#include "pal4/config_value_lookup.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Memory final : public pal4::IConfigValueMemory {
public:
    std::uint32_t StringLength(const std::uint32_t address) override {
        events.emplace_back("strlen");
        std::uint32_t length = 0u;
        while (ReadByte(address + length) != 0u) {
            ++length;
        }
        return length;
    }
    std::uint32_t ReadDword(const std::uint32_t address) override {
        std::uint32_t value = 0u;
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            value |= static_cast<std::uint32_t>(ReadByte(address + index))
                     << (8u * index);
        }
        return value;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        const auto found = bytes.find(address);
        return found == bytes.end() ? 0u : found->second;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    void Free(const std::uint32_t address) override {
        events.emplace_back("free");
        freed.push_back(address);
    }
    void CopyBytes(const std::uint32_t destination,
                   const std::uint32_t source,
                   const std::uint32_t count) override {
        events.emplace_back("copy");
        for (std::uint32_t index = 0u; index < count; ++index) {
            WriteByte(destination + index, ReadByte(source + index));
        }
    }
    void SetErrorFlag(pal4::ConfigStringState& text,
                      const std::uint8_t flag) override {
        events.emplace_back("flag");
        text.flag = flag;
    }
    void Reallocate(pal4::ConfigStringState& text,
                    const std::uint32_t size) override {
        events.emplace_back("reallocate");
        text.data = 0x2000u + allocation_count++ * 0x100u;
        text.capacity = size;
        WriteByte(text.data - 1u, initial_references);
    }
    void LengthError() override {
        events.emplace_back("length-error");
        ++length_errors;
        throw std::length_error("config string length");
    }
    std::uint32_t EmptyStringAddress() override {
        events.emplace_back("empty");
        return 0x7000u;
    }
    std::int32_t CompareRange(const std::uint32_t key_address,
                              const std::uint32_t start,
                              const std::uint32_t count,
                              const std::uint32_t other,
                              const std::uint32_t other_length) override {
        events.emplace_back("compare-range");
        Check(start == 0u &&
                  count == ReadDword(key_address + 8u),
              "tree helper compares full node key from offset zero");
        const std::uint32_t left = ReadDword(key_address + 4u);
        return CompareBytes(left, count, other, other_length);
    }
    std::int32_t CompareBytes(const std::uint32_t left,
                              const std::uint32_t left_length,
                              const std::uint32_t right,
                              const std::uint32_t right_length) {
        const auto common = std::min(left_length, right_length);
        for (std::uint32_t index = 0u; index < common; ++index) {
            if (ReadByte(left + index) < ReadByte(right + index)) {
                return -1;
            }
            if (ReadByte(left + index) > ReadByte(right + index)) {
                return 1;
            }
        }
        return left_length < right_length ? -1
             : left_length > right_length ? 1
             : 0;
    }
    void PutDword(const std::uint32_t address,
                  const std::uint32_t value) {
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            WriteByte(address + index,
                      static_cast<std::uint8_t>(value >> (8u * index)));
        }
    }
    void PutText(const std::uint32_t address,
                 const std::string_view text) {
        for (std::size_t index = 0u; index < text.size(); ++index) {
            WriteByte(address + static_cast<std::uint32_t>(index),
                      static_cast<std::uint8_t>(text[index]));
        }
        WriteByte(address + static_cast<std::uint32_t>(text.size()), 0u);
    }
    void PutNode(const std::uint32_t address, const std::uint32_t left,
                 const std::uint32_t right, const std::uint32_t text_address,
                 const std::string_view key) {
        PutDword(address, left);
        PutDword(address + 8u, right);
        PutDword(address + 0x10u, text_address);
        PutDword(address + 0x14u,
                 static_cast<std::uint32_t>(key.size()));
        PutText(text_address, key);
    }

    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;
    std::vector<std::string> events;
    std::uint8_t initial_references = 0u;
    std::uint32_t allocation_count = 0u;
    std::uint32_t length_errors = 0u;
};

void SetupTree(Memory& memory) {
    constexpr std::uint32_t nil = 0x9000u;
    constexpr std::uint32_t header = 0x1000u;
    memory.PutDword(pal4::ConfigValueLookup::kNilNodeGlobal, nil);
    memory.PutDword(0x8000u + 0x18u, header);
    memory.PutDword(header + 4u, 0x1100u);
    memory.PutNode(0x1100u, 0x1200u, 0x1300u, 0x6000u, "m");
    memory.PutNode(0x1200u, nil, nil, 0x6100u, "a");
    memory.PutNode(0x1300u, nil, nil, 0x6200u, "z");
}

void MessageAssignmentBranches() {
    Memory memory;
    memory.PutText(0x5000u, "abc");
    pal4::ConfigStringState text{};
    Check(&pal4::ConfigValueLookup::AssignMessage(
              text, 0x5000u, 3u, memory) == &text &&
              text.data == 0x2000u && text.size == 3u &&
              text.capacity == 3u &&
              memory.ReadByte(0x2000u) == 'a' &&
              memory.ReadByte(0x2003u) == 0u &&
              memory.events ==
                  std::vector<std::string>{
                      "flag", "reallocate", "copy"},
          "empty destination allocates, copies and terminates");

    memory.events.clear();
    text.data = 0x3000u;
    text.size = 2u;
    text.capacity = 10u;
    memory.WriteByte(0x2fffu, 3u);
    Check(&pal4::ConfigValueLookup::AssignMessage(
              text, 0x5000u, 0u, memory) == &text &&
              text.data == 0u && text.size == 0u &&
              text.capacity == 0u &&
              memory.ReadByte(0x2fffu) == 2u &&
              memory.events.empty(),
          "zero-length assignment decrements shared reference and clears");

    memory.events.clear();
    text = {};
    (void)pal4::ConfigValueLookup::AssignMessage(
        text, 0x5000u, 0u, memory);
    Check(text.flag == 1u &&
              memory.events == std::vector<std::string>{"flag"},
          "zero-length unshared assignment only sets error flag");

    memory.events.clear();
    text = {0u, 0x3000u, 2u, 10u};
    memory.WriteByte(0x2fffu, 2u);
    (void)pal4::ConfigValueLookup::AssignMessage(
        text, 0x5000u, 3u, memory);
    Check(text.data == 0x2100u && text.flag == 0u &&
              memory.events ==
                  std::vector<std::string>{"reallocate", "copy"},
          "nonzero shared reference forces reallocation without flag");

    memory.events.clear();
    text = {0u, 0x3000u, 2u, 40u};
    memory.WriteByte(0x2fffu, 0u);
    (void)pal4::ConfigValueLookup::AssignMessage(
        text, 0x5000u, 3u, memory);
    Check(text.flag == 1u &&
              memory.events ==
                  std::vector<std::string>{
                      "flag", "reallocate", "copy"},
          "capacity over 31 triggers flag and reallocation");

    memory.events.clear();
    text = {0u, 0x3000u, 2u, 3u};
    memory.WriteByte(0x2fffu, 0u);
    (void)pal4::ConfigValueLookup::AssignMessage(
        text, 0x5000u, 3u, memory);
    Check(memory.events == std::vector<std::string>{"copy"},
          "unshared capacity within 31 and large enough is reused");

    memory.events.clear();
    bool length_error = false;
    try {
        (void)pal4::ConfigValueLookup::AssignMessage(
            text, 0x5000u, 0xfffffffeu, memory);
    } catch (const std::length_error&) {
        length_error = true;
    }
    Check(length_error && memory.length_errors == 1u &&
              memory.events.front() == "length-error",
          "length greater than 0xfffffffd invokes exception boundary");
}

void CompareTextBranches() {
    Memory memory;
    memory.PutText(0x5000u, "abc");
    memory.PutText(0x6000u, "abc");
    memory.PutDword(0x1000u + 4u, 0x6000u);
    memory.PutDword(0x1000u + 8u, 3u);
    pal4::ConfigStringState left{0u, 0x5000u, 3u, 3u};
    Check(pal4::ConfigValueLookup::CompareText(left, 0x1000u,
                                                memory) == 0,
          "equal keys compare zero");
    memory.WriteByte(0x6001u, 'd');
    Check(pal4::ConfigValueLookup::CompareText(left, 0x1000u,
                                                memory) == -1,
          "first differing unsigned byte orders keys");
    memory.WriteByte(0x6001u, 'b');
    left.size = 2u;
    Check(pal4::ConfigValueLookup::CompareText(left, 0x1000u,
                                                memory) == -1,
          "shorter matching prefix compares negative");
    left.size = 4u;
    Check(pal4::ConfigValueLookup::CompareText(left, 0x1000u,
                                                memory) == 1,
          "longer matching prefix compares positive");
    left.size = 0u;
    memory.PutDword(0x1000u + 4u, 0u);
    memory.PutDword(0x1000u + 8u, 0u);
    Check(pal4::ConfigValueLookup::CompareText(left, 0x1000u,
                                                memory) == 0 &&
              !memory.events.empty() &&
              memory.events.back() == "empty",
          "null right pointer uses shared empty string");
}

void HostStringComparisonBoundary() {
    for (const auto& [left_text, right_text] : {
             std::pair{std::string("a\x80", 2), std::string("a\x7f", 2)},
             std::pair{std::string("a\0z", 3), std::string("a\0y", 3)},
             std::pair{std::string("a\0", 2), std::string("a\0z", 3)},
             std::pair{std::string("same"), std::string("same")},
         }) {
        Memory memory;
        memory.PutText(0x5000u, left_text);
        memory.PutText(0x6000u, right_text);
        memory.PutDword(0x1004u, 0x6000u);
        memory.PutDword(0x1008u,
                        static_cast<std::uint32_t>(right_text.size()));
        const pal4::ConfigStringState left{
            0u, 0x5000u, static_cast<std::uint32_t>(left_text.size()), 0u};
        const auto original = pal4::ConfigValueLookup::CompareText(
            left, 0x1000u, memory);
        const auto host = left_text.compare(right_text);
        Check((original < 0) == (host < 0) &&
                  (original == 0) == (host == 0) &&
                  (original > 0) == (host > 0),
              "host std::string ordering matches VC6 byte and length branches");
    }
}

void TreeAndValueLookup() {
    Memory memory;
    SetupTree(memory);
    for (const auto [text, expected] : {
             std::pair{"a", 0x1200u + 0x1cu},
             std::pair{"m", 0x1100u + 0x1cu},
             std::pair{"z", 0x1300u + 0x1cu},
             std::pair{"b", 0u},
             std::pair{"zz", 0u},
         }) {
        memory.PutText(0x5000u, text);
        memory.freed.clear();
        Check(pal4::ConfigValueLookup::GetConfigStringValue(
                  0x8000u, 0x5000u, memory) == expected &&
                  memory.freed.size() == 1u &&
                  memory.freed.back() ==
                      0x1fffu + (memory.allocation_count - 1u) * 0x100u,
              "tree lookup returns node+0x1c only for exact keys");
    }
    pal4::ConfigStringState key{0u, 0u, 0u, 0u};
    memory.events.clear();
    Check(pal4::ConfigValueLookup::FindConfigItem(
              0x8000u + 0x14u, key, memory) == 0x1200u &&
              std::find(memory.events.begin(), memory.events.end(),
                        "empty") != memory.events.end(),
          "null temporary text uses shared empty string in tree compare");
}

void SharedLookupCleanup() {
    Memory memory;
    SetupTree(memory);
    memory.PutText(0x5000u, "m");
    memory.initial_references = 3u;
    Check(pal4::ConfigValueLookup::GetConfigStringValue(
              0x8000u, 0x5000u, memory) == 0x111cu &&
              memory.freed.empty() &&
              memory.ReadByte(0x1fffu) == 2u,
          "positive reference count is decremented instead of freed");
    memory.initial_references = 0xffu;
    Check(pal4::ConfigValueLookup::GetConfigStringValue(
              0x8000u, 0x5000u, memory) == 0x111cu &&
              memory.freed.back() == 0x20ffu,
          "0xff reference sentinel frees the temporary header");
}

}  // namespace

int main() {
    MessageAssignmentBranches();
    CompareTextBranches();
    HostStringComparisonBoundary();
    TreeAndValueLookup();
    SharedLookupCleanup();
    std::cout << "5 config value lookup behavior groups passed\n";
}
