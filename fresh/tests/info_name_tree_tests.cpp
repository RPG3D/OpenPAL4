#include "pal4/info_name_tree.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::NpcInfoRuntimeStub {
    std::uint32_t next = 0x8000u;
    bool fail_allocation = false;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::uint32_t> frees;
    std::uint32_t Allocate(std::uint32_t) override {
        if (fail_allocation) return 0;
        const auto result = next;
        next += 0x100u;
        return result;
    }
    void Free(const std::uint32_t address) override { frees.push_back(address); }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override { words[address] = value; }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override { bytes[address] = value; }
    void SetStringFlag(const std::uint32_t address,
                       const std::uint8_t value) override {
        bytes[address] = value;
    }
    void InitializeString(const std::uint32_t address) override {
        strings[address].clear();
    }
    void AssignString(const std::uint32_t address,
                      const std::string_view value) override {
        strings[address] = value;
    }
    std::string ReadStringValue(const std::uint32_t address) override {
        return strings[address];
    }
    void CleanupString(const std::uint32_t address) override {
        strings.erase(address);
    }
};

constexpr auto tree = 0x5000u;
constexpr auto head = 0x6000u;
constexpr auto nil = 0x7000u;

void Initialize(Runtime& runtime) {
    runtime.words[tree + 4u] = head;
    runtime.words[head] = head;
    runtime.words[head + 4u] = nil;
    runtime.words[head + 8u] = head;
    runtime.words[nil + 32u] = 1;
}

int CheckNode(const std::uint32_t node, const std::uint32_t parent,
              const std::string* lower, const std::string* upper,
              Runtime& runtime) {
    if (node == nil) return 1;
    Check(runtime.words[node + 4u] == parent, "parent link");
    const auto key = runtime.strings[node + 12u];
    Check((!lower || *lower < key) && (!upper || key < *upper),
          "binary-search order");
    const auto left = runtime.words[node];
    const auto right = runtime.words[node + 8u];
    const auto black = runtime.words[node + 32u] != 0u;
    if (!black) {
        Check(left == nil || runtime.words[left + 32u] != 0u,
              "red left child");
        Check(right == nil || runtime.words[right + 32u] != 0u,
              "red right child");
    }
    const auto left_height = CheckNode(left, node, lower, &key, runtime);
    const auto right_height = CheckNode(right, node, &key, upper, runtime);
    Check(left_height == right_height, "equal black height");
    return left_height + (black ? 1 : 0);
}

void CheckTree(const std::set<std::string>& expected, Runtime& runtime) {
    const auto root = runtime.words[head + 4u];
    Check(runtime.words[tree + 12u] == expected.size(), "tree count");
    if (expected.empty()) {
        Check(root == nil && runtime.words[head] == head &&
              runtime.words[head + 8u] == head, "empty head links");
        return;
    }
    Check(runtime.words[root + 32u] == 1u &&
          runtime.words[root + 4u] == head,
          "black root and head parent");
    (void)CheckNode(root, head, nullptr, nullptr, runtime);
    auto node = runtime.words[head];
    Check(runtime.strings[node + 12u] == *expected.begin(), "minimum link");
    Check(runtime.strings[runtime.words[head + 8u] + 12u] ==
              *expected.rbegin(), "maximum link");
    for (const auto& key : expected) {
        Check(node != head && runtime.strings[node + 12u] == key &&
              pal4::InfoNameTree::Find(tree, key, nil, runtime) == node,
              "in-order walk and exact lookup");
        node = pal4::InfoNameTree::Next(node, head, nil, runtime);
    }
    Check(node == head, "successor reaches head");
    node = pal4::InfoNameTree::Previous(head, head, nil, runtime);
    for (auto it = expected.rbegin(); it != expected.rend(); ++it) {
        Check(node != head && runtime.strings[node + 12u] == *it,
              "reverse walk");
        node = pal4::InfoNameTree::Previous(node, head, nil, runtime);
    }
    Check(node == head, "predecessor reaches head");
}

void InsertDeleteAndBalance() {
    Runtime runtime;
    Initialize(runtime);
    std::set<std::string> expected;
    const std::vector<std::string> insertion = {
        "m", "d", "t", "b", "h", "p", "w", "a", "c", "f",
        "j", "n", "r", "u", "z", "e", "g", "i", "k", "q"};
    for (std::uint32_t index = 0; index < insertion.size(); ++index) {
        const auto& name = insertion[index];
        const auto result = pal4::InfoNameTree::Insert(
            tree, name, 0x10000u + index, nil, runtime);
        Check(result.inserted && result.node != 0 &&
              runtime.words[result.node + 28u] == 0x10000u + index,
              "new node payload");
        expected.insert(name);
        CheckTree(expected, runtime);
    }
    const auto old_next = runtime.next;
    const auto duplicate = pal4::InfoNameTree::Insert(
        tree, "h", 0xdeadu, nil, runtime);
    Check(!duplicate.inserted && runtime.next == old_next &&
          runtime.words[duplicate.node + 28u] != 0xdeadu,
          "duplicate preserves prior payload and allocation count");
    Check(pal4::InfoNameTree::Find(tree, "missing", nil, runtime) == head &&
          runtime.strings[pal4::InfoNameTree::LowerBound(
              tree, "l", nil, runtime) + 12u] == "m",
          "miss and lower bound");
    const std::vector<std::string> deletion = {
        "m", "a", "z", "d", "t", "h", "p", "w", "b", "c",
        "f", "j", "n", "r", "u", "e", "g", "i", "k", "q"};
    for (const auto& name : deletion) {
        const auto node = pal4::InfoNameTree::Find(tree, name, nil, runtime);
        Check(node != head, "node present before erase");
        pal4::InfoNameTree::Erase(tree, node, nil, runtime);
        expected.erase(name);
        CheckTree(expected, runtime);
    }
    Check(runtime.frees.size() == deletion.size(), "all nodes freed");
}

void InterleavedMutations() {
    Runtime runtime;
    Initialize(runtime);
    std::set<std::string> expected;
    for (auto step = 0u; step < 320u; ++step) {
        const auto value = (step * 37u) % 97u;
        const auto key = std::to_string(value);
        if (step % 4u == 0u) {
            const auto node = pal4::InfoNameTree::Find(tree, key, nil, runtime);
            if (node != head) {
                pal4::InfoNameTree::Erase(tree, node, nil, runtime);
                expected.erase(key);
            }
        } else {
            const auto result = pal4::InfoNameTree::Insert(
                tree, key, value, nil, runtime);
            Check(result.inserted == expected.insert(key).second,
                  "interleaved duplicate decision");
        }
        CheckTree(expected, runtime);
    }
    pal4::InfoNameTree::ClearNodes(tree, nil, runtime);
    CheckTree({}, runtime);
}

void ClearAndCopy() {
    Runtime runtime;
    Initialize(runtime);
    for (const auto name : {"middle", "left", "right"})
        (void)pal4::InfoNameTree::Insert(tree, name, 1, nil, runtime);
    pal4::InfoNameTree::ClearNodes(tree, nil, runtime);
    CheckTree({}, runtime);
    Check(runtime.frees.size() == 3 && runtime.strings.empty(),
          "right-first branch cleanup frees all tree keys");
}

void CopyEntryCases() {
    Runtime runtime;
    constexpr auto source = 0x9000u;
    constexpr auto destination = 0xa000u;
    constexpr auto text = 0xb001u;
    runtime.bytes[source] = 7;
    runtime.words[source + 4u] = text;
    runtime.words[source + 8u] = 3;
    runtime.words[source + 12u] = 8;
    runtime.words[source + 16u] = 0xdecau;
    runtime.bytes[text - 1u] = 2;
    runtime.bytes[text] = 'a';
    runtime.bytes[text + 1u] = 'b';
    runtime.bytes[text + 2u] = 'c';
    pal4::InfoNameTree::CopyEntry(destination, source, runtime);
    Check(runtime.bytes[destination] == 1 &&
          runtime.words[destination + 4u] == text &&
          runtime.words[destination + 8u] == 3 &&
          runtime.words[destination + 12u] == 8 &&
          runtime.words[destination + 16u] == 0xdecau &&
          runtime.bytes[text - 1u] == 3,
          "share eligible text and increment its count");
    runtime.bytes[text - 1u] = 0xfeu;
    constexpr auto deep = 0xa100u;
    pal4::InfoNameTree::CopyEntry(deep, source, runtime);
    const auto deep_text = runtime.words[deep + 4u];
    Check(deep_text != text && runtime.words[deep + 8u] == 3 &&
          runtime.words[deep + 12u] == 3 &&
          runtime.words[deep + 16u] == 0xdecau &&
          runtime.bytes[deep_text - 1u] == 0 &&
          runtime.bytes[deep_text] == 'a' &&
          runtime.bytes[deep_text + 2u] == 'c' &&
          runtime.bytes[deep_text + 3u] == 0 &&
          runtime.bytes[text - 1u] == 0xfeu,
          "saturated text is copied to a new buffer");
    runtime.words[source + 8u] = 0;
    constexpr auto empty = 0xa200u;
    pal4::InfoNameTree::CopyEntry(empty, source, runtime);
    Check(runtime.words[empty + 4u] != 0 &&
          runtime.words[empty + 8u] == 0 &&
          runtime.bytes[runtime.words[empty + 4u]] == 0,
          "empty text creates an empty buffer");
    const auto next = runtime.next;
    pal4::InfoNameTree::CopyEntry(0, source, runtime);
    pal4::InfoNameTree::CopyEntry(source, source, runtime);
    Check(runtime.next == next && runtime.words[source + 4u] == text,
          "null destination and same-record alias do not allocate");
    runtime.bytes[text - 1u] = 0xfdu;
    constexpr auto last_shared = 0xa300u;
    runtime.words[source + 8u] = 3;
    pal4::InfoNameTree::CopyEntry(last_shared, source, runtime);
    Check(runtime.words[last_shared + 4u] == text &&
          runtime.bytes[text - 1u] == 0xfeu,
          "last shareable reference reaches saturation");
    runtime.fail_allocation = true;
    constexpr auto failed = 0xa400u;
    pal4::InfoNameTree::CopyEntry(failed, source, runtime);
    Check(runtime.words[failed + 4u] == 0 &&
          runtime.words[failed + 8u] == 0 &&
          runtime.words[failed + 16u] == 0xdecau,
          "copy allocation failure retains metadata and empty destination");
}

}  // namespace

int main() {
    try {
        InsertDeleteAndBalance();
        InterleavedMutations();
        ClearAndCopy();
        CopyEntryCases();
        std::cout << "info_name_tree_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
