#include "pal4/xml_declaration_parser.h"

#include "pal4/compact_string_buffer.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IXmlNodeFactoryRuntime {
    Runtime() {
        words[0x8c20a0u + 12u * 4u] = 0x7000u;
        SetText(0x7000u, "bad declaration");
    }
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::uint32_t next_attribute = 0x4000u;
    std::uint32_t next_block = 0x8000u;
    std::uint32_t parent = 0u;
    bool attribute_allocation_fails = false;

    std::uint32_t ReadWord(std::uint32_t address) override { return words[address]; }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override { return bytes[address]; }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Free(std::uint32_t address) override {
        frees.push_back(address);
        return address;
    }
    std::uint32_t DeleteNode(std::uint32_t node, std::uint32_t,
                             std::uint32_t) override { return node; }
    std::uint32_t StringLength(std::uint32_t address) override {
        std::uint32_t length = 0u;
        while (ReadByte(address + length) != 0u) ++length;
        return length;
    }
    std::uint32_t AssignString(std::uint32_t destination, std::uint32_t source,
                               std::uint32_t length) override {
        if (length == 0u) {
            words[destination] = pal4::CompactStringBuffer::kEmptyBlock;
            return destination;
        }
        const auto block = next_block;
        next_block += 0x100u;
        words[destination] = block;
        words[block] = length;
        words[block + 4u] = length;
        for (std::uint32_t i = 0u; i < length; ++i)
            bytes[block + 8u + i] = ReadByte(source + i);
        bytes[block + 8u + length] = 0u;
        return destination;
    }
    std::int32_t CompareText(std::uint32_t, std::uint32_t) override { return 0; }
    std::uint32_t CallVirtual(std::uint32_t node, std::uint32_t slot) override {
        Check(slot == 16u, "parent lookup slot");
        return node == 0x3000u ? parent : 0u;
    }
    std::uint32_t AttributeSetup(std::uint32_t, std::uint32_t,
                                 std::uint32_t, std::uint32_t,
                                 std::uint32_t) override { return 0u; }
    std::uint32_t Allocate(std::uint32_t size) override {
        allocations.push_back(size);
        if (size < 36u) return 0xa000u;
        if (size != 36u || attribute_allocation_fails) return 0u;
        const auto result = next_attribute;
        next_attribute += 0x100u;
        return result;
    }
    std::uint32_t ConstructElement(std::uint32_t address) override {
        return address;
    }
    void SetText(std::uint32_t address, const std::string& text) {
        for (std::size_t i = 0u; i < text.size(); ++i)
            bytes[address + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(text[i]);
        bytes[address + static_cast<std::uint32_t>(text.size())] = 0u;
    }
    std::string Field(std::uint32_t offset) {
        auto address = ReadWord(0x3000u + offset) + 8u;
        std::string value;
        while (ReadByte(address) != 0u)
            value.push_back(static_cast<char>(ReadByte(address++)));
        return value;
    }
};

void TestKnownAttributes() {
    Runtime runtime;
    const std::string source =
        "<?xml version='1.0' encoding='uTf-8' standalone='yes'?>";
    runtime.SetText(0x2000u, source);
    Check(pal4::XmlDeclarationParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, runtime) ==
              0x2000u + source.size() &&
              runtime.Field(44u) == "1.0" &&
              runtime.Field(48u) == "uTf-8" &&
              runtime.Field(52u) == "yes" &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{36u, 4u, 36u, 6u, 36u, 4u} &&
              runtime.frees.size() == 12u,
          "declaration stores and cleans three recognized attributes");
    Runtime folded;
    folded.SetText(0x2000u, " <?XML VERSION='2'?>");
    std::array<std::uint32_t, 4> position{2u, 4u, 0x2000u, 4u};
    Check(pal4::XmlDeclarationParser::Parse(
              0x3000u, 0x2000u, &position, 0, folded) ==
                  0x2000u + folded.StringLength(0x2000u) &&
              folded.Field(44u) == "2" &&
              folded.words[0x3004u] == 2u &&
              folded.words[0x3008u] == 5u,
          "prefix and attribute names are case folded; location is copied");
}

void TestUnknownAndFailurePaths() {
    Runtime unknown;
    unknown.SetText(0x2000u, "<?xml custom version='1'?>");
    Check(pal4::XmlDeclarationParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, unknown) ==
                  0x2000u + unknown.StringLength(0x2000u) &&
              unknown.Field(44u) == "1" &&
              unknown.Field(48u).empty(),
          "unknown token is skipped before known attributes");
    Runtime invalid;
    invalid.parent = 0x1000u;
    invalid.SetText(0x2000u, "<?other?>");
    Check(pal4::XmlDeclarationParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, invalid) == 0u &&
              invalid.words[0x1030u] == 12u &&
              invalid.bytes[0x102cu] == 1u,
          "invalid declaration prefix reports error twelve");
    Runtime empty;
    Check(pal4::XmlDeclarationParser::Parse(
              0x3000u, 0u, nullptr, 0, empty) == 0u,
          "null declaration source returns null");
    Runtime truncated;
    truncated.SetText(0x2000u, "<?xml version='1'");
    Check(pal4::XmlDeclarationParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, truncated) == 0u &&
              truncated.Field(44u) == "1",
          "missing declaration terminator preserves parsed value");
    Runtime allocation;
    allocation.SetText(0x2000u, "<?xml version='1'?>");
    allocation.attribute_allocation_fails = true;
    Check(pal4::XmlDeclarationParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, allocation) == 0u,
          "temporary attribute allocation failure returns null");
}

}  // namespace

int main() {
    TestKnownAttributes();
    TestUnknownAndFailurePaths();
}
