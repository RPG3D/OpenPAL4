#include "pal4/xml_generic_tag_parser.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IXmlParserNodeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::string assigned;
    std::uint32_t assigned_to = 0u;
    std::uint32_t parent = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override { return words[address]; }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override { return bytes[address]; }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Free(std::uint32_t address) override { return address; }
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
    void SetText(std::uint32_t address, const std::string& text) {
        for (std::size_t i = 0; i < text.size(); ++i)
            bytes[address + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(text[i]);
        bytes[address + static_cast<std::uint32_t>(text.size())] = 0u;
    }
};

void TestNormalAndEmptyTag() {
    Runtime runtime;
    runtime.SetText(0x2000u, "<!DOCTYPE root><next>");
    Check(pal4::XmlGenericTagParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, runtime) == 0x200fu &&
              runtime.assigned == "!DOCTYPE root" &&
              runtime.assigned_to == 0x3020u,
          "generic tag captures all bytes between angle brackets");
    runtime.SetText(0x2100u, "<>");
    Check(pal4::XmlGenericTagParser::Parse(
              0x3000u, 0x2100u, nullptr, 0, runtime) == 0x2102u &&
              runtime.assigned.empty(),
          "empty generic tag clears prior text");
}

void TestTerminatorAndInvalidOpen() {
    Runtime runtime;
    runtime.SetText(0x2000u, "<unfinished");
    Check(pal4::XmlGenericTagParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, runtime) == 0x200bu &&
              runtime.assigned == "unfinished",
          "missing closing bracket returns terminator and retains text");
    runtime.assigned = "old";
    runtime.SetText(0x2100u, "other");
    Check(pal4::XmlGenericTagParser::Parse(
              0x3000u, 0x2100u, nullptr, 0, runtime) == 0u &&
              runtime.assigned == "old",
          "invalid opening byte does not alter text");
    Check(pal4::XmlGenericTagParser::Parse(
              0x3000u, 0u, nullptr, 0, runtime) == 0u,
          "null source returns null");
    runtime.SetText(0x2200u, "");
    Check(pal4::XmlGenericTagParser::Parse(
              0x3000u, 0x2200u, nullptr, 0, runtime) == 0u,
          "initial NUL returns null");
}

void TestPositionAndParentError() {
    Runtime runtime;
    runtime.SetText(0x2000u, " \n<opaque>");
    runtime.words[0x8c20a0u + 10u * 4u] = 0x7000u;
    runtime.SetText(0x7000u, "bad tag");
    std::array<std::uint32_t, 4> position{2u, 5u, 0x2000u, 4u};
    Check(pal4::XmlGenericTagParser::Parse(
              0x3000u, 0x2000u, &position, 0, runtime) == 0x200au &&
              position[0] == 3u && position[1] == 0u &&
              position[2] == 0x2002u &&
              runtime.words[0x3004u] == 3u && runtime.words[0x3008u] == 0u &&
              runtime.assigned == "opaque",
          "leading whitespace advances and copies node position");
    Runtime bom;
    bom.bytes[0x2000u] = 0xefu;
    bom.bytes[0x2001u] = 0xbbu;
    bom.bytes[0x2002u] = 0xbfu;
    bom.SetText(0x2003u, "<!x>");
    Check(pal4::XmlGenericTagParser::Parse(
              0x3000u, 0x2000u, nullptr, 1, bom) == 0x2007u &&
              bom.assigned == "!x",
          "UTF-8 BOM is skipped before opening bracket");
    runtime.parent = 0x1000u;
    runtime.SetText(0x2100u, "unexpected");
    Check(pal4::XmlGenericTagParser::Parse(
              0x3000u, 0x2100u, nullptr, 0, runtime) == 0u &&
              runtime.bytes[0x102cu] == 1u &&
              runtime.words[0x1030u] == 10u &&
              runtime.assigned_to == 0x1034u &&
              runtime.assigned == "bad tag",
          "invalid open reports error ten to parent document");
}

}  // namespace

int main() {
    TestNormalAndEmptyTag();
    TestTerminatorAndInvalidOpen();
    TestPositionAndParentError();
}
