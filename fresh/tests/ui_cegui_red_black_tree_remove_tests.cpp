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

struct Runtime final : pal4::IUiCeguiRedBlackTreeRemoveRuntime {
    static constexpr std::uint32_t tree = 0x1000u;
    static constexpr std::uint32_t head = 0x2000u;
    static constexpr std::uint32_t nil = 0x3000u;
    static constexpr std::uint32_t output = 0x4000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool throw_on_destructor = false;

    Runtime() {
        words[tree + 4u] = head;
        words[tree + 12u] = 0u;
        words[head] = head;
        words[head + 4u] = nil;
        words[head + 8u] = head;
        words[nil] = nil;
        words[nil + 4u] = head;
        words[nil + 8u] = nil;
        words[nil + 180u] = 1u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("byte " + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t SharedNil() override { return nil; }
    void FreeMemory(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    void DestroyCeguiString(std::uint32_t address) override {
        calls.push_back("cegui " + std::to_string(address));
        if (throw_on_destructor)
            throw std::runtime_error("CEGUI string destructor");
    }
    void Node(std::uint32_t address, std::uint32_t left,
              std::uint32_t parent, std::uint32_t right,
              std::uint32_t colour, std::uint32_t text = 0u) {
        words[address] = left;
        words[address + 4u] = parent;
        words[address + 8u] = right;
        words[address + 168u] = text;
        words[address + 172u] = 1u;
        words[address + 176u] = 2u;
        words[address + 180u] = colour;
        ++words[tree + 12u];
    }
    void Root(std::uint32_t node, std::uint32_t first,
              std::uint32_t last) {
        words[head] = first;
        words[head + 4u] = node;
        words[head + 8u] = last;
    }
    int Validate(std::uint32_t node, std::uint32_t parent,
                 unsigned& visited) {
        if (node == nil) return 1;
        Check(words.at(node + 4u) == parent, "parent link");
        const auto left = words.at(node);
        const auto right = words.at(node + 8u);
        if (words.at(node + 180u) == 0u)
            Check(words.at(left + 180u) == 1u &&
                      words.at(right + 180u) == 1u,
                  "red node children");
        ++visited;
        const auto left_height = Validate(left, node, visited);
        const auto right_height = Validate(right, node, visited);
        Check(left_height == right_height, "black height");
        return left_height + (words.at(node + 180u) == 1u);
    }
    void CheckTree() {
        unsigned visited = 0u;
        const auto root = words.at(head + 4u);
        if (root == nil) {
            Check(words.at(head) == head && words.at(head + 8u) == head,
                  "empty head");
        } else {
            Check(words.at(root + 180u) == 1u, "black root");
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
        Check(pal4::UiRedBlackTreeRemove::RemoveCegui(
                  tree, output, node, *this) == output,
              "output address returned");
        CheckTree();
        return words.at(output);
    }
};

void CheckLastNodeAndCleanup() {
    Runtime r;
    r.Node(0x5000u, r.nil, r.head, r.nil, 1u, 0x8001u);
    r.Root(0x5000u, 0x5000u, 0x5000u);
    r.bytes[0x8000u] = 0u;
    Check(r.Remove(0x5000u) == r.head, "last iterator is head");
    Check(r.calls == (std::vector<std::string>{
              "free 32768", "cegui 20492", "free 20480"}),
          "text freed, CEGUI destroyed, then node freed");
    Check(r.words.at(0x5000u + 168u) == 0u &&
              r.words.at(0x5000u + 172u) == 0u &&
              r.words.at(0x5000u + 176u) == 0u,
          "text metadata cleared before destructor");
}

void CheckSuccessorAndRepair() {
    Runtime successor;
    successor.Node(0x5000u, 0x5100u, successor.head,
                   0x5200u, 1u, 0x8101u);
    successor.Node(0x5100u, successor.nil, 0x5000u,
                   successor.nil, 0u);
    successor.Node(0x5200u, successor.nil, 0x5000u,
                   successor.nil, 0u);
    successor.Root(0x5000u, 0x5100u, 0x5200u);
    successor.bytes[0x8100u] = 3u;
    Check(successor.Remove(0x5000u) == 0x5200u &&
              successor.words.at(successor.head + 4u) == 0x5200u &&
              successor.words.at(0x5200u + 180u) == 1u &&
              successor.bytes.at(0x8100u) == 2u,
          "direct successor and shared text");
    Check(successor.calls == (std::vector<std::string>{
              "byte 33024", "cegui 20492", "free 20480"}),
          "shared text decremented before CEGUI destruction");

    Runtime repair;
    repair.Node(0x5000u, 0x5100u, repair.head, 0x5200u, 1u);
    repair.Node(0x5100u, repair.nil, 0x5000u, repair.nil, 1u);
    repair.Node(0x5200u, 0x5300u, 0x5000u, 0x5400u, 1u);
    repair.Node(0x5300u, repair.nil, 0x5200u, repair.nil, 0u);
    repair.Node(0x5400u, repair.nil, 0x5200u, repair.nil, 0u);
    repair.Root(0x5000u, 0x5100u, 0x5400u);
    Check(repair.Remove(0x5100u) == 0x5000u &&
              repair.words.at(repair.head + 4u) == 0x5200u,
          "large-node colour offset participates in rebalancing");
}

void CheckDestructorFailure() {
    Runtime r;
    r.Node(0x5000u, r.nil, r.head, r.nil, 1u);
    r.Root(0x5000u, 0x5000u, 0x5000u);
    r.throw_on_destructor = true;
    bool threw = false;
    try {
        (void)pal4::UiRedBlackTreeRemove::RemoveCegui(
            r.tree, r.output, 0x5000u, r);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && r.words.at(r.tree + 12u) == 1u &&
              r.words.find(r.output) == r.words.end() &&
              r.calls == (std::vector<std::string>{"cegui 20492"}),
          "destructor failure precedes node free, count and output");
}

}  // namespace

int main() {
    try {
        CheckLastNodeAndCleanup();
        CheckSuccessorAndRepair();
        CheckDestructorFailure();
        std::cout << "ui_cegui_red_black_tree_remove_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
