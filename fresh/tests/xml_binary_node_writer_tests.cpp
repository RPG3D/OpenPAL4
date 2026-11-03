#include "pal4/xml_binary_node_writer.h"

#include <bit>
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

struct Attribute {
    std::uint32_t type{};
    std::string name;
    std::uint8_t character{};
    std::int32_t integer{};
    double floating{};
    std::string text;
    std::vector<std::uint8_t> raw;
};

struct Node {
    std::string name;
    std::vector<std::uint32_t> attributes;
    std::vector<std::uint32_t> children;
    std::int32_t attribute_count_override{-1};
    std::int32_t child_count_override{-1};
};

struct Runtime final : pal4::IXmlBinaryNodeWriteRuntime {
    std::map<std::uint32_t, Node> nodes;
    std::map<std::uint32_t, Attribute> attributes;
    std::vector<std::uint8_t> bytes;
    std::vector<std::uint32_t> write_sizes;
    std::vector<std::uint32_t> visited_nodes;
    std::vector<std::uint32_t> visited_attributes;

    void Write(const std::uint32_t stream, const void* data,
               const std::uint32_t size) override {
        Check(stream == 0x900u, "all writes use caller stream");
        write_sizes.push_back(size);
        const auto* source = static_cast<const std::uint8_t*>(data);
        bytes.insert(bytes.end(), source, source + size);
    }
    [[nodiscard]] std::string NodeName(const std::uint32_t node) override {
        visited_nodes.push_back(node);
        return nodes.at(node).name;
    }
    [[nodiscard]] std::int32_t AttributeCount(
        const std::uint32_t node) override {
        const auto& value = nodes.at(node);
        return value.attribute_count_override >= 0
                   ? value.attribute_count_override
                   : static_cast<std::int32_t>(value.attributes.size());
    }
    [[nodiscard]] std::uint32_t AttributeAt(
        const std::uint32_t node, const std::int32_t index) override {
        const auto attribute = nodes.at(node).attributes.at(index);
        visited_attributes.push_back(attribute);
        return attribute;
    }
    [[nodiscard]] std::uint32_t AttributeType(
        const std::uint32_t attribute) override {
        return attributes.at(attribute).type;
    }
    [[nodiscard]] std::string AttributeName(
        const std::uint32_t attribute) override {
        return attributes.at(attribute).name;
    }
    [[nodiscard]] std::uint8_t AttributeChar(
        const std::uint32_t attribute) override {
        return attributes.at(attribute).character;
    }
    [[nodiscard]] std::int32_t AttributeInt(
        const std::uint32_t attribute) override {
        return attributes.at(attribute).integer;
    }
    [[nodiscard]] double AttributeFloat(
        const std::uint32_t attribute) override {
        return attributes.at(attribute).floating;
    }
    [[nodiscard]] std::string AttributeString(
        const std::uint32_t attribute) override {
        return attributes.at(attribute).text;
    }
    [[nodiscard]] std::vector<std::uint8_t> AttributeRaw(
        const std::uint32_t attribute) override {
        return attributes.at(attribute).raw;
    }
    [[nodiscard]] std::int32_t ChildCount(
        const std::uint32_t node) override {
        const auto& value = nodes.at(node);
        return value.child_count_override >= 0
                   ? value.child_count_override
                   : static_cast<std::int32_t>(value.children.size());
    }
    [[nodiscard]] std::uint32_t ChildAt(
        const std::uint32_t node, const std::int32_t index) override {
        return nodes.at(node).children.at(index);
    }
};

struct RecordMemory final : pal4::IXmlDocumentRecordRuntime,
                            pal4::IXmlAttributeRecordRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> successors;

    [[nodiscard]] std::uint32_t AllocateMemory(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t FreeMemory(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t AllocateSharedNil() override { return 0u; }
    [[nodiscard]] std::uint32_t FreeSharedNil(std::uint32_t) override { return 0u; }
    void DocumentReferenceAdded() noexcept override {}
    void DocumentReferenceRemoved() noexcept override {}
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void SetErrorFlag(std::uint32_t, std::uint8_t) override {}
    void Reallocate(std::uint32_t, std::uint32_t) override {}
    void AddChild(std::uint32_t, std::uint32_t) override {}
    void DeleteNode(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t GetNextNode(std::uint32_t node) override {
        return successors.at(node);
    }
    void RemoveNode(std::uint32_t, std::uint32_t) override {}
    void GetFirstElement(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t EmptyTextAddress() override { return 0u; }
    [[nodiscard]] std::uint32_t CopyAttributeToTemporary(std::uint32_t,
        std::span<std::uint32_t, 7>) override {
        return 0u;
    }
    void DestroyTemporaryAttribute(std::uint32_t) override {}
    void CleanupNode(std::uint32_t) override {}
    void FreeAttributeValue(std::uint32_t) override {}
    void ClearAttributeTree(std::uint32_t, std::uint32_t,
                            std::uint32_t) override {}
    [[nodiscard]] std::uint8_t AllocateTextBuffer(
        std::uint32_t, std::uint32_t) override { return 0u; }
    void ReallocateTextIfNeeded(std::uint32_t) override {}
    void DetachTextBuffer(std::uint32_t) override {}
    [[nodiscard]] std::uint8_t SetTextSize(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t FindAttributeNode(
        std::uint32_t, std::string_view) override { return 0u; }
    [[nodiscard]] std::uint32_t InsertAttributeNode(
        std::uint32_t, std::string_view, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t FindAttributeNodeForAddition(
        std::uint32_t, std::string_view) override { return 0u; }
    void DestroyEmbeddedAttribute(std::uint32_t) override {}
    [[nodiscard]] std::int32_t CompareAttributeRange(
        std::uint32_t, std::uint32_t) override { return 0; }
    [[nodiscard]] std::int32_t CompareAttributeText(
        std::uint32_t, std::uint32_t) override { return 0; }
    void CopyEmbeddedAttribute(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] bool AllocateText(std::uint32_t, std::uint32_t,
                                    bool) override { return false; }
    void AssignName(std::uint32_t, std::uint32_t, std::uint32_t,
                    std::uint32_t) override {}
    [[nodiscard]] std::uint32_t AllocateRaw(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t FreeBlock(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t FreeRaw(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::int32_t FloatToInt(std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint8_t ScanChar(
        std::uint32_t, std::uint8_t value) override { return value; }
    [[nodiscard]] std::int32_t ScanInt(
        std::uint32_t, std::int32_t value) override { return value; }
    [[nodiscard]] std::uint32_t ScanFloatBits(
        std::uint32_t, std::uint32_t value) override { return value; }
    [[nodiscard]] std::string FormatFloat(std::uint32_t) override { return {}; }
    [[nodiscard]] std::int32_t ParseInt(std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t ParseFloatBits(std::uint32_t) override {
        return 0u;
    }
    void CString(const std::uint32_t address, const std::string& text) {
        for (std::uint32_t index = 0u; index < text.size(); ++index)
            bytes[address + index] = static_cast<std::uint8_t>(text[index]);
        bytes[address + static_cast<std::uint32_t>(text.size())] = 0u;
    }
};

struct Output final : pal4::IXmlBinaryOutput {
    std::vector<std::uint8_t> bytes;
    void Write(const std::uint32_t stream, const void* data,
               const std::uint32_t size) override {
        Check(stream == 0x900u, "record output uses caller stream");
        const auto* source = static_cast<const std::uint8_t*>(data);
        bytes.insert(bytes.end(), source, source + size);
    }
};

void Word(std::vector<std::uint8_t>& bytes, const std::uint32_t word) {
    for (unsigned shift = 0u; shift < 32u; shift += 8u)
        bytes.push_back(static_cast<std::uint8_t>(word >> shift));
}

void Text(std::vector<std::uint8_t>& bytes, const std::string& text) {
    bytes.insert(bytes.end(), text.begin(), text.end());
}

void TestHeaderAndAllAttributeTypes() {
    Runtime runtime;
    runtime.nodes[1u] = Node{"root", {10u, 11u, 12u, 13u, 14u, 15u},
                             {2u, 3u}};
    runtime.nodes[2u] = Node{"left"};
    runtime.nodes[3u] = Node{""};
    runtime.attributes[10u] = Attribute{0u, "c", 0x80u};
    runtime.attributes[11u] = Attribute{1u, "i", 0u, -42};
    runtime.attributes[12u] = Attribute{2u, "f", 0u, 0, 1.25};
    runtime.attributes[13u] = Attribute{3u, "s", 0u, 0, 0.0, "hello"};
    runtime.attributes[14u] = Attribute{4u, "b", 0u, 0, 0.0, "",
                                        {0u, 0xffu, 7u}};
    runtime.attributes[15u] = Attribute{99u, "u"};

    Check(pal4::XmlBinaryNodeWriter::WriteConfig(
              1u, 0x900u, runtime) == 1u,
          "config writer returns recursive success");
    std::vector<std::uint8_t> expected;
    Word(expected, 1u); Word(expected, 1u);
    Word(expected, 4u); Text(expected, "root");
    Word(expected, 6u);
    Word(expected, 0u); Word(expected, 1u); Text(expected, "c");
    expected.push_back(0x80u);
    Word(expected, 1u); Word(expected, 1u); Text(expected, "i");
    Word(expected, static_cast<std::uint32_t>(-42));
    Word(expected, 2u); Word(expected, 1u); Text(expected, "f");
    Word(expected, std::bit_cast<std::uint32_t>(1.25f));
    Word(expected, 3u); Word(expected, 1u); Text(expected, "s");
    Word(expected, 5u); Text(expected, "hello");
    Word(expected, 4u); Word(expected, 1u); Text(expected, "b");
    Word(expected, 3u); expected.insert(expected.end(), {0u, 0xffu, 7u});
    Word(expected, 99u); Word(expected, 1u); Text(expected, "u");
    Word(expected, 2u);
    Word(expected, 4u); Text(expected, "left");
    Word(expected, 0u); Word(expected, 0u);
    Word(expected, 0u); Word(expected, 0u); Word(expected, 0u);
    Check(runtime.bytes == expected &&
              runtime.visited_nodes == (std::vector<std::uint32_t>{1u, 2u, 3u}) &&
              runtime.visited_attributes ==
                  (std::vector<std::uint32_t>{10u, 11u, 12u, 13u, 14u, 15u}),
          "header, values and recursive nodes match binary byte layout");
}

void TestNullAndEmptyValues() {
    Runtime runtime;
    runtime.nodes[1u] = Node{"", {10u, 11u}};
    runtime.attributes[10u] = Attribute{3u, "", 0u, 0, 0.0, ""};
    runtime.attributes[11u] = Attribute{4u, "raw"};
    Check(pal4::XmlBinaryNodeWriter::WriteConfig(
              0u, 0x900u, runtime) == 0u &&
              pal4::XmlBinaryNodeWriter::WriteConfig(
                  1u, 0u, runtime) == 0u && runtime.bytes.empty(),
          "null root or stream does not write a header");

    Check(pal4::XmlBinaryNodeWriter::WriteNode(
              1u, 0x900u, runtime) == 1u,
          "empty node writes successfully");
    std::vector<std::uint8_t> expected;
    Word(expected, 0u); Word(expected, 2u);
    Word(expected, 3u); Word(expected, 0u); Word(expected, 0u);
    Word(expected, 4u); Word(expected, 3u); Text(expected, "raw");
    Word(expected, 0u); Word(expected, 0u);
    Check(runtime.bytes == expected &&
              runtime.write_sizes == (std::vector<std::uint32_t>{
                  4u, 4u, 4u, 4u, 0u, 4u, 4u, 4u, 3u, 4u, 4u}),
          "empty name skips its payload but empty attribute name writes zero bytes");
}

void TestRecordAdapter() {
    RecordMemory memory;
    Output output;
    constexpr std::uint32_t root = 0x1000u;
    constexpr std::uint32_t child = 0x2000u;
    constexpr std::uint32_t first_attribute = 0x5000u + 28u;
    constexpr std::uint32_t second_attribute = 0x6000u + 28u;
    memory.words[root + 4u] = 0x3000u;
    memory.CString(0x3000u, "root");
    memory.words[root + 24u] = 0x4000u;
    memory.words[0x4000u] = 0x5000u;
    memory.successors[0x5000u] = 0x6000u;
    memory.words[root + 52u] = 2u;
    memory.words[root + 40u] = 0x7000u;
    memory.words[0x7000u] = child;
    memory.words[root + 56u] = 1u;
    memory.words[child + 4u] = 0x3100u;
    memory.CString(0x3100u, "child");
    memory.words[child + 52u] = 0u;
    memory.words[child + 56u] = 0u;
    memory.words[first_attribute + 4u] = 0x3200u;
    memory.CString(0x3200u, "c");
    memory.words[first_attribute + 16u] = 0u;
    memory.bytes[first_attribute + 24u] = 0x7fu;
    memory.words[second_attribute + 4u] = 0x3300u;
    memory.CString(0x3300u, "s");
    memory.words[second_attribute + 16u] = 3u;
    memory.words[second_attribute + 24u] = 0x8000u;
    memory.words[0x8000u + 4u] = 0x3400u;
    memory.CString(0x3400u, "text");

    pal4::XmlBinaryNodeRecordAdapter adapter(output, memory, memory);
    Check(adapter.AttributeAt(root, 0) == first_attribute &&
              adapter.AttributeAt(root, 1) == second_attribute &&
              adapter.ChildAt(root, 0) == child &&
              adapter.AttributeAt(root, -1) == 0u,
          "record adapter follows XML tree and child-vector layouts");
    Check(pal4::XmlBinaryNodeWriter::WriteConfig(root, 0x900u, adapter) == 1u,
          "record adapter supplies writer from address-based records");
    std::vector<std::uint8_t> expected;
    Word(expected, 1u); Word(expected, 1u);
    Word(expected, 4u); Text(expected, "root");
    Word(expected, 2u);
    Word(expected, 0u); Word(expected, 1u); Text(expected, "c");
    expected.push_back(0x7fu);
    Word(expected, 3u); Word(expected, 1u); Text(expected, "s");
    Word(expected, 4u); Text(expected, "text");
    Word(expected, 1u);
    Word(expected, 5u); Text(expected, "child");
    Word(expected, 0u); Word(expected, 0u);
    Check(output.bytes == expected,
          "record adapter emits exact root, attributes and child bytes");
}

}  // namespace

int main() {
    TestHeaderAndAllAttributeTypes();
    TestNullAndEmptyValues();
    TestRecordAdapter();
}
