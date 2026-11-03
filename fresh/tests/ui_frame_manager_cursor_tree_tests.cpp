#include "pal4/ui_frame_manager_cursor_tree.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerCursorTreeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::string> events;
    std::uint32_t next_node = 0x7000u;
    std::uint32_t predecessor = 0x7000u;
    std::uint32_t inserted_child = 0u;
    std::uint32_t inserted_parent = 0u;
    std::uint32_t inserted_key = 0u;
    bool throw_compare = false;

    Runtime() {
        words[0x5004u] = 0x6000u;
        words[0x500cu] = 0u;
        words[0x6000u] = 0x6000u;
        words[0x6004u] = 0x9000u;
        words[0x6008u] = 0x6000u;
        words[0x9020u] = 1u;
        bytes[0x5008u] = 1u;
        strings[0xa000u] = "m";
        strings[0xa100u] = "z";
        strings[0xa200u] = "zz";
        strings[0xa300u] = "a";
        strings[0xa400u] = "y";
        strings[0xa500u] = "b";
        strings[0xb000u] = "";
        for (const auto key : {0xa000u, 0xa100u, 0xa200u,
                               0xa300u, 0xa400u, 0xa500u}) {
            words[key + 4u] = key;
            words[key + 8u] =
                static_cast<std::uint32_t>(strings.at(key).size());
        }
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
    std::uint32_t SharedNil() override { return 0x9000u; }
    std::uint32_t EmptyText() override {
        events.emplace_back("empty text");
        return 0xb000u;
    }
    std::int32_t CompareRange(std::uint32_t key,
                              std::uint32_t text,
                              std::uint32_t length) override {
        Check(strings.at(text).size() == length, "node text length");
        events.emplace_back("compare range");
        if (throw_compare) throw std::runtime_error("compare failed");
        return strings.at(key).compare(strings.at(text));
    }
    std::int32_t CompareText(std::uint32_t first,
                             std::uint32_t second) override {
        events.emplace_back("compare text");
        return strings.at(first).compare(strings.at(second));
    }
    std::int32_t CompareBytes(std::uint32_t first,
                              std::uint32_t second,
                              std::uint32_t length) override {
        events.emplace_back("compare bytes");
        Check(strings.at(first).size() >= length &&
                  strings.at(second).size() >= length,
              "memcmp bounded by both strings");
        return strings.at(first).compare(
            0u, length, strings.at(second), 0u, length);
    }
    std::uint32_t AllocateNode(std::uint32_t tree,
                               std::uint32_t parent,
                               std::uint32_t flag) override {
        Check(tree == 0x5000u && flag == 0u,
              "duplicate-mode allocator arguments");
        const auto node = next_node;
        next_node += 0x100u;
        words[node + 4u] = parent;
        words[node + 0x20u] = 0u;
        events.emplace_back("allocate node");
        return node;
    }
    void CopyText(std::uint32_t destination,
                  std::uint32_t source) override {
        strings[destination] = strings.at(source);
        words[destination + 4u] = source;
        words[destination + 8u] =
            static_cast<std::uint32_t>(strings.at(source).size());
        events.emplace_back("copy text");
    }
    std::uint32_t Predecessor(std::uint32_t node) override {
        Check(node == 0x7100u, "predecessor starts at right node");
        events.emplace_back("predecessor");
        return predecessor;
    }
    std::uint32_t InsertNode(std::uint32_t tree,
                             std::uint32_t child,
                             std::uint32_t parent,
                             std::uint32_t key) override {
        Check(tree == 0x5000u, "unique-mode helper owner");
        inserted_child = child;
        inserted_parent = parent;
        inserted_key = key;
        events.emplace_back("insert helper");
        return 0x7700u;
    }
    void SetExistingResult(std::uint32_t output,
                           std::uint32_t node) override {
        words[output] = node;
        bytes[output + 4u] = 0u;
        events.emplace_back("existing result");
    }
    void RotateLeft(std::uint32_t tree,
                    std::uint32_t node) override {
        Check(tree == 0x5000u, "left rotation owner");
        const auto right = words.at(node + 8u);
        const auto middle = words.at(right);
        words[node + 8u] = middle;
        if (middle != 0x9000u) words[middle + 4u] = node;
        const auto parent = words.at(node + 4u);
        words[right + 4u] = parent;
        if (node == words.at(0x6004u)) words[0x6004u] = right;
        else if (node == words.at(parent)) words[parent] = right;
        else words[parent + 8u] = right;
        words[right] = node;
        words[node + 4u] = right;
        events.emplace_back("rotate left");
    }
    void RotateRight(std::uint32_t tree,
                     std::uint32_t node) override {
        Check(tree == 0x5000u, "right rotation owner");
        const auto left = words.at(node);
        const auto middle = words.at(left + 8u);
        words[node] = middle;
        if (middle != 0x9000u) words[middle + 4u] = node;
        const auto parent = words.at(node + 4u);
        words[left + 4u] = parent;
        if (node == words.at(0x6004u)) words[0x6004u] = left;
        else if (node == words.at(parent)) words[parent] = left;
        else words[parent + 8u] = left;
        words[left + 8u] = node;
        words[node + 4u] = left;
        events.emplace_back("rotate right");
    }

    void ConfigureRoot(std::uint32_t node,
                       std::uint32_t key) {
        next_node = node + 0x100u;
        words[0x6000u] = node;
        words[0x6004u] = node;
        words[0x6008u] = node;
        words[node] = 0x9000u;
        words[node + 4u] = 0x6000u;
        words[node + 8u] = 0x9000u;
        words[node + 0x20u] = 1u;
        CopyText(node + 0x0cu, key);
        events.clear();
        words[0x500cu] = 1u;
    }
};

void CheckDuplicateMode() {
    Runtime runtime;
    Check(pal4::UiFrameManagerCursorTree::InsertOrFind(
              0x5000u, 0xc000u, 0xa000u, runtime) == 0xc000u,
          "empty insert returns output");
    Check(runtime.words[0xc000u] == 0x7000u &&
              runtime.bytes[0xc004u] == 1u &&
              runtime.words[0x500cu] == 1u &&
              runtime.words[0x6000u] == 0x7000u &&
              runtime.words[0x6004u] == 0x7000u &&
              runtime.words[0x6008u] == 0x7000u &&
              runtime.words[0x7020u] == 1u,
          "first node links head and becomes black root");
    (void)pal4::UiFrameManagerCursorTree::InsertOrFind(
        0x5000u, 0xc000u, 0xa100u, runtime);
    (void)pal4::UiFrameManagerCursorTree::InsertOrFind(
        0x5000u, 0xc000u, 0xa200u, runtime);
    Check(runtime.words[0x6004u] == 0x7100u &&
              runtime.words[0x7100u] == 0x7000u &&
              runtime.words[0x7108u] == 0x7200u &&
              runtime.words[0x7120u] == 1u &&
              runtime.words[0x500cu] == 3u,
          "right chain rotates to balanced black root");
    Check(runtime.events.end() != std::find(runtime.events.begin(),
                                            runtime.events.end(),
                                            "rotate left"),
          "right-chain repair invokes left rotation");

    Runtime left_chain;
    (void)pal4::UiFrameManagerCursorTree::InsertOrFind(
        0x5000u, 0xc000u, 0xa100u, left_chain);
    (void)pal4::UiFrameManagerCursorTree::InsertOrFind(
        0x5000u, 0xc000u, 0xa000u, left_chain);
    (void)pal4::UiFrameManagerCursorTree::InsertOrFind(
        0x5000u, 0xc000u, 0xa300u, left_chain);
    Check(left_chain.words[0x6004u] == 0x7100u &&
              left_chain.words[0x7100u] == 0x7200u &&
              left_chain.words[0x7108u] == 0x7000u &&
              left_chain.words[0x7120u] == 1u &&
              left_chain.events.end() !=
                  std::find(left_chain.events.begin(),
                            left_chain.events.end(), "rotate right"),
          "left chain rotates to balanced black root");

    Runtime red_uncle;
    for (const auto key : {0xa000u, 0xa300u, 0xa100u, 0xa500u})
        (void)pal4::UiFrameManagerCursorTree::InsertOrFind(
            0x5000u, 0xc000u, key, red_uncle);
    Check(red_uncle.words[0x6004u] == 0x7000u &&
              red_uncle.words[0x7020u] == 1u &&
              red_uncle.words[0x7120u] == 1u &&
              red_uncle.words[0x7220u] == 1u &&
              red_uncle.words[0x500cu] == 4u,
          "red uncle recolors without changing black root");

    Runtime equal_key;
    equal_key.ConfigureRoot(0x7000u, 0xa000u);
    (void)pal4::UiFrameManagerCursorTree::InsertOrFind(
        0x5000u, 0xc000u, 0xa000u, equal_key);
    Check(equal_key.words[0x7008u] == 0x7100u,
          "duplicate key enters right insertion path");
    Check(equal_key.words[0x500cu] == 2u &&
              equal_key.bytes[0xc004u] == 1u,
          "duplicate mode increments count for equal key");
}

void CheckUniqueMode() {
    Runtime empty;
    empty.bytes[0x5008u] = 0u;
    (void)pal4::UiFrameManagerCursorTree::InsertOrFind(
        0x5000u, 0xc000u, 0xa000u, empty);
    Check(empty.inserted_child == 0x9000u &&
              empty.inserted_parent == 0x6000u &&
              empty.inserted_key == 0xa000u &&
              empty.words[0xc000u] == 0x7700u &&
              empty.bytes[0xc004u] == 1u,
          "empty unique tree forwards to insertion helper");

    Runtime existing;
    existing.bytes[0x5008u] = 0u;
    existing.ConfigureRoot(0x7000u, 0xa000u);
    (void)pal4::UiFrameManagerCursorTree::InsertOrFind(
        0x5000u, 0xc000u, 0xa000u, existing);
    Check(existing.words[0xc000u] == 0x7000u &&
              existing.bytes[0xc004u] == 0u &&
              existing.events.back() == "existing result",
          "equal key reuses existing node");

    Runtime leftmost;
    leftmost.bytes[0x5008u] = 0u;
    leftmost.ConfigureRoot(0x7000u, 0xa000u);
    (void)pal4::UiFrameManagerCursorTree::InsertOrFind(
        0x5000u, 0xc000u, 0xa300u, leftmost);
    Check(leftmost.inserted_parent == 0x7000u &&
              leftmost.inserted_key == 0xa300u &&
              leftmost.events.back() == "insert helper",
          "new leftmost key directly inserts");

    Runtime successor;
    successor.bytes[0x5008u] = 0u;
    successor.ConfigureRoot(0x7000u, 0xa000u);
    (void)pal4::UiFrameManagerCursorTree::InsertOrFind(
        0x5000u, 0xc000u, 0xa100u, successor);
    Check(successor.inserted_parent == 0x7000u &&
              successor.inserted_key == 0xa100u,
          "greater key inserts after candidate comparison");

    Runtime predecessor;
    predecessor.bytes[0x5008u] = 0u;
    predecessor.ConfigureRoot(0x7000u, 0xa000u);
    predecessor.words[0x7008u] = 0x7100u;
    predecessor.words[0x6008u] = 0x7100u;
    predecessor.words[0x7100u] = 0x9000u;
    predecessor.words[0x7104u] = 0x7000u;
    predecessor.words[0x7108u] = 0x9000u;
    predecessor.CopyText(0x710cu, 0xa100u);
    predecessor.events.clear();
    (void)pal4::UiFrameManagerCursorTree::InsertOrFind(
        0x5000u, 0xc000u, 0xa400u, predecessor);
    Check(predecessor.inserted_parent == 0x7100u &&
              predecessor.events.end() !=
                  std::find(predecessor.events.begin(),
                            predecessor.events.end(), "predecessor"),
          "left branch away from minimum checks predecessor then inserts");
}

void CheckEmptyTextAndException() {
    Runtime runtime;
    runtime.bytes[0x5008u] = 0u;
    runtime.ConfigureRoot(0x7000u, 0xa000u);
    runtime.words[0x7010u] = 0u;
    runtime.words[0x7014u] = 0u;
    (void)pal4::UiFrameManagerCursorTree::InsertOrFind(
        0x5000u, 0xc000u, 0xa100u, runtime);
    Check(runtime.events.end() != std::find(runtime.events.begin(),
                                            runtime.events.end(),
                                            "empty text"),
          "null node text uses global empty string");

    Runtime failure;
    failure.ConfigureRoot(0x7000u, 0xa000u);
    failure.throw_compare = true;
    bool caught = false;
    try {
        (void)pal4::UiFrameManagerCursorTree::InsertOrFind(
            0x5000u, 0xc000u, 0xa100u, failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failure.words.find(0xc000u) == failure.words.end() &&
              failure.next_node == 0x7100u,
          "comparison exception stops before allocation and output write");
}

void CheckFind() {
    Runtime empty;
    Check(pal4::UiFrameManagerCursorTree::Find(
              0x5000u, 0xc100u, 0xa000u, empty) == 0xc100u &&
              empty.words[0xc100u] == 0x6000u,
          "empty tree returns head through output");

    Runtime runtime;
    runtime.ConfigureRoot(0x7000u, 0xa000u);
    (void)pal4::UiFrameManagerCursorTree::Find(
        0x5000u, 0xc100u, 0xa000u, runtime);
    Check(runtime.words[0xc100u] == 0x7000u &&
              runtime.events.back() == "compare bytes",
          "exact key returns candidate after byte check");
    (void)pal4::UiFrameManagerCursorTree::Find(
        0x5000u, 0xc100u, 0xa300u, runtime);
    Check(runtime.words[0xc100u] == 0x6000u,
          "key below candidate is absent");
    (void)pal4::UiFrameManagerCursorTree::Find(
        0x5000u, 0xc100u, 0xa100u, runtime);
    Check(runtime.words[0xc100u] == 0x6000u,
          "key above the only node is absent");

    runtime.words[0x7008u] = 0x7100u;
    runtime.words[0x7100u] = 0x9000u;
    runtime.words[0x7108u] = 0x9000u;
    runtime.CopyText(0x710cu, 0xa100u);
    (void)pal4::UiFrameManagerCursorTree::Find(
        0x5000u, 0xc100u, 0xa100u, runtime);
    Check(runtime.words[0xc100u] == 0x7100u,
          "search descends right to exact key");

    Runtime prefix;
    prefix.strings[0xa600u] = "maze";
    prefix.words[0xa604u] = 0xa600u;
    prefix.words[0xa608u] = 4u;
    prefix.ConfigureRoot(0x7000u, 0xa600u);
    (void)pal4::UiFrameManagerCursorTree::Find(
        0x5000u, 0xc100u, 0xa000u, prefix);
    Check(prefix.words[0xc100u] == 0x6000u,
          "shorter key with equal prefix is absent");

    Runtime null_text;
    null_text.strings[0xa700u] = "";
    null_text.words[0xa704u] = 0xa700u;
    null_text.words[0xa708u] = 0u;
    null_text.ConfigureRoot(0x7000u, 0xa700u);
    null_text.words[0x7010u] = 0u;
    null_text.events.clear();
    (void)pal4::UiFrameManagerCursorTree::Find(
        0x5000u, 0xc100u, 0xa700u, null_text);
    Check(null_text.words[0xc100u] == 0x7000u &&
              null_text.events.end() !=
                  std::find(null_text.events.begin(),
                            null_text.events.end(), "empty text"),
          "null node text falls back to global empty string");
}

}  // namespace

int main() {
    try {
        CheckDuplicateMode();
        CheckUniqueMode();
        CheckEmptyTextAndException();
        CheckFind();
        std::cout << "ui_frame_manager_cursor_tree_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
