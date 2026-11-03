#include "pal4/ui_red_black_tree_remove.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiRedBlackTreeRemoveRuntime {
    static constexpr std::uint32_t tree = 0x1000u;
    static constexpr std::uint32_t head = 0x2000u;
    static constexpr std::uint32_t nil = 0x3000u;
    static constexpr std::uint32_t output = 0x4000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;

    Runtime() {
        words[tree + 4u] = head;
        words[head] = head;
        words[head + 4u] = nil;
        words[head + 8u] = head;
        words[nil] = nil;
        words[nil + 4u] = head;
        words[nil + 8u] = nil;
        words[nil + 32u] = 1u;
        words[tree + 12u] = 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t SharedNil() override { return nil; }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
    }
    void Node(std::uint32_t address, std::uint32_t left,
              std::uint32_t parent, std::uint32_t right,
              std::uint32_t colour, std::uint32_t text = 0u) {
        words[address] = left;
        words[address + 4u] = parent;
        words[address + 8u] = right;
        words[address + 16u] = text;
        words[address + 20u] = 1u;
        words[address + 24u] = 2u;
        words[address + 32u] = colour;
        ++words[tree + 12u];
    }
    void Root(std::uint32_t address, std::uint32_t first,
              std::uint32_t last) {
        words[head] = first;
        words[head + 4u] = address;
        words[head + 8u] = last;
    }
    int Validate(std::uint32_t node, std::uint32_t parent,
                 unsigned& visited) {
        if (node == nil) return 1;
        Check(words.at(node + 4u) == parent, "parent link");
        const auto left = words.at(node);
        const auto right = words.at(node + 8u);
        if (words.at(node + 32u) == 0u) {
            Check(words.at(left + 32u) == 1u &&
                      words.at(right + 32u) == 1u,
                  "red node has black children");
        }
        ++visited;
        const auto left_height = Validate(left, node, visited);
        const auto right_height = Validate(right, node, visited);
        Check(left_height == right_height, "black height");
        return left_height + (words.at(node + 32u) == 1u);
    }
    void CheckTree() {
        unsigned visited = 0;
        const auto root = words.at(head + 4u);
        if (root == nil) {
            Check(words.at(head) == head && words.at(head + 8u) == head,
                  "empty head links");
        } else {
            Check(words.at(root + 32u) == 1u, "black root");
            Validate(root, head, visited);
            auto first = root;
            while (words.at(first) != nil) first = words.at(first);
            auto last = root;
            while (words.at(last + 8u) != nil)
                last = words.at(last + 8u);
            Check(words.at(head) == first &&
                      words.at(head + 8u) == last,
                  "head extrema");
        }
        Check(visited == words.at(tree + 12u), "node count");
    }
    std::uint32_t Remove(std::uint32_t node) {
        const auto result = pal4::UiRedBlackTreeRemove::Remove(
            tree, output, node, *this);
        Check(result == output, "return output address");
        CheckTree();
        return words.at(output);
    }
};

void CheckEmptyAndLeaf() {
    Runtime r;
    r.Node(0x5000u, r.nil, r.head, r.nil, 1u, 0x8001u);
    r.Root(0x5000u, 0x5000u, 0x5000u);
    r.bytes[0x8000u] = 0u;
    Check(r.Remove(0x5000u) == r.head, "last iterator is head");
    Check(r.freed == (std::vector<std::uint32_t>{0x8000u, 0x5000u}),
          "zero-count text freed before node");
    Check(r.words.at(0x5000u + 16u) == 0u &&
              r.words.at(0x5000u + 20u) == 0u &&
              r.words.at(0x5000u + 24u) == 0u,
          "text fields cleared");
}

void CheckOneChildAndSharedText() {
    Runtime r;
    r.Node(0x5000u, 0x5100u, r.head, r.nil, 1u, 0x8101u);
    r.Node(0x5100u, r.nil, 0x5000u, r.nil, 0u);
    r.Root(0x5000u, 0x5100u, 0x5000u);
    r.bytes[0x8100u] = 2u;
    Check(r.Remove(0x5000u) == r.head, "maximum iterator advances to head");
    Check(r.words.at(r.head + 4u) == 0x5100u &&
              r.words.at(0x5100u + 32u) == 1u,
          "single child replaces root and becomes black");
    Check(r.bytes.at(0x8100u) == 1u &&
              r.freed == (std::vector<std::uint32_t>{0x5000u}),
          "shared text decremented");
}

void CheckSuccessorMoves() {
    Runtime direct;
    direct.Node(0x5000u, 0x5100u, direct.head, 0x5200u, 1u, 0x8201u);
    direct.Node(0x5100u, direct.nil, 0x5000u, direct.nil, 0u);
    direct.Node(0x5200u, direct.nil, 0x5000u, direct.nil, 0u);
    direct.Root(0x5000u, 0x5100u, 0x5200u);
    direct.bytes[0x8200u] = 0xffu;
    Check(direct.Remove(0x5000u) == 0x5200u,
          "direct successor returned");
    Check(direct.words.at(direct.head + 4u) == 0x5200u &&
              direct.words.at(0x5200u + 32u) == 1u,
          "direct successor transplanted");
    Check(direct.freed.front() == 0x8200u,
          "sentinel text count frees buffer");

    Runtime deep;
    deep.Node(0x5000u, 0x5100u, deep.head, 0x5200u, 1u);
    deep.Node(0x5100u, deep.nil, 0x5000u, deep.nil, 1u);
    deep.Node(0x5200u, 0x5300u, 0x5000u, deep.nil, 1u);
    deep.Node(0x5300u, deep.nil, 0x5200u, deep.nil, 0u);
    deep.Root(0x5000u, 0x5100u, 0x5200u);
    Check(deep.Remove(0x5000u) == 0x5300u,
          "deep successor returned");
    Check(deep.words.at(deep.head + 4u) == 0x5300u &&
              deep.words.at(0x5200u) == deep.nil &&
              deep.words.at(deep.nil + 4u) == 0x5200u,
          "deep successor old location detached");
}

void CheckRepairBothSides() {
    Runtime left;
    left.Node(0x5000u, 0x5100u, left.head, 0x5200u, 1u);
    left.Node(0x5100u, left.nil, 0x5000u, left.nil, 1u);
    left.Node(0x5200u, 0x5300u, 0x5000u, 0x5400u, 1u);
    left.Node(0x5300u, left.nil, 0x5200u, left.nil, 0u);
    left.Node(0x5400u, left.nil, 0x5200u, left.nil, 0u);
    left.Root(0x5000u, 0x5100u, 0x5400u);
    Check(left.Remove(0x5100u) == 0x5000u,
          "left deletion successor");
    Check(left.words.at(left.head + 4u) == 0x5200u,
          "left-side repair rotates root");

    Runtime right;
    right.Node(0x5000u, 0x5100u, right.head, 0x5200u, 1u);
    right.Node(0x5100u, 0x5300u, 0x5000u, 0x5400u, 1u);
    right.Node(0x5200u, right.nil, 0x5000u, right.nil, 1u);
    right.Node(0x5300u, right.nil, 0x5100u, right.nil, 0u);
    right.Node(0x5400u, right.nil, 0x5100u, right.nil, 0u);
    right.Root(0x5000u, 0x5300u, 0x5200u);
    Check(right.Remove(0x5200u) == right.head,
          "rightmost deletion successor is head");
    Check(right.words.at(right.head + 4u) == 0x5100u,
          "right-side repair rotates root");
}

void CheckRedSibling() {
    Runtime r;
    r.Node(0x5000u, 0x5100u, r.head, 0x5200u, 1u);
    r.Node(0x5100u, r.nil, 0x5000u, r.nil, 1u);
    r.Node(0x5200u, 0x5300u, 0x5000u, 0x5400u, 0u);
    r.Node(0x5300u, r.nil, 0x5200u, r.nil, 1u);
    r.Node(0x5400u, r.nil, 0x5200u, r.nil, 1u);
    r.Root(0x5000u, 0x5100u, 0x5400u);
    Check(r.Remove(0x5100u) == 0x5000u,
          "red-sibling successor");
    Check(r.words.at(r.head + 4u) == 0x5200u,
          "red sibling becomes root");
}

void CheckNearChildAndBlackSibling() {
    Runtime near_left;
    near_left.Node(0x5000u, 0x5100u, near_left.head, 0x5200u, 1u);
    near_left.Node(0x5100u, near_left.nil, 0x5000u,
                   near_left.nil, 1u);
    near_left.Node(0x5200u, 0x5300u, 0x5000u,
                   near_left.nil, 1u);
    near_left.Node(0x5300u, near_left.nil, 0x5200u,
                   near_left.nil, 0u);
    near_left.Root(0x5000u, 0x5100u, 0x5200u);
    Check(near_left.Remove(0x5100u) == 0x5000u,
          "left near-child iterator");
    Check(near_left.words.at(near_left.head + 4u) == 0x5300u,
          "left near child rotates twice");

    Runtime near_right;
    near_right.Node(0x5000u, 0x5100u, near_right.head, 0x5200u, 1u);
    near_right.Node(0x5100u, near_right.nil, 0x5000u,
                    0x5300u, 1u);
    near_right.Node(0x5200u, near_right.nil, 0x5000u,
                    near_right.nil, 1u);
    near_right.Node(0x5300u, near_right.nil, 0x5100u,
                    near_right.nil, 0u);
    near_right.Root(0x5000u, 0x5100u, 0x5200u);
    Check(near_right.Remove(0x5200u) == near_right.head,
          "right near-child iterator");
    Check(near_right.words.at(near_right.head + 4u) == 0x5300u,
          "right near child rotates twice");

    Runtime black_sibling;
    black_sibling.Node(0x5000u, 0x5100u,
                       black_sibling.head, 0x5200u, 1u);
    black_sibling.Node(0x5100u, black_sibling.nil, 0x5000u,
                       black_sibling.nil, 1u);
    black_sibling.Node(0x5200u, black_sibling.nil, 0x5000u,
                       black_sibling.nil, 1u);
    black_sibling.Root(0x5000u, 0x5100u, 0x5200u);
    Check(black_sibling.Remove(0x5100u) == 0x5000u,
          "all-black sibling iterator");
    Check(black_sibling.words.at(0x5200u + 32u) == 0u &&
              black_sibling.words.at(0x5000u + 32u) == 1u,
          "all-black sibling recolours and climbs");
}

}  // namespace

int main() {
    try {
        CheckEmptyAndLeaf();
        CheckOneChildAndSharedText();
        CheckSuccessorMoves();
        CheckRepairBothSides();
        CheckRedSibling();
        CheckNearChildAndBlackSibling();
        std::cout << "ui_red_black_tree_remove_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
