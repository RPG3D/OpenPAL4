#include "pal4/scenario_portrait_tree.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScenarioPortraitTreeRuntime {
    static constexpr std::uint32_t kTree = 0x1000u;
    static constexpr std::uint32_t kHeader = 0x4000u;
    static constexpr std::uint32_t kNil = 0x5000u;
    static constexpr std::uint32_t kOutput = 0x2000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, std::string> keys;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> cleaned;
    std::uint32_t flagged_address = 0u;
    std::uint32_t flagged_value = 0u;
    bool allocate_text_result = false;
    std::uint32_t allocate_text_data = 0xf000u;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> erases;
    std::uint32_t next_memory = 0x6000u;
    std::uint32_t shared_nil = kNil;
    std::uint32_t shared_refcount = 1u;
    std::uint32_t predecessor_input = 0u;

    Runtime() {
        words[kTree + 4u] = kHeader;
        words[kTree + 12u] = 0u;
        words[kHeader] = kHeader;
        words[kHeader + 4u] = kNil;
        words[kHeader + 8u] = kHeader;
        words[kHeader + 44u] = 0u;
        words[kNil + 44u] = 1u;
        words[kNil] = kNil;
        words[kNil + 8u] = kNil;
        bytes[kTree + 8u] = 1u;
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
        bytes[address] = value;
    }
    std::uint32_t SharedNil() override { return shared_nil; }
    void SetSharedNil(std::uint32_t address) override {
        calls.emplace_back("set-shared-nil");
        shared_nil = address;
    }
    std::uint32_t SharedNilRefCount() override {
        return shared_refcount;
    }
    void SetSharedNilRefCount(std::uint32_t count) override {
        calls.emplace_back("set-shared-refcount");
        shared_refcount = count;
    }
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        Check(size == 0x30u, "tree allocation size");
        calls.emplace_back("allocate-memory");
        const auto address = next_memory;
        next_memory += 0x100u;
        return address;
    }
    int CompareSearch(std::uint32_t value,
                      std::uint32_t node_data) override {
        calls.emplace_back("search");
        return keys.at(value).compare(keys.at(node_data));
    }
    int CompareData(std::uint32_t left,
                    std::uint32_t right) override {
        calls.emplace_back("compare");
        return keys.at(left).compare(keys.at(right));
    }
    void CopyData(std::uint32_t destination,
                  std::uint32_t source) override {
        calls.emplace_back("copy");
        keys[destination] = keys.at(source);
        words[destination + 4u] = 0u;
        words[destination + 8u] = 0u;
        words[destination + 12u] = 0u;
    }
    std::uint32_t EmptyData() override { return 0xe000u; }
    void AssignFirstBuffer(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t start,
                           std::uint32_t count) override {
        Check(destination != 0u && source != 0u &&
                  start == 0u && count == 0xffffffffu,
              "first buffer assignment arguments");
        calls.emplace_back("assign-first");
    }
    void EraseBuffer(std::uint32_t buffer,
                     std::uint32_t start,
                     std::uint32_t count) override {
        calls.emplace_back("erase");
        erases.emplace_back(start, count);
        Check(buffer != 0u, "erase buffer address");
    }
    bool AllocateText(std::uint32_t buffer,
                      std::uint32_t length,
                      std::uint8_t mode) override {
        Check(mode == 1u, "text allocation mode");
        calls.emplace_back("allocate-text");
        if (allocate_text_result)
            words[buffer + 4u] = allocate_text_data;
        (void)length;
        return allocate_text_result;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        calls.emplace_back("copy-bytes");
        for (std::uint32_t index = 0u; index < length; ++index)
            bytes[destination + index] = bytes.at(source + index);
    }
    void SetTextSize(std::uint32_t buffer,
                     std::uint32_t length) override {
        calls.emplace_back("set-text-size");
        words[buffer + 8u] = length;
        bytes[words.at(buffer + 4u) + length] = 0u;
    }
    std::uint32_t Predecessor(std::uint32_t node) override {
        calls.emplace_back("predecessor");
        predecessor_input = node;
        return pal4::ScenarioPortraitTree::Predecessor(
            node, *this);
    }
    std::uint32_t Successor(std::uint32_t node) override {
        calls.emplace_back("successor");
        return pal4::ScenarioPortraitTree::Successor(
            node, *this);
    }
    void RotateLeft(std::uint32_t tree,
                    std::uint32_t node) override {
        calls.emplace_back("rotate-left");
        (void)pal4::ScenarioPortraitTree::RotateLeft(
            tree, node, *this);
    }
    void RotateRight(std::uint32_t tree,
                     std::uint32_t node) override {
        calls.emplace_back("rotate-right");
        (void)pal4::ScenarioPortraitTree::RotateRight(
            tree, node, *this);
    }
    void SetErrorFlag(std::uint32_t address,
                      std::uint32_t value) override {
        calls.emplace_back("set-error-flag");
        flagged_address = address;
        flagged_value = value;
    }
    void FreeMemory(std::uint32_t address) override {
        calls.emplace_back("free");
        freed.push_back(address);
    }
    void CleanupProperties(std::uint32_t address) override {
        calls.emplace_back("cleanup-properties");
        cleaned.push_back(address);
    }

    std::uint32_t InsertValue(const std::string& value) {
        const auto address = 0x9000u +
            static_cast<std::uint32_t>(keys.size()) * 0x100u;
        keys[address] = value;
        Check(pal4::ScenarioPortraitTree::Insert(
                  kTree, kOutput, address, *this) == kOutput,
              "insert returns result pointer");
        return words.at(kOutput);
    }
};

int ValidateSubtree(const Runtime& runtime, std::uint32_t node,
                    std::vector<std::string>& sorted,
                    unsigned depth = 0u) {
    if (node == Runtime::kNil) return 1;
    Check(depth < 32u, "tree has no cycle");
    const auto left = runtime.words.at(node);
    const auto right = runtime.words.at(node + 8u);
    const auto colour = runtime.words.at(node + 44u);
    if (colour == 0u) {
        Check(runtime.words.at(left + 44u) == 1u &&
                  runtime.words.at(right + 44u) == 1u,
              "red node has black children");
    }
    if (left != Runtime::kNil)
        Check(runtime.words.at(left + 4u) == node,
              "left parent link");
    if (right != Runtime::kNil)
        Check(runtime.words.at(right + 4u) == node,
              "right parent link");
    const auto left_height = ValidateSubtree(
        runtime, left, sorted, depth + 1u);
    sorted.push_back(runtime.keys.at(node + 12u));
    const auto right_height = ValidateSubtree(
        runtime, right, sorted, depth + 1u);
    Check(left_height == right_height, "equal black heights");
    return left_height + static_cast<int>(colour);
}

void ValidateTree(const Runtime& runtime,
                  std::size_t expected_count) {
    const auto root = runtime.words.at(Runtime::kHeader + 4u);
    Check(runtime.words.at(root + 44u) == 1u &&
              runtime.words.at(Runtime::kTree + 12u) ==
                  expected_count,
          "root black and count updated");
    std::vector<std::string> sorted;
    (void)ValidateSubtree(runtime, root, sorted);
    Check(sorted.size() == expected_count &&
              std::is_sorted(sorted.begin(), sorted.end()),
          "in-order keys sorted");
    auto leftmost = root;
    while (runtime.words.at(leftmost) != Runtime::kNil)
        leftmost = runtime.words.at(leftmost);
    auto rightmost = root;
    while (runtime.words.at(rightmost + 8u) != Runtime::kNil)
        rightmost = runtime.words.at(rightmost + 8u);
    Check(runtime.words.at(Runtime::kHeader) == leftmost &&
              runtime.words.at(Runtime::kHeader + 8u) == rightmost,
          "header tracks both extrema");
}

void CheckDuplicateMode(const std::vector<std::string>& values,
                        const std::vector<std::string>& rotations) {
    Runtime runtime;
    for (std::size_t index = 0u; index < values.size(); ++index) {
        const auto node = runtime.InsertValue(values[index]);
        Check(runtime.bytes.at(Runtime::kOutput + 4u) == 1u &&
                  node != Runtime::kNil,
              "duplicate mode reports insertion");
        ValidateTree(runtime, index + 1u);
    }
    std::vector<std::string> actual_rotations;
    for (const auto& call : runtime.calls)
        if (call == "rotate-left" || call == "rotate-right")
            actual_rotations.push_back(call);
    Check(actual_rotations == rotations, "rotation sequence");
}

void CheckUniqueMode() {
    Runtime empty;
    empty.bytes[Runtime::kTree + 8u] = 0u;
    Check(empty.InsertValue("key") == 0x6000u &&
              empty.bytes.at(Runtime::kOutput + 4u) == 1u,
          "empty unique tree inserts node");
    ValidateTree(empty, 1u);

    Runtime runtime;
    const auto root = runtime.InsertValue("key");
    runtime.bytes[Runtime::kTree + 8u] = 0u;
    Check(runtime.InsertValue("key") == root &&
              runtime.bytes.at(Runtime::kOutput + 4u) == 0u &&
              runtime.words.at(Runtime::kTree + 12u) == 1u,
          "unique duplicate returns existing node");
    const auto later = runtime.InsertValue("later");
    Check(later == 0x6100u &&
              runtime.words.at(later + 4u) == root,
          "unique greater key inserts right node");
    const auto earlier = runtime.InsertValue("earlier");
    Check(earlier == 0x6200u,
          "unique smaller key inserts left node");
    ValidateTree(runtime, 3u);

    Runtime predecessor;
    predecessor.InsertValue("middle");
    predecessor.InsertValue("z");
    predecessor.bytes[Runtime::kTree + 8u] = 0u;
    const auto inserted = predecessor.InsertValue("n");
    Check(inserted == 0x6200u &&
              predecessor.predecessor_input == 0x6100u &&
              predecessor.words.at(Runtime::kHeader + 4u) ==
                  inserted,
          "unique left descent checks predecessor before insert");
    ValidateTree(predecessor, 3u);
}

void CheckFind() {
    Runtime empty;
    empty.keys[0x9000u] = "any";
    Check(pal4::ScenarioPortraitTree::Find(
              Runtime::kTree, Runtime::kOutput, 0x9000u,
              empty) == Runtime::kOutput &&
              empty.words.at(Runtime::kOutput) == Runtime::kHeader,
          "empty lookup returns header");

    Runtime runtime;
    const auto middle = runtime.InsertValue("middle");
    const auto first = runtime.InsertValue("alpha");
    const auto last = runtime.InsertValue("zulu");
    runtime.InsertValue("middle");
    const auto find = [&](const char* key) {
        const auto address = 0xf000u;
        runtime.keys[address] = key;
        Check(pal4::ScenarioPortraitTree::Find(
                  Runtime::kTree, Runtime::kOutput,
                  address, runtime) == Runtime::kOutput,
              "lookup returns output pointer");
        return runtime.words.at(Runtime::kOutput);
    };
    Check(find("alpha") == first, "smallest key lookup");
    Check(find("zulu") == last, "largest key lookup");
    Check(find("middle") == middle,
          "duplicate lookup chooses lower-bound node");
    Check(find("aardvark") == Runtime::kHeader,
          "key below minimum is absent");
    Check(find("mids") == Runtime::kHeader,
          "key between nodes is absent");
    Check(find("zz") == Runtime::kHeader,
          "key above maximum is absent");
    Check(find("middl") == Runtime::kHeader,
          "short prefix is absent");
    Check(find("middle2") == Runtime::kHeader,
          "long prefix is absent");
}

void CheckRemoveSequence(const std::vector<std::string>& values,
                         const std::vector<std::size_t>& order) {
    Runtime runtime;
    std::vector<std::uint32_t> nodes;
    for (const auto& value : values)
        nodes.push_back(runtime.InsertValue(value));
    ValidateTree(runtime, values.size());
    std::vector<std::size_t> remaining(values.size());
    std::iota(remaining.begin(), remaining.end(), 0u);
    std::sort(remaining.begin(), remaining.end(),
              [&](std::size_t left, std::size_t right) {
                  return values[left] < values[right];
              });
    for (const auto index : order) {
        const auto found = std::find(remaining.begin(),
                                     remaining.end(), index);
        Check(found != remaining.end(), "remove index is live");
        const auto next = std::next(found);
        const auto expected_successor = next == remaining.end()
            ? Runtime::kHeader : nodes[*next];
        Check(pal4::ScenarioPortraitTree::Remove(
                  Runtime::kTree, Runtime::kOutput,
                  nodes[index], runtime) == Runtime::kOutput &&
                  runtime.words.at(Runtime::kOutput) ==
                      expected_successor,
              "remove returns original successor");
        Check(runtime.flagged_address == nodes[index] + 28u &&
                  runtime.flagged_value == 1u &&
                  runtime.freed.back() == nodes[index],
              "remove destroys original node");
        remaining.erase(found);
        if (remaining.empty()) {
            Check(runtime.words.at(Runtime::kHeader + 4u) ==
                      Runtime::kNil &&
                      runtime.words.at(Runtime::kHeader) ==
                          Runtime::kHeader &&
                      runtime.words.at(Runtime::kHeader + 8u) ==
                          Runtime::kHeader &&
                      runtime.words.at(Runtime::kTree + 12u) == 0u,
                  "empty tree resets root and extrema");
        } else {
            ValidateTree(runtime, remaining.size());
        }
    }
}

void CheckRemove() {
    const std::vector<std::string> values = {
        "a", "b", "c", "d", "e", "f", "g", "h", "i"};
    CheckRemoveSequence(values, {0, 1, 2, 3, 4, 5, 6, 7, 8});
    CheckRemoveSequence(values, {8, 7, 6, 5, 4, 3, 2, 1, 0});
    CheckRemoveSequence(values, {4, 2, 6, 1, 3, 5, 7, 0, 8});
    std::mt19937 random(0x4b87e0u);
    std::vector<std::size_t> order(values.size());
    std::iota(order.begin(), order.end(), 0u);
    for (int trial = 0; trial < 64; ++trial) {
        std::shuffle(order.begin(), order.end(), random);
        CheckRemoveSequence(values, order);
        auto insertion_order = values;
        std::shuffle(insertion_order.begin(),
                     insertion_order.end(), random);
        CheckRemoveSequence(insertion_order, order);
    }

    Runtime duplicates;
    const std::vector<std::string> duplicate_values = {
        "b", "a", "b", "c", "b", "a"};
    std::vector<std::uint32_t> duplicate_nodes;
    for (const auto& value : duplicate_values)
        duplicate_nodes.push_back(duplicates.InsertValue(value));
    for (const auto index : {2u, 0u, 5u, 4u, 1u, 3u}) {
        const auto expected = duplicates.Successor(
            duplicate_nodes[index]);
        (void)pal4::ScenarioPortraitTree::Remove(
            Runtime::kTree, Runtime::kOutput,
            duplicate_nodes[index], duplicates);
        Check(duplicates.words.at(Runtime::kOutput) == expected,
              "duplicate removal returns in-order successor");
        const auto remaining = duplicates.words.at(
            Runtime::kTree + 12u);
        if (remaining != 0u)
            ValidateTree(duplicates, remaining);
    }

    for (const auto count : {0u, 1u, 255u}) {
        Runtime runtime;
        const auto node = runtime.InsertValue("owned");
        const auto buffer = 0xa000u;
        runtime.words[node + 16u] = buffer;
        runtime.bytes[buffer - 1u] =
            static_cast<std::uint8_t>(count);
        (void)pal4::ScenarioPortraitTree::Remove(
            Runtime::kTree, Runtime::kOutput, node, runtime);
        Check(runtime.words.at(node + 16u) == 0u &&
                  runtime.words.at(node + 20u) == 0u &&
                  runtime.words.at(node + 24u) == 0u,
              "removed node clears string fields");
        if (count == 1u) {
            Check(runtime.bytes.at(buffer - 1u) == 0u &&
                      runtime.freed.size() == 1u,
                  "shared buffer decrements reference");
        } else {
            Check(runtime.freed.size() == 2u &&
                      runtime.freed.front() == buffer - 1u,
                  "unshared buffer is freed before node");
        }
    }
}

void CollectFreeOrder(const Runtime& runtime,
                      std::uint32_t node,
                      std::vector<std::uint32_t>& order) {
    if (node == Runtime::kNil) return;
    CollectFreeOrder(runtime,
                     runtime.words.at(node + 8u), order);
    order.push_back(node);
    CollectFreeOrder(runtime,
                     runtime.words.at(node), order);
}

void CheckFreeNodes() {
    Runtime empty;
    Check(pal4::ScenarioPortraitTree::FreeNodes(
              Runtime::kNil, empty) == Runtime::kNil &&
              empty.freed.empty() && empty.cleaned.empty(),
          "empty subtree only returns nil");

    Runtime runtime;
    for (const auto* key : {"d", "b", "f", "a", "c", "e", "g"})
        (void)runtime.InsertValue(key);
    std::vector<std::uint32_t> expected;
    CollectFreeOrder(runtime,
                     runtime.words.at(Runtime::kHeader + 4u),
                     expected);
    runtime.calls.clear();
    Check(pal4::ScenarioPortraitTree::FreeNodes(
              runtime.words.at(Runtime::kHeader + 4u),
              runtime) == Runtime::kNil &&
              runtime.freed == expected &&
              runtime.cleaned.size() == expected.size(),
          "subtree cleanup follows right-node-left order");
    for (std::size_t index = 0; index < expected.size(); ++index) {
        Check(runtime.cleaned[index] == expected[index] + 12u &&
                  runtime.calls[index * 2u] ==
                      "cleanup-properties" &&
                  runtime.calls[index * 2u + 1u] == "free",
              "each property cleanup precedes its node free");
    }
}

void CheckInitialize() {
    Runtime fresh;
    fresh.next_memory = 0x3000u;
    fresh.shared_nil = 0u;
    fresh.shared_refcount = 0u;
    fresh.calls.clear();
    const auto header = pal4::ScenarioPortraitTree::Initialize(
        Runtime::kTree, fresh);
    Check(header == 0x3100u && fresh.shared_nil == 0x3000u &&
              fresh.shared_refcount == 1u && fresh.freed.empty() &&
              fresh.words.at(0x3000u) == 0u &&
              fresh.words.at(0x3000u + 4u) == 0u &&
              fresh.words.at(0x3000u + 8u) == 0u &&
              fresh.words.at(0x3000u + 44u) == 1u,
          "first tree creates shared nil");
    Check(fresh.words.at(Runtime::kTree + 4u) == header &&
              fresh.words.at(Runtime::kTree + 12u) == 0u &&
              fresh.words.at(header) == header &&
              fresh.words.at(header + 4u) == fresh.shared_nil &&
              fresh.words.at(header + 8u) == header &&
              fresh.words.at(header + 44u) == 0u,
          "first tree creates empty header");

    Runtime subsequent;
    subsequent.next_memory = 0x3000u;
    subsequent.shared_refcount = 3u;
    subsequent.calls.clear();
    const auto next_header = pal4::ScenarioPortraitTree::Initialize(
        Runtime::kTree, subsequent);
    Check(next_header == 0x3100u &&
              subsequent.shared_nil == Runtime::kNil &&
              subsequent.shared_refcount == 4u &&
              subsequent.freed ==
                  std::vector<std::uint32_t>{0x3000u} &&
              subsequent.words.at(next_header + 4u) ==
                  Runtime::kNil,
          "subsequent tree discards temporary nil");
    Check(subsequent.calls ==
              std::vector<std::string>{
                  "allocate-memory", "free",
                  "set-shared-refcount", "allocate-memory"},
          "shared nil reuse call order");
}

void CheckAllocateNode() {
    Runtime runtime;
    const auto node = pal4::ScenarioPortraitTree::AllocateNode(
        0x1234u, 1u, runtime);
    Check(node == 0x6000u &&
              runtime.words.at(node + 4u) == 0x1234u &&
              runtime.words.at(node + 44u) == 1u &&
              !runtime.words.contains(node) &&
              !runtime.words.contains(node + 8u),
          "node allocation writes only parent and colour");
}

void CheckPredecessor() {
    Runtime runtime;
    const auto middle = runtime.InsertValue("middle");
    const auto first = runtime.InsertValue("alpha");
    const auto last = runtime.InsertValue("zulu");
    Check(pal4::ScenarioPortraitTree::Predecessor(
              Runtime::kHeader, runtime) == last,
          "header predecessor is rightmost node");
    Check(pal4::ScenarioPortraitTree::Predecessor(
              last, runtime) == middle &&
              pal4::ScenarioPortraitTree::Predecessor(
                  middle, runtime) == first &&
              pal4::ScenarioPortraitTree::Predecessor(
                  first, runtime) == Runtime::kHeader,
          "predecessor walks subtree and ancestors");
}

void CheckSuccessor() {
    Runtime runtime;
    const auto middle = runtime.InsertValue("middle");
    const auto first = runtime.InsertValue("alpha");
    const auto last = runtime.InsertValue("zulu");
    Check(pal4::ScenarioPortraitTree::Successor(
              first, runtime) == middle &&
              pal4::ScenarioPortraitTree::Successor(
                  middle, runtime) == last &&
              pal4::ScenarioPortraitTree::Successor(
                  last, runtime) == Runtime::kHeader,
          "successor walks subtree and ancestors");
    Check(pal4::ScenarioPortraitTree::Successor(
              Runtime::kHeader, runtime) == last,
          "header successor follows its right link");
}

void CheckCopyData() {
    Runtime runtime;
    pal4::ScenarioPortraitTree::CopyData(0u, 0xc000u,
                                         runtime);
    Check(runtime.calls.empty(), "null destination is ignored");
    const auto source = 0xc000u;
    const auto destination = 0xd000u;
    const auto source_data = 0xe100u;
    runtime.bytes[source] = 7u;
    runtime.bytes[source + 16u] = 9u;
    runtime.bytes[source_data - 1u] = 1u;
    runtime.words[source + 20u] = source_data;
    runtime.words[source + 24u] = 3u;
    runtime.words[source + 28u] = 8u;
    runtime.bytes[source_data] = 'a';
    runtime.bytes[source_data + 1u] = 'b';
    runtime.bytes[source_data + 2u] = 'c';
    pal4::ScenarioPortraitTree::CopyData(destination, source,
                                         runtime);
    Check(runtime.bytes.at(destination) == 7u &&
              runtime.bytes.at(destination + 16u) == 9u &&
              runtime.words.at(destination + 4u) == 0u &&
              runtime.words.at(destination + 8u) == 0u &&
              runtime.words.at(destination + 12u) == 0u &&
              runtime.words.at(destination + 20u) == source_data &&
              runtime.words.at(destination + 24u) == 3u &&
              runtime.words.at(destination + 28u) == 8u &&
              runtime.bytes.at(source_data - 1u) == 2u,
          "shareable second buffer copies fields and reference");

    Runtime allocated;
    allocated.bytes[source] = 3u;
    allocated.bytes[source + 16u] = 4u;
    allocated.bytes[source_data - 1u] = 0xfeu;
    allocated.words[source + 20u] = source_data;
    allocated.words[source + 24u] = 3u;
    allocated.words[source + 28u] = 8u;
    allocated.bytes[source_data] = 'a';
    allocated.bytes[source_data + 1u] = 'b';
    allocated.bytes[source_data + 2u] = 'c';
    allocated.allocate_text_result = true;
    pal4::ScenarioPortraitTree::CopyData(destination, source,
                                         allocated);
    Check(allocated.words.at(destination + 20u) == 0xf000u &&
              allocated.words.at(destination + 24u) == 3u &&
              allocated.bytes.at(0xf000u) == 'a' &&
              allocated.bytes.at(0xf001u) == 'b' &&
              allocated.bytes.at(0xf002u) == 'c' &&
              allocated.bytes.at(0xf003u) == 0u &&
              allocated.bytes.at(source_data - 1u) == 0xfeu,
          "unshareable second buffer allocates and copies bytes");
    allocated.calls.clear();
    allocated.allocate_text_result = false;
    pal4::ScenarioPortraitTree::CopyData(0xd100u, source,
                                         allocated);
    Check(allocated.words.at(0xd100u + 20u) == 0u &&
              allocated.words.at(0xd100u + 24u) == 0u &&
              allocated.calls.back() == "allocate-text",
          "failed allocation leaves second buffer empty");

    Runtime self;
    self.bytes[source] = 2u;
    self.bytes[source + 16u] = 5u;
    self.words[source + 20u] = source_data;
    self.words[source + 24u] = 3u;
    self.words[source + 28u] = 8u;
    pal4::ScenarioPortraitTree::CopyData(source, source, self);
    Check(self.erases ==
              std::vector<std::pair<std::uint32_t,
                                    std::uint32_t>>{
                  {0u, 0xffffffffu}, {0u, 0u}},
          "self copy executes both erase calls");
}

}  // namespace

int main() {
    try {
        CheckDuplicateMode({"10", "20", "30"},
                           {"rotate-right"});
        CheckDuplicateMode({"30", "20", "10"},
                           {"rotate-left"});
        CheckDuplicateMode({"10", "30", "20"},
                           {"rotate-left", "rotate-right"});
        CheckDuplicateMode({"30", "10", "20"},
                           {"rotate-right", "rotate-left"});
        CheckDuplicateMode({"10", "05", "15", "10", "00",
                            "20", "10", "05", "25"}, {});
        CheckUniqueMode();
        CheckFind();
        CheckRemove();
        CheckFreeNodes();
        CheckInitialize();
        CheckAllocateNode();
        CheckPredecessor();
        CheckSuccessor();
        CheckCopyData();
        std::cout << "scenario_portrait_tree_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
