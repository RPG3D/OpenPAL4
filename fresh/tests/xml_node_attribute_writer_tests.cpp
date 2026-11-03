#include "pal4/xml_node_attribute_writer.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IXmlNodeAttributeWriterRuntime,
                       pal4::IConfigValueMemory,
                       pal4::IStringAssignRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool fail_add{};
    bool fail_copy{};
    std::uint8_t reference{};

    Runtime() { Configure(); }
    void Configure() {
        words[0x1018u] = 0x3000u;
        words[0x3004u] = 0x3100u;
        words[0x3100u] = 0x4000u;
        words[0x3108u] = 0x4000u;
        words[0x3110u] = 0x5000u;
        words[0x3114u] = 3u;
        words[pal4::ConfigValueLookup::kNilNodeGlobal] = 0x4000u;
        for (std::uint32_t i = 0; i < 3u; ++i) {
            bytes[0x1100u + i] = "foo"[i];
            bytes[0x5000u + i] = "foo"[i];
        }
        bytes[0x1103u] = 0u;
        bytes[0x5003u] = 0u;
        bytes[0x2000u] = reference;
    }
    pal4::IConfigValueMemory& Memory() override { return *this; }
    pal4::IStringAssignRuntime& Strings() override { return *this; }
    std::uint32_t StringLength(std::uint32_t address) override {
        Check(address == 0x1100u || address == 0x1200u,
              "key source address");
        return address == 0x1200u ? 0u : 3u;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("write-byte:" + std::to_string(address));
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x2000u, "temporary key reference header");
        calls.emplace_back("free-key");
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t count) override {
        Copy(destination, source, count);
    }
    void SetErrorFlag(pal4::ConfigStringState& text,
                      std::uint8_t) override { text = {}; }
    void Reallocate(pal4::ConfigStringState& text,
                    std::uint32_t length) override {
        text.data = 0x2001u;
        text.capacity = length | 0x1fu;
    }
    void LengthError() override { throw std::length_error("length"); }
    std::uint32_t EmptyStringAddress() override { return 0x7000u; }
    std::int32_t CompareRange(std::uint32_t,
                              std::uint32_t, std::uint32_t,
                              std::uint32_t, std::uint32_t) override {
        return 0;
    }

    void ThrowRange() override { throw std::out_of_range("range"); }
    void ThrowLength() override { throw std::length_error("length"); }
    void DetachAndReset(pal4::StringBufferState& text) override {
        text = {};
    }
    bool AllocateText(pal4::StringBufferState&,
                      std::uint32_t, std::uint8_t) override {
        Check(false, "writer composes StringBuffer::AllocateText");
        return false;
    }
    void SetTextSize(pal4::StringBufferState& text,
                     std::uint32_t length) override {
        text.length = length;
    }
    void SetErrorFlag(pal4::StringBufferState& text,
                      std::uint8_t) override { text = {}; }
    void Reallocate(pal4::StringBufferState& text,
                    std::uint32_t length) override {
        text.data = 0x2001u;
        text.capacity = length | 0x1fu;
        calls.emplace_back("allocate-key");
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override {
        calls.emplace_back("copy-key");
        if (fail_copy) throw std::runtime_error("copy");
        for (std::uint32_t i = 0u; i < length; ++i)
            bytes[destination + i] = bytes.at(source + i);
    }

    std::uint8_t SetInt(std::uint32_t field,
                        std::uint32_t value) override {
        Check(field == 0x311cu && value == 0x6000u,
              "existing int attribute field and value pointer");
        calls.emplace_back("set-int");
        return 0x3cu;
    }
    std::uint8_t SetFloat(std::uint32_t field,
                          std::uint32_t value) override {
        Check(field == 0x311cu && value == 0x6000u,
              "existing float attribute field and value pointer");
        calls.emplace_back("set-float");
        return 0x7bu;
    }
    std::uint8_t SetString(std::uint32_t field,
                           std::uint32_t value) override {
        Check(field == 0x311cu && value == 0x6000u,
              "existing string attribute field and value pointer");
        calls.emplace_back("set-string");
        return 0x4du;
    }
    std::uint32_t InitializeIntAttribute(
        pal4::XmlAttributeWriterTemporary&, std::uint32_t key,
        std::uint32_t value) override {
        Check(key == 0x1100u || key == 0x1200u,
              "new integer attribute key");
        Check(value == 0x6000u, "new integer attribute value");
        calls.emplace_back("new-int");
        return 0xa100u;
    }
    std::uint32_t InitializeFloatAttribute(
        pal4::XmlAttributeWriterTemporary&, std::uint32_t key,
        std::uint32_t value) override {
        Check(key == 0x1100u && value == 0x6000u,
              "new float attribute parameters");
        calls.emplace_back("new-float");
        return 0xa200u;
    }
    std::uint32_t InitializeStringAttribute(
        pal4::XmlAttributeWriterTemporary&, std::uint32_t key,
        std::uint32_t value) override {
        Check(key == 0x1100u && value == 0x6000u,
              "new string attribute parameters");
        calls.emplace_back("new-string");
        return 0xa300u;
    }
    std::uint8_t AddAttribute(std::uint32_t document,
                              std::uint32_t attribute) override {
        Check(document == 0x1000u &&
                  (attribute == 0xa100u || attribute == 0xa200u ||
                   attribute == 0xa300u),
              "node insertion arguments");
        calls.emplace_back("add");
        if (fail_add) throw std::runtime_error("add");
        return 0x5au;
    }
    void DestroyAttribute(pal4::XmlAttributeWriterTemporary&) override {
        calls.emplace_back("destroy-attribute");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::XmlNodeAttributeWriter::WriteInt(
              0x1000u, 0x1100u, 0x6000u, runtime) == 0x3cu &&
              runtime.calls == std::vector<std::string>{
                  "allocate-key", "copy-key", "write-byte:8196",
                  "free-key", "set-int"},
          "existing int attribute composes key lookup and releases key");

    runtime.calls.clear();
    Check(pal4::XmlNodeAttributeWriter::WriteFloat(
              0x1000u, 0x1100u, 0x6000u, runtime) == 0x7bu &&
              runtime.calls.back() == "set-float",
          "existing float attribute uses distinct setter");

    runtime.calls.clear();
    Check(pal4::XmlNodeAttributeWriter::WriteString(
              0x1000u, 0x1100u, 0x6000u, runtime) == 0x4du &&
              runtime.calls.back() == "set-string",
          "existing string attribute uses string setter");

    runtime.calls.clear();
    runtime.bytes[0x5000u] = 'z';
    Check(pal4::XmlNodeAttributeWriter::WriteInt(
              0x1000u, 0x1100u, 0x6000u, runtime) == 0x5au &&
              runtime.calls == std::vector<std::string>{
                  "allocate-key", "copy-key", "write-byte:8196",
                  "free-key", "new-int", "add", "destroy-attribute"},
          "lower-bound key greater than query inserts int attribute");

    runtime.calls.clear();
    Check(pal4::XmlNodeAttributeWriter::WriteFloat(
              0x1000u, 0x1100u, 0x6000u, runtime) == 0x5au &&
              runtime.calls[runtime.calls.size() - 3] == "new-float",
          "lower-bound miss inserts float attribute");

    runtime.calls.clear();
    Check(pal4::XmlNodeAttributeWriter::WriteString(
              0x1000u, 0x1100u, 0x6000u, runtime) == 0x5au &&
              runtime.calls[runtime.calls.size() - 3] == "new-string",
          "lower-bound miss inserts string attribute");

    runtime.calls.clear();
    runtime.fail_add = true;
    try {
        (void)pal4::XmlNodeAttributeWriter::WriteInt(
            0x1000u, 0x1100u, 0x6000u, runtime);
        Check(false, "node insertion should throw");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "destroy-attribute",
              "insertion exception destroys temporary attribute");
    }

    runtime.calls.clear();
    runtime.fail_add = false;
    runtime.fail_copy = true;
    try {
        (void)pal4::XmlNodeAttributeWriter::WriteInt(
            0x1000u, 0x1100u, 0x6000u, runtime);
        Check(false, "key copy should throw");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "free-key",
              "copy exception releases allocated key");
    }

    runtime.calls.clear();
    runtime.fail_copy = false;
    runtime.words[0x3004u] = 0x4000u;
    Check(pal4::XmlNodeAttributeWriter::WriteInt(
              0x1000u, 0x1200u, 0x6000u, runtime) == 0x5au &&
              runtime.calls == std::vector<std::string>{
                  "new-int", "add", "destroy-attribute"},
          "empty key and empty tree insert without string allocation");
}
