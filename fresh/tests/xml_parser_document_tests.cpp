#include "pal4/xml_parser_document.h"
#include "pal4/xml_node_workflow.h"
#include "pal4/xml_file_parser.h"

#include "pal4/compact_string_buffer.h"

#include <algorithm>
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

struct Runtime final : pal4::IXmlDocumentRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> texts;
    std::map<std::uint32_t, std::uint32_t> virtual_values;
    std::vector<std::string> calls;
    std::uint32_t next_allocation{0x5000u};
    std::uint32_t setup_result{0x56u};
    bool fail_allocation{};
    bool throw_on_assign{};
    std::map<std::uint32_t, std::uint32_t> following;
    std::map<std::uint32_t, std::uint32_t> clones;
    bool enter_result{true};
    std::uint32_t stop_child{};
    std::uint32_t leave_result{0xbeefu};

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
        return text == pal4::XmlFileParser::kEmptyErrorText ? 0u :
               text == 0x7000u ? 3u : 5u;
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
        return setup_result;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        if (fail_allocation) return 0u;
        const auto result = next_allocation;
        next_allocation += 0x100u;
        return result;
    }
    void DestroyAttribute(const std::uint32_t root) override {
        calls.push_back("destroy_attribute:" + std::to_string(root));
    }
    std::uint32_t NextSibling(const std::uint32_t node) override {
        calls.push_back("next:" + std::to_string(node));
        return following.at(node);
    }
    std::uint32_t CloneChild(const std::uint32_t node,
                             const std::uint32_t slot) override {
        calls.push_back("clone:" + std::to_string(node) + ":" +
                        std::to_string(slot));
        return clones.at(node);
    }
    bool VisitorEnter(const std::uint32_t visitor,
                      const std::uint32_t node,
                      const std::uint32_t attributes,
                      const std::uint32_t slot) override {
        calls.push_back("enter:" + std::to_string(visitor) + ":" +
                        std::to_string(node) + ":" +
                        std::to_string(attributes) + ":" +
                        std::to_string(slot));
        return enter_result;
    }
    bool VisitChild(const std::uint32_t child,
                    const std::uint32_t visitor,
                    const std::uint32_t slot) override {
        calls.push_back("visit:" + std::to_string(child) + ":" +
                        std::to_string(visitor) + ":" +
                        std::to_string(slot));
        return child != stop_child;
    }
    std::uint32_t VisitorLeave(const std::uint32_t visitor,
                               const std::uint32_t node,
                               const std::uint32_t slot) override {
        calls.push_back("leave:" + std::to_string(visitor) + ":" +
                        std::to_string(node) + ":" +
                        std::to_string(slot));
        return leave_result;
    }
};

void TestParseAndCleanup() {
    Runtime runtime;
    Check(pal4::XmlParserDocument::Parse(
              0x1000u, 0x7000u, runtime) == 0x1000u &&
              runtime.words.at(0x1000u) == 0x8499c0u &&
              runtime.words.at(0x1014u) == 1u &&
              runtime.words.at(0x1018u) == 0u &&
              runtime.words.at(0x101cu) == 0u &&
              runtime.words.at(0x102cu) == 0x849a04u &&
              runtime.words.at(0x1030u) == 0xffffffffu &&
              runtime.words.at(0x1034u) == 0xffffffffu &&
              runtime.words.at(0x1038u) == 0u &&
              runtime.words.at(0x103cu) == 0u &&
              runtime.words.at(0x1040u) ==
                  pal4::CompactStringBuffer::kEmptyBlock &&
              runtime.words.at(0x1044u) ==
                  pal4::CompactStringBuffer::kEmptyBlock &&
              runtime.words.at(0x104cu) == 0x102cu &&
              runtime.words.at(0x1048u) == 0x102cu &&
              runtime.calls.back() == "assign:4128:28672:3",
          "parse initializes parser, attribute root and compact source text");
    runtime.words[0x1018u] = 0x3000u;
    runtime.words[0x3028u] = 0u;
    runtime.words[0x104cu] = 0x4000u;
    runtime.words[0x1048u] = 0x4100u;
    runtime.words[0x401cu] = 0x102cu;
    runtime.words[0x4020u] = 0x4100u;
    runtime.words[0x411cu] = 0x4000u;
    runtime.words[0x4120u] = 0x102cu;
    runtime.words[0x1020u] = 0x6000u;
    runtime.calls.clear();
    Check(pal4::XmlParserDocument::CleanupNodes(
              0x1000u, runtime) == 0x102cu &&
              runtime.words.at(0x1018u) == 0u &&
              runtime.words.at(0x104cu) == 0x102cu,
          "cleanup nodes releases ordinary and attribute child chains");
    std::vector<std::string> actions;
    for (const auto& call : runtime.calls)
        if (call.starts_with("delete:"))
            actions.push_back(call);
    Check(actions == std::vector<std::string>{
              "delete:12288:1:0", "delete:16384:1:0",
              "delete:16640:1:0"} &&
              runtime.words.at(0x1048u) == 0x102cu &&
              runtime.words.at(0x401cu) == 0u &&
              runtime.words.at(0x4020u) == 0u &&
              runtime.words.at(0x411cu) == 0u &&
              runtime.words.at(0x4120u) == 0u,
          "ordinary children are deleted before root attributes");
    runtime.calls.clear();
    pal4::XmlParserDocument::Cleanup(0x1000u, runtime);
    Check(runtime.words.at(0x1000u) == 0x8499b4u &&
              runtime.calls[runtime.calls.size() - 3u] == "read:4128" &&
              runtime.calls[runtime.calls.size() - 2u] == "free:24576" &&
              runtime.calls.back() ==
                  "write:4096:" + std::to_string(0x8499b4u),
          "cleanup destroys attribute root and compact storage through base");

    Runtime failing;
    failing.throw_on_assign = true;
    bool caught = false;
    try {
        (void)pal4::XmlParserDocument::Parse(
            0x1000u, 0x7000u, failing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failing.words.at(0x1000u) == 0x8499b4u &&
              std::find(failing.calls.begin(), failing.calls.end(),
                        "destroy_attribute:4140") != failing.calls.end() &&
              failing.calls.back() ==
                  "write:4096:" + std::to_string(0x8499b4u),
          "failed parse unwinds attribute root then base parser state");
}

void TestAttributeReadAndWrite() {
    Runtime runtime;
    (void)pal4::XmlAttributeRoot::Initialize(0x102cu, runtime);
    runtime.words[0x5018u] = 0x6000u;
    runtime.words[0x5014u] = 0x6100u;
    runtime.texts[0x6108u] = "key";
    runtime.texts[0x7000u] = "key";
    (void)pal4::XmlAttributeRoot::AddChild(0x102cu, 0x5000u, runtime);
    runtime.calls.clear();
    Check(pal4::XmlParserDocument::GetAttribute(
              0x1000u, 0x7000u, runtime) == 0x6008u &&
              runtime.calls == std::vector<std::string>{
                  "read:4172", "read:20500",
                  "compare:24840:28672", "read:20504"},
          "attribute read returns payload after compact block header");
    runtime.calls.clear();
    Check(pal4::XmlParserDocument::SetAttribute(
              0x1000u, 0x7000u, 0x7100u, runtime) == 0x5018u &&
              runtime.calls == std::vector<std::string>{
                  "read:4172", "read:20500",
                  "compare:24840:28672", "length:28928",
                  "assign:20504:28928:5"},
          "existing attribute assigns value only and forwards result");
    runtime.texts[0x7000u] = "missing";
    runtime.calls.clear();
    Check(pal4::XmlParserDocument::GetAttribute(
              0x1000u, 0x7000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:4172", "read:20500", "compare:24840:28672",
                  "read:20512"},
          "missing attribute reads no value block");

    runtime.words[0x104cu] = 0x102cu;
    runtime.words[0x1048u] = 0x102cu;
    runtime.calls.clear();
    Check(pal4::XmlParserDocument::SetAttribute(
              0x1000u, 0x7000u, 0x7100u, runtime) == 0x5000u &&
              runtime.words.at(0x5000u) == 0x849a04u &&
              runtime.words.at(0x5004u) == 0xffffffffu &&
              runtime.words.at(0x5008u) == 0xffffffffu &&
              runtime.words.at(0x5014u) ==
                  pal4::CompactStringBuffer::kEmptyBlock &&
              runtime.words.at(0x5018u) ==
                  pal4::CompactStringBuffer::kEmptyBlock &&
              runtime.words.at(0x5010u) == 0u &&
              runtime.words.at(0x501cu) == 0x102cu &&
              runtime.words.at(0x5020u) == 0x102cu &&
              runtime.words.at(0x1048u) == 0x5000u &&
              runtime.words.at(0x104cu) == 0x5000u,
          "new attribute initializes 36-byte record and links it into root");
    std::vector<std::string> assignments;
    for (const auto& call : runtime.calls)
        if (call.starts_with("assign:")) assignments.push_back(call);
    Check(assignments == std::vector<std::string>{
              "assign:20500:28672:3", "assign:20504:28928:5"},
          "new attribute writes name before value");

    Runtime unavailable;
    (void)pal4::XmlAttributeRoot::Initialize(0x102cu, unavailable);
    unavailable.calls.clear();
    unavailable.fail_allocation = true;
    unavailable.virtual_values[0x1000u * 256u + 16u] = 0x90001234u;
    Check(pal4::XmlParserDocument::SetAttribute(
              0x1000u, 0x7000u, 0x7100u, unavailable) ==
              0x90001256u &&
              unavailable.calls.back() ==
                  "setup:2415923764:3:0:0:0",
          "allocation failure preserves ancestor high bytes around setup byte");
    Runtime no_parent;
    (void)pal4::XmlAttributeRoot::Initialize(0x102cu, no_parent);
    no_parent.calls.clear();
    no_parent.fail_allocation = true;
    no_parent.words[0x1010u] = 0u;
    Check(pal4::XmlParserDocument::SetAttribute(
              0x1000u, 0x7000u, 0x7100u, no_parent) == 0u,
          "allocation failure without ancestor returns zero");

    Runtime exceptional;
    (void)pal4::XmlAttributeRoot::Initialize(0x102cu, exceptional);
    exceptional.calls.clear();
    exceptional.throw_on_assign = true;
    bool caught = false;
    try {
        (void)pal4::XmlParserDocument::SetAttribute(
            0x1000u, 0x7000u, 0x7100u, exceptional);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && exceptional.calls[exceptional.calls.size() - 2u] ==
              "destroy_attribute:20480" &&
              exceptional.calls.back() == "free:20480",
          "failed new-attribute assignment releases partial record");
    pal4::XmlDocumentRuntimeStub stub;
    Check(stub.Allocate(36u) == 0u &&
              stub.NextSibling(0u) == 0u &&
              stub.CloneChild(0u, 60u) == 0u,
          "XML document native boundary has a linkable stub");
}

void TestAttributeCleanup() {
    Runtime runtime;
    runtime.words[0x5018u] = 0x6000u;
    runtime.words[0x5014u] = 0x6100u;
    Check(pal4::XmlAttributeLifecycle::Cleanup(
              0x5000u, runtime) == 0xdeadu &&
              runtime.words.at(0x5000u) == 0x8499b4u &&
              runtime.calls == std::vector<std::string>{
                  "read:20504", "free:24576", "read:20500",
                  "free:24832", "write:20480:" +
                      std::to_string(0x8499b4u)},
          "attribute cleanup frees value before name and installs base vtable");
    runtime.words[0x5018u] = pal4::CompactStringBuffer::kEmptyBlock;
    runtime.words[0x5014u] = pal4::CompactStringBuffer::kEmptyBlock;
    runtime.calls.clear();
    Check(pal4::XmlAttributeLifecycle::Cleanup(
              0x5000u, runtime) ==
              pal4::CompactStringBuffer::kEmptyBlock &&
              runtime.calls.size() == 3u,
          "empty compact blocks are never freed");
    runtime.words[0x5018u] = 0x6000u;
    runtime.words[0x5014u] = 0x6100u;
    runtime.calls.clear();
    Check(pal4::XmlAttributeLifecycle::CleanupAlternate(
              0x5000u, runtime) == 0xdeadu &&
              runtime.calls == std::vector<std::string>{
                  "read:20504", "free:24576", "read:20500",
                  "free:24832", "write:20480:" +
                      std::to_string(0x8499b4u)},
          "second destructor entry reuses attribute block cleanup");
}

void TestAttributeRootLinks() {
    Runtime runtime;
    const auto root = 0x1000u;
    Check(pal4::XmlAttributeRoot::Initialize(root, runtime) == root &&
              runtime.words.at(root + 28u) == root &&
              runtime.words.at(root + 32u) == root,
          "attribute root begins as a circular empty list");
    Check(pal4::XmlAttributeRoot::RemoveChild(
              root, 0x3000u, runtime) == root &&
              pal4::XmlAttributeRoot::FindByName(
                  root, 0x8000u, runtime) == 0u,
          "empty list removal and lookup return root or null");
    for (const auto child : {0x2000u, 0x3000u, 0x4000u})
        Check(pal4::XmlAttributeRoot::AddChild(root, child, runtime) == child,
              "attribute append returns child");
    Check(runtime.words.at(root + 32u) == 0x2000u &&
              runtime.words.at(root + 28u) == 0x4000u &&
              runtime.words.at(0x2020u) == 0x3000u &&
              runtime.words.at(0x301cu) == 0x2000u &&
              runtime.words.at(0x4020u) == root,
          "append maintains circular first, last, next and previous links");
    runtime.words[0x2014u] = 0x5000u;
    runtime.words[0x3014u] = 0x5100u;
    runtime.words[0x4014u] = 0x5200u;
    runtime.texts[0x5008u] = "first";
    runtime.texts[0x5108u] = "second";
    runtime.texts[0x5208u] = "third";
    runtime.texts[0x8000u] = "second";
    Check(pal4::XmlAttributeRoot::FindByName(
              root, 0x8000u, runtime) == 0x3000u,
          "name lookup traverses circular sibling chain");
    runtime.texts[0x8000u] = "missing";
    Check(pal4::XmlAttributeRoot::FindByName(
              root, 0x8000u, runtime) == 0u,
          "name lookup stops at root sentinel after a miss");
    Check(pal4::XmlAttributeRoot::RemoveChild(
              root, 0x3000u, runtime) == 0x3000u &&
              runtime.words.at(0x2020u) == 0x4000u &&
              runtime.words.at(0x401cu) == 0x2000u &&
              runtime.words.at(0x301cu) == 0u &&
              runtime.words.at(0x3020u) == 0u,
          "middle removal reconnects neighbors and clears removed links");
    Check(pal4::XmlAttributeRoot::RemoveChild(
              root, 0x3000u, runtime) == root &&
              runtime.words.at(root + 32u) == 0x2000u,
          "removing an absent child preserves the list");
}

void TestCloneInto() {
    Runtime runtime;
    runtime.words[0x1020u] = 0x5000u;
    runtime.words[0x100cu] = 0x44u;
    runtime.words[0x104cu] = 0x102cu;
    runtime.words[0x1018u] = 0u;
    Check(pal4::XmlParserDocument::CloneInto(
              0x1000u, 0x2000u, runtime) == 0x102cu &&
              runtime.words.at(0x200cu) == 0x44u &&
              runtime.calls[1] == "length:20488" &&
              runtime.calls[2] == "assign:8224:20488:5",
          "empty clone copies node text and returns source attribute root");

    runtime.words[0x104cu] = 0x3000u;
    runtime.words[0x3014u] = 0x6000u;
    runtime.words[0x3018u] = 0x6100u;
    runtime.words[0x3020u] = 0x102cu;
    runtime.words[0x202cu + 32u] = 0x7000u;
    runtime.words[0x7014u] = 0x6200u;
    runtime.texts[0x6008u] = "name";
    runtime.texts[0x6208u] = "name";
    runtime.following[0x3000u] = 0u;
    runtime.calls.clear();
    Check(pal4::XmlParserDocument::CloneInto(
              0x1000u, 0x2000u, runtime) == 0u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "assign:28696:24840:5") != runtime.calls.end() &&
              runtime.calls[runtime.calls.size() - 2u] == "next:12288",
          "attribute clone forwards name and value then returns final next");

    runtime.words[0x104cu] = 0x102cu;
    runtime.words[0x1018u] = 0x3000u;
    runtime.words[0x3028u] = 0u;
    runtime.clones[0x3000u] = 0x4000u;
    runtime.words[0x4014u] = 1u;
    runtime.words[0x201cu] = 0u;
    runtime.calls.clear();
    Check(pal4::XmlParserDocument::CloneInto(
              0x1000u, 0x2000u, runtime) == 0x4000u &&
              runtime.words.at(0x2018u) == 0x4000u &&
              runtime.words.at(0x201cu) == 0x4000u &&
              runtime.words.at(0x4010u) == 0x2000u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "clone:12288:60") != runtime.calls.end(),
          "child clone uses virtual slot sixty and links result to destination");
}

void TestVisitorAndParseClone() {
    Runtime runtime;
    runtime.words[0x104cu] = 0x102cu;
    runtime.words[0x1018u] = 0x3000u;
    runtime.words[0x3028u] = 0x3100u;
    runtime.words[0x3128u] = 0u;
    runtime.stop_child = 0x3100u;
    Check(pal4::XmlNodeWorkflow::AcceptVisitor(
              0x1000u, 0x8000u, runtime) == 0xbeefu &&
              runtime.calls == std::vector<std::string>{
                  "read:4172", "enter:32768:4096:0:4",
                  "read:4120", "visit:12288:32768:64", "read:12328",
                  "visit:12544:32768:64", "leave:32768:4096:12"},
          "visitor short-circuits child chain but always calls leave");
    runtime.enter_result = false;
    runtime.words[0x104cu] = 0x5000u;
    runtime.calls.clear();
    Check(pal4::XmlNodeWorkflow::AcceptVisitor(
              0x1000u, 0x8000u, runtime) == 0xbeefu &&
              runtime.calls == std::vector<std::string>{
                  "read:4172", "enter:32768:4096:20480:4",
                  "leave:32768:4096:12"},
          "visitor refusal skips children and forwards attribute head");

    Runtime unavailable;
    unavailable.fail_allocation = true;
    Check(pal4::XmlNodeWorkflow::ParseAndClone(
              0x1000u, unavailable) == 0u &&
              unavailable.calls ==
                  std::vector<std::string>{"allocate:80"},
          "node parse stops on failed 80-byte allocation");
    Runtime parsed;
    parsed.words[0x1020u] = 0x6000u;
    parsed.words[0x100cu] = 0x44u;
    parsed.words[0x104cu] = 0x102cu;
    parsed.words[0x1018u] = 0u;
    Check(pal4::XmlNodeWorkflow::ParseAndClone(
              0x1000u, parsed) == 0x5000u &&
              parsed.words.at(0x5000u) == 0x8499c0u &&
              parsed.words.at(0x500cu) == 0x44u &&
              parsed.calls[0] == "allocate:80" &&
              parsed.calls[1] == "read:4128",
          "node parse builds document from compact name then clones node state");
    Runtime exceptional;
    exceptional.words[0x1020u] = 0x6000u;
    exceptional.throw_on_assign = true;
    bool caught = false;
    try {
        (void)pal4::XmlNodeWorkflow::ParseAndClone(
            0x1000u, exceptional);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && exceptional.calls.back() == "free:20480",
          "failed parse releases newly allocated document storage");

    runtime.enter_result = true;
    runtime.words[0x104cu] = 0x102cu;
    runtime.calls.clear();
    Check(pal4::XmlNodeWorkflow::AcceptVisitor2(
              0x1000u, 0x8000u, runtime) == 0xbeefu &&
              runtime.calls == std::vector<std::string>{
                  "enter:32768:4096:0:8", "read:4120",
                  "visit:12288:32768:64", "read:12328",
                  "visit:12544:32768:64", "leave:32768:4096:16"},
          "second visitor uses enter eight and leave sixteen");
    runtime.enter_result = false;
    runtime.calls.clear();
    Check(pal4::XmlNodeWorkflow::AcceptVisitor2(
              0x1000u, 0x8000u, runtime) == 0xbeefu &&
              runtime.calls == std::vector<std::string>{
                  "enter:32768:4096:0:8", "leave:32768:4096:16"},
          "second visitor refusal skips child reads");
}

void TestFileParserLifecycleAndClone() {
    Runtime runtime;
    Check(pal4::XmlFileParser::Initialize(0x1000u, runtime) ==
              0x1000u && runtime.words.at(0x1000u) == 0x849a10u &&
              runtime.words.at(0x1014u) == 0u &&
              runtime.words.at(0x1034u) ==
                  pal4::CompactStringBuffer::kEmptyBlock &&
              runtime.words.at(0x1038u) == 4u &&
              runtime.words.at(0x103cu) == 0u &&
              runtime.words.at(0x1040u) == 0u &&
              runtime.bytes.at(0x102cu) == 0u &&
              runtime.bytes.at(0x1044u) == 0u &&
              runtime.calls[runtime.calls.size() - 3u] ==
                  "assign:4148:9291648:0",
          "file parser initializes base, error buffer, flags and counters");
    runtime.words[0x1034u] = 0x6000u;
    runtime.words[0x1020u] = 0x6100u;
    runtime.calls.clear();
    pal4::XmlFileParser::Cleanup(0x1000u, runtime);
    std::vector<std::string> frees;
    for (const auto& call : runtime.calls)
        if (call.starts_with("free:")) frees.push_back(call);
    Check(frees == std::vector<std::string>{
              "free:24576", "free:24832"} &&
              runtime.words.at(0x1000u) == 0x8499b4u,
          "file parser frees error block before XML base block");

    Runtime copied;
    copied.words[0x1020u] = 0x5000u;
    copied.words[0x100cu] = 0x44u;
    copied.bytes[0x102cu] = 7u;
    copied.words[0x1034u] = 0x6000u;
    copied.words[0x1018u] = 0u;
    Check(pal4::XmlFileParser::CloneInto(
              0x1000u, 0x2000u, copied) == 0x2034u &&
              copied.words.at(0x200cu) == 0x44u &&
              copied.bytes.at(0x202cu) == 7u &&
              copied.calls[copied.calls.size() - 2u] ==
                  "assign:8244:24584:5",
          "file clone copies base text, byte flag and error string");
    copied.words[0x1018u] = 0x3000u;
    copied.words[0x3028u] = 0u;
    copied.clones[0x3000u] = 0x4000u;
    copied.words[0x4014u] = 1u;
    copied.words[0x201cu] = 0u;
    copied.calls.clear();
    Check(pal4::XmlFileParser::CloneInto(
              0x1000u, 0x2000u, copied) == 0x4000u &&
              copied.words.at(0x2018u) == 0x4000u &&
              copied.words.at(0x4010u) == 0x2000u,
          "file clone attaches virtual child clones");

    Runtime allocated;
    allocated.words[0x1020u] = 0x6000u;
    allocated.words[0x100cu] = 0x44u;
    allocated.bytes[0x102cu] = 9u;
    allocated.words[0x1034u] = 0x6100u;
    allocated.words[0x1018u] = 0u;
    Check(pal4::XmlFileParser::CloneNew(
              0x1000u, allocated) == 0x5000u &&
              allocated.calls.front() == "allocate:72" &&
              allocated.words.at(0x5000u) == 0x849a10u &&
              allocated.bytes.at(0x502cu) == 9u,
          "new file clone allocates 72 bytes and copies source state");
    Runtime unavailable;
    unavailable.fail_allocation = true;
    Check(pal4::XmlFileParser::CloneNew(
              0x1000u, unavailable) == 0u &&
              unavailable.calls ==
                  std::vector<std::string>{"allocate:72"},
          "new file clone returns null on allocation failure");
    Runtime exceptional;
    exceptional.throw_on_assign = true;
    bool caught = false;
    try {
        (void)pal4::XmlFileParser::CloneNew(0x1000u, exceptional);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && exceptional.calls.back() == "free:20480",
          "failed file parser construction releases new storage");
}

}  // namespace

int main() {
    TestParseAndCleanup();
    TestAttributeReadAndWrite();
    TestAttributeCleanup();
    TestAttributeRootLinks();
    TestCloneInto();
    TestVisitorAndParseClone();
    TestFileParserLifecycleAndClone();
}
