#include "pal4/xml_node_factory.h"

#include "pal4/compact_string_buffer.h"

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

struct Runtime final : pal4::IXmlNodeFactoryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocations;
    std::uint32_t next_allocation = 0x3000u;
    std::uint32_t ancestor = 0u;
    std::uint32_t element_calls = 0u;
    std::uint32_t string_destination = 0u;

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
    std::uint32_t StringLength(const std::uint32_t address) override {
        std::uint32_t length = 0u;
        while (ReadByte(address + length) != 0u) ++length;
        return length;
    }
    std::uint32_t AssignString(const std::uint32_t destination,
                               std::uint32_t, std::uint32_t) override {
        string_destination = destination;
        return destination;
    }
    std::int32_t CompareText(std::uint32_t, std::uint32_t) override { return 0; }
    std::uint32_t CallVirtual(const std::uint32_t node,
                              const std::uint32_t slot) override {
        Check(slot == 16u, "ancestor lookup uses virtual slot 16");
        return node == 0x1000u ? ancestor : 0u;
    }
    std::uint32_t AttributeSetup(std::uint32_t, std::uint32_t,
                                 std::uint32_t, std::uint32_t,
                                 std::uint32_t) override { return 0u; }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        allocations.push_back(bytes);
        return next_allocation;
    }
    std::uint32_t ConstructElement(const std::uint32_t address) override {
        ++element_calls;
        return pal4::XmlParserLifecycle::Initialize(address, 1u, *this);
    }
    void SetText(const std::uint32_t address, const std::string& text) {
        for (std::size_t i = 0u; i < text.size(); ++i)
            bytes[address + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(text[i]);
        bytes[address + static_cast<std::uint32_t>(text.size())] = 0u;
    }
};

void CheckType(const std::string& text, const int mode,
               const std::uint32_t size, const std::uint32_t kind,
               const std::uint32_t vtable) {
    Runtime runtime;
    runtime.SetText(0x2000u, text);
    Check(pal4::XmlNodeFactory::Create(
              0x1000u, 0x2000u, mode, runtime) == 0x3000u &&
              runtime.allocations == std::vector<std::uint32_t>{size} &&
              runtime.words[0x3014u] == kind &&
              runtime.words[0x3000u] == vtable &&
              runtime.words[0x3010u] == 0x1000u,
          "factory branch initializes expected node size, kind and owner");
}

void TestTypes() {
    CheckType("<?XML version='1.0'?>", 0, 56u, 5u, 0x849ae8u);
    CheckType("<!-- comment -->", 0, 44u, 2u, 0x849a58u);
    CheckType("<!DOCTYPE root>", 0, 44u, 3u, 0x849b30u);
    CheckType("<?other?>", 0, 44u, 3u, 0x849b30u);
    CheckType("<9bad>", 0, 44u, 3u, 0x849b30u);
    Runtime declaration;
    declaration.SetText(0x2000u, "<?xml?>");
    Check(pal4::XmlNodeFactory::Create(
              0x1000u, 0x2000u, 0, declaration) == 0x3000u &&
              declaration.words[0x302cu] == pal4::CompactStringBuffer::kEmptyBlock &&
              declaration.words[0x3030u] == pal4::CompactStringBuffer::kEmptyBlock &&
              declaration.words[0x3034u] == pal4::CompactStringBuffer::kEmptyBlock,
          "declaration owns three initially empty compact strings");
    Runtime cdata;
    cdata.SetText(0x2000u, "<![CDATA[hello]]>");
    Check(pal4::XmlNodeFactory::Create(
              0x1000u, 0x2000u, 0, cdata) == 0x3000u &&
              cdata.allocations == std::vector<std::uint32_t>{48u} &&
              cdata.words[0x3014u] == 4u &&
              cdata.words[0x3000u] == 0x849aa0u &&
              cdata.string_destination == 0x3020u &&
              cdata.bytes[0x302cu] == 1u,
          "CDATA initializes text and its byte flag");
    Runtime element;
    element.SetText(0x2000u, "<_root/>");
    Check(pal4::XmlNodeFactory::Create(
              0x1000u, 0x2000u, 0, element) == 0x3000u &&
              element.allocations == std::vector<std::uint32_t>{80u} &&
              element.element_calls == 1u && element.words[0x3010u] == 0x1000u,
          "element path uses the injected 80-byte constructor");
}

void TestWhitespaceAndFailures() {
    Runtime runtime;
    runtime.SetText(0x2000u, " \tplain");
    Check(pal4::XmlNodeFactory::Create(
              0x1000u, 0x2000u, 0, runtime) == 0u &&
              runtime.allocations.empty(),
          "non-tag text is rejected before allocation");
    runtime.SetText(0x2000u, " ");
    Check(pal4::XmlNodeFactory::Create(
              0x1000u, 0x2000u, 0, runtime) == 0u &&
              runtime.allocations.empty(),
          "whitespace-only input is rejected");
    runtime.SetText(0x2000u, "<root>");
    runtime.next_allocation = 0u;
    runtime.ancestor = 0x4000u;
    runtime.words[0x8c20a0u + 3u * 4u] = 0x5000u;
    runtime.SetText(0x5000u, "Memory allocation failed.");
    Check(pal4::XmlNodeFactory::Create(
              0x1000u, 0x2000u, 0, runtime) == 0u &&
              runtime.bytes[0x402cu] == 1u &&
              runtime.words[0x4030u] == 3u,
          "allocation failure reports code three on ancestor");
    Runtime marked;
    marked.SetText(0x2000u, "<![CDATA[x]]>");
    marked.bytes[0x1ffdu] = 0xefu;
    marked.bytes[0x1ffeu] = 0xbbu;
    marked.bytes[0x1fffu] = 0xbfu;
    Check(pal4::XmlNodeFactory::Create(
              0x1000u, 0x1ffdu, 1, marked) == 0x3000u &&
              marked.allocations == std::vector<std::uint32_t>{48u},
          "UTF-8 mode skips leading BOM before classifying node");
}

}  // namespace

int main() {
    TestTypes();
    TestWhitespaceAndFailures();
}
