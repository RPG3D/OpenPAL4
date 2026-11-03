#include "pal4/xml_attribute_parser.h"

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
        words[0x3010u] = 0x1000u;
        words[0x8c20a0u + 7u * 4u] = 0x7000u;
        SetText(0x7000u, "bad attribute");
    }
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> assigned;
    std::vector<std::uint32_t> frees;
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
    std::uint32_t CallVirtual(std::uint32_t, std::uint32_t) override { return 0u; }
    std::uint32_t AttributeSetup(std::uint32_t, std::uint32_t,
                                 std::uint32_t, std::uint32_t,
                                 std::uint32_t) override { return 0u; }
    std::uint32_t Allocate(std::uint32_t) override {
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

void TestQuotedValues() {
    Runtime single;
    single.SetText(0x2000u, "name='a&amp;b' rest");
    Check(pal4::XmlAttributeParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, single) == 0x200eu &&
              single.assigned[0x3014u] == "name" &&
              single.assigned[0x3018u] == "a&b" &&
              single.frees == std::vector<std::uint32_t>{0x8000u},
          "single-quoted value decodes entities and consumes quote");
    Runtime dual;
    dual.SetText(0x2000u, "q=\"x\"/>");
    Check(pal4::XmlAttributeParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, dual) == 0x2005u &&
              dual.assigned[0x3014u] == "q" &&
              dual.assigned[0x3018u] == "x",
          "double-quoted value consumes quote");
    Runtime empty;
    empty.SetText(0x2000u, "q='' />");
    Check(pal4::XmlAttributeParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, empty) == 0x2004u &&
              empty.assigned[0x3018u].empty() && empty.frees.empty(),
          "empty quoted value does not allocate scratch storage");
}

void TestUnquotedAndPosition() {
    Runtime runtime;
    runtime.SetText(0x2000u, " \n id = value/>");
    std::array<std::uint32_t, 4> position{2u, 4u, 0x2000u, 4u};
    Check(pal4::XmlAttributeParser::Parse(
              0x3000u, 0x2000u, &position, 0, runtime) == 0x200du &&
              runtime.assigned[0x3014u] == "id" &&
              runtime.assigned[0x3018u] == "value" &&
              runtime.words[0x3004u] == 3u &&
              runtime.words[0x3008u] == 1u &&
              position[2] == 0x2003u,
          "unquoted value stops at slash and position tracks leading space");
    Runtime bom;
    bom.bytes[0x2000u] = 0xefu;
    bom.bytes[0x2001u] = 0xbbu;
    bom.bytes[0x2002u] = 0xbfu;
    bom.SetText(0x2003u, "a=b>");
    Check(pal4::XmlAttributeParser::Parse(
              0x3000u, 0x2000u, nullptr, 1, bom) == 0x2006u &&
              bom.assigned[0x3018u] == "b",
          "UTF-8 BOM is skipped before name");
}

void TestErrorsAndTerminator() {
    for (const char* text : {"bad value", "a=", "a=b'c", "a=b\"c", "a"}) {
        Runtime runtime;
        runtime.SetText(0x2000u, text);
        Check(pal4::XmlAttributeParser::Parse(
                  0x3000u, 0x2000u, nullptr, 0, runtime) == 0u &&
                  runtime.words[0x1030u] == 7u,
              "malformed attribute reports error seven");
    }
    Runtime empty;
    empty.SetText(0x2000u, "");
    Check(pal4::XmlAttributeParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, empty) == 0u &&
              empty.words[0x1030u] == 0u,
          "initial NUL exits without an error");
    Runtime missing_quote;
    missing_quote.SetText(0x2000u, "a='x");
    Check(pal4::XmlAttributeParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, missing_quote) == 0x2005u &&
              missing_quote.assigned[0x3018u] == "x",
          "missing closing quote uses string parser NUL return");
    Runtime failed;
    failed.SetText(0x2000u, "a='x'");
    failed.allocation_fails = true;
    Check(pal4::XmlAttributeParser::Parse(
              0x3000u, 0x2000u, nullptr, 0, failed) == 0u,
          "decoded value allocation failure returns null");
}

}  // namespace

int main() {
    TestQuotedValues();
    TestUnquotedAndPosition();
    TestErrorsAndTerminator();
}
