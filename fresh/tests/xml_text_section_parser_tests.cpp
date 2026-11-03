#include "pal4/xml_text_section_parser.h"

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
        words[0x8c20a0u + 15u * 4u] = 0x7000u;
        SetText(0x7000u, "bad CDATA");
    }
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> assigned;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
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
        auto& output = assigned[destination];
        output.clear();
        for (std::uint32_t i = 0u; i < length; ++i)
            output.push_back(static_cast<char>(ReadByte(source + i)));
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

void TestOrdinaryText() {
    Runtime runtime;
    runtime.SetText(0x2000u, "a&amp;b<next>");
    Check(pal4::XmlTextSectionParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, runtime) == 0x2007u &&
              runtime.assigned[0x3020u] == "a&b" &&
              runtime.bytes[0x302cu] == 0u &&
              runtime.allocations == std::vector<std::uint32_t>{4u} &&
              runtime.frees == std::vector<std::uint32_t>{0x8000u},
          "ordinary text decodes entities and leaves next opening tag");
    Runtime normalized;
    normalized.SetText(0x2000u, "  a  b<next>");
    normalized.bytes[0x8c1f98u] = 1u;
    Check(pal4::XmlTextSectionParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, normalized) == 0x2006u &&
              normalized.assigned[0x3020u] == "a b",
          "global normalization gate collapses whitespace");
    Runtime trailing;
    trailing.SetText(0x2000u, "text");
    Check(pal4::XmlTextSectionParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, trailing) == 0x2004u &&
              trailing.assigned[0x3020u] == "text",
          "missing next tag returns terminator");
}

void TestCData() {
    Runtime runtime;
    runtime.bytes[0x302cu] = 1u;
    runtime.SetText(0x2000u, "<![CDATA[a&amp;b]]>tail");
    Check(pal4::XmlTextSectionParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, runtime) == 0x2013u &&
              runtime.assigned[0x3020u] == "a&amp;b" &&
              runtime.bytes[0x302cu] == 1u,
          "CDATA body is retained without entity decoding");
    Runtime detected;
    detected.SetText(0x2000u, "<![CDATA[]]>");
    Check(pal4::XmlTextSectionParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, detected) == 0x200cu &&
              detected.assigned[0x3020u].empty() &&
              detected.bytes[0x302cu] == 1u &&
              detected.allocations.empty(),
          "CDATA prefix changes text mode and accepts empty body");
    Runtime unterminated;
    unterminated.bytes[0x302cu] = 1u;
    unterminated.SetText(0x2000u, "<![CDATA[x");
    Check(pal4::XmlTextSectionParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, unterminated) == 0x200du &&
              unterminated.assigned[0x3020u] == "x",
          "missing CDATA delimiter follows string parser NUL return");
}

void TestErrorPositionAndAllocationFailure() {
    Runtime invalid;
    invalid.parent = 0x1000u;
    invalid.bytes[0x302cu] = 1u;
    invalid.SetText(0x2000u, "not CDATA");
    Check(pal4::XmlTextSectionParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, invalid) == 0u &&
              invalid.words[0x1030u] == 15u &&
              invalid.bytes[0x102cu] == 1u &&
              invalid.assigned[0x3020u].empty(),
          "CDATA mode rejects missing prefix with error fifteen");
    Runtime positioned;
    positioned.SetText(0x2000u, "xy<next>");
    std::array<std::uint32_t, 4> position{4u, 6u, 0x2000u, 4u};
    Check(pal4::XmlTextSectionParser::Parse(
              0x3000u, 0x2002u, &position, 0, positioned) == 0x2002u &&
              positioned.words[0x3004u] == 4u &&
              positioned.words[0x3008u] == 8u &&
              position[2] == 0x2002u,
          "node position is copied from advanced cursor");
    Runtime failed;
    failed.SetText(0x2000u, "text<next>");
    failed.allocation_fails = true;
    Check(pal4::XmlTextSectionParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, failed) == 0u &&
              failed.assigned[0x3020u].empty(),
          "decoded text allocation failure returns null");
}

}  // namespace

int main() {
    TestOrdinaryText();
    TestCData();
    TestErrorPositionAndAllocationFailure();
}
