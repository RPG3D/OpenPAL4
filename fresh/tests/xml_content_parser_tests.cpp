#include "pal4/xml_content_parser.h"

#include "pal4/xml_parser_document.h"
#include "pal4/compact_string_buffer.h"

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
        words[0x8c20a0u + 3u * 4u] = 0x7000u;
        words[0x8c20a0u + 6u * 4u] = 0x7100u;
        SetText(0x7000u, "allocation");
        SetText(0x7100u, "content");
    }

    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> deleted;
    std::map<std::uint32_t, std::string> assigned;
    std::uint32_t next_text = 0x5000u;
    std::uint32_t next_child = 0x4000u;
    std::uint32_t next_generic = 0x6000u;
    std::uint32_t next_block = 0x9000u;

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
    std::uint32_t Free(std::uint32_t address) override { return address; }
    std::uint32_t DeleteNode(const std::uint32_t node, std::uint32_t,
                             std::uint32_t) override {
        deleted.push_back(node);
        return node;
    }
    std::uint32_t StringLength(const std::uint32_t source) override {
        std::uint32_t length = 0u;
        while (ReadByte(source + length) != 0u) ++length;
        return length;
    }
    std::uint32_t AssignString(const std::uint32_t destination,
                               const std::uint32_t source,
                               const std::uint32_t length) override {
        auto& value = assigned[destination];
        value.clear();
        for (std::uint32_t i = 0u; i < length; ++i)
            value.push_back(static_cast<char>(ReadByte(source + i)));
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
    std::uint32_t CallVirtual(const std::uint32_t node,
                              const std::uint32_t slot) override {
        Check(slot == 16u, "parent lookup slot");
        return node == 0x3000u || node == 0x4000u ? 0x1000u : 0u;
    }
    std::uint32_t AttributeSetup(std::uint32_t, std::uint32_t,
                                 std::uint32_t, std::uint32_t,
                                 std::uint32_t) override { return 0u; }
    std::uint32_t Allocate(const std::uint32_t size) override {
        allocations.push_back(size);
        if (size == 48u) return next_text;
        if (size == 80u) return next_child;
        if (size == 44u) return next_generic;
        if (size < 44u) return 0x8000u;
        return 0u;
    }
    std::uint32_t ConstructElement(const std::uint32_t address) override {
        (void)pal4::XmlParserLifecycle::Initialize(address, 1u, *this);
        (void)pal4::XmlAttributeRoot::Initialize(address + 44u, *this);
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

void TestEndAndMissingContent() {
    Runtime closing;
    closing.SetText(0x2000u, " </a>");
    Check(pal4::XmlContentParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, closing) == 0x2001u &&
              closing.allocations.empty(),
          "closing tag is returned without constructing a child");
    Runtime empty;
    Check(pal4::XmlContentParser::Parse(
              0x3000u, 0u, nullptr, 0, empty) == 0u &&
              empty.words[0x1030u] == 6u,
          "missing content input reports code six");
    Runtime trailing;
    trailing.SetText(0x2000u, "  ");
    Check(pal4::XmlContentParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, trailing) == 0x2002u &&
              trailing.words[0x1030u] == 0u,
          "whitespace consumed to terminator returns its address");
}

void TestTextHandling() {
    Runtime plain;
    plain.SetText(0x2000u, "abc</a>");
    Check(pal4::XmlContentParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, plain) == 0x2003u &&
              plain.allocations == std::vector<std::uint32_t>{48u, 4u} &&
              plain.words[0x3018u] == 0x5000u &&
              plain.words[0x5010u] == 0x3000u &&
              plain.bytes[0x502cu] == 0u &&
              plain.assigned[0x5020u] == "abc",
          "text node is initialized and linked before closing tag");
    Runtime filtered;
    filtered.SetText(0x2000u, "&#32;</a>");
    Check(pal4::XmlContentParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, filtered) == 0x2005u &&
              filtered.assigned[0x5020u] == " " &&
              filtered.deleted == std::vector<std::uint32_t>{0x5000u} &&
              filtered.words[0x3018u] == 0u,
          "decoded whitespace-only text is destroyed without linking");
    Runtime raw;
    raw.SetText(0x2000u, "  abc</a>");
    Check(pal4::XmlContentParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, raw) == 0x2005u &&
              raw.assigned[0x5020u] == "  abc",
          "disabled global condense passes original source to text parser");
    Runtime condense;
    condense.SetText(0x2000u, "  abc</a>");
    condense.bytes[0x8c1f98u] = 1u;
    Check(pal4::XmlContentParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, condense) == 0x2005u &&
              condense.assigned[0x5020u] == "abc",
          "enabled global condense passes skipped source to text parser");
}

void TestNestedAndAllocationFailure() {
    Runtime nested;
    nested.SetText(0x2000u, "<b/></a>");
    Check(pal4::XmlContentParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, nested) == 0x2004u &&
              nested.allocations == std::vector<std::uint32_t>{80u} &&
              nested.words[0x3018u] == 0x4000u &&
              nested.words[0x4010u] == 0x3000u,
          "nested element is parsed and linked before parent end tag");
    Runtime failed;
    failed.SetText(0x2000u, "text</a>");
    failed.next_text = 0u;
    Check(pal4::XmlContentParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, failed) == 0u &&
              failed.words[0x1030u] == 3u,
          "text allocation failure reports code three");
}

void TestGenericTagIntegration() {
    Runtime runtime;
    runtime.SetText(0x2000u, "<!unknown></a>");
    Check(pal4::XmlContentParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, runtime) == 0x200au &&
              runtime.allocations == std::vector<std::uint32_t>{44u} &&
              runtime.words[0x3018u] == 0x6000u &&
              runtime.words[0x6014u] == 3u,
          "generic child uses reconstructed tag parser and links node");
}

void TestCommentIntegration() {
    Runtime runtime;
    runtime.SetText(0x2000u, "<!--x--></a>");
    Check(pal4::XmlContentParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, runtime) == 0x2008u &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{44u, 2u} &&
              runtime.words[0x3018u] == 0x6000u &&
              runtime.words[0x6014u] == 2u,
          "comment child uses reconstructed parser before closing tag");
}

void TestCDataIntegration() {
    Runtime runtime;
    runtime.SetText(0x2000u, "<![CDATA[x]]></a>");
    Check(pal4::XmlContentParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, runtime) == 0x200du &&
              runtime.allocations == std::vector<std::uint32_t>{48u, 2u} &&
              runtime.words[0x3018u] == 0x5000u &&
              runtime.bytes[0x502cu] == 1u &&
              runtime.assigned[0x5020u] == "x",
          "CDATA child uses reconstructed text section parser");
}

}  // namespace

int main() {
    TestEndAndMissingContent();
    TestTextHandling();
    TestNestedAndAllocationFailure();
    TestGenericTagIntegration();
    TestCommentIntegration();
    TestCDataIntegration();
}
