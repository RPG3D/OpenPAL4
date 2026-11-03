#include "pal4/pal_script_tree.h"

#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalScriptTreeClearRuntime,
                       pal4::IPalScriptTreeInsertRuntime {
    static constexpr std::uint32_t tree = 0x2000u;
    static constexpr std::uint32_t header = 0x4000u;
    static constexpr std::uint32_t nil = 0x3000u;
    static constexpr std::uint32_t output = 0x6000u;

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> allocations;
    std::size_t allocation_index = 0u;
    std::uint32_t first_element_calls = 0u;
    std::uint32_t nil_value = nil;

    Runtime() {
        words[tree + 4u] = header;
        words[tree + 12u] = 0u;
        words[header] = header;
        words[header + 4u] = nil;
        words[header + 8u] = header;
        words[nil] = nil;
        words[nil + 4u] = header;
        words[nil + 8u] = nil;
        words[nil + 20u] = 1u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(words.contains(address), "script tree reads mapped word");
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        Check(bytes.contains(address), "script tree reads mapped byte");
        return bytes.at(address);
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t SharedNil() override { return nil_value; }
    void FreeMemory(const std::uint32_t address) override {
        freed.push_back(address);
    }
    void FirstElement(const std::uint32_t address) override {
        Check(address == tree, "first-element callback receives tree");
        ++first_element_calls;
    }
    std::uint32_t TemporaryCursor() override { return 0x7000u; }
    std::uint32_t ReleaseMemory(const std::uint32_t address) override {
        freed.push_back(address);
        return address + 1u;
    }
    std::uint32_t AllocateMemory(const std::uint32_t size) override {
        Check(size == 24u && allocation_index < allocations.size(),
              "script tree allocates 24-byte nodes");
        return allocations[allocation_index++];
    }
    void Node(const std::uint32_t address, const std::uint32_t left,
              const std::uint32_t parent, const std::uint32_t right,
              const std::uint32_t colour) {
        words[address] = left;
        words[address + 4u] = parent;
        words[address + 8u] = right;
        words[address + 20u] = colour;
        ++words[tree + 12u];
    }
};

void TestOnlyNode() {
    Runtime runtime;
    runtime.Node(0x5000u, runtime.nil, runtime.header,
                 runtime.nil, 1u);
    runtime.words[runtime.header] = 0x5000u;
    runtime.words[runtime.header + 4u] = 0x5000u;
    runtime.words[runtime.header + 8u] = 0x5000u;
    Check(pal4::PalScriptTree::RemoveNode(
              runtime.tree, runtime.output, 0x5000u, runtime) ==
              runtime.output &&
              runtime.words[runtime.output] == runtime.header &&
              runtime.words[runtime.tree + 12u] == 0u &&
              runtime.words[runtime.header] == runtime.header &&
              runtime.words[runtime.header + 4u] == runtime.nil &&
              runtime.words[runtime.header + 8u] == runtime.header &&
              runtime.freed == std::vector<std::uint32_t>{0x5000u},
          "last node removal resets header and writes next iterator");
}

void TestPkNodeTagCopy() {
    pal4::PkScriptNodeTag tag{0xffffffffu, 0xffu};
    auto& result = pal4::PalScriptTree::CopyPkNodeTag(
        tag, 0x12345678u, 0x5au);
    Check(&result == &tag && tag.value == 0x12345678u &&
              tag.flag == 0x5au,
          "PK tagged node data copies word and trailing flag");
}

void TestPkNodeUsesRuntimeNil() {
    Runtime runtime;
    runtime.nil_value = 0x9000u;
    runtime.words[0x9000u] = 0x9000u;
    runtime.words[0x9004u] = runtime.header;
    runtime.words[0x9008u] = 0x9000u;
    runtime.words[0x9014u] = 1u;
    runtime.Node(0x5000u, runtime.nil_value, runtime.header,
                 runtime.nil_value, 1u);
    runtime.words[runtime.header] = 0x5000u;
    runtime.words[runtime.header + 4u] = 0x5000u;
    runtime.words[runtime.header + 8u] = 0x5000u;
    Check(pal4::PalScriptTree::RemovePkNode(
              runtime.tree, runtime.output, 0x5000u, runtime) ==
              runtime.output &&
              runtime.words[runtime.output] == runtime.header &&
              runtime.words[runtime.header + 4u] == runtime.nil_value &&
              runtime.words[runtime.tree + 12u] == 0u &&
              runtime.freed == std::vector<std::uint32_t>{0x5000u},
          "PK delete uses its own shared nil and generic 24-byte node repair");
}

void TestSuccessorTransplant() {
    Runtime runtime;
    runtime.Node(0x5000u, 0x5100u, runtime.header, 0x5200u, 1u);
    runtime.Node(0x5100u, runtime.nil, 0x5000u, runtime.nil, 0u);
    runtime.Node(0x5200u, runtime.nil, 0x5000u, runtime.nil, 0u);
    runtime.words[runtime.header] = 0x5100u;
    runtime.words[runtime.header + 4u] = 0x5000u;
    runtime.words[runtime.header + 8u] = 0x5200u;
    Check(pal4::PalScriptTree::RemoveNode(
              runtime.tree, runtime.output, 0x5000u, runtime) ==
              runtime.output &&
              runtime.words[runtime.output] == 0x5200u &&
              runtime.words[runtime.header + 4u] == 0x5200u &&
              runtime.words[0x5200u + 20u] == 1u &&
              runtime.words[runtime.tree + 12u] == 2u &&
              runtime.freed == std::vector<std::uint32_t>{0x5000u},
          "two-child removal transplants successor and preserves colour");
}

void TestSuccessorIterator() {
    Runtime runtime;
    runtime.Node(0x5000u, 0x5100u, runtime.header, 0x5200u, 1u);
    runtime.Node(0x5100u, runtime.nil, 0x5000u, runtime.nil, 0u);
    runtime.Node(0x5200u, 0x5300u, 0x5000u, runtime.nil, 1u);
    runtime.Node(0x5300u, runtime.nil, 0x5200u, runtime.nil, 0u);
    runtime.words[runtime.header] = 0x5100u;
    runtime.words[runtime.header + 4u] = 0x5000u;
    runtime.words[runtime.header + 8u] = 0x5200u;
    runtime.words[0x7000u] = 0x5000u;
    Check(pal4::PalScriptTree::NextNode(0x7000u, runtime) == runtime.nil &&
              runtime.words[0x7000u] == 0x5300u,
          "right-subtree successor returns leftmost node and nil residue");

    runtime.words[0x7000u] = 0x5100u;
    Check(pal4::PalScriptTree::NextNode(0x7000u, runtime) == 0x5000u &&
              runtime.words[0x7000u] == 0x5000u,
          "left child without right subtree advances to parent");

    runtime.words[0x7000u] = 0x5200u;
    Check(pal4::PalScriptTree::NextNode(0x7000u, runtime) ==
              runtime.header &&
              runtime.words[0x7000u] == runtime.header,
          "maximum node advances to header after ancestor walk");
}

void TestRecursiveFree() {
    Runtime runtime;
    Check(pal4::PalScriptTree::FreeNodesRecursive(
              runtime.nil, runtime) == runtime.nil && runtime.freed.empty(),
          "empty recursive free returns shared nil");

    runtime.Node(0x5000u, 0x5100u, runtime.header, 0x5200u, 1u);
    runtime.Node(0x5100u, runtime.nil, 0x5000u, runtime.nil, 1u);
    runtime.Node(0x5200u, 0x5300u, 0x5000u, 0x5400u, 1u);
    runtime.Node(0x5300u, runtime.nil, 0x5200u, runtime.nil, 1u);
    runtime.Node(0x5400u, runtime.nil, 0x5200u, runtime.nil, 1u);
    Check(pal4::PalScriptTree::FreeNodesRecursive(
              0x5000u, runtime) == runtime.nil &&
              runtime.freed == std::vector<std::uint32_t>{
                  0x5400u, 0x5200u, 0x5300u, 0x5000u, 0x5100u},
          "recursive free visits right subtrees before left spine");
}

void TestPkRecursiveFree() {
    Runtime runtime;
    runtime.nil_value = 0x9000u;
    Check(pal4::PalScriptTree::FreePkNodesRecursive(
              runtime.nil_value, runtime) == runtime.nil_value &&
              runtime.freed.empty(),
          "empty PK subtree returns the PK shared nil");

    runtime.Node(0x5000u, 0x5100u, runtime.header, 0x5200u, 1u);
    runtime.Node(0x5100u, runtime.nil_value, 0x5000u,
                 runtime.nil_value, 1u);
    runtime.Node(0x5200u, runtime.nil_value, 0x5000u,
                 runtime.nil_value, 1u);
    Check(pal4::PalScriptTree::FreePkNodesRecursive(
              0x5000u, runtime) == runtime.nil_value &&
              runtime.freed == std::vector<std::uint32_t>{
                  0x5200u, 0x5000u, 0x5100u},
          "PK recursive free uses its nil and right-first release order");
}

void TestInsertionAndRotation() {
    Runtime runtime;
    runtime.allocations = {0x5000u, 0x5100u, 0x5200u};
    runtime.words[0x8000u] = 3u;
    runtime.words[0x8004u] = 0x9000u;
    runtime.words[0x8010u] = 1u;
    runtime.words[0x8014u] = 0x9001u;
    runtime.words[0x8020u] = 2u;
    runtime.words[0x8024u] = 0x9002u;

    Check(pal4::PalScriptTree::InsertNode(
              runtime.tree, runtime.output, runtime.nil,
              runtime.header, 0x8000u, runtime) == runtime.output &&
              runtime.words[runtime.output] == 0x5000u &&
              runtime.words[runtime.header + 4u] == 0x5000u &&
              runtime.words[runtime.header] == 0x5000u &&
              runtime.words[runtime.header + 8u] == 0x5000u &&
              runtime.words[0x500cu] == 3u &&
              runtime.words[0x5010u] == 0x9000u &&
              runtime.words[0x5014u] == 1u,
          "empty script tree inserts pair and blackens root");

    (void)pal4::PalScriptTree::InsertNode(
        runtime.tree, runtime.output, runtime.nil,
        0x5000u, 0x8010u, runtime);
    (void)pal4::PalScriptTree::InsertNode(
        runtime.tree, runtime.output, runtime.nil,
        0x5100u, 0x8020u, runtime);
    Check(runtime.words[runtime.output] == 0x5200u &&
              runtime.words[runtime.tree + 12u] == 3u &&
              runtime.words[runtime.header + 4u] == 0x5200u &&
              runtime.words[runtime.header] == 0x5100u &&
              runtime.words[runtime.header + 8u] == 0x5000u &&
              runtime.words[0x5200u] == 0x5100u &&
              runtime.words[0x5208u] == 0x5000u &&
              runtime.words[0x520cu] == 2u &&
              runtime.words[0x5210u] == 0x9002u &&
              runtime.words[0x5214u] == 1u,
          "left-right insertion rotates and preserves extrema and payload");
}

void TestPkInsertionUnsignedAndRotation() {
    Runtime runtime;
    runtime.nil_value = 0x9000u;
    runtime.words[0x9014u] = 1u;
    runtime.words[runtime.header + 4u] = runtime.nil_value;
    runtime.allocations = {0x5000u, 0x5100u, 0x5200u};
    runtime.words[0x8000u] = 0x80000000u;
    runtime.words[0x8010u] = 1u;
    runtime.words[0x8020u] = 2u;
    runtime.words[0x8004u] = 0x1234565au;
    runtime.words[0x8014u] = 0xabcdefa5u;
    runtime.words[0x8024u] = 0x7654323cu;

    Check(pal4::PalScriptTree::InsertPkNode(
              runtime.tree, runtime.output, runtime.nil_value,
              runtime.header, 0x8000u, runtime) == runtime.output &&
              runtime.words[runtime.output] == 0x5000u &&
              runtime.words[runtime.header + 4u] == 0x5000u &&
              runtime.words[0x500cu] == 0x80000000u &&
              runtime.words[0x5010u] == 0x1234565au &&
              runtime.words[0x5014u] == 1u,
          "first PK insertion copies full handle and blackens root");

    (void)pal4::PalScriptTree::InsertPkNode(
        runtime.tree, runtime.output, runtime.nil_value,
        0x5000u, 0x8010u, runtime);
    Check(runtime.words[0x5000u] == 0x5100u &&
              runtime.words[0x5008u] == runtime.nil_value &&
              runtime.words[runtime.header] == 0x5100u &&
              runtime.words[0x5110u] == 0xabcdefa5u,
          "unsigned PK comparison puts key one left of 0x80000000");

    (void)pal4::PalScriptTree::InsertPkNode(
        runtime.tree, runtime.output, runtime.nil_value,
        0x5100u, 0x8020u, runtime);
    Check(runtime.words[runtime.tree + 12u] == 3u &&
              runtime.words[runtime.header + 4u] == 0x5200u &&
              runtime.words[runtime.header] == 0x5100u &&
              runtime.words[runtime.header + 8u] == 0x5000u &&
              runtime.words[0x5200u] == 0x5100u &&
              runtime.words[0x5208u] == 0x5000u &&
              runtime.words[0x520cu] == 2u &&
              runtime.words[0x5210u] == 0x7654323cu &&
              runtime.words[0x5214u] == 1u,
          "PK left-right insertion rebalances while retaining unsigned order");
}

void TestPkInsertionMirrorAndRecolour() {
    Runtime runtime;
    runtime.nil_value = 0x9000u;
    runtime.words[0x9014u] = 1u;
    runtime.words[runtime.header + 4u] = runtime.nil_value;
    runtime.allocations = {0x5000u, 0x5100u, 0x5200u, 0x5300u};
    runtime.words[0x8000u] = 1u;
    runtime.words[0x8010u] = 3u;
    runtime.words[0x8020u] = 2u;
    runtime.words[0x8030u] = 4u;
    for (const auto address : {0x8004u, 0x8014u, 0x8024u, 0x8034u})
        runtime.words[address] = 0u;

    (void)pal4::PalScriptTree::InsertPkNode(
        runtime.tree, runtime.output, runtime.nil_value,
        runtime.header, 0x8000u, runtime);
    (void)pal4::PalScriptTree::InsertPkNode(
        runtime.tree, runtime.output, runtime.nil_value,
        0x5000u, 0x8010u, runtime);
    (void)pal4::PalScriptTree::InsertPkNode(
        runtime.tree, runtime.output, runtime.nil_value,
        0x5100u, 0x8020u, runtime);
    Check(runtime.words[runtime.header + 4u] == 0x5200u &&
              runtime.words[0x5200u] == 0x5000u &&
              runtime.words[0x5208u] == 0x5100u &&
              runtime.words[0x5214u] == 1u,
          "PK right-left insertion performs mirrored rotations");

    (void)pal4::PalScriptTree::InsertPkNode(
        runtime.tree, runtime.output, runtime.nil_value,
        0x5100u, 0x8030u, runtime);
    Check(runtime.words[runtime.header + 4u] == 0x5200u &&
              runtime.words[0x5014u] == 1u &&
              runtime.words[0x5114u] == 1u &&
              runtime.words[0x5214u] == 1u &&
              runtime.words[0x5314u] == 0u &&
              runtime.words[runtime.header + 8u] == 0x5300u &&
              runtime.words[runtime.tree + 12u] == 4u,
          "PK insertion recolours both sides and retains the root black");
}

void TestPkLowerBoundUnsigned() {
    Runtime runtime;
    runtime.nil_value = 0x9000u;
    runtime.Node(0x5000u, 0x5100u, runtime.header, 0x5200u, 1u);
    runtime.Node(0x5100u, runtime.nil_value, 0x5000u,
                 runtime.nil_value, 1u);
    runtime.Node(0x5200u, runtime.nil_value, 0x5000u,
                 runtime.nil_value, 1u);
    runtime.words[runtime.header + 4u] = 0x5000u;
    runtime.words[0x500cu] = 0x80000000u;
    runtime.words[0x510cu] = 1u;
    runtime.words[0x520cu] = 0xfffffffeu;
    Check(pal4::PalScriptTree::LowerBoundPk(
              runtime.tree, 0u, runtime) == 0x5100u &&
              pal4::PalScriptTree::LowerBoundPk(
                  runtime.tree, 1u, runtime) == 0x5100u &&
              pal4::PalScriptTree::LowerBoundPk(
                  runtime.tree, 2u, runtime) == 0x5000u &&
              pal4::PalScriptTree::LowerBoundPk(
                  runtime.tree, 0x80000000u, runtime) == 0x5000u &&
              pal4::PalScriptTree::LowerBoundPk(
                  runtime.tree, 0x80000001u, runtime) == 0x5200u &&
              pal4::PalScriptTree::LowerBoundPk(
                  runtime.tree, 0xffffffffu, runtime) == runtime.header,
          "PK lower bound uses unsigned keys and returns header on miss");
}

void TestPkPredecessor() {
    Runtime runtime;
    runtime.nil_value = 0x9000u;
    runtime.Node(0x5000u, 0x5100u, runtime.header, 0x5200u, 1u);
    runtime.Node(0x5100u, runtime.nil_value, 0x5000u,
                 runtime.nil_value, 0u);
    runtime.Node(0x5200u, runtime.nil_value, 0x5000u,
                 runtime.nil_value, 0u);
    runtime.words[runtime.header + 4u] = 0x5000u;
    runtime.words[runtime.header + 8u] = 0x5200u;
    runtime.words[runtime.header + 20u] = 0u;
    runtime.words[0x7000u] = runtime.header;
    Check(pal4::PalScriptTree::PreviousPkNode(
              0x7000u, runtime) == 0x5200u &&
              runtime.words[0x7000u] == 0x5200u,
          "PK header predecessor selects maximum node");

    runtime.words[0x7000u] = 0x5000u;
    Check(pal4::PalScriptTree::PreviousPkNode(
              0x7000u, runtime) == runtime.nil_value &&
              runtime.words[0x7000u] == 0x5100u,
          "PK predecessor walks left subtree using PK nil");

    runtime.words[0x7000u] = 0x5200u;
    Check(pal4::PalScriptTree::PreviousPkNode(
              0x7000u, runtime) == 0x5000u &&
              runtime.words[0x7000u] == 0x5000u,
          "PK predecessor climbs to parent without left child");
}

void TestPkSuccessor() {
    Runtime runtime;
    runtime.nil_value = 0x9000u;
    runtime.Node(0x5000u, 0x5100u, runtime.header, 0x5200u, 1u);
    runtime.Node(0x5100u, runtime.nil_value, 0x5000u,
                 runtime.nil_value, 0u);
    runtime.Node(0x5200u, runtime.nil_value, 0x5000u,
                 runtime.nil_value, 0u);
    runtime.words[runtime.header + 4u] = 0x5000u;
    runtime.words[runtime.header + 8u] = 0x5200u;
    runtime.words[0x7000u] = 0x5000u;
    Check(pal4::PalScriptTree::NextPkNode(
              0x7000u, runtime) == runtime.nil_value &&
              runtime.words[0x7000u] == 0x5200u,
          "PK successor walks right subtree using PK nil");

    runtime.words[0x7000u] = 0x5100u;
    Check(pal4::PalScriptTree::NextPkNode(
              0x7000u, runtime) == 0x5000u &&
              runtime.words[0x7000u] == 0x5000u,
          "PK successor climbs from left child to parent");

    runtime.words[0x7000u] = 0x5200u;
    Check(pal4::PalScriptTree::NextPkNode(
              0x7000u, runtime) == runtime.header &&
              runtime.words[0x7000u] == runtime.header,
          "PK maximum successor becomes header");
}

void TestLowerBoundAndPrevious() {
    Runtime runtime;
    runtime.Node(0x5000u, 0x5100u, runtime.header, 0x5200u, 1u);
    runtime.Node(0x5100u, runtime.nil, 0x5000u, runtime.nil, 0u);
    runtime.Node(0x5200u, runtime.nil, 0x5000u, runtime.nil, 0u);
    runtime.words[runtime.header] = 0x5100u;
    runtime.words[runtime.header + 4u] = 0x5000u;
    runtime.words[runtime.header + 8u] = 0x5200u;
    runtime.words[runtime.header + 20u] = 0u;
    runtime.words[0x500cu] = 0u;
    runtime.words[0x510cu] = 0xfffffffeu;
    runtime.words[0x520cu] = 5u;

    for (const auto& [key, expected] : std::vector<std::pair<
             std::uint32_t, std::uint32_t>>{
             {0xfffffffdu, 0x5100u}, {0xfffffffeu, 0x5100u},
             {0xffffffffu, 0x5000u}, {0u, 0x5000u},
             {1u, 0x5200u}, {5u, 0x5200u},
             {6u, runtime.header}}) {
        Check(pal4::PalScriptTree::LowerBound(
                  runtime.tree, key, runtime) == expected,
              "signed lower-bound selection or header miss");
    }

    runtime.words[0x7000u] = runtime.header;
    Check(pal4::PalScriptTree::PreviousNode(
              0x7000u, runtime) == 0x5200u &&
              runtime.words[0x7000u] == 0x5200u,
          "header predecessor returns maximum node");

    runtime.words[0x7000u] = 0x5000u;
    Check(pal4::PalScriptTree::PreviousNode(
              0x7000u, runtime) == runtime.nil &&
              runtime.words[0x7000u] == 0x5100u,
          "left-subtree predecessor returns rightmost and nil residue");

    runtime.words[0x7000u] = 0x5200u;
    Check(pal4::PalScriptTree::PreviousNode(
              0x7000u, runtime) == 0x5000u &&
              runtime.words[0x7000u] == 0x5000u,
          "right child without left subtree advances to parent");
}

void TestClearRange() {
    Runtime empty;
    Check(pal4::PalScriptTree::ClearRange(
              empty.tree, empty.output, empty.header,
              empty.header, empty) == empty.output &&
              empty.words[empty.output] == empty.header &&
              empty.freed.empty(),
          "empty interval preserves the header");

    Runtime full;
    full.Node(0x5000u, 0x5100u, full.header, 0x5200u, 1u);
    full.Node(0x5100u, full.nil, 0x5000u, full.nil, 0u);
    full.Node(0x5200u, full.nil, 0x5000u, full.nil, 0u);
    full.words[full.header] = 0x5100u;
    full.words[full.header + 4u] = 0x5000u;
    full.words[full.header + 8u] = 0x5200u;
    Check(pal4::PalScriptTree::ClearRange(
              full.tree, full.output, 0x5100u, full.header,
              full) == full.output &&
              full.freed == std::vector<std::uint32_t>{
                  0x5200u, 0x5000u, 0x5100u} &&
              full.words[full.tree + 12u] == 0u &&
              full.words[full.header] == full.header &&
              full.words[full.header + 4u] == full.nil &&
              full.words[full.header + 8u] == full.header &&
              full.words[full.output] == full.header,
          "whole-tree interval frees right subtrees then left spine");

    Runtime partial;
    partial.Node(0x5000u, 0x5100u, partial.header,
                 partial.nil, 1u);
    partial.Node(0x5100u, partial.nil, 0x5000u,
                 partial.nil, 0u);
    partial.words[partial.header] = 0x5100u;
    partial.words[partial.header + 4u] = 0x5000u;
    partial.words[partial.header + 8u] = 0x5000u;
    Check(pal4::PalScriptTree::ClearRange(
              partial.tree, partial.output, 0x5100u,
              0x5000u, partial) == partial.output &&
              partial.freed == std::vector<std::uint32_t>{0x5100u} &&
              partial.words[partial.tree + 12u] == 1u &&
              partial.words[partial.header] == 0x5000u &&
              partial.words[partial.output] == 0x5000u,
          "partial interval deletes only nodes preceding the bound");
}

void TestLeftmostNode() {
    Runtime runtime;
    runtime.Node(0x5000u, runtime.nil, runtime.header,
                 runtime.nil, 1u);
    Check(pal4::PalScriptTree::LeftmostNode(
              0x5000u, runtime) == 0x5000u,
          "root with nil left child is the leftmost node");
    runtime.Node(0x5100u, 0x5200u, 0x5000u,
                 runtime.nil, 0u);
    runtime.Node(0x5200u, runtime.nil, 0x5100u,
                 runtime.nil, 1u);
    runtime.words[0x5000u] = 0x5100u;
    Check(pal4::PalScriptTree::LeftmostNode(
              0x5000u, runtime) == 0x5200u,
          "leftmost traversal follows a multi-node left chain");
}

void TestClearAll() {
    Runtime full;
    full.Node(0x5000u, full.nil, full.header, full.nil, 1u);
    full.words[full.header] = 0x5000u;
    full.words[full.header + 4u] = 0x5000u;
    full.words[full.header + 8u] = 0x5000u;
    std::uint32_t count = 2u;
    std::uint32_t shared = full.nil;
    Check(pal4::PalScriptTree::ClearAll(
              full.tree, count, shared, full) == 1u &&
              count == 1u && shared == full.nil &&
              full.freed == std::vector<std::uint32_t>{
                  0x5000u, full.header} &&
              full.first_element_calls == 1u &&
              full.words[full.tree + 4u] == 0u &&
              full.words[full.tree + 12u] == 0u,
          "counted tree frees nodes and header while retaining shared nil");

    Runtime empty;
    count = 1u;
    shared = 0u;
    Check(pal4::PalScriptTree::ClearAll(
              empty.tree, count, shared, empty) == 0u &&
              count == 0u &&
              empty.freed == std::vector<std::uint32_t>{empty.header} &&
              empty.first_element_calls == 0u,
          "last empty tree skips null shared nil release");

    Runtime residual;
    residual.Node(0x5000u, residual.nil, residual.header,
                  residual.nil, 1u);
    residual.words[residual.tree + 12u] = 0u;
    residual.words[residual.header] = 0x5000u;
    residual.words[residual.header + 4u] = 0x5000u;
    residual.words[residual.header + 8u] = 0x5000u;
    count = 1u;
    shared = residual.nil;
    Check(pal4::PalScriptTree::ClearAll(
              residual.tree, count, shared, residual) ==
              residual.nil + 1u &&
              count == 0u && shared == 0u &&
              residual.freed == std::vector<std::uint32_t>{
                  0x5000u, residual.header, residual.nil} &&
              residual.first_element_calls == 0u,
          "zero-count residual node uses iterator removal before final nil free");
}

}  // namespace

int main() {
    TestOnlyNode();
    TestPkNodeTagCopy();
    TestPkNodeUsesRuntimeNil();
    TestSuccessorTransplant();
    TestSuccessorIterator();
    TestRecursiveFree();
    TestPkRecursiveFree();
    TestInsertionAndRotation();
    TestPkInsertionUnsignedAndRotation();
    TestPkInsertionMirrorAndRecolour();
    TestPkLowerBoundUnsigned();
    TestPkPredecessor();
    TestPkSuccessor();
    TestLowerBoundAndPrevious();
    TestClearRange();
    TestLeftmostNode();
    TestClearAll();
}
