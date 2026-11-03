#include "pal4/monster_behavior_tree.h"

#include <cstdlib>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kNil = 0x7000u;
constexpr std::uint32_t kHeader = 0x8000u;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Fixture final : pal4::IMonsterBehaviorTreeRuntime {
    pal4::MonsterBehaviorTreeState state{};
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> freed;

    Fixture() {
        state.words[2] = kHeader;
        Node(kNil, kNil, kNil, kNil, 1u, 0u);
        Node(kHeader, kHeader, kNil, kHeader, 1u, 0u);
    }

    void Node(std::uint32_t address, std::uint32_t left,
              std::uint32_t parent, std::uint32_t right,
              std::uint32_t color, std::uint32_t key) {
        words[address] = left;
        words[address + 4u] = parent;
        words[address + 8u] = right;
        words[address + 12u] = key;
        words[address + 20u] = color;
    }

    void Root(std::uint32_t root, std::uint32_t leftmost,
              std::uint32_t rightmost, std::uint32_t count) {
        words[kHeader] = leftmost;
        words[kHeader + 4u] = root;
        words[kHeader + 8u] = rightmost;
        state.words[4] = count;
    }

    std::uint32_t Field(std::uint32_t node, std::uint32_t offset) const {
        return words.at(node + offset);
    }

    void InitializeRedBlackTree(pal4::MonsterBehaviorTreeState&,
                                std::uint32_t) override {}
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t) override { return 0u; }
    void DestroySubtree(std::uint32_t, std::uint32_t) override {}
    void GetFirstElement(std::uint32_t) override {}
    void RemoveNode(std::uint32_t, std::uint32_t) override {}
    std::uint32_t SharedNil() override { return kNil; }
    void SetSharedNil(std::uint32_t) override {}
    std::uint32_t SharedReferenceCount() override { return 1u; }
    void SetSharedReferenceCount(std::uint32_t) override {}
    std::uint32_t FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
        return 0u;
    }
};

int ValidateNode(const Fixture& fixture, std::uint32_t node,
                 std::uint32_t parent, std::set<std::uint32_t>& seen,
                 std::vector<std::uint32_t>& keys) {
    if (node == kNil) return 1;
    Check(seen.insert(node).second, "no cycle or duplicate node");
    Check(fixture.Field(node, 4u) == parent, "parent link");
    const auto left = fixture.Field(node, 0u);
    const auto right = fixture.Field(node, 8u);
    const auto color = fixture.Field(node, 20u);
    Check(color <= 1u, "node color");
    if (color == 0u)
        Check(fixture.Field(left, 20u) == 1u &&
                  fixture.Field(right, 20u) == 1u,
              "red node children are black");
    const auto left_height = ValidateNode(fixture, left, node, seen, keys);
    keys.push_back(fixture.Field(node, 12u));
    const auto right_height = ValidateNode(fixture, right, node, seen, keys);
    Check(left_height == right_height, "equal black heights");
    return left_height + static_cast<int>(color);
}

void Validate(const Fixture& fixture, std::vector<std::uint32_t> expected) {
    const auto root = fixture.Field(kHeader, 4u);
    if (expected.empty()) {
        Check(root == kNil && fixture.Field(kHeader, 0u) == kHeader &&
                  fixture.Field(kHeader, 8u) == kHeader &&
                  fixture.state.words[4] == 0u, "empty header and count");
        return;
    }
    Check(fixture.Field(root, 20u) == 1u, "black root");
    std::set<std::uint32_t> seen;
    std::vector<std::uint32_t> keys;
    ValidateNode(fixture, root, kHeader, seen, keys);
    Check(keys == expected && seen.size() == expected.size() &&
              fixture.state.words[4] == expected.size(),
          "ordered keys and node count");
    auto first = root;
    while (fixture.Field(first, 0u) != kNil) first = fixture.Field(first, 0u);
    auto last = root;
    while (fixture.Field(last, 8u) != kNil) last = fixture.Field(last, 8u);
    Check(fixture.Field(kHeader, 0u) == first &&
              fixture.Field(kHeader, 8u) == last, "header extrema");
}

void Remove(Fixture& fixture, std::uint32_t node,
            std::uint32_t expected_successor,
            std::vector<std::uint32_t> expected_keys) {
    std::uint32_t output = 0xdeadbeefu;
    auto& result = pal4::MonsterBehaviorTree::RemoveNode(
        fixture.state, node, output, fixture);
    if (&result != &output || output != expected_successor ||
        fixture.freed != std::vector<std::uint32_t>{node}) {
        std::cerr << "node=" << node << " successor=" << output
                  << " expected=" << expected_successor << " freed="
                  << (fixture.freed.empty() ? 0u : fixture.freed.front()) << '\n';
    }
    Check(&result == &output && output == expected_successor &&
              fixture.freed == std::vector<std::uint32_t>{node},
          "successor output, return identity and detached-node free");
    Validate(fixture, expected_keys);
}

void SingleAndOneChild() {
    Fixture single;
    single.Node(0x1000u, kNil, kHeader, kNil, 1u, 10u);
    single.Root(0x1000u, 0x1000u, 0x1000u, 1u);
    Remove(single, 0x1000u, kHeader, {});

    for (const bool left : {true, false}) {
        Fixture tree;
        const auto child = left ? 0x0500u : 0x1500u;
        tree.Node(0x1000u, left ? child : kNil, kHeader,
                  left ? kNil : child, 1u, 10u);
        tree.Node(child, kNil, 0x1000u, kNil, 0u, left ? 5u : 15u);
        tree.Root(0x1000u, left ? child : 0x1000u,
                  left ? 0x1000u : child, 2u);
        Remove(tree, 0x1000u, left ? kHeader : child,
               {left ? 5u : 15u});
        Check(tree.Field(child, 20u) == 1u, "promoted child turns black");
    }
}

void TwoChildren() {
    Fixture direct;
    direct.Node(0x1000u, 0x0500u, kHeader, 0x1500u, 1u, 10u);
    direct.Node(0x0500u, kNil, 0x1000u, kNil, 1u, 5u);
    direct.Node(0x1500u, kNil, 0x1000u, kNil, 1u, 15u);
    direct.Root(0x1000u, 0x0500u, 0x1500u, 3u);
    Remove(direct, 0x1000u, 0x1500u, {5u, 15u});

    Fixture deep;
    deep.Node(0x1000u, 0x0500u, kHeader, 0x2000u, 1u, 10u);
    deep.Node(0x0500u, kNil, 0x1000u, kNil, 1u, 5u);
    deep.Node(0x2000u, 0x1500u, 0x1000u, 0x2500u, 0u, 20u);
    deep.Node(0x1500u, kNil, 0x2000u, kNil, 1u, 15u);
    deep.Node(0x2500u, kNil, 0x2000u, kNil, 1u, 25u);
    deep.Root(0x1000u, 0x0500u, 0x2500u, 5u);
    Remove(deep, 0x1000u, 0x1500u, {5u, 15u, 20u, 25u});
}

void RotationCases() {
    for (const bool mirror : {false, true}) {
        for (const bool red_sibling : {false, true}) {
            Fixture tree;
            const auto victim = mirror ? 0x1500u : 0x0500u;
            const auto sibling = mirror ? 0x0500u : 0x1500u;
            const auto inner = mirror ? 0x0700u : 0x1200u;
            const auto outer = mirror ? 0x0200u : 0x1800u;
            tree.Node(0x1000u, mirror ? sibling : victim, kHeader,
                      mirror ? victim : sibling, 1u, 10u);
            tree.Node(victim, kNil, 0x1000u, kNil, 1u,
                      mirror ? 15u : 5u);
            tree.Node(sibling, mirror ? outer : inner, 0x1000u,
                      mirror ? inner : outer, red_sibling ? 0u : 1u,
                      mirror ? 5u : 15u);
            tree.Node(inner, kNil, sibling, kNil,
                      red_sibling ? 1u : 0u, mirror ? 7u : 12u);
            if (red_sibling)
                tree.Node(outer, kNil, sibling, kNil, 1u,
                          mirror ? 2u : 18u);
            else if (mirror)
                tree.words[sibling] = kNil;
            else
                tree.words[sibling + 8u] = kNil;
            tree.Root(0x1000u, mirror ? (red_sibling ? outer : sibling) : victim,
                      mirror ? victim : (red_sibling ? outer : sibling),
                      red_sibling ? 5u : 4u);
            const auto expected = mirror
                ? (red_sibling ? std::vector<std::uint32_t>{2u, 5u, 7u, 10u}
                               : std::vector<std::uint32_t>{5u, 7u, 10u})
                : (red_sibling ? std::vector<std::uint32_t>{10u, 12u, 15u, 18u}
                               : std::vector<std::uint32_t>{10u, 12u, 15u});
            Remove(tree, victim, mirror ? kHeader : 0x1000u, expected);
        }
    }
}

void OuterAndRecolorCases() {
    for (const bool mirror : {false, true}) {
        for (const bool outer_red : {false, true}) {
            Fixture tree;
            const auto victim = mirror ? 0x1500u : 0x0500u;
            const auto sibling = mirror ? 0x0500u : 0x1500u;
            const auto outer = mirror ? 0x0200u : 0x1800u;
            tree.Node(0x1000u, mirror ? sibling : victim, kHeader,
                      mirror ? victim : sibling, 1u, 10u);
            tree.Node(victim, kNil, 0x1000u, kNil, 1u,
                      mirror ? 15u : 5u);
            tree.Node(sibling, mirror ? (outer_red ? outer : kNil) : kNil,
                      0x1000u,
                      mirror ? kNil : (outer_red ? outer : kNil),
                      1u, mirror ? 5u : 15u);
            if (outer_red)
                tree.Node(outer, kNil, sibling, kNil, 0u,
                          mirror ? 2u : 18u);
            tree.Root(0x1000u,
                      mirror ? (outer_red ? outer : sibling) : victim,
                      mirror ? victim : (outer_red ? outer : sibling),
                      outer_red ? 4u : 3u);
            const auto expected = mirror
                ? (outer_red ? std::vector<std::uint32_t>{2u, 5u, 10u}
                             : std::vector<std::uint32_t>{5u, 10u})
                : (outer_red ? std::vector<std::uint32_t>{10u, 15u, 18u}
                             : std::vector<std::uint32_t>{10u, 15u});
            Remove(tree, victim, mirror ? kHeader : 0x1000u, expected);
        }
    }
}

}  // namespace

int main() {
    SingleAndOneChild();
    TwoChildren();
    RotationCases();
    OuterAndRecolorCases();
}
