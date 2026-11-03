#include "pal4/xml_parser_node.h"
#include "pal4/xml_generic_node.h"
#include "pal4/xml_leaf_runtime.h"

#include "pal4/compact_string_buffer.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
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
    std::vector<std::string> calls;
    bool throw_on_assign{};
    std::map<std::uint32_t, std::string> texts;
    std::map<std::uint32_t, std::uint32_t> virtual_values;
    std::map<std::uint32_t, std::uint32_t> virtual_calls;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        calls.push_back("read_byte:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        calls.push_back("write_byte:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    std::uint32_t Free(const std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return 0xdeadu;
    }
    std::uint32_t DeleteNode(const std::uint32_t node,
                             const std::uint32_t flag,
                             const std::uint32_t slot) override {
        calls.push_back("delete:" + std::to_string(node) + ":" +
                        std::to_string(flag) + ":" +
                        std::to_string(slot));
        return node + 1u;
    }
    std::uint32_t StringLength(const std::uint32_t text) override {
        calls.push_back("length:" + std::to_string(text));
        return 4u;
    }
    std::uint32_t AssignString(const std::uint32_t destination,
                               const std::uint32_t source,
                               const std::uint32_t length) override {
        calls.push_back("assign:" + std::to_string(destination) + ":" +
                        std::to_string(source) + ":" +
                        std::to_string(length));
        if (throw_on_assign) throw std::runtime_error("assign");
        return destination;
    }
    std::int32_t CompareText(const std::uint32_t left,
                             const std::uint32_t right) override {
        calls.push_back("compare:" + std::to_string(left) + ":" +
                        std::to_string(right));
        return texts.at(left) == texts.at(right) ? 0 : 1;
    }
    std::uint32_t CallVirtual(const std::uint32_t node,
                              const std::uint32_t slot) override {
        calls.push_back("virtual:" + std::to_string(node) + ":" +
                        std::to_string(slot));
        ++virtual_calls[node * 256u + slot];
        return virtual_values[node * 256u + slot];
    }
    std::uint32_t AttributeSetup(const std::uint32_t node,
                                 const std::uint32_t kind,
                                 const std::uint32_t first,
                                 const std::uint32_t second,
                                 const std::uint32_t third) override {
        calls.push_back("setup:" + std::to_string(node) + ":" +
                        std::to_string(kind) + ":" +
                        std::to_string(first) + ":" +
                        std::to_string(second) + ":" +
                        std::to_string(third));
        return 0u;
    }
};

struct VirtualRuntime final : pal4::IXmlVirtualDispatchRuntime {
    std::vector<std::uint32_t> call;
    std::uint32_t Call(const std::uint32_t node,
                       const std::uint32_t slot,
                       const std::uint32_t first,
                       const std::uint32_t second,
                       const std::uint32_t third) override {
        call = {node, slot, first, second, third};
        return 0x77u;
    }
};

struct LeafRuntime final : pal4::IXmlLeafRuntime {
    std::uint32_t allocation{0x3000u};
    std::vector<std::uint32_t> visitor_call;
    std::uint32_t Allocate(const std::uint32_t size) override {
        Check(size == 44u, "generic node allocation size is 44 bytes");
        return allocation;
    }
    std::uint32_t CallVisitor(const std::uint32_t visitor,
                              const std::uint32_t node,
                              const std::uint32_t slot) override {
        visitor_call = {visitor, node, slot};
        return 0x55u;
    }
};

void TestParserInitializeAndDelete() {
    Runtime runtime;
    Check(pal4::XmlParserLifecycle::Initialize(
              0x1000u, 0x2000u, runtime) == 0x1000u &&
              runtime.words.size() == 11u &&
              runtime.words.at(0x1000u) == 0x849970u &&
              runtime.words.at(0x1004u) == 0xffffffffu &&
              runtime.words.at(0x1008u) == 0xffffffffu &&
              runtime.words.at(0x1014u) == 0x2000u &&
              runtime.words.at(0x1020u) ==
                  pal4::CompactStringBuffer::kEmptyBlock &&
              runtime.words.at(0x1028u) == 0u,
          "parser initialization writes all eleven 32-bit fields");
    runtime.words[0x1018u] = 0x3000u;
    runtime.words[0x3028u] = 0x3100u;
    runtime.words[0x3128u] = 0u;
    runtime.words[0x1020u] = 0x5000u;
    runtime.calls.clear();
    Check(pal4::XmlParserLifecycle::Delete(
              0x1000u, 0u, runtime) == 0x1000u &&
              runtime.calls == std::vector<std::string>{
                  "write:4096:" + std::to_string(0x849970u),
                  "read:4120", "read:12328",
                  "delete:12288:1:0", "read:12584", "delete:12544:1:0",
                  "read:4128", "free:20480",
                  "write:4096:" + std::to_string(0x8499b4u)},
          "parser destroys chained nodes before compact block and base vtable");
    runtime.words[0x1018u] = 0u;
    runtime.words[0x1020u] = pal4::CompactStringBuffer::kEmptyBlock;
    runtime.calls.clear();
    Check(pal4::XmlParserLifecycle::Delete(
              0x1000u, 1u, runtime) == 0x1000u &&
              runtime.calls.back() == "free:4096" &&
              runtime.calls.size() == 5u,
          "scalar deleting destructor frees object only with low flag bit");
}

void TestNodeAttributeAndCleanup() {
    Runtime runtime;
    pal4::XmlNode::Initialize(0x2000u, runtime);
    Check(runtime.words.at(0x2000u) == 0x8499b4u,
          "base XML node initialization writes its vtable");
    runtime.words[0x2020u] = 0x5000u;
    runtime.words[0x200cu] = 0x44u;
    runtime.calls.clear();
    Check(pal4::XmlNode::SetAttribute(
              0x2000u, 0x3000u, runtime) == 0x44u &&
              runtime.words.at(0x300cu) == 0x44u &&
              runtime.calls == std::vector<std::string>{
                  "read:8224", "length:20488", "assign:12320:20488:4",
                  "read:8204", "write:12300:68"},
          "attribute copy assigns compact string before source field twelve");
    runtime.throw_on_assign = true;
    runtime.calls.clear();
    bool caught = false;
    try {
        (void)pal4::XmlNode::SetAttribute(
            0x2000u, 0x3100u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "assign:12576:20488:4" &&
              !runtime.words.contains(0x310cu),
          "failed string copy leaves destination field twelve untouched");
    runtime.throw_on_assign = false;
    runtime.bytes[0x202cu] = 1u;
    runtime.calls.clear();
    Check(pal4::XmlNode::SetAttributeWithFlag(
              0x2000u, 0x3100u, runtime) == 1u &&
              runtime.bytes.at(0x312cu) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "read:8224", "length:20488", "assign:12576:20488:4",
                  "read:8204", "write:12556:68", "read_byte:8236",
                  "write_byte:12588:1"},
          "flag copy follows base string and attribute assignment");

    runtime.words[0x2018u] = 0x4000u;
    runtime.words[0x4018u] = 0u;
    runtime.words[0x4028u] = 0x4100u;
    runtime.words[0x4128u] = 0u;
    runtime.words[0x201cu] = 0x4100u;
    runtime.calls.clear();
    Check(pal4::XmlNode::Cleanup(0x2000u, runtime) == 0x4101u &&
              runtime.words.at(0x2018u) == 0u &&
              runtime.words.at(0x201cu) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:8216", "read:16424", "delete:16384:1:0",
                  "read:16680", "delete:16640:1:0",
                  "write:8216:0", "write:8220:0"},
          "node cleanup deletes linked children and resets both links");
    runtime.calls.clear();
    Check(pal4::XmlNode::Cleanup(0x2000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:8216", "write:8216:0", "write:8220:0"},
          "empty node cleanup remains idempotent");
    pal4::XmlParserNodeRuntimeStub stub;
    Check(stub.ReadWord(0u) == 0u && stub.Free(0u) == 0u &&
              stub.DeleteNode(0u, 1u, 0u) == 0u,
          "XML parser boundary has a linkable stub");
}

void TestAddChildAndParent() {
    Runtime runtime;
    runtime.words[0x101cu] = 0u;
    runtime.words[0x3014u] = 1u;
    runtime.words[0x3114u] = 1u;
    Check(pal4::XmlNode::AddChild(0x1000u, 0x3000u, runtime) ==
              0x3000u && runtime.words.at(0x1018u) == 0x3000u &&
              runtime.words.at(0x101cu) == 0x3000u &&
              runtime.words.at(0x3010u) == 0x1000u &&
              runtime.words.at(0x3024u) == 0u &&
              runtime.words.at(0x3028u) == 0u,
          "first child sets parent, head, tail and sibling links");
    Check(pal4::XmlNode::AddChild(0x1000u, 0x3100u, runtime) ==
              0x3100u && runtime.words.at(0x1018u) == 0x3000u &&
              runtime.words.at(0x101cu) == 0x3100u &&
              runtime.words.at(0x3124u) == 0x3000u &&
              runtime.words.at(0x3028u) == 0x3100u,
          "later child links after prior tail");

    runtime.words[0x3214u] = 0u;
    runtime.virtual_values[0x1000u * 256u + 16u] = 0x9000u;
    runtime.calls.clear();
    Check(pal4::XmlNode::AddChild(0x1000u, 0x3200u, runtime) == 0u &&
              runtime.words.at(0x101cu) == 0x3100u &&
              runtime.virtual_calls[0x1000u * 256u + 16u] == 4u &&
              runtime.calls == std::vector<std::string>{
                  "read:12820", "delete:12800:1:0",
                  "virtual:4096:16", "virtual:4096:16",
                  "virtual:4096:16", "virtual:4096:16",
                  "setup:36864:16:0:0:0"},
          "invalid child is deleted and ancestor setup follows two parent lookups");
    runtime.words[0x3314u] = 0u;
    runtime.virtual_values[0x1000u * 256u + 16u] = 0u;
    runtime.words[0x1010u] = 0u;
    runtime.calls.clear();
    Check(pal4::XmlNode::AddChild(0x1000u, 0x3300u, runtime) == 0u &&
              runtime.calls.back() == "read:4112",
          "invalid child without ancestor skips attribute setup");

    runtime.words[0x4010u] = 0x4100u;
    runtime.virtual_values[0x4100u * 256u + 16u] = 0x9000u;
    runtime.calls.clear();
    Check(pal4::XmlNode::GetParent(0x4000u, runtime) == 0x9000u &&
              runtime.virtual_calls[0x4100u * 256u + 16u] == 2u,
          "parent lookup climbs through zero-return nodes and calls match twice");
    Check(pal4::XmlNode::GetParent(0u, runtime) == 0u,
          "null parent lookup returns zero");
}

void TestChildQueries() {
    Runtime runtime;
    runtime.words[0x1018u] = 0x3000u;
    runtime.words[0x3028u] = 0x3100u;
    runtime.words[0x3128u] = 0x3200u;
    runtime.words[0x3228u] = 0u;
    runtime.words[0x3020u] = 0x5000u;
    runtime.words[0x3120u] = 0x5100u;
    runtime.words[0x3220u] = 0x5200u;
    runtime.texts[0x5008u] = "other";
    runtime.texts[0x5108u] = "target";
    runtime.texts[0x5208u] = "target";
    runtime.texts[0x6000u] = "target";
    runtime.texts[0x6010u] = "missing";
    Check(pal4::XmlNode::FindChildByName(
              0x1000u, 0x6000u, runtime) == 0x3100u &&
              pal4::XmlNode::FindFollowingSiblingByName(
                  0x3100u, 0x6000u, runtime) == 0x3200u,
          "name search starts at first child or next sibling respectively");
    Check(pal4::XmlNode::FindChildByName(
              0x1000u, 0x6010u, runtime) == 0u,
          "name search returns zero after the sibling chain");

    runtime.virtual_values[0x3200u * 256u + 24u] = 0x7000u;
    runtime.calls.clear();
    Check(pal4::XmlNode::GetFirstChild(0x1000u, runtime) == 0x7000u &&
              runtime.virtual_calls[0x3000u * 256u + 24u] == 1u &&
              runtime.virtual_calls[0x3100u * 256u + 24u] == 1u &&
              runtime.virtual_calls[0x3200u * 256u + 24u] == 2u,
          "first-child lookup skips false virtual results and calls match twice");
    runtime.virtual_calls.clear();
    Check(pal4::XmlNode::GetChildByName(
              0x1000u, 0x6000u, runtime) == 0x7000u &&
              runtime.virtual_calls[0x3100u * 256u + 24u] == 1u &&
              runtime.virtual_calls[0x3200u * 256u + 24u] == 2u,
          "named child lookup retries at the next matching sibling");
    runtime.virtual_calls.clear();
    Check(pal4::XmlNode::GetChildByNameFromNext(
              0x3000u, 0x6000u, runtime) == 0x7000u &&
              runtime.virtual_calls[0x3100u * 256u + 24u] == 1u,
          "following-sibling named lookup begins after the current node");
    Check(pal4::XmlNode::GetChildByName(
              0x1000u, 0x6010u, runtime) == 0u,
          "named child lookup returns zero when no name matches");
}

void TestNextSiblingSentinel() {
    Runtime runtime;
    runtime.words[0x1020u] = 0x3000u;
    runtime.words[0x3018u] = 0x5000u;
    runtime.words[0x3014u] = 0x5100u;
    runtime.words[0x5000u] = 0u;
    runtime.words[0x5100u] = 0u;
    Check(pal4::XmlNode::GetNextSibling(0x1000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:4128", "read:12312", "read:20480",
                  "read:12308", "read:20736"},
          "next attribute with empty value and name is the list sentinel");
    runtime.words[0x5100u] = 3u;
    runtime.calls.clear();
    Check(pal4::XmlNode::GetNextSibling(
              0x1000u, runtime) == 0x3000u,
          "nonempty name keeps an otherwise empty successor");
    runtime.words[0x5000u] = 2u;
    runtime.calls.clear();
    Check(pal4::XmlNode::GetNextSibling(
              0x1000u, runtime) == 0x3000u &&
              runtime.calls.size() == 3u,
          "nonempty value short-circuits name length lookup");
}

void TestVirtual68() {
    VirtualRuntime runtime;
    Check(pal4::XmlNode::CallVirtual68(
              0x1000u, 0x2000u, 0x3000u, runtime) == 0x77u &&
              runtime.call == std::vector<std::uint32_t>{
                  0x1000u, 68u, 0x2000u, 0x3000u, 0u},
          "virtual slot 0x44 forwards both arguments and a trailing zero");
}

void TestGenericNode() {
    Runtime parser;
    LeafRuntime runtime;
    parser.words[0x2020u] = 0x5000u;
    parser.words[0x200cu] = 9u;
    Check(pal4::XmlNode::SetAttributeThunk(
              0x2000u, 0x3100u, parser) == 9u &&
              parser.words[0x310cu] == 9u,
          "attribute thunk forwards to the base node copy");
    parser.calls.clear();
    Check(pal4::XmlGenericNode::Create(
              0x2000u, parser, runtime) == 0x3000u &&
              parser.words[0x3000u] == pal4::XmlGenericNode::kVtable &&
              parser.words[0x3014u] == 3u &&
              parser.words[0x300cu] == 9u &&
              parser.words[0x3020u] ==
                  pal4::CompactStringBuffer::kEmptyBlock,
          "generic node initializes parser kind three then copies source");
    Check(pal4::XmlGenericNode::AcceptVisitor(
              0x3000u, 0x7000u, runtime) == 0x55u &&
              runtime.visitor_call == std::vector<std::uint32_t>{
                  0x7000u, 0x3000u, 20u},
          "generic visitor dispatches slot 0x14");
    parser.calls.clear();
    pal4::XmlGenericNode::Cleanup(0x3000u, parser);
    Check(parser.words[0x3000u] == 0x8499b4u &&
              parser.calls.front() == "write:12288:8690480" &&
              parser.calls.back() == "write:12288:8690100",
          "generic cleanup installs derived then base vtables");
    runtime.allocation = 0u;
    parser.calls.clear();
    Check(pal4::XmlGenericNode::Create(
              0x2000u, parser, runtime) == 0u &&
              parser.calls.empty(),
          "allocation failure leaves parser memory untouched");
}

}  // namespace

int main() {
    TestParserInitializeAndDelete();
    TestNodeAttributeAndCleanup();
    TestAddChildAndParent();
    TestChildQueries();
    TestNextSiblingSentinel();
    TestVirtual68();
    TestGenericNode();
}
