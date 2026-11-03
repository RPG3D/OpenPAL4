#include "pal4/pal_game_object_tree.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct NameRuntime final : pal4::IPalGameObjectNameInsertRuntime,
                           pal4::IUiRedBlackTreeRemoveRuntime,
                           pal4::IUiRedBlackTreeStorageRuntime,
                           pal4::IUiRedBlackTreeIteratorRuntime,
                           pal4::IUiRedBlackTreeLookupRuntime {
    static constexpr std::uint32_t tree = 0x1000u;
    static constexpr std::uint32_t head = 0x2000u;
    static constexpr std::uint32_t nil = 0x3000u;
    static constexpr std::uint32_t output = 0x4000u;
    static constexpr std::uint32_t query = 0x4100u;
    static constexpr std::uint32_t query_text = 0x4200u;
    static constexpr std::uint32_t empty_text = 0x4300u;
    static constexpr std::uint32_t cursor = 0x4400u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> strings;
    std::map<std::string, std::uint32_t> inserted_nodes;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> cleaned;
    std::uint32_t next_node = 0x5000u;
    std::uint32_t next_text = 0x8001u;
    std::uint32_t nil_users = 0u;

    NameRuntime() {
        words[tree + 4u] = head;
        words[tree + 12u] = 0u;
        bytes[tree + 8u] = 0u;
        words[head] = head;
        words[head + 4u] = nil;
        words[head + 8u] = head;
        words[head + 32u] = 0u;
        words[nil] = nil;
        words[nil + 4u] = head;
        words[nil + 8u] = nil;
        words[nil + 32u] = 1u;
        strings[empty_text] = "";
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t SharedNil() override { return nil; }
    void SetSharedNil(std::uint32_t address) override {
        Check(address == nil, "name-tree nil address");
    }
    std::uint32_t SharedNilUsers() override { return nil_users; }
    void SetSharedNilUsers(std::uint32_t users) override {
        nil_users = users;
    }
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        Check(size == 0x24u, "name-tree node size");
        const auto node = next_node;
        next_node += 0x100u;
        return node;
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
    }
    void CopyText(std::uint32_t destination,
                  std::uint32_t source) override {
        strings[destination] = strings.at(source);
        words[destination + 4u] = next_text;
        words[destination + 8u] =
            static_cast<std::uint32_t>(strings[destination].size());
        words[destination + 12u] = 31u;
        bytes[next_text - 1u] = 0u;
        next_text += 0x100u;
    }
    std::int32_t CompareText(std::uint32_t first,
                             std::uint32_t second) override {
        const auto result = strings.at(first).compare(strings.at(second));
        return (result > 0) - (result < 0);
    }
    void CleanupData(std::uint32_t address) override {
        cleaned.push_back(address);
        const auto data = words.at(address + 4u);
        if (data != 0u) FreeMemory(data - 1u);
    }
    std::uint32_t EmptyText() override { return empty_text; }
    std::int32_t CompareRange(std::uint32_t node_text,
                              std::uint32_t node_length,
                              std::uint32_t text,
                              std::uint32_t query_length) override {
        const auto a = strings.at(node_text).substr(0u, node_length);
        const auto b = strings.at(text).substr(0u, query_length);
        const auto result = a.compare(b);
        return (result > 0) - (result < 0);
    }

    int Validate(std::uint32_t node, std::uint32_t parent,
                 std::vector<std::string>& order) {
        if (node == nil) return 1;
        Check(words.at(node + 4u) == parent, "name-tree parent link");
        const auto left = words.at(node);
        const auto right = words.at(node + 8u);
        if (words.at(node + 32u) == 0u)
            Check(words.at(left + 32u) == 1u &&
                  words.at(right + 32u) == 1u,
                  "name-tree red children are black");
        const auto left_height = Validate(left, node, order);
        order.push_back(strings.at(node + 12u));
        const auto right_height = Validate(right, node, order);
        Check(left_height == right_height, "name-tree black height");
        return left_height + (words.at(node + 32u) == 1u);
    }
    void CheckTree(std::vector<std::string> expected) {
        const auto root = words.at(head + 4u);
        if (expected.empty()) {
            Check(root == nil && words.at(head) == head &&
                  words.at(head + 8u) == head,
                  "empty name-tree head links");
        } else {
            Check(root != nil && words.at(root + 32u) == 1u,
                  "name-tree black root");
            std::vector<std::string> order;
            Validate(root, head, order);
            std::sort(expected.begin(), expected.end());
            Check(order == expected, "name-tree sorted order");
            auto first = root;
            while (words.at(first) != nil) first = words.at(first);
            auto last = root;
            while (words.at(last + 8u) != nil)
                last = words.at(last + 8u);
            Check(words.at(head) == first &&
                  words.at(head + 8u) == last,
                  "name-tree head extrema");
        }
        Check(words.at(tree + 12u) == expected.size(),
              "name-tree count");
    }
    std::uint32_t Insert(std::string name,
                         bool expected_new = true) {
        const auto key = 0xa000u +
            static_cast<std::uint32_t>(strings.size()) * 0x100u;
        strings[key] = name;
        Check(pal4::PalGameObjectTree::InsertName(
                  tree, output, key, *this) == output &&
              bytes.at(output + 4u) == (expected_new ? 1u : 0u),
              "name-tree insertion status");
        if (expected_new) inserted_nodes[name] = words.at(output);
        return words.at(output);
    }
    std::uint32_t LowerBound(const std::string& name) {
        strings[query_text] = name;
        words[query + 4u] = query_text;
        words[query + 8u] = static_cast<std::uint32_t>(name.size());
        Check(pal4::PalGameObjectTree::LowerBoundName(
                  tree, output, query, *this) == output,
              "name-tree lower-bound output");
        return words.at(output);
    }
};

void CheckNameTree() {
    NameRuntime runtime;
    runtime.CheckTree({});
    std::vector<std::string> expected;
    for (const auto& name : {"m", "d", "t", "a", "f", "z", "e"}) {
        runtime.Insert(name);
        expected.emplace_back(name);
        runtime.CheckTree(expected);
    }
    const auto original = runtime.inserted_nodes.at("d");
    Check(runtime.Insert("d", false) == original &&
          runtime.words.at(NameRuntime::tree + 12u) == expected.size(),
          "duplicate name preserves existing node");
    Check(runtime.LowerBound("b") == runtime.inserted_nodes.at("d") &&
          runtime.LowerBound("e") == runtime.inserted_nodes.at("e") &&
          runtime.LowerBound("zz") == NameRuntime::head,
          "name-tree lower bounds");

    runtime.words[NameRuntime::cursor] = runtime.inserted_nodes.at("d");
    static_cast<void>(pal4::PalGameObjectTree::NextName(
        NameRuntime::cursor, runtime));
    Check(runtime.words.at(NameRuntime::cursor) ==
              runtime.inserted_nodes.at("e"),
          "name-tree successor");
    runtime.words[NameRuntime::cursor] = runtime.inserted_nodes.at("e");
    static_cast<void>(pal4::PalGameObjectTree::PreviousName(
        NameRuntime::cursor, runtime));
    Check(runtime.words.at(NameRuntime::cursor) ==
              runtime.inserted_nodes.at("d"),
          "name-tree predecessor");
    runtime.words[NameRuntime::cursor] = runtime.inserted_nodes.at("d");
    Check(pal4::PalGameObjectTree::AdvanceName(
              NameRuntime::cursor, NameRuntime::output, runtime) ==
              NameRuntime::output &&
          runtime.words.at(NameRuntime::cursor) ==
              runtime.inserted_nodes.at("e") &&
          runtime.words.at(NameRuntime::output) ==
              runtime.inserted_nodes.at("d"),
          "name-tree post-increment");
    Check(pal4::PalGameObjectTree::LeftmostName(
              NameRuntime::head + 4u, runtime) ==
              runtime.inserted_nodes.at("a"),
          "name-tree leftmost slot");

    Check(pal4::PalGameObjectTree::RemoveName(
              NameRuntime::tree, NameRuntime::output,
              runtime.inserted_nodes.at("m"), runtime) ==
              NameRuntime::output &&
          runtime.words.at(NameRuntime::output) ==
              runtime.inserted_nodes.at("t"),
          "name-tree removal returns next iterator");
    expected.erase(std::find(expected.begin(), expected.end(), "m"));
    runtime.CheckTree(expected);
    const auto root = runtime.words.at(NameRuntime::head + 4u);
    Check(pal4::PalGameObjectTree::ClearNameNodes(root, runtime) ==
              NameRuntime::nil &&
          runtime.cleaned.size() == expected.size(),
          "name-tree recursive cleanup");
}

void CheckNameNodeDirect() {
    NameRuntime runtime;
    runtime.strings[0xa000u] = "x";
    Check(pal4::PalGameObjectTree::InsertNameNode(
              NameRuntime::tree, NameRuntime::output,
              NameRuntime::nil, NameRuntime::head,
              0xa000u, runtime) == NameRuntime::output,
          "direct name-node insert output");
    runtime.CheckTree({"x"});
    Check(pal4::PalGameObjectTree::ClearNameNodes(
              NameRuntime::nil, runtime) == NameRuntime::nil,
          "empty recursive cleanup");
}

void CheckNameRotationsAndDuplicates() {
    NameRuntime runtime;
    const auto root = runtime.Insert("b");
    runtime.Insert("a");
    const auto right = runtime.Insert("c");
    auto& links = static_cast<pal4::IPalGameObjectNameInsertRuntime&>(runtime);
    Check(pal4::PalGameObjectTree::RotateNameLeft(
              NameRuntime::tree, root, links) == right &&
          runtime.words.at(NameRuntime::head + 4u) == right &&
          runtime.words.at(root + 4u) == right,
          "name-tree left rotation");
    Check(pal4::PalGameObjectTree::RotateNameRight(
              NameRuntime::tree, right, links) == root &&
          runtime.words.at(NameRuntime::head + 4u) == root,
          "name-tree right rotation");
    runtime.CheckTree({"a", "b", "c"});
    runtime.bytes[NameRuntime::tree + 8u] = 1u;
    runtime.Insert("b");
    runtime.CheckTree({"a", "b", "b", "c"});
}

struct FactoryRuntime final : pal4::IUiRedBlackTreeRemoveRuntime,
                              pal4::IPalGameObjectFactoryClearRuntime,
                              pal4::IUiRedBlackTreeIteratorRuntime {
    static constexpr std::uint32_t tree = 0x1000u;
    static constexpr std::uint32_t head = 0x2000u;
    static constexpr std::uint32_t nil = 0x3000u;
    static constexpr std::uint32_t left = 0x4000u;
    static constexpr std::uint32_t root = 0x5000u;
    static constexpr std::uint32_t right = 0x6000u;
    static constexpr std::uint32_t cursor = 0x7000u;
    static constexpr std::uint32_t output = 0x7100u;
    static constexpr std::uint32_t key = 0x7200u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> freed;

    FactoryRuntime() {
        words[tree + 4u] = head;
        words[tree + 12u] = 3u;
        words[head] = left;
        words[head + 4u] = root;
        words[head + 8u] = right;
        words[nil] = nil;
        words[nil + 4u] = head;
        words[nil + 8u] = nil;
        words[nil + 20u] = 1u;
        words[root] = left;
        words[root + 4u] = head;
        words[root + 8u] = right;
        words[root + 12u] = 0u;
        words[root + 20u] = 1u;
        words[left] = nil;
        words[left + 4u] = root;
        words[left + 8u] = nil;
        words[left + 12u] = 0xfffffffeu;
        words[left + 20u] = 0u;
        words[right] = nil;
        words[right + 4u] = root;
        words[right + 8u] = nil;
        words[right + 12u] = 5u;
        words[right + 20u] = 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override { words[address] = value; }
    std::uint32_t SharedNil() override { return nil; }
    std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
    }
    std::uint32_t Find(std::int32_t value) {
        words[key] = static_cast<std::uint32_t>(value);
        auto& links = static_cast<pal4::IPalGameObjectFactoryClearRuntime&>(*this);
        Check(pal4::PalGameObjectTree::FindFactory(
                  tree, output, key, links) == output,
              "factory lookup output");
        return words.at(output);
    }
};

void CheckFactoryTree() {
    FactoryRuntime runtime;
    Check(runtime.Find(-2) == FactoryRuntime::left &&
          runtime.Find(0) == FactoryRuntime::root &&
          runtime.Find(5) == FactoryRuntime::right &&
          runtime.Find(-3) == FactoryRuntime::head &&
          runtime.Find(1) == FactoryRuntime::head &&
          runtime.Find(6) == FactoryRuntime::head,
          "signed factory exact lookup");
    runtime.words[FactoryRuntime::cursor] = FactoryRuntime::left;
    Check(pal4::PalGameObjectTree::AdvanceFactory(
              FactoryRuntime::cursor, FactoryRuntime::output,
              runtime) == FactoryRuntime::output &&
          runtime.words.at(FactoryRuntime::cursor) == FactoryRuntime::root &&
          runtime.words.at(FactoryRuntime::output) == FactoryRuntime::left,
          "factory post-increment");
    static_cast<void>(pal4::PalGameObjectTree::NextFactory(
        FactoryRuntime::cursor, runtime));
    Check(runtime.words.at(FactoryRuntime::cursor) == FactoryRuntime::right,
          "factory successor");
    Check(pal4::PalGameObjectTree::RemoveFactory(
              FactoryRuntime::tree, FactoryRuntime::output,
              FactoryRuntime::root, runtime) == FactoryRuntime::output &&
          runtime.words.at(FactoryRuntime::output) == FactoryRuntime::right &&
          runtime.words.at(FactoryRuntime::tree + 12u) == 2u &&
          runtime.freed == std::vector<std::uint32_t>{FactoryRuntime::root} &&
          runtime.Find(-2) == FactoryRuntime::left &&
          runtime.Find(5) == FactoryRuntime::right,
          "factory two-child deletion");
    const auto remaining = runtime.words.at(FactoryRuntime::head + 4u);
    Check(pal4::PalGameObjectTree::ClearFactoryNodes(
              remaining,
              static_cast<pal4::IPalGameObjectFactoryClearRuntime&>(runtime)) ==
              FactoryRuntime::nil &&
          runtime.freed.size() == 3u &&
          runtime.freed[1] == FactoryRuntime::right &&
          runtime.freed[2] == FactoryRuntime::left,
          "factory recursive cleanup order");
    Check(pal4::PalGameObjectTree::ClearFactoryNodes(
              FactoryRuntime::nil,
              static_cast<pal4::IPalGameObjectFactoryClearRuntime&>(runtime)) ==
              FactoryRuntime::nil,
          "empty factory cleanup");
}

struct FactoryInsertRuntime final : pal4::IPalGameObjectFactoryInsertRuntime,
                                    pal4::IUiRedBlackTreeIteratorRuntime {
    static constexpr std::uint32_t tree = 0x1000u;
    static constexpr std::uint32_t head = 0x2000u;
    static constexpr std::uint32_t nil = 0x3000u;
    static constexpr std::uint32_t output = 0x4000u;
    static constexpr std::uint32_t cursor = 0x4100u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::int32_t, std::uint32_t> nodes;
    std::uint32_t next_node = 0x8000u;

    FactoryInsertRuntime() {
        words[tree + 4u] = head;
        words[tree + 12u] = 0u;
        words[head] = head;
        words[head + 4u] = nil;
        words[head + 8u] = head;
        words[nil] = nil;
        words[nil + 4u] = head;
        words[nil + 8u] = nil;
        words[nil + 20u] = 1u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override { words[address] = value; }
    std::uint32_t SharedNil() override { return nil; }
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        Check(size == 0x18u, "factory node allocation size");
        const auto result = next_node;
        next_node += 0x100u;
        return result;
    }
    std::uint32_t Insert(std::int32_t key) {
        const auto value = 0x5000u +
            static_cast<std::uint32_t>(nodes.size()) * 0x100u;
        words[value] = static_cast<std::uint32_t>(key);
        words[value + 4u] = value + 0x80u;
        auto parent = head;
        auto child = words.at(head + 4u);
        while (child != nil) {
            parent = child;
            child = words.at(child +
                (key < static_cast<std::int32_t>(words.at(child + 12u))
                    ? 0u : 8u));
        }
        auto& links = static_cast<pal4::IPalGameObjectFactoryInsertRuntime&>(
            *this);
        Check(pal4::PalGameObjectTree::InsertFactoryNode(
                  tree, output, child, parent, value, links) == output,
              "factory node insert output");
        const auto node = words.at(output);
        Check(words.at(node + 12u) == static_cast<std::uint32_t>(key) &&
              words.at(node + 16u) == value + 0x80u,
              "factory copied key and callback pair");
        nodes[key] = node;
        return node;
    }
    int Validate(std::uint32_t node, std::uint32_t parent,
                 std::vector<std::int32_t>& order) {
        if (node == nil) return 1;
        Check(words.at(node + 4u) == parent, "factory parent link");
        const auto left = words.at(node);
        const auto right = words.at(node + 8u);
        if (words.at(node + 20u) == 0u)
            Check(words.at(left + 20u) == 1u &&
                  words.at(right + 20u) == 1u,
                  "factory red node has black children");
        const auto left_height = Validate(left, node, order);
        order.push_back(static_cast<std::int32_t>(words.at(node + 12u)));
        const auto right_height = Validate(right, node, order);
        Check(left_height == right_height, "factory black height");
        return left_height + (words.at(node + 20u) == 1u);
    }
    void CheckTree(std::vector<std::int32_t> expected) {
        const auto root = words.at(head + 4u);
        Check(root != nil && words.at(root + 20u) == 1u,
              "factory black root");
        std::vector<std::int32_t> order;
        Validate(root, head, order);
        std::sort(expected.begin(), expected.end());
        Check(order == expected && words.at(tree + 12u) == expected.size(),
              "factory signed order and count");
        Check(words.at(head) == nodes.at(expected.front()) &&
              words.at(head + 8u) == nodes.at(expected.back()),
              "factory head extrema");
        for (const auto key : expected) {
            const auto query = 0x7000u;
            words[query] = static_cast<std::uint32_t>(key);
            Check(pal4::PalGameObjectTree::FindFactory(
                      tree, output, query,
                      static_cast<pal4::IPalGameObjectFactoryInsertRuntime&>(
                          *this)) == output &&
                  words.at(output) == nodes.at(key),
                  "factory inserted key lookup");
        }
    }
};

void CheckFactoryInsertions() {
    std::vector<std::int32_t> keys{-7, -2, 0, 3, 8};
    do {
        FactoryInsertRuntime runtime;
        std::vector<std::int32_t> inserted;
        for (const auto key : keys) {
            runtime.Insert(key);
            inserted.push_back(key);
            runtime.CheckTree(inserted);
        }
        runtime.words[FactoryInsertRuntime::cursor] = runtime.nodes.at(3);
        static_cast<void>(pal4::PalGameObjectTree::PreviousFactory(
            FactoryInsertRuntime::cursor, runtime));
        Check(runtime.words.at(FactoryInsertRuntime::cursor) ==
                  runtime.nodes.at(0),
              "factory predecessor");
    } while (std::next_permutation(keys.begin(), keys.end()));
}

}  // namespace

int main() {
    try {
        CheckNameTree();
        CheckNameNodeDirect();
        CheckNameRotationsAndDuplicates();
        CheckFactoryTree();
        CheckFactoryInsertions();
        std::cout << "pal_game_object_tree_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
