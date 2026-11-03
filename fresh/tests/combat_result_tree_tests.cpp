#include "pal4/combat_result_tree.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kNil = 0xffffffffu;
constexpr std::uint32_t kTree = 0x1000u;
constexpr std::uint32_t kHead = 0x2000u;
constexpr std::uint32_t kSource = 0x3000u;
constexpr std::uint32_t kSourceHead = 0x4000u;

struct Runtime final : pal4::ICombatResultTreeRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kTree + 4u, kHead}, {kTree + 12u, 0u},
        {kHead, kHead}, {kHead + 4u, kNil},
        {kHead + 8u, kHead}, {kNil + 20u, 1u},
        {kSource + 4u, kSourceHead},
        {kSource + 12u, 0u}, {kSourceHead, kSourceHead},
        {kSourceHead + 4u, kNil}, {kSourceHead + 8u, kSourceHead}};
    std::unordered_map<std::uint32_t, std::uint8_t> bytes{
        {kSource + 1u, 7u}, {kTree + 1u, 0u}};
    std::vector<std::string> calls;
    std::uint32_t freed = 0u;
    std::vector<std::uint32_t> freed_nodes;
    std::uint32_t copied = 0u;
    std::uint32_t next_tree_node = 0x6100u;

    std::uint32_t Nil() override { return kNil; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.count(address) != 0u, "unexpected tree word read");
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(bytes.count(address) != 0u, "unexpected tree byte read");
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte");
    }
    void Free(std::uint32_t node) override {
        freed = node;
        freed_nodes.push_back(node);
        calls.emplace_back("free");
    }
    std::uint32_t CreateNode(std::uint32_t parent,
                              std::uint32_t value) override {
        Check(parent == kHead && value == 77u,
              "copied node parent and value");
        words[0x6000u] = kNil;
        words[0x6004u] = parent;
        words[0x6008u] = kNil;
        calls.emplace_back("create");
        return 0x6000u;
    }
    void CopyData(std::uint32_t destination,
                  std::uint32_t source) override {
        Check(destination == 0x600cu && source == 0x500cu,
              "node payload copy addresses");
        ++copied;
        calls.emplace_back("copy-data");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 16u || bytes == 24u, "tree allocation size");
        calls.emplace_back("allocate");
        if (bytes == 16u)
            return 0x8000u;
        const auto node = next_tree_node;
        next_tree_node += 0x100u;
        return node;
    }
};

void SetSingleNode(Runtime& runtime) {
    runtime.words[kTree + 12u] = 1u;
    runtime.words[kHead] = 0x5000u;
    runtime.words[kHead + 4u] = 0x5000u;
    runtime.words[kHead + 8u] = 0x5000u;
    runtime.words[0x5000u] = kNil;
    runtime.words[0x5004u] = kHead;
    runtime.words[0x5008u] = kNil;
    runtime.words[0x5014u] = 0u;
}

void SetTwoNodes(Runtime& runtime) {
    SetSingleNode(runtime);
    runtime.words[kTree + 12u] = 2u;
    runtime.words[kHead + 8u] = 0x5100u;
    runtime.words[0x5008u] = 0x5100u;
    runtime.words[0x5014u] = 1u;
    runtime.words[0x5100u] = kNil;
    runtime.words[0x5104u] = 0x5000u;
    runtime.words[0x5108u] = kNil;
    runtime.words[0x5114u] = 0u;
}

void SetNode(Runtime& runtime, std::uint32_t address,
             std::uint32_t left, std::uint32_t parent,
             std::uint32_t right, std::uint32_t color) {
    runtime.words[address] = left;
    runtime.words[address + 4u] = parent;
    runtime.words[address + 8u] = right;
    runtime.words[address + 20u] = color;
}

void AssignSelfAndEmpty() {
    Runtime runtime;
    Check(pal4::CombatResultTree::Assign(kTree, kTree, runtime) == kTree &&
              runtime.calls.empty(),
          "self assignment is untouched");
    Check(pal4::CombatResultTree::Assign(
              kTree, kSource, runtime) == kTree &&
              runtime.words.at(kHead) == kHead &&
              runtime.words.at(kHead + 4u) == kNil &&
              runtime.words.at(kHead + 8u) == kHead &&
              runtime.bytes.at(kTree + 1u) == 7u,
          "empty source keeps destination header self linked");
}

void AssignOneNode() {
    Runtime runtime;
    runtime.words[kSource + 12u] = 1u;
    runtime.words[kSourceHead] = 0x5000u;
    runtime.words[kSourceHead + 4u] = 0x5000u;
    runtime.words[kSourceHead + 8u] = 0x5000u;
    runtime.words[0x5000u] = kNil;
    runtime.words[0x5008u] = kNil;
    runtime.words[0x5014u] = 77u;
    Check(pal4::CombatResultTree::Assign(
              kTree, kSource, runtime) == kTree &&
              runtime.words.at(kHead) == 0x6000u &&
              runtime.words.at(kHead + 4u) == 0x6000u &&
              runtime.words.at(kHead + 8u) == 0x6000u &&
              runtime.words.at(kTree + 12u) == 1u &&
              runtime.words.at(0x6000u) == kNil &&
              runtime.copied == 1u,
          "source node copies and rebuilds all header links");
    Check(runtime.calls.at(1) == "create" &&
              runtime.calls.at(4) == "copy-data",
          "node creation precedes payload copy");
}

void RecursiveCopy() {
    Runtime empty;
    Check(pal4::CombatResultTree::CopyTree(kNil, 0x7000u, empty) == kNil &&
              empty.words.at(0x7000u) == kNil &&
              empty.next_tree_node == 0x6100u,
          "empty recursive copy terminates parent left link");

    Runtime runtime;
    SetNode(runtime, 0x5000u, 0x5100u, 0x7000u, 0x5200u, 1u);
    SetNode(runtime, 0x5100u, kNil, 0x5000u, kNil, 0u);
    SetNode(runtime, 0x5200u, 0x5300u, 0x5000u, kNil, 0u);
    SetNode(runtime, 0x5300u, kNil, 0x5200u, kNil, 1u);
    for (const auto node : {0x5000u, 0x5100u, 0x5200u, 0x5300u}) {
        runtime.words[node + 12u] = node + 1u;
        runtime.words[node + 16u] = node + 2u;
    }
    Check(pal4::CombatResultTree::CopyTree(0x5000u, 0x7000u, runtime) ==
                  0x6100u &&
              runtime.words.at(0x7000u) == 0x6100u &&
              runtime.words.at(0x6100u) == 0x6400u &&
              runtime.words.at(0x6108u) == 0x6200u &&
              runtime.words.at(0x6200u) == 0x6300u &&
              runtime.words.at(0x6300u) == kNil &&
              runtime.words.at(0x6400u) == kNil &&
              runtime.words.at(0x6104u) == 0x7000u &&
              runtime.words.at(0x6204u) == 0x6100u &&
              runtime.words.at(0x6304u) == 0x6200u &&
              runtime.words.at(0x6404u) == 0x6100u &&
              runtime.words.at(0x6114u) == 1u &&
              runtime.words.at(0x6314u) == 1u &&
              runtime.words.at(0x610cu) == 0x5001u &&
              runtime.words.at(0x6110u) == 0x5002u &&
              runtime.words.at(0x620cu) == 0x5201u &&
              runtime.next_tree_node == 0x6500u,
          "recursive copy preserves topology, parents, colors and payload");
}

void ClearAndEraseRange() {
    Runtime runtime;
    SetSingleNode(runtime);
    Check(pal4::CombatResultTree::Assign(
              kTree, kSource, runtime) == kTree &&
              runtime.freed == 0x5000u &&
              runtime.words.at(kTree + 12u) == 0u &&
              runtime.words.at(kHead) == kHead,
          "assignment clears populated destination");
    Check(runtime.freed_nodes == std::vector<std::uint32_t>{0x5000u},
          "destination node is freed");

    Runtime erase;
    SetSingleNode(erase);
    Check(pal4::CombatResultTree::EraseRange(
              kTree, 0x9000u, 0x5000u, kHead, erase) == 0x9000u &&
              erase.freed == 0x5000u &&
              erase.words.at(0x9000u) == kHead &&
              erase.words.at(kTree + 12u) == 0u,
          "whole-range erase resets header and result iterator");

    Runtime partial;
    SetTwoNodes(partial);
    Check(pal4::CombatResultTree::EraseRange(
              kTree, 0x9000u, 0x5000u, 0x5000u,
              partial) == 0x9000u &&
              partial.freed == 0u &&
              partial.words.at(0x9000u) == 0x5000u,
          "empty range does not erase");
    Check(pal4::CombatResultTree::EraseRange(
              kTree, 0x9000u, 0x5100u, kHead,
              partial) == 0x9000u &&
              partial.freed == 0x5100u &&
              partial.words.at(0x9000u) == kHead &&
              partial.words.at(kTree + 12u) == 1u &&
              partial.words.at(0x5008u) == kNil,
          "generic range erases rightmost node after finding successor");
}

void EraseNodeTransplants() {
    Runtime red_leaf;
    SetTwoNodes(red_leaf);
    Check(pal4::CombatResultTree::EraseNode(
              kTree, 0x9000u, 0x5100u, red_leaf) == 0x9000u &&
              red_leaf.words.at(0x9000u) == kHead &&
              red_leaf.words.at(kTree + 12u) == 1u &&
              red_leaf.words.at(kHead + 8u) == 0x5000u &&
              red_leaf.freed == 0x5100u,
          "red leaf deletion relinks rightmost header");

    Runtime black_root;
    SetSingleNode(black_root);
    black_root.words[0x5014u] = 1u;
    Check(pal4::CombatResultTree::EraseNode(
              kTree, 0x9000u, 0x5000u, black_root) == 0x9000u &&
              black_root.words.at(0x9000u) == kHead &&
              black_root.words.at(kTree + 12u) == 0u &&
              black_root.words.at(kHead + 4u) == kNil &&
              black_root.words.at(kHead) == kHead &&
              black_root.words.at(kHead + 8u) == kHead &&
              black_root.words.at(kNil + 20u) == 1u,
          "last black node resets header and sentinel color");

    Runtime two_children;
    SetTwoNodes(two_children);
    two_children.words[kTree + 12u] = 3u;
    two_children.words[kHead] = 0x5200u;
    two_children.words[0x5000u] = 0x5200u;
    two_children.words[0x5200u] = kNil;
    two_children.words[0x5204u] = 0x5000u;
    two_children.words[0x5208u] = kNil;
    two_children.words[0x5214u] = 0u;
    Check(pal4::CombatResultTree::EraseNode(
              kTree, 0x9000u, 0x5000u, two_children) == 0x9000u &&
              two_children.words.at(0x9000u) == 0x5100u &&
              two_children.words.at(kHead + 4u) == 0x5100u &&
              two_children.words.at(0x5100u) == 0x5200u &&
              two_children.words.at(0x5204u) == 0x5100u &&
              two_children.words.at(kNil + 4u) == 0x5100u &&
              two_children.words.at(0x5114u) == 1u &&
              two_children.words.at(kTree + 12u) == 2u &&
              two_children.freed == 0x5000u,
          "two-child deletion transplants successor and swaps color");
}

void EraseNodeRebalances() {
    Runtime left_leaf;
    SetSingleNode(left_leaf);
    left_leaf.words[kTree + 12u] = 4u;
    left_leaf.words[kHead] = 0x5100u;
    left_leaf.words[kHead + 8u] = 0x5300u;
    left_leaf.words[0x5000u] = 0x5100u;
    left_leaf.words[0x5008u] = 0x5200u;
    left_leaf.words[0x5014u] = 1u;
    left_leaf.words[0x5100u] = kNil;
    left_leaf.words[0x5104u] = 0x5000u;
    left_leaf.words[0x5108u] = kNil;
    left_leaf.words[0x5114u] = 1u;
    left_leaf.words[0x5200u] = kNil;
    left_leaf.words[0x5204u] = 0x5000u;
    left_leaf.words[0x5208u] = 0x5300u;
    left_leaf.words[0x5214u] = 1u;
    left_leaf.words[0x5300u] = kNil;
    left_leaf.words[0x5304u] = 0x5200u;
    left_leaf.words[0x5308u] = kNil;
    left_leaf.words[0x5314u] = 0u;
    Check(pal4::CombatResultTree::EraseNode(
              kTree, 0x9000u, 0x5100u, left_leaf) == 0x9000u &&
              left_leaf.words.at(kHead + 4u) == 0x5200u &&
              left_leaf.words.at(0x5200u) == 0x5000u &&
              left_leaf.words.at(0x5004u) == 0x5200u &&
              left_leaf.words.at(0x5314u) == 1u &&
              left_leaf.words.at(kTree + 12u) == 3u,
          "black left leaf rotates toward red far nephew");

    Runtime right_leaf;
    SetSingleNode(right_leaf);
    right_leaf.words[kTree + 12u] = 4u;
    right_leaf.words[kHead] = 0x5300u;
    right_leaf.words[kHead + 8u] = 0x5100u;
    right_leaf.words[0x5000u] = 0x5200u;
    right_leaf.words[0x5008u] = 0x5100u;
    right_leaf.words[0x5014u] = 1u;
    right_leaf.words[0x5100u] = kNil;
    right_leaf.words[0x5104u] = 0x5000u;
    right_leaf.words[0x5108u] = kNil;
    right_leaf.words[0x5114u] = 1u;
    right_leaf.words[0x5200u] = 0x5300u;
    right_leaf.words[0x5204u] = 0x5000u;
    right_leaf.words[0x5208u] = kNil;
    right_leaf.words[0x5214u] = 1u;
    right_leaf.words[0x5300u] = kNil;
    right_leaf.words[0x5304u] = 0x5200u;
    right_leaf.words[0x5308u] = kNil;
    right_leaf.words[0x5314u] = 0u;
    Check(pal4::CombatResultTree::EraseNode(
              kTree, 0x9000u, 0x5100u, right_leaf) == 0x9000u &&
              right_leaf.words.at(kHead + 4u) == 0x5200u &&
              right_leaf.words.at(0x5208u) == 0x5000u &&
              right_leaf.words.at(0x5004u) == 0x5200u &&
              right_leaf.words.at(0x5314u) == 1u &&
              right_leaf.words.at(kTree + 12u) == 3u,
          "black right leaf rotates toward red far nephew");
}

void EraseNearNephewAndRedSibling() {
    Runtime near_nephew;
    near_nephew.words[kTree + 12u] = 4u;
    near_nephew.words[kHead] = 0x5100u;
    near_nephew.words[kHead + 4u] = 0x5000u;
    near_nephew.words[kHead + 8u] = 0x5200u;
    SetNode(near_nephew, 0x5000u, 0x5100u, kHead, 0x5200u, 1u);
    SetNode(near_nephew, 0x5100u, kNil, 0x5000u, kNil, 1u);
    SetNode(near_nephew, 0x5200u, 0x5300u, 0x5000u, kNil, 1u);
    SetNode(near_nephew, 0x5300u, kNil, 0x5200u, kNil, 0u);
    Check(pal4::CombatResultTree::EraseNode(
              kTree, 0x9000u, 0x5100u, near_nephew) == 0x9000u &&
              near_nephew.words.at(kHead + 4u) == 0x5300u &&
              near_nephew.words.at(0x5300u) == 0x5000u &&
              near_nephew.words.at(0x5308u) == 0x5200u &&
              near_nephew.words.at(0x5014u) == 1u &&
              near_nephew.words.at(0x5214u) == 1u &&
              near_nephew.words.at(kTree + 12u) == 3u,
          "near nephew requires two rotations");

    Runtime red_sibling;
    red_sibling.words[kTree + 12u] = 5u;
    red_sibling.words[kHead] = 0x5100u;
    red_sibling.words[kHead + 4u] = 0x5000u;
    red_sibling.words[kHead + 8u] = 0x5400u;
    SetNode(red_sibling, 0x5000u, 0x5100u, kHead, 0x5200u, 1u);
    SetNode(red_sibling, 0x5100u, kNil, 0x5000u, kNil, 1u);
    SetNode(red_sibling, 0x5200u, 0x5300u, 0x5000u, 0x5400u, 0u);
    SetNode(red_sibling, 0x5300u, kNil, 0x5200u, kNil, 1u);
    SetNode(red_sibling, 0x5400u, kNil, 0x5200u, kNil, 1u);
    Check(pal4::CombatResultTree::EraseNode(
              kTree, 0x9000u, 0x5100u, red_sibling) == 0x9000u &&
              red_sibling.words.at(kHead + 4u) == 0x5200u &&
              red_sibling.words.at(0x5200u) == 0x5000u &&
              red_sibling.words.at(0x5008u) == 0x5300u &&
              red_sibling.words.at(0x5314u) == 0u &&
              red_sibling.words.at(0x5014u) == 1u &&
              red_sibling.words.at(kTree + 12u) == 4u,
          "red sibling rotates before recoloring black nephews");
}

void IteratorAndListNode() {
    Runtime runtime;
    runtime.words[0x5000u] = kNil;
    runtime.words[0x5004u] = kHead;
    runtime.words[0x5008u] = 0x5100u;
    runtime.words[0x5100u] = kNil;
    runtime.words[0x5104u] = 0x5000u;
    runtime.words[0x5108u] = kNil;
    runtime.words[0x7000u] = 0x5000u;
    Check(pal4::CombatResultTree::Next(0x7000u, runtime) == kNil &&
              runtime.words.at(0x7000u) == 0x5100u,
          "right subtree advances to leftmost descendant");

    runtime.words[0x5200u] = kNil;
    runtime.words[0x5204u] = 0x5000u;
    runtime.words[0x5208u] = kNil;
    runtime.words[0x5008u] = kNil;
    runtime.words[0x7000u] = 0x5200u;
    Check(pal4::CombatResultTree::Next(0x7000u, runtime) == 0x5000u &&
              runtime.words.at(0x7000u) == 0x5000u,
          "left child advances to parent");

    Check(pal4::CombatResultTree::CreateListNode(
              0u, 0u, runtime) == 0x8000u &&
              runtime.words.at(0x8000u) == 0x8000u &&
              runtime.words.at(0x8004u) == 0x8000u,
          "null list links become self links");
    Check(pal4::CombatResultTree::CreateListNode(
              0x100u, 0x200u, runtime) == 0x8000u &&
              runtime.words.at(0x8000u) == 0x100u &&
              runtime.words.at(0x8004u) == 0x200u,
          "provided list links are retained");
}

void RecursiveDeleteAndTreeIterators() {
    Runtime empty;
    Check(pal4::CombatResultTree::DeleteSubtree(kNil, empty) == kNil &&
              empty.freed_nodes.empty(),
          "sentinel subtree is empty");

    Runtime clear;
    clear.words[kTree + 12u] = 5u;
    clear.words[kHead] = 0x5100u;
    clear.words[kHead + 4u] = 0x5000u;
    clear.words[kHead + 8u] = 0x5400u;
    SetNode(clear, 0x5000u, 0x5100u, kHead, 0x5200u, 1u);
    SetNode(clear, 0x5100u, kNil, 0x5000u, kNil, 0u);
    SetNode(clear, 0x5200u, 0x5300u, 0x5000u, 0x5400u, 0u);
    SetNode(clear, 0x5300u, kNil, 0x5200u, kNil, 1u);
    SetNode(clear, 0x5400u, kNil, 0x5200u, kNil, 1u);
    Check(pal4::CombatResultTree::Assign(kTree, kSource, clear) == kTree &&
              clear.freed_nodes ==
                  std::vector<std::uint32_t>{0x5400u, 0x5200u,
                                              0x5300u, 0x5000u, 0x5100u} &&
              clear.words.at(kHead + 4u) == kNil &&
              clear.words.at(kTree + 12u) == 0u,
          "clear recursively frees right branches before the left spine");

    Runtime iter;
    iter.words[0x7000u] = kNil;
    Check(pal4::CombatResultTree::FindMin(0x7000u, iter) == 0x7000u,
          "empty minimum returns input slot");
    iter.words[0x7000u] = 0x5000u;
    iter.words[0x5000u] = 0x5100u;
    iter.words[0x5100u] = kNil;
    Check(pal4::CombatResultTree::FindMin(0x7000u, iter) == 0x5100u,
          "minimum follows left links");

    iter.words[0x5004u] = kHead;
    iter.words[0x5008u] = 0x5200u;
    iter.words[0x5200u] = 0x5300u;
    iter.words[0x5204u] = 0x5000u;
    iter.words[0x5208u] = kNil;
    iter.words[0x5300u] = kNil;
    iter.words[0x5304u] = 0x5200u;
    iter.words[0x5308u] = kNil;
    Check(pal4::CombatResultTree::Advance(0x7000u, 0x7100u, iter) == 0x7100u &&
              iter.words.at(0x7000u) == 0x5300u &&
              iter.words.at(0x7100u) == 0x5000u,
          "advance returns old node through output and follows right minimum");

    iter.words[0x7000u] = 0x5100u;
    iter.words[0x5104u] = 0x5000u;
    iter.words[0x5108u] = kNil;
    Check(pal4::CombatResultTree::Advance(0x7000u, 0x7100u, iter) == 0x7100u &&
              iter.words.at(0x7000u) == 0x5000u &&
              iter.words.at(0x7100u) == 0x5100u,
          "advance ascends from left child and preserves old iterator");
}

}  // namespace

int main() {
    try {
        AssignSelfAndEmpty();
        AssignOneNode();
        RecursiveCopy();
        ClearAndEraseRange();
        EraseNodeTransplants();
        EraseNodeRebalances();
        EraseNearNephewAndRedSibling();
        IteratorAndListNode();
        RecursiveDeleteAndTreeIterators();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
