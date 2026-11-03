#include "pal4/xml_node_data_reader.h"
#include "pal4/xml_document_data_reader.h"
#include "pal4/xml_versioned_stream_reader.h"
#include "pal4/xml_document_file_loader.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct AttributeValue {
    std::uint32_t type{};
    std::string name;
    std::string kind;
    std::uint32_t scalar{};
    std::vector<std::uint8_t> bytes;
};

struct Runtime final : pal4::IXmlNodeDataRuntime {
    std::vector<std::uint8_t> stream;
    std::size_t cursor{};
    std::uint32_t next_address{0x1000u};
    std::map<std::uint32_t, std::uint8_t> memory;
    std::map<std::uint32_t, std::uint32_t> allocations;
    std::map<std::uint32_t, AttributeValue> live_attributes;
    std::map<std::uint32_t, std::vector<AttributeValue>> node_attributes;
    std::map<std::uint32_t, std::string> node_names;
    std::map<std::uint32_t, std::uint32_t> node_parents;
    std::vector<std::string> events;
    std::uint32_t next_attribute{0x8000u};
    std::uint32_t next_node{0x20000u};
    bool fail_add{};
    bool fail_read{};
    bool check_binary_name_lifetime{};
    std::map<std::uint32_t, std::uint32_t> attribute_names;

    void Word(const std::int32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8)
            stream.push_back(static_cast<std::uint8_t>(
                static_cast<std::uint32_t>(value) >> shift));
    }
    void Byte(const std::uint8_t value) { stream.push_back(value); }
    void Text(const std::string& value) {
        stream.insert(stream.end(), value.begin(), value.end());
    }
    void NamedNode(const std::string& name) {
        Word(static_cast<std::int32_t>(name.size()));
        Text(name);
    }
    [[nodiscard]] std::uint8_t Next() {
        if (fail_read) throw std::runtime_error("stream failure");
        Check(cursor < stream.size(), "stream contains requested bytes");
        return stream[cursor++];
    }
    [[nodiscard]] std::int32_t ReadInt32() override {
        std::uint32_t value = 0u;
        for (unsigned shift = 0; shift < 32; shift += 8)
            value |= static_cast<std::uint32_t>(Next()) << shift;
        return static_cast<std::int32_t>(value);
    }
    [[nodiscard]] std::uint8_t ReadByte() override { return Next(); }
    void ReadBytes(const std::uint32_t destination,
                   const std::uint32_t size) override {
        for (std::uint32_t index = 0; index < size; ++index)
            memory[destination + index] = Next();
    }
    [[nodiscard]] std::uint32_t AllocateTemporary(
        const std::uint32_t size) override {
        const auto address = next_address;
        next_address += size + 16u;
        allocations[address] = size;
        events.emplace_back("allocate");
        return address;
    }
    void FreeTemporary(const std::uint32_t address) override {
        Check(allocations.erase(address) == 1u, "temporary freed once");
        events.emplace_back("free");
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        memory[address] = value;
    }
    [[nodiscard]] std::string String(const std::uint32_t address) {
        std::string result;
        if (address == 0u) return result;
        for (std::uint32_t index = 0u; memory[address + index] != 0u; ++index)
            result.push_back(static_cast<char>(memory[address + index]));
        return result;
    }
    void SetName(const std::uint32_t node, const std::uint32_t name,
                 const std::uint32_t parent) override {
        node_names[node] = String(name);
        node_parents[node] = parent;
        events.emplace_back("name");
    }
    [[nodiscard]] std::uint32_t InitializeAttribute(
        const std::uint32_t type, const std::uint32_t name,
        std::span<std::uint32_t, 7>) override {
        const auto address = next_attribute++;
        live_attributes[address] = AttributeValue{type, String(name)};
        attribute_names[address] = name;
        events.emplace_back("attribute-init");
        return address;
    }
    void SetChar(const std::uint32_t attribute,
                 const std::uint8_t value) override {
        live_attributes[attribute].kind = "char";
        live_attributes[attribute].scalar = value;
    }
    void SetInt(const std::uint32_t attribute,
                const std::int32_t value) override {
        live_attributes[attribute].kind = "int";
        live_attributes[attribute].scalar = static_cast<std::uint32_t>(value);
    }
    void SetFloatBits(const std::uint32_t attribute,
                      const std::uint32_t bits) override {
        live_attributes[attribute].kind = "float";
        live_attributes[attribute].scalar = bits;
    }
    void SetString(const std::uint32_t attribute,
                   const std::uint32_t text) override {
        live_attributes[attribute].kind = "string";
        live_attributes[attribute].name += ":" + String(text);
    }
    void SetRaw(const std::uint32_t attribute, const std::uint32_t data,
                const std::uint32_t size) override {
        auto& value = live_attributes[attribute];
        value.kind = "raw";
        for (std::uint32_t index = 0; index < size; ++index)
            value.bytes.push_back(memory[data + index]);
    }
    void AddAttribute(const std::uint32_t node,
                      const std::uint32_t attribute) override {
        if (check_binary_name_lifetime)
            Check(!allocations.contains(attribute_names.at(attribute)),
                  "binary attribute name buffer is freed before value insertion (IDA5F78BC)");
        if (fail_add) throw std::runtime_error("attribute insertion failed");
        node_attributes[node].push_back(live_attributes.at(attribute));
        events.emplace_back("attribute-add");
    }
    void DestroyAttribute(const std::uint32_t attribute) override {
        Check(live_attributes.erase(attribute) == 1u,
              "attribute destroyed once");
        attribute_names.erase(attribute);
        events.emplace_back("attribute-destroy");
    }
    [[nodiscard]] std::uint32_t CreateNode() override {
        events.emplace_back("child-create");
        return next_node++;
    }
    void DestroyNode(std::uint32_t node) override {
        node_names.erase(node);node_parents.erase(node);
        events.emplace_back("child-destroy");
    }
};

struct Source final : pal4::IXmlDocumentDataSource {
    std::map<std::uint32_t, std::map<std::string, std::string>> attributes;
    std::map<std::uint32_t, std::vector<std::pair<std::string, std::uint32_t>>> children;
    std::map<std::uint32_t, std::uint32_t> parents;

    void Child(const std::uint32_t parent, const std::string& kind,
               const std::uint32_t child) {
        children[parent].emplace_back(kind, child);
        parents[child] = parent;
    }
    [[nodiscard]] std::string GetAttribute(
        const std::uint32_t node, const std::string_view name) override {
        return attributes[node][std::string(name)];
    }
    [[nodiscard]] std::uint32_t FirstChild(
        const std::uint32_t node, const std::string_view name) override {
        for (const auto& [kind, child] : children[node])
            if (kind == name) return child;
        return 0u;
    }
    [[nodiscard]] std::uint32_t NextChild(
        const std::uint32_t node, const std::string_view name) override {
        bool after_current = false;
        for (const auto& [kind, child] : children[parents.at(node)]) {
            if (after_current && kind == name) return child;
            if (child == node) after_current = true;
        }
        return 0u;
    }
    [[nodiscard]] std::uint32_t GetType(
        const std::string_view name) override {
        constexpr std::string_view types[]{
            "XML_CHAR", "XML_INT", "XML_FLOAT", "XML_STRING", "XML_BINARY"};
        for (std::uint32_t index = 0u; index < 5u; ++index)
            if (types[index] == name) return index;
        return 99u;
    }
};

struct Input final : pal4::IXmlNodeDataInput {
    Runtime& runtime;
    explicit Input(Runtime& source) : runtime(source) {}
    void Read(void* destination, const std::uint32_t size) override {
        auto* bytes = static_cast<std::uint8_t*>(destination);
        for (std::uint32_t index = 0u; index < size; ++index)
            bytes[index] = runtime.Next();
    }
};

struct FileRuntime final : pal4::IXmlDocumentFileRuntime {
    std::vector<std::string> events;
    std::map<std::uint32_t, std::uint8_t> memory;
    std::string contents{"<node/>"};
    std::int32_t length{7};
    bool prefix_success{true};
    bool parse_error{};
    bool throw_parse{};
    std::uint32_t first_child{1u};

    [[nodiscard]] bool ReadLengthWord(const std::uint32_t stream,
                                      std::int32_t& result) override {
        Check(stream == 0x100u, "length read uses supplied stream");
        events.emplace_back("length");
        result = length;
        return prefix_success;
    }
    void SeekToStart(const std::uint32_t stream) override {
        Check(stream == 0x100u, "seek uses supplied stream");
        events.emplace_back("seek");
    }
    [[nodiscard]] std::int32_t GetSize(const std::uint32_t stream) override {
        Check(stream == 0x100u, "size uses supplied stream");
        events.emplace_back("size");
        return length;
    }
    void ReadBytes(const std::uint32_t stream, const std::uint32_t destination,
                   const std::uint32_t size) override {
        Check(stream == 0x100u && size == static_cast<std::uint32_t>(length),
              "data read uses selected length");
        events.emplace_back("read");
        for (std::uint32_t index = 0u; index < size; ++index)
            memory[destination + index] =
                static_cast<std::uint8_t>(contents[index]);
    }
    [[nodiscard]] std::uint32_t Allocate(const std::uint32_t size) override {
        Check(size == static_cast<std::uint32_t>(length) + 1u,
              "text allocation adds terminator");
        events.emplace_back("allocate");
        return 0x3000u;
    }
    void Free(const std::uint32_t address) override {
        Check(address == 0x3000u, "frees input buffer");
        events.emplace_back("free");
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        memory[address] = value;
        events.emplace_back("terminator");
    }
    void ConstructParser() override { events.emplace_back("construct"); }
    void Parse(const std::uint32_t source) override {
        Check(source == 0x3000u && memory[source + length] == 0u,
              "parser sees terminated input");
        events.emplace_back("parse");
        if (throw_parse) throw std::runtime_error("parse failed");
    }
    [[nodiscard]] bool HasParseError() override {
        events.emplace_back("error");
        return parse_error;
    }
    [[nodiscard]] std::uint32_t FirstParsedChild() override {
        events.emplace_back("first");
        return first_child;
    }
    void DestroyParser() override { events.emplace_back("destroy"); }
};

void TestAllTypesAndRecursiveChildren() {
    Runtime runtime;
    runtime.check_binary_name_lifetime = true;
    runtime.NamedNode("root");
    runtime.Word(7);
    runtime.Word(0); runtime.Word(1); runtime.Text("c"); runtime.Byte(0x80u);
    runtime.Word(1); runtime.Word(1); runtime.Text("i"); runtime.Word(-42);
    runtime.Word(2); runtime.Word(1); runtime.Text("f"); runtime.Word(0x3f800000);
    runtime.Word(3); runtime.Word(1); runtime.Text("s"); runtime.Word(3); runtime.Text("abc");
    runtime.Word(3); runtime.Word(1); runtime.Text("e"); runtime.Word(0);
    runtime.Word(4); runtime.Word(1); runtime.Text("b"); runtime.Word(3);
    runtime.Byte(0u); runtime.Byte(0xffu); runtime.Byte(7u);
    runtime.Word(99); runtime.Word(1); runtime.Text("u");
    runtime.Word(2);
    runtime.NamedNode("left"); runtime.Word(0); runtime.Word(0);
    runtime.NamedNode("right"); runtime.Word(0); runtime.Word(1);
    runtime.NamedNode("leaf"); runtime.Word(0); runtime.Word(0);

    Check(pal4::XmlNodeDataReader::Read(0u, 0x400u, runtime) == 1u,
          "reader returns one");
    Check(runtime.cursor == runtime.stream.size() &&
              runtime.allocations.empty() && runtime.live_attributes.empty(),
          "all bytes consumed and temporary objects released");
    Check(runtime.node_names[0x400u] == "root" &&
              runtime.node_names[0x20000u] == "left" &&
              runtime.node_names[0x20001u] == "right" &&
              runtime.node_names[0x20002u] == "leaf" &&
              runtime.node_parents[0x20002u] == 0x20001u,
          "child nodes follow depth-first order and receive parent");
    const auto& values = runtime.node_attributes[0x400u];
    Check(values.size() == 7u && values[0].scalar == 0x80u &&
              values[1].scalar == static_cast<std::uint32_t>(-42) &&
              values[2].scalar == 0x3f800000u &&
              values[3].name == "s:abc" && values[4].name == "e:" &&
              values[5].bytes == (std::vector<std::uint8_t>{0u, 0xffu, 7u}) &&
              values[6].kind.empty(),
          "all native attribute type branches retain their values");
    for (std::size_t index = 0; index < runtime.events.size(); ++index)
        if (runtime.events[index] == "attribute-add")
            Check(runtime.events[index + 1] == "attribute-destroy",
                  "temporary attribute dies after insertion");
}

void TestEmptyAndNonpositiveFields() {
    Runtime runtime;
    runtime.Word(0); runtime.Word(-1); runtime.Word(-3);
    Check(pal4::XmlNodeDataReader::Read(0x55u, 0x66u, runtime) == 1u &&
              runtime.node_names[0x66u].empty() &&
              runtime.node_parents[0x66u] == 0x55u &&
              runtime.allocations.empty() && runtime.next_node == 0x20000u,
          "nonpositive name and counts skip allocation and loops");

    Runtime zero_payload;
    zero_payload.Word(-4); zero_payload.Word(2);
    zero_payload.Word(3); zero_payload.Word(0); zero_payload.Word(-1);
    zero_payload.Word(4); zero_payload.Word(0); zero_payload.Word(0);
    zero_payload.Word(0);
    Check(pal4::XmlNodeDataReader::Read(0u, 0x77u, zero_payload) == 1u &&
              zero_payload.node_attributes[0x77u].size() == 2u &&
              zero_payload.node_attributes[0x77u][0].kind == "string" &&
              zero_payload.node_attributes[0x77u][1].kind.empty() &&
              zero_payload.allocations.empty(),
          "empty names, empty string and zero raw length match branches");
}

void TestFailureCleanup() {
    Runtime bad_length;
    bad_length.Word(0); bad_length.Word(1);
    bad_length.Word(1); bad_length.Word(-1);
    bool caught = false;
    try { (void)pal4::XmlNodeDataReader::Read(0u, 1u, bad_length); }
    catch (const std::length_error&) { caught = true; }
    Check(caught && bad_length.allocations.empty(),
          "negative attribute name length is rejected before allocation");

    Runtime failed_insert;
    failed_insert.Word(0); failed_insert.Word(1);
    failed_insert.Word(1); failed_insert.Word(1);
    failed_insert.Text("a"); failed_insert.Word(7);
    failed_insert.fail_add = true;
    caught = false;
    try { (void)pal4::XmlNodeDataReader::Read(0u, 1u, failed_insert); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && failed_insert.allocations.empty() &&
              failed_insert.live_attributes.empty() &&
              failed_insert.events == std::vector<std::string>{
                  "name", "allocate", "attribute-init", "free", "attribute-destroy"},
          "insertion exception destroys attribute after its name buffer was already freed");

    Runtime failed_read;
    failed_read.Word(2); failed_read.Text("ab");
    failed_read.fail_read = true;
    caught = false;
    try { (void)pal4::XmlNodeDataReader::Read(0u, 1u, failed_read); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && failed_read.allocations.empty(),
          "stream exception does not leak temporary buffers");
}

void TestDocumentDataConversion() {
    Source source;
    Runtime output;
    source.attributes[1u]["Name"] = "root";
    source.Child(1u, "Xnd_Attr", 2u);
    source.attributes[2u] = {{"Name", "code"}, {"Type", "XML_BINARY"},
                             {"Value", "0a1b"}};
    source.Child(1u, "other", 8u);
    source.Child(1u, "Xnd_Attr", 3u);
    source.attributes[3u] = {{"Name", "number"}, {"Type", "XML_INT"},
                             {"Value", "-15"}};
    source.Child(1u, "Xnd_Attr", 4u);
    source.attributes[4u] = {{"Name", "odd"}, {"Type", "unknown"},
                             {"Value", "text"}};
    source.Child(1u, "Xnd_Node", 5u);
    source.attributes[5u]["Name"] = "child";
    source.Child(5u, "Xnd_Node", 6u);
    source.attributes[6u]["Name"] = "leaf";

    Check(pal4::XmlDocumentDataReader::Read(0u, 0x400u, 1u,
                                              source, output) == 1u,
          "text XML document reader returns one");
    const auto& values = output.node_attributes[0x400u];
    Check(values.size() == 3u && values[0].type == 4u &&
              values[0].bytes == (std::vector<std::uint8_t>{0x0au, 0x1bu}) &&
              values[1].type == 1u && values[1].name == "number:-15" &&
              values[2].type == 99u && values[2].name == "odd:text",
          "text XML attributes retain order, type and decoded values");
    Check(output.node_names[0x400u] == "root" &&
              output.node_names[0x20000u] == "child" &&
              output.node_names[0x20001u] == "leaf" &&
              output.node_parents[0x20001u] == 0x20000u &&
              output.allocations.empty() && output.live_attributes.empty(),
          "text XML children recurse and temporary resources are released");
}

void TestDocumentDataEmptyAndFailure() {
    Source source;
    Runtime output;
    source.attributes[1u]["Name"] = "";
    source.Child(1u, "Xnd_Attr", 2u);
    source.attributes[2u] = {{"Name", "empty"}, {"Type", "XML_BINARY"},
                             {"Value", "f"}};
    Check(pal4::XmlDocumentDataReader::Read(0x55u, 0x66u, 1u,
                                              source, output) == 1u &&
              output.node_names[0x66u].empty() &&
              output.node_parents[0x66u] == 0x55u &&
              output.node_attributes[0x66u][0].bytes.empty() &&
              output.allocations.empty(),
          "empty node name and half-byte binary text skip raw setting");

    Runtime failed;
    failed.fail_add = true;
    bool caught = false;
    try {
        (void)pal4::XmlDocumentDataReader::Read(0u, 0x66u, 1u,
                                                   source, failed);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && failed.allocations.empty() &&
              failed.live_attributes.empty(),
          "text XML insertion failure frees node name and attribute buffers");
}

void TestVersionedVirtualStream() {
    Runtime null_runtime;
    Input null_input(null_runtime);
    Check(pal4::XmlVersionedStreamReader::Read(
              0u, 0x400u, null_input, null_runtime) == 0u &&
              pal4::XmlVersionedStreamReader::Read(
                  0x100u, 0u, null_input, null_runtime) == 0u &&
              null_runtime.cursor == 0u,
          "null stream or destination returns before reading");

    for (const std::pair<std::int32_t, std::int32_t> version : {
             std::pair{2, 0}, std::pair{1, 2}}) {
        Runtime rejected;
        rejected.Word(version.first);
        rejected.Word(version.second);
        Input input(rejected);
        Check(pal4::XmlVersionedStreamReader::Read(
                  0x100u, 0x400u, input, rejected) == 0u &&
                  rejected.cursor == 8u && rejected.node_names.empty(),
              "unsupported major or minor stops before node parsing");
    }

    for (const std::pair<std::int32_t, std::int32_t> version : {
             std::pair{0, 99}, std::pair{1, 1}}) {
        Runtime accepted;
        accepted.Word(version.first);
        accepted.Word(version.second);
        accepted.NamedNode("accepted");
        accepted.Word(0);
        accepted.Word(0);
        Input input(accepted);
        Check(pal4::XmlVersionedStreamReader::Read(
                  0x100u, 0x400u, input, accepted) == 1u &&
                  accepted.node_names[0x400u] == "accepted" &&
                  accepted.cursor == accepted.stream.size() &&
                  accepted.allocations.empty(),
              "accepted version delegates to virtual-stream node reader");
    }
}

void TestDocumentFileBranchesAndCleanup() {
    Source source;
    source.attributes[1u]["Name"] = "loaded";
    Runtime output;
    FileRuntime file;
    Check(pal4::XmlDocumentFileLoader::Load(
              0u, 0x400u, true, file, source, output) == 0u &&
              pal4::XmlDocumentFileLoader::Load(
                  0x100u, 0u, false, file, source, output) == 0u &&
              file.events.empty(),
          "null stream or destination returns without side effects");

    file.prefix_success = false;
    Check(pal4::XmlDocumentFileLoader::Load(
              0x100u, 0x400u, true, file, source, output) == 0u &&
              file.events == (std::vector<std::string>{"length"}),
          "failed length read returns without parser construction");

    FileRuntime prefixed;
    Check(pal4::XmlDocumentFileLoader::Load(
              0x100u, 0x400u, true, prefixed, source, output) == 1u &&
              output.node_names[0x400u] == "loaded" &&
              prefixed.events == (std::vector<std::string>{
                  "length", "allocate", "read", "terminator", "construct",
                  "parse", "free", "error", "first", "destroy"}),
          "prefixed input parses and releases text before document transfer");

    FileRuntime whole_stream;
    whole_stream.parse_error = true;
    Check(pal4::XmlDocumentFileLoader::Load(
              0x100u, 0x500u, false, whole_stream, source, output) == 0u &&
              whole_stream.events == (std::vector<std::string>{
                  "seek", "size", "allocate", "read", "terminator",
                  "construct", "parse", "free", "error", "destroy"}) &&
              output.node_names.count(0x500u) == 0u,
          "whole-stream parse error skips document conversion and cleans up");

    FileRuntime thrown;
    thrown.throw_parse = true;
    bool caught = false;
    try {
        (void)pal4::XmlDocumentFileLoader::Load(
            0x100u, 0x600u, true, thrown, source, output);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && thrown.events[thrown.events.size() - 2u] == "destroy" &&
              thrown.events.back() == "free",
          "parse exception destroys parser then frees input buffer");
}

}  // namespace

int main() {
    TestAllTypesAndRecursiveChildren();
    TestEmptyAndNonpositiveFields();
    TestFailureCleanup();
    TestDocumentDataConversion();
    TestDocumentDataEmptyAndFailure();
    TestVersionedVirtualStream();
    TestDocumentFileBranchesAndCleanup();
}
