#include "pal4/xml_document_content_parser.h"
#include "pal4/xml_parser_document.h"
#include "pal4/compact_string_buffer.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IXmlDocumentContentRuntime {
    Runtime() {
        words[0x8c20a0u + 13u * 4u] = 0x8c2194u;
        SetText(0x8c2194u, {'E', 'r', 'r', 'o', 'r'});
    }
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> nodes;
    std::vector<std::uint32_t> ends;
    std::vector<std::uint32_t> allocations;
    std::vector<std::tuple<std::uint32_t, int>> parses;
    std::array<std::uint32_t, 4> last_cursor{};
    std::uint32_t declaration = 0u;
    std::uint32_t error_kind = 0u;
    std::uint32_t error_mode = 0u;
    std::uint32_t assigned_destination = 0u;
    std::uint32_t assigned_length = 99u;
    std::uint32_t next_block = 0x9000u;
    std::size_t next_node = 0u;

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
    std::uint32_t Free(std::uint32_t) override { return 0u; }
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
        assigned_destination = destination;
        assigned_length = length;
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
        if (slot == 16u) return 0u;
        Check(slot == 52u, "declaration virtual slot");
        return node == nodes.front() ? declaration : 0u;
    }
    std::uint32_t AttributeSetup(std::uint32_t, const std::uint32_t kind,
                                 std::uint32_t, std::uint32_t,
                                 const std::uint32_t mode) override {
        error_kind = kind;
        error_mode = mode;
        return 0u;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        allocations.push_back(bytes);
        if (bytes == 36u) return 0x8000u;
        if (bytes < 44u) return 0xa000u;
        const auto index = next_node++;
        return index < nodes.size() ? nodes[index] : 0u;
    }
    std::uint32_t ConstructElement(const std::uint32_t address) override {
        (void)pal4::XmlParserLifecycle::Initialize(address, 1u, *this);
        (void)pal4::XmlAttributeRoot::Initialize(address + 44u, *this);
        return address;
    }
    std::uint32_t ParseNode(
        std::uint32_t node, const std::uint32_t source,
        std::array<std::uint32_t, 4>* cursor,
        const int mode) override {
        parses.emplace_back(source, mode);
        if (cursor != nullptr) last_cursor = *cursor;
        const auto index = parses.size() - 1u;
        Check(index < ends.size() && node == nodes[index],
              "node parse matches factory output");
        return ends[index];
    }
    void SetText(const std::uint32_t address,
                 const std::vector<std::uint8_t>& text) {
        for (std::size_t i = 0u; i < text.size(); ++i)
            bytes[address + static_cast<std::uint32_t>(i)] = text[i];
        bytes[address + static_cast<std::uint32_t>(text.size())] = 0u;
    }
};

void TestEmptyDocument() {
    Runtime runtime;
    runtime.words[0x1004u] = 99u;
    Check(pal4::XmlDocumentContentParser::Parse(
              0x1000u, 0u, nullptr, 2, runtime) == 0u &&
              runtime.bytes[0x102cu] == 1u &&
              runtime.words[0x1030u] == 13u &&
              runtime.assigned_destination == 0x1034u &&
              runtime.assigned_length == 5u &&
              runtime.words[0x1004u] == 99u,
          "empty source resets document and reports error without position init");
    runtime.SetText(0x2000u, {' ', '<', 'x', '>'});
    Check(pal4::XmlDocumentContentParser::Parse(
              0x1000u, 0x2000u, nullptr, 2, runtime) == 0u &&
              runtime.words[0x1030u] == 13u &&
              runtime.words[0x1004u] == 0u && runtime.words[0x1008u] == 0u &&
              runtime.allocations == std::vector<std::uint32_t>{80u},
          "failed factory reports error with current encoding mode");
}

void TestBomAndChildInsertion() {
    Runtime runtime;
    runtime.SetText(0x2000u,
                    {0xefu, 0xbbu, 0xbfu, ' ', '<', 'x', '/', '>'});
    runtime.nodes = {0x3000u};
    runtime.ends = {0x2008u};
    runtime.words[0x3014u] = 0x5000u;
    runtime.words[0x1038u] = 4u;
    const std::array<std::uint32_t, 2> position{7u, 9u};
    Check(pal4::XmlDocumentContentParser::Parse(
              0x1000u, 0x2000u, &position, 0, runtime) == 0u &&
              runtime.bytes[0x1044u] == 1u &&
              runtime.words[0x1004u] == 7u && runtime.words[0x1008u] == 9u &&
              runtime.words[0x1018u] == 0x3000u &&
              runtime.words[0x3010u] == 0x1000u &&
              runtime.bytes[0x102cu] == 0u &&
              runtime.allocations == std::vector<std::uint32_t>{80u} &&
              runtime.parses.empty() &&
              runtime.words[0x3004u] == 7u &&
              runtime.words[0x3008u] == 10u,
          "initial BOM selects UTF-8 and node is linked with copied cursor");
}

void TestDeclarationEncoding() {
    Runtime runtime;
    const std::string source = "<?xml encoding='uTf-8-x'?> <r/>";
    runtime.SetText(0x2000u,
                    std::vector<std::uint8_t>(source.begin(), source.end()));
    runtime.nodes = {0x3000u, 0x6000u};
    runtime.declaration = 0x3000u;
    Check(pal4::XmlDocumentContentParser::Parse(
              0x1000u, 0x2000u, nullptr, 0, runtime) == 0u &&
              runtime.parses.empty() &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{56u, 36u, 8u, 80u} &&
              runtime.words[0x1018u] == 0x3000u &&
              runtime.words[0x101cu] == 0x6000u &&
              runtime.words[0x3028u] == 0x6000u &&
              runtime.ReadByte(runtime.ReadWord(0x3030u) + 8u) == 'u',
          "UTF-8 declaration changes mode before parsing next node");
}

void TestGenericTagIntegration() {
    Runtime runtime;
    runtime.SetText(0x2000u, {'<', '!', 'x', '>'});
    runtime.nodes = {0x3000u};
    Check(pal4::XmlDocumentContentParser::Parse(
              0x1000u, 0x2000u, nullptr, 0, runtime) == 0u &&
              runtime.allocations == std::vector<std::uint32_t>{44u} &&
              runtime.parses.empty() &&
              runtime.words[0x3014u] == 3u &&
              runtime.words[0x1018u] == 0x3000u &&
              runtime.assigned_destination == 0x3020u &&
              runtime.assigned_length == 2u,
          "document dispatches generic nodes through reconstructed parser");
}

void TestCommentIntegration() {
    Runtime runtime;
    runtime.SetText(0x2000u, {'<', '!', '-', '-', 'x', '-', '-', '>'});
    runtime.nodes = {0x3000u};
    Check(pal4::XmlDocumentContentParser::Parse(
              0x1000u, 0x2000u, nullptr, 0, runtime) == 0u &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{44u, 2u} &&
              runtime.parses.empty() &&
              runtime.words[0x3014u] == 2u &&
              runtime.words[0x1018u] == 0x3000u &&
              runtime.assigned_destination == 0x3020u &&
              runtime.assigned_length == 1u,
          "document dispatches comments through reconstructed parser");
}

void TestCDataIntegration() {
    Runtime runtime;
    runtime.SetText(0x2000u,
                    {'<', '!', '[', 'C', 'D', 'A', 'T', 'A', '[', 'x', ']', ']', '>'});
    runtime.nodes = {0x3000u};
    Check(pal4::XmlDocumentContentParser::Parse(
              0x1000u, 0x2000u, nullptr, 0, runtime) == 0u &&
              runtime.allocations == std::vector<std::uint32_t>{48u, 2u} &&
              runtime.parses.empty() &&
              runtime.words[0x3014u] == 4u &&
              runtime.bytes[0x302cu] == 1u &&
              runtime.words[0x1018u] == 0x3000u &&
              runtime.assigned_destination == 0x3020u &&
              runtime.assigned_length == 1u,
          "document dispatches CDATA through reconstructed parser");
}

}  // namespace

int main() {
    TestEmptyDocument();
    TestBomAndChildInsertion();
    TestDeclarationEncoding();
    TestGenericTagIntegration();
    TestCommentIntegration();
    TestCDataIntegration();
}
