#include "pal4/cegui_map_operations.h"

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <map>
#include <set>
#include <vector>

namespace {

constexpr std::uint32_t kMap = 0x1000u;
constexpr std::uint32_t kHeader = 0x2000u;
constexpr std::uint32_t kNil = 0x9000u;
constexpr std::uint32_t kBlack = 1u;
constexpr std::uint32_t kRed = 0u;

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct NodeImage {
    std::uint32_t address;
    std::uint32_t left;
    std::uint32_t parent;
    std::uint32_t right;
    std::uint32_t key;
    std::uint32_t color;
};

class Runtime final : public pal4::ICeguiMapEraseRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> successors;
    std::vector<std::uint32_t> freed;
    bool successor_was_first = false;
    int writes = 0;

    std::uint32_t ReadDword(const std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        ++writes;
        words[address] = value;
    }
    std::uint32_t Successor(const std::uint32_t node) override {
        successor_was_first = writes == 0 && freed.empty();
        return successors.at(node);
    }
    void FreeNode(const std::uint32_t node) override {
        Check(words.at(kMap + 0x0cu) != 0u,
              "node free occurs before count decrement");
        freed.push_back(node);
    }

    void SetNode(const NodeImage& node) {
        words[node.address] = node.left;
        words[node.address + 4u] = node.parent;
        words[node.address + 8u] = node.right;
        words[node.address + 0x0cu] = node.key;
        words[node.address + 0x14u] = node.color;
    }
    void SetTree(const std::uint32_t root, const std::uint32_t minimum,
                 const std::uint32_t maximum,
                 const std::initializer_list<NodeImage> nodes) {
        words[kMap + 4u] = kHeader;
        words[kMap + 0x0cu] = static_cast<std::uint32_t>(nodes.size());
        words[kHeader] = minimum;
        words[kHeader + 4u] = root;
        words[kHeader + 8u] = maximum;
        SetNode({kNil, kNil, kNil, kNil, 0u, kBlack});
        for (const auto& node : nodes) SetNode(node);
    }
};

std::uint32_t VerifySubtree(Runtime& runtime, const std::uint32_t node,
                            const std::uint32_t parent,
                            std::set<std::uint32_t>& seen,
                            std::vector<std::uint32_t>& keys) {
    if (node == kNil) return 1u;
    Check(seen.insert(node).second, "tree contains no repeated node");
    Check(runtime.ReadDword(node + 4u) == parent,
          "each child points back to its parent");
    const auto left = runtime.ReadDword(node);
    const auto right = runtime.ReadDword(node + 8u);
    const auto color = runtime.ReadDword(node + 0x14u);
    Check(color == kBlack || color == kRed, "node color is binary");
    if (color == kRed) {
        Check(runtime.ReadDword(left + 0x14u) == kBlack &&
                  runtime.ReadDword(right + 0x14u) == kBlack,
              "red node has black children");
    }
    const auto left_height = VerifySubtree(runtime, left, node, seen, keys);
    keys.push_back(runtime.ReadDword(node + 0x0cu));
    const auto right_height = VerifySubtree(runtime, right, node, seen, keys);
    Check(left_height == right_height, "both paths have equal black height");
    return left_height + (color == kBlack ? 1u : 0u);
}

void VerifyTree(Runtime& runtime, const std::uint32_t root,
                const std::vector<std::uint32_t>& expected_keys) {
    Check(runtime.ReadDword(kHeader + 4u) == root,
          "header retains expected root");
    Check(runtime.ReadDword(kMap + 0x0cu) == expected_keys.size(),
          "map count matches remaining nodes");
    Check(runtime.ReadDword(kNil + 0x14u) == kBlack,
          "nil sentinel remains black");
    if (root == kNil) {
        Check(expected_keys.empty() &&
                  runtime.ReadDword(kHeader) == kHeader &&
                  runtime.ReadDword(kHeader + 8u) == kHeader,
              "empty tree points extrema to header");
        return;
    }
    Check(runtime.ReadDword(root + 0x14u) == kBlack,
          "root is black");
    std::set<std::uint32_t> seen;
    std::vector<std::uint32_t> keys;
    static_cast<void>(VerifySubtree(runtime, root, kHeader, seen, keys));
    Check(keys == expected_keys && std::is_sorted(keys.begin(), keys.end()),
          "in-order keys retain sorted contents");
    Check(seen.size() == expected_keys.size(),
          "all counted nodes are reachable");
    Check(runtime.ReadDword(kHeader + 0u) == *std::min_element(
              seen.begin(), seen.end(), [&](const auto a, const auto b) {
                  return runtime.ReadDword(a + 0x0cu) <
                         runtime.ReadDword(b + 0x0cu);
              }) &&
              runtime.ReadDword(kHeader + 8u) == *std::max_element(
                  seen.begin(), seen.end(), [&](const auto a, const auto b) {
                      return runtime.ReadDword(a + 0x0cu) <
                             runtime.ReadDword(b + 0x0cu);
                  }),
          "header points at minimum and maximum nodes");
}

void EraseAndVerify(Runtime& runtime, const std::uint32_t target,
                    const std::uint32_t next, const std::uint32_t root,
                    const std::vector<std::uint32_t>& keys) {
    runtime.successors[target] = next;
    const pal4::CeguiTreeGlobals globals{1u, kNil};
    Check(pal4::CeguiMapOperations::Erase(
              kMap, target, globals, runtime) == next &&
              runtime.successor_was_first &&
              runtime.freed == std::vector<std::uint32_t>{target},
          "erase returns precomputed successor and frees only target");
    VerifyTree(runtime, root, keys);
}

void EraseOnlyNode() {
    Runtime runtime;
    runtime.SetTree(0x3000u, 0x3000u, 0x3000u,
                    {{0x3000u, kNil, kHeader, kNil, 20u, kBlack}});
    EraseAndVerify(runtime, 0x3000u, kHeader, kNil, {});
}

void EraseRedLeaf() {
    Runtime runtime;
    runtime.SetTree(0x3000u, 0x3100u, 0x3200u,
                    {{0x3000u, 0x3100u, kHeader, 0x3200u, 20u, kBlack},
                     {0x3100u, kNil, 0x3000u, kNil, 10u, kRed},
                     {0x3200u, kNil, 0x3000u, kNil, 30u, kRed}});
    EraseAndVerify(runtime, 0x3100u, 0x3000u, 0x3000u, {20u, 30u});
}

void EraseBlackNodeWithRedChild() {
    Runtime runtime;
    runtime.SetTree(0x3000u, 0x3300u, 0x3200u,
                    {{0x3000u, 0x3100u, kHeader, 0x3200u, 20u, kBlack},
                     {0x3100u, 0x3300u, 0x3000u, kNil, 10u, kBlack},
                     {0x3200u, kNil, 0x3000u, kNil, 30u, kBlack},
                     {0x3300u, kNil, 0x3100u, kNil, 5u, kRed}});
    EraseAndVerify(runtime, 0x3100u, 0x3000u, 0x3000u,
                   {5u, 20u, 30u});
}

void EraseWithImmediateSuccessor() {
    Runtime runtime;
    runtime.SetTree(0x3000u, 0x3100u, 0x3200u,
                    {{0x3000u, 0x3100u, kHeader, 0x3200u, 20u, kBlack},
                     {0x3100u, kNil, 0x3000u, kNil, 10u, kRed},
                     {0x3200u, kNil, 0x3000u, kNil, 30u, kRed}});
    EraseAndVerify(runtime, 0x3000u, 0x3200u, 0x3200u, {10u, 30u});
}

void EraseWithDeeperSuccessor() {
    Runtime runtime;
    runtime.SetTree(0x3000u, 0x3100u, 0x3200u,
                    {{0x3000u, 0x3100u, kHeader, 0x3200u, 20u, kBlack},
                     {0x3100u, kNil, 0x3000u, kNil, 10u, kBlack},
                     {0x3200u, 0x3300u, 0x3000u, kNil, 30u, kBlack},
                     {0x3300u, kNil, 0x3200u, kNil, 25u, kRed}});
    EraseAndVerify(runtime, 0x3000u, 0x3300u, 0x3300u,
                   {10u, 25u, 30u});
}

void EraseWithFarRedNephew() {
    Runtime runtime;
    runtime.SetTree(0x3000u, 0x3100u, 0x3400u,
                    {{0x3000u, 0x3100u, kHeader, 0x3200u, 20u, kBlack},
                     {0x3100u, kNil, 0x3000u, kNil, 10u, kBlack},
                     {0x3200u, 0x3300u, 0x3000u, 0x3400u, 30u, kBlack},
                     {0x3300u, kNil, 0x3200u, kNil, 25u, kRed},
                     {0x3400u, kNil, 0x3200u, kNil, 35u, kRed}});
    EraseAndVerify(runtime, 0x3100u, 0x3000u, 0x3200u,
                   {20u, 25u, 30u, 35u});
}

void EraseWithNearRedNephew() {
    Runtime runtime;
    runtime.SetTree(0x3000u, 0x3100u, 0x3200u,
                    {{0x3000u, 0x3100u, kHeader, 0x3200u, 20u, kBlack},
                     {0x3100u, kNil, 0x3000u, kNil, 10u, kBlack},
                     {0x3200u, 0x3300u, 0x3000u, kNil, 30u, kBlack},
                     {0x3300u, kNil, 0x3200u, kNil, 25u, kRed}});
    EraseAndVerify(runtime, 0x3100u, 0x3000u, 0x3300u,
                   {20u, 25u, 30u});
}

void EraseWithRedSibling() {
    Runtime runtime;
    runtime.SetTree(0x3000u, 0x3100u, 0x3400u,
                    {{0x3000u, 0x3100u, kHeader, 0x3200u, 20u, kBlack},
                     {0x3100u, kNil, 0x3000u, kNil, 10u, kBlack},
                     {0x3200u, 0x3300u, 0x3000u, 0x3400u, 30u, kRed},
                     {0x3300u, kNil, 0x3200u, kNil, 25u, kBlack},
                     {0x3400u, kNil, 0x3200u, kNil, 35u, kBlack}});
    EraseAndVerify(runtime, 0x3100u, 0x3000u, 0x3200u,
                   {20u, 25u, 30u, 35u});
}

void EraseWithMirroredRotation() {
    Runtime runtime;
    runtime.SetTree(0x3000u, 0x3300u, 0x3200u,
                    {{0x3000u, 0x3100u, kHeader, 0x3200u, 20u, kBlack},
                     {0x3100u, 0x3300u, 0x3000u, 0x3400u, 10u, kBlack},
                     {0x3200u, kNil, 0x3000u, kNil, 30u, kBlack},
                     {0x3300u, kNil, 0x3100u, kNil, 5u, kRed},
                     {0x3400u, kNil, 0x3100u, kNil, 15u, kRed}});
    EraseAndVerify(runtime, 0x3200u, kHeader, 0x3100u,
                   {5u, 10u, 15u, 20u});
}

}  // namespace

int main() {
    EraseOnlyNode();
    EraseRedLeaf();
    EraseBlackNodeWithRedChild();
    EraseWithImmediateSuccessor();
    EraseWithDeeperSuccessor();
    EraseWithFarRedNephew();
    EraseWithNearRedNephew();
    EraseWithRedSibling();
    EraseWithMirroredRotation();
    std::cout << "9 CEGUI map erase test cases passed\n";
}
