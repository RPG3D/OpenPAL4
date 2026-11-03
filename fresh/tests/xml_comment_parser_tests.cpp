#include "pal4/xml_comment_parser.h"

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
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::string assigned;
    std::uint32_t assigned_to = 0u;
    std::uint32_t parent = 0u;
    bool allocation_fails = false;

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
        assigned_to = destination;
        assigned.clear();
        for (std::uint32_t i = 0u; i < length; ++i)
            assigned.push_back(static_cast<char>(ReadByte(source + i)));
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
        return allocation_fails ? 0u : 0x8000u;
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
};

void TestBodyAndEntityDecoding() {
    Runtime runtime;
    runtime.SetText(0x2000u, "<!--a&amp;b-->tail");
    Check(pal4::XmlCommentParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, runtime) == 0x200eu &&
              runtime.assigned_to == 0x3020u &&
              runtime.assigned == "a&b" &&
              runtime.allocations == std::vector<std::uint32_t>{4u} &&
              runtime.frees == std::vector<std::uint32_t>{0x8000u},
          "comment body decodes entities and consumes closing delimiter");
    runtime.SetText(0x2100u, "<!---->");
    Check(pal4::XmlCommentParser::Parse(
              0x3000u, 0x2100u, nullptr, 0, runtime) == 0x2107u &&
              runtime.assigned.empty() && runtime.allocations.size() == 1u,
          "empty comment clears text without scratch allocation");
}

void TestMissingDelimiterAndInvalidPrefix() {
    Runtime runtime;
    runtime.SetText(0x2000u, "<!--x");
    Check(pal4::XmlCommentParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, runtime) == 0x2008u &&
              runtime.assigned == "x",
          "missing delimiter follows XML string parser NUL return");
    runtime.parent = 0x1000u;
    runtime.words[0x8c20a0u + 11u * 4u] = 0x7000u;
    runtime.SetText(0x7000u, "bad comment");
    runtime.SetText(0x2100u, "<!other>");
    Check(pal4::XmlCommentParser::Parse(
              0x3000u, 0x2100u, nullptr, 0, runtime) == 0u &&
              runtime.words[0x1030u] == 11u &&
              runtime.bytes[0x102cu] == 1u &&
              runtime.assigned == "bad comment",
          "wrong prefix reports error eleven to parent document");
    Runtime no_parent;
    Check(pal4::XmlCommentParser::Parse(
              0x3000u, 0u, nullptr, 0, no_parent) == 0u,
          "null source with no parent exits");
}

void TestCursorBomAndAllocationFailure() {
    Runtime runtime;
    runtime.bytes[0x2000u] = 0xefu;
    runtime.bytes[0x2001u] = 0xbbu;
    runtime.bytes[0x2002u] = 0xbfu;
    runtime.SetText(0x2003u, " <!--z-->");
    std::array<std::uint32_t, 4> position{4u, 2u, 0x2000u, 4u};
    Check(pal4::XmlCommentParser::Parse(
              0x3000u, 0x2000u, &position, 1, runtime) == 0x200cu &&
              runtime.words[0x3004u] == 4u &&
              runtime.words[0x3008u] == 3u &&
              position[2] == 0x2004u && runtime.assigned == "z",
          "BOM and whitespace update node position before parsing");
    runtime.allocation_fails = true;
    runtime.SetText(0x2100u, "<!--new-->");
    Check(pal4::XmlCommentParser::Parse(
              0x3000u, 0x2100u, nullptr, 0, runtime) == 0u &&
              runtime.assigned.empty(),
          "decoded text allocation failure returns null after clearing node");
}

}  // namespace

int main() {
    TestBodyAndEntityDecoding();
    TestMissingDelimiterAndInvalidPrefix();
    TestCursorBomAndAllocationFailure();
}
