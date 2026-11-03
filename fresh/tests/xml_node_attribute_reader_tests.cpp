#include "pal4/xml_node_attribute_reader.h"

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

struct TreeMemoryProbe final : pal4::IScriptAttributeTreeRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    bool fail_read{};
    std::uint32_t NilAddress() override { return 0u; }
    std::uint32_t EmptyStringAddress() override { return 0u; }
    std::uint32_t ReadDword(std::uint32_t address) override {
        if (fail_read) throw std::runtime_error("tree");
        return words.at(address);
    }
    std::int32_t CompareNodeKey(std::uint32_t node,
                                std::uint32_t query) override {
        Check(node == 0x810cu && query == 0x9000u,
              "tree node and temporary query");
        return 0;
    }
    std::int32_t CompareBytes(std::uint32_t query,
                              std::uint32_t node,
                              std::uint32_t length) override {
        Check(query == 0xa100u && node == 0xa000u && length == 7u,
              "string equality comparison");
        return 0;
    }
};

struct StringRuntime final : pal4::IStringAssignRuntime {
    std::vector<std::string>& calls;
    bool fail_copy{};
    explicit StringRuntime(std::vector<std::string>& log) : calls(log) {}
    void ThrowRange() override { throw std::out_of_range("range"); }
    void ThrowLength() override { throw std::length_error("length"); }
    void DetachAndReset(pal4::StringBufferState& state) override {
        state = {};
    }
    bool AllocateText(pal4::StringBufferState&,
                      std::uint32_t, std::uint8_t) override {
        Check(false, "reader composes StringBuffer::AllocateText");
        return false;
    }
    void SetTextSize(pal4::StringBufferState& state,
                     std::uint32_t length) override {
        state.length = length;
    }
    void SetErrorFlag(pal4::StringBufferState& state,
                      std::uint8_t) override { state = {}; }
    void Reallocate(pal4::StringBufferState& state,
                    std::uint32_t length) override {
        Check(length == 4u, "fallback length");
        state.data = 0xd001u;
        state.capacity = 31u;
        calls.emplace_back("allocate");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0xd000u, "fallback reference header");
        calls.emplace_back("reference");
        return 0u;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0xd005u && value == 0u,
              "fallback terminator");
        calls.emplace_back("terminator");
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override {
        Check(destination == 0xd001u && source == 0xc000u && length == 4u,
              "fallback copy parameters");
        calls.emplace_back("copy");
        if (fail_copy) throw std::runtime_error("copy");
    }
    void Free(std::uint32_t address) override {
        Check(address == 0xd000u, "fallback allocation release");
        calls.emplace_back("free");
    }
};

struct Runtime final : pal4::IXmlNodeAttributeReaderRuntime {
    std::vector<std::string> calls;
    TreeMemoryProbe tree;
    StringRuntime strings{calls};
    Runtime() {
        tree.words[0x6018u] = 0x8000u;
        tree.words[0x8004u] = 0x8100u;
        tree.words[0x8100u] = 0u;
        tree.words[0x8110u] = 0xa000u;
        tree.words[0x8114u] = 7u;
        tree.words[0x9004u] = 0xa100u;
        tree.words[0x9008u] = 7u;
    }
    std::uint32_t CreateQuery(std::uint32_t key,
                              std::uint8_t incoming) override {
        Check(key == 0xb000u && incoming == 0u,
              "query key and fallback low byte");
        calls.emplace_back("create-query");
        return 0x9000u;
    }
    void DestroyQuery(std::uint32_t query) override {
        Check(query == 0x9000u, "query release address");
        calls.emplace_back("destroy-query");
    }
    pal4::IScriptAttributeTreeRuntime& TreeRuntime() override {
        return tree;
    }
    pal4::IStringAssignRuntime& Strings() override { return strings; }
    void AttributeToString(std::uint32_t field,
                           pal4::ConfigStringState& output,
                           std::uint32_t fallback) override {
        Check(field == 0x811cu && fallback == 0xc000u,
              "existing attribute conversion parameters");
        output.data = 0xe000u;
        output.size = 3u;
        calls.emplace_back("to-string");
    }
    std::uint32_t StringLength(std::uint32_t address) override {
        Check(address == 0xc000u || address == 0xc100u,
              "default C string address");
        calls.emplace_back("length");
        return address == 0xc100u ? 0u : 4u;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::ConfigStringState output{};
    Check(&pal4::XmlNodeAttributeReader::ReadString(
              0x6000u, output, 0xb000u, 0xc000u, runtime) == &output &&
              output.data == 0xe000u && output.size == 3u &&
              runtime.calls == std::vector<std::string>{
                  "create-query", "destroy-query", "to-string"},
          "found attribute converts into caller-owned output");

    runtime.calls.clear();
    runtime.tree.words[0x8004u] = 0u;
    Check(&pal4::XmlNodeAttributeReader::ReadString(
              0x6000u, output, 0xb000u, 0xc000u, runtime) == &output &&
              output.data == 0xd001u && output.size == 4u &&
              output.capacity == 31u &&
              runtime.calls == std::vector<std::string>{
                  "create-query", "destroy-query", "length",
                  "allocate", "copy", "terminator"},
          "missing attribute copies default string");

    runtime.calls.clear();
    (void)pal4::XmlNodeAttributeReader::ReadString(
        0x6000u, output, 0xb000u, 0xc100u, runtime);
    Check(output.data == 0u && output.size == 0u &&
              runtime.calls == std::vector<std::string>{
                  "create-query", "destroy-query", "length"},
          "empty default leaves a null string buffer");

    runtime.calls.clear();
    runtime.tree.fail_read = true;
    try {
        (void)pal4::XmlNodeAttributeReader::ReadString(
            0x6000u, output, 0xb000u, 0xc000u, runtime);
        Check(false, "tree read should throw");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "create-query", "destroy-query"},
              "query cleanup after lookup failure");
    }

    runtime.calls.clear();
    runtime.tree.fail_read = false;
    runtime.strings.fail_copy = true;
    try {
        (void)pal4::XmlNodeAttributeReader::ReadString(
            0x6000u, output, 0xb000u, 0xc000u, runtime);
        Check(false, "fallback copy should throw");
    } catch (const std::runtime_error&) {
        Check(output.data == 0u &&
                  runtime.calls[runtime.calls.size() - 2] == "reference" &&
                  runtime.calls.back() == "free",
              "fallback copy failure releases allocated string");
    }
}
