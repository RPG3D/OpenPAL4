#include "pal4/ui_red_black_tree_insert.h"

#include <algorithm>
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

struct Runtime final : pal4::IUiRedBlackTreeInsertRuntime {
    static constexpr std::uint32_t tree = 0x1000u;
    static constexpr std::uint32_t head = 0x2000u;
    static constexpr std::uint32_t nil = 0x3000u;
    static constexpr std::uint32_t output = 0x4000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, int> text_values;
    std::uint32_t next = 0x5000u;
    unsigned copies = 0u;

    Runtime() {
        words[tree + 4u] = head;
        words[tree + 12u] = 0u;
        words[head] = head;
        words[head + 4u] = nil;
        words[head + 8u] = head;
        words[head + 32u] = 0u;
        words[nil] = nil;
        words[nil + 4u] = 0u;
        words[nil + 8u] = nil;
        words[nil + 32u] = 1u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t SharedNil() override { return nil; }
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        Check(size == 0x24u, "node allocation size");
        const auto node = next;
        next += 0x100u;
        return node;
    }
    void CopyText(std::uint32_t destination,
                  std::uint32_t source) override {
        ++copies;
        text_values[destination] = text_values.at(source);
    }
    std::int32_t CompareText(std::uint32_t first,
                             std::uint32_t second) override {
        const auto a = text_values.at(first);
        const auto b = text_values.at(second);
        return (a > b) - (a < b);
    }
    int Validate(std::uint32_t node, std::uint32_t parent,
                 std::vector<int>& order) {
        if (node == nil) return 1;
        Check(words.at(node + 4u) == parent, "parent link");
        const auto left = words.at(node);
        const auto right = words.at(node + 8u);
        if (words.at(node + 32u) == 0u)
            Check(words.at(left + 32u) == 1u &&
                      words.at(right + 32u) == 1u,
                  "red node children");
        const auto left_height = Validate(left, node, order);
        order.push_back(text_values.at(node + 12u));
        const auto right_height = Validate(right, node, order);
        Check(left_height == right_height, "black height");
        return left_height + (words.at(node + 32u) == 1u);
    }
    void CheckTree(const std::vector<int>& inserted) {
        const auto root = words.at(head + 4u);
        Check(root != nil && words.at(root + 32u) == 1u,
              "root is black");
        std::vector<int> order;
        Validate(root, head, order);
        auto sorted = inserted;
        std::sort(sorted.begin(), sorted.end());
        Check(order == sorted, "in-order values");
        Check(words.at(tree + 12u) == inserted.size(), "node count");
        auto minimum = root;
        while (words.at(minimum) != nil) minimum = words.at(minimum);
        auto maximum = root;
        while (words.at(maximum + 8u) != nil)
            maximum = words.at(maximum + 8u);
        Check(words.at(head) == minimum &&
                  words.at(head + 8u) == maximum,
              "head extrema");
    }
    void InsertKey(int value, std::vector<int>& inserted) {
        const auto key = 0xa000u + static_cast<std::uint32_t>(value) * 4u;
        text_values[key] = value;
        auto parent = head;
        auto child = words.at(head + 4u);
        while (child != nil) {
            parent = child;
            child = value < text_values.at(parent + 12u)
                        ? words.at(parent) : words.at(parent + 8u);
        }
        const auto expected = next;
        Check(pal4::UiRedBlackTreeInsert::Insert(
                  tree, output, child, parent, key, *this) == output &&
                  words.at(output) == expected,
              "insert result");
        inserted.push_back(value);
        CheckTree(inserted);
        Check(copies == inserted.size(), "one text copy per node");
    }
};

void CheckPermutations() {
    std::vector<int> permutation{1, 2, 3, 4, 5};
    do {
        Runtime runtime;
        std::vector<int> inserted;
        for (const auto value : permutation)
            runtime.InsertKey(value, inserted);
    } while (std::next_permutation(permutation.begin(), permutation.end()));
}

void CheckDuplicates() {
    Runtime runtime;
    std::vector<int> inserted;
    for (const auto value : {5, 5, 5, 3, 7, 5})
        runtime.InsertKey(value, inserted);
}

}  // namespace

int main() {
    try {
        CheckPermutations();
        CheckDuplicates();
        std::cout << "ui_red_black_tree_insert_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
