#include "pal4/xml_element_parser.h"

#include "pal4/compact_string_buffer.h"
#include "pal4/xml_parser_document.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IXmlElementRuntime {
    Runtime() {
        (void)pal4::XmlParserLifecycle::Initialize(0x3000u, 1u, *this);
        (void)pal4::XmlAttributeRoot::Initialize(0x302cu, *this);
        words[0x3010u] = 0x1000u;
        for (const auto code : {3u, 4u, 5u, 7u, 8u, 9u}) {
            const auto address = 0x7000u + code * 0x40u;
            words[0x8c20a0u + code * 4u] = address;
            SetText(address, "error");
        }
    }

    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;
    std::uint32_t next_attribute = 0x4000u;
    std::uint32_t next_block = 0x8000u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override { words[address] = value; }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override { bytes[address] = value; }
    std::uint32_t Free(const std::uint32_t address) override {
        if (address != 0xa000u) freed.push_back(address);
        return address;
    }
    std::uint32_t DeleteNode(std::uint32_t, std::uint32_t,
                             std::uint32_t) override { return 0u; }
    std::uint32_t StringLength(const std::uint32_t text) override {
        std::uint32_t length = 0u;
        while (ReadByte(text + length) != 0u) ++length;
        return length;
    }
    std::uint32_t AssignString(const std::uint32_t destination,
                               const std::uint32_t source,
                               const std::uint32_t length) override {
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
    std::int32_t CompareText(std::uint32_t left,
                             std::uint32_t right) override {
        for (;;) {
            const auto a = ReadByte(left++);
            const auto b = ReadByte(right++);
            if (a != b) return static_cast<int>(a) - static_cast<int>(b);
            if (a == 0u) return 0;
        }
    }
    std::uint32_t CallVirtual(const std::uint32_t node,
                              const std::uint32_t slot) override {
        Check(slot == 16u, "parent lookup slot");
        return node == 0x3000u ? 0x1000u : 0u;
    }
    std::uint32_t AttributeSetup(std::uint32_t, std::uint32_t,
                                 std::uint32_t, std::uint32_t,
                                 std::uint32_t) override { return 0u; }
    std::uint32_t Allocate(const std::uint32_t size) override {
        if (size == 48u) return 0x5000u;
        if (size < 36u) return 0xa000u;
        Check(size == 36u, "element parser allocates attributes or text");
        const auto address = next_attribute;
        next_attribute += 0x100u;
        return address;
    }
    std::uint32_t ConstructElement(std::uint32_t address) override {
        return address;
    }
    std::uint32_t ParseNode(
        std::uint32_t, std::uint32_t,
        std::array<std::uint32_t, 4>*, int) override { return 0u; }
    void SetText(const std::uint32_t address, const std::string& text) {
        for (std::size_t i = 0u; i < text.size(); ++i)
            bytes[address + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(text[i]);
        bytes[address + static_cast<std::uint32_t>(text.size())] = 0u;
    }
};

void TestSelfClosing() {
    Runtime runtime;
    runtime.SetText(0x2000u, " <root/>");
    std::array<std::uint32_t, 4> position{2u, 3u, 0x2000u, 4u};
    Check(pal4::XmlElementParser::Parse(
              0x3000u, 0x2000u, &position, 0, runtime) == 0x2008u &&
              runtime.words[0x3004u] == 2u &&
              runtime.words[0x3008u] == 4u &&
              position[2] == 0x2001u &&
              runtime.bytes[0x102cu] == 0u &&
              runtime.words[0x3018u] == 0u,
          "self-closing element parses name and advances location");
}

void TestContentAndEndTag() {
    Runtime runtime;
    runtime.SetText(0x2000u, "<a>text</a>");
    Check(pal4::XmlElementParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, runtime) == 0x200bu &&
              runtime.words[0x3018u] == 0x5000u,
          "matching closing tag is consumed");
    Runtime mismatch;
    mismatch.SetText(0x2000u, "<a>text</b>");
    Check(pal4::XmlElementParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, mismatch) == 0u &&
              mismatch.words[0x1030u] == 9u,
          "mismatched closing tag reports code nine");
}

void TestAttributes() {
    Runtime runtime;
    runtime.SetText(0x2000u, "<a id='x'/>");
    Check(pal4::XmlElementParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, runtime) == 0x200bu &&
              runtime.words[0x304cu] == 0x4000u &&
              runtime.words[0x4010u] == 0x1000u &&
              runtime.freed.empty(),
          "parsed attribute is linked to the element root");
    Runtime duplicate;
    duplicate.SetText(0x2000u, "<a id='x' id='y'/>");
    Check(pal4::XmlElementParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, duplicate) == 0u &&
              duplicate.words[0x304cu] == 0x4000u &&
              duplicate.freed.size() == 3u &&
              duplicate.freed.back() == 0x4100u,
          "duplicate name updates existing value and destroys new attribute");
}

void TestErrors() {
    struct Case { const char* text; std::uint32_t code; };
    for (const Case test : {Case{"text", 4u}, Case{"<1bad>", 5u},
                            Case{"<a ", 7u}, Case{"<a/x", 8u}}) {
        Runtime runtime;
        runtime.SetText(0x2000u, test.text);
        Check(pal4::XmlElementParser::Parse(
                  0x3000u, 0x2000u, nullptr, 0, runtime) == 0u &&
                  runtime.words[0x1030u] == test.code,
              "malformed element reports its observed error code");
    }
    Runtime failed_attribute;
    failed_attribute.SetText(0x2000u, "<a id='x");
    Check(pal4::XmlElementParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, failed_attribute) == 0u &&
              failed_attribute.words[0x1030u] == 4u &&
              failed_attribute.freed.back() == 0x4000u,
          "failed attribute parse releases the new object");
    Runtime allocation;
    allocation.SetText(0x2000u, "<a id='x'/>");
    allocation.next_attribute = 0u;
    Check(pal4::XmlElementParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, allocation) == 0u &&
              allocation.words[0x1030u] == 3u,
          "attribute allocation failure reports code three");
}

}  // namespace

int main() {
    TestSelfClosing();
    TestContentAndEndTag();
    TestAttributes();
    TestErrors();
}
