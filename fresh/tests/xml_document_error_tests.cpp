#include "pal4/xml_document_error.h"

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

struct Runtime final : pal4::IXmlParserNodeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> assigned;
    std::string last_message;

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
                               const std::uint32_t source,
                               const std::uint32_t length) override {
        assigned.push_back(destination);
        last_message.clear();
        for (std::uint32_t i = 0u; i < length; ++i)
            last_message.push_back(static_cast<char>(ReadByte(source + i)));
        return destination;
    }
    std::int32_t CompareText(std::uint32_t, std::uint32_t) override { return 0; }
    std::uint32_t CallVirtual(std::uint32_t, std::uint32_t) override { return 0u; }
    std::uint32_t AttributeSetup(std::uint32_t, std::uint32_t,
                                 std::uint32_t, std::uint32_t,
                                 std::uint32_t) override { return 0u; }
    void SetText(const std::uint32_t address, const std::string& text) {
        for (std::size_t i = 0u; i < text.size(); ++i)
            bytes[address + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(text[i]);
        bytes[address + static_cast<std::uint32_t>(text.size())] = 0u;
    }
};

void TestFirstErrorOnly() {
    Runtime runtime;
    runtime.words[0x8c20a0u + 13u * 4u] = 0x5000u;
    runtime.SetText(0x5000u, "Error document empty.");
    Check(pal4::XmlDocumentError::Set(
              0x1000u, 13u, 0u, 0u, 1, runtime) == 0u &&
              runtime.bytes[0x102cu] == 1u &&
              runtime.words[0x1030u] == 13u &&
              runtime.words[0x103cu] == 0xffffffffu &&
              runtime.words[0x1040u] == 0xffffffffu &&
              runtime.assigned == std::vector<std::uint32_t>{0x1034u} &&
              runtime.last_message == "Error document empty.",
          "first error stores code, message and absent position");
    runtime.bytes[0x102cu] = 7u;
    Check(pal4::XmlDocumentError::Set(
              0x1000u, 4u, 0x2000u, 0u, 0, runtime) == 7u &&
              runtime.words[0x1030u] == 13u && runtime.assigned.size() == 1u,
          "existing error flag prevents any replacement");
}

void TestPositionAdvance() {
    Runtime runtime;
    runtime.words[0x8c20a0u + 4u * 4u] = 0x5000u;
    runtime.SetText(0x5000u, "Element error");
    runtime.SetText(0x2000u, "a\t\nb");
    runtime.words[0x3000u] = 2u;
    runtime.words[0x3004u] = 0u;
    runtime.words[0x3008u] = 0x2000u;
    runtime.words[0x300cu] = 4u;
    Check(pal4::XmlDocumentError::Set(
              0x1000u, 4u, 0x2004u, 0x3000u, 0, runtime) == 1u &&
              runtime.words[0x3000u] == 3u &&
              runtime.words[0x3004u] == 1u &&
              runtime.words[0x3008u] == 0x2004u &&
              runtime.words[0x103cu] == 3u &&
              runtime.words[0x1040u] == 1u,
          "error advances cursor and records line and column");
    Runtime no_cursor;
    no_cursor.words[0x8c20a0u + 4u * 4u] = 0x5000u;
    no_cursor.SetText(0x5000u, "Element error");
    Check(pal4::XmlDocumentError::Set(
              0x1000u, 4u, 0x20aau, 0u, 1, no_cursor) == 0xaau &&
              no_cursor.words[0x103cu] == 0xffffffffu,
          "nonzero end without cursor leaves unknown position");
}

}  // namespace

int main() {
    TestFirstErrorOnly();
    TestPositionAdvance();
}
