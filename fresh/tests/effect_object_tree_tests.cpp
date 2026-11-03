#include "pal4/effect_object_tree.h"
#include "pal4/effect_name_count_tree.h"
#include "pal4/arena_sequence_resources.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <initializer_list>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IEffectObjectTreeRuntime,
                       pal4::IArenaSequenceRuntime {
    static constexpr std::uint32_t kTree = 0x1000u;
    static constexpr std::uint32_t kHead = 0x2000u;
    static constexpr std::uint32_t kNil = 0x93cdb0u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> texts;
    std::vector<std::uint32_t> frees;
    std::uint32_t next_node = 0x4000u;
    std::uint32_t allocations = 0u;
    std::uint32_t existing_updates = 0u;

    explicit Runtime(bool allow_duplicates) {
        words[kTree + 4u] = kHead;
        words[kTree + 8u] = allow_duplicates ? 1u : 0u;
        words[kTree + 12u] = 0u;
        words[kHead] = kHead;
        words[kHead + 4u] = kNil;
        words[kHead + 8u] = kHead;
        words[kNil + 32u] = 1u;
    }
    std::uint32_t SharedNil() const override { return kNil; }
    std::uint32_t ReadDword(std::uint32_t address) const override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) const override {
        if (const auto it = bytes.find(address); it != bytes.end())
            return it->second;
        const auto word = ReadDword(address & ~3u);
        return static_cast<std::uint8_t>(word >> ((address & 3u) * 8u));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return static_cast<const Runtime&>(*this).ReadByte(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return ReadDword(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        WriteDword(address, value);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        words[address] = value;
    }
    std::int32_t CompareRange(std::uint32_t input,
                              std::uint32_t node_text) override {
        return CompareText(input, node_text);
    }
    std::int32_t CompareText(std::uint32_t first,
                             std::uint32_t second) override {
        const auto& a = texts.at(first);
        const auto& b = texts.at(second);
        return a < b ? -1 : (b < a ? 1 : 0);
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        Check(bytes == 36u, "effect-object node allocation is 36 bytes");
        const auto node = next_node;
        next_node += 0x40u;
        ++allocations;
        return node;
    }
    std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    std::uint32_t Free(std::uint32_t pointer) override {
        frees.push_back(pointer);
        return 0u;
    }
    std::uint32_t Call(pal4::ArenaSequenceCall,
                       std::initializer_list<std::uint32_t>) override {
        return 0u;
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        texts[destination] = texts.at(source);
        const auto allocation = 0xa000u + allocations * 0x100u;
        words[destination + 4u] = allocation + 1u;
        words[destination + 8u] =
            static_cast<std::uint32_t>(texts.at(source).size());
        words[destination + 12u] = words[destination + 8u];
        bytes[allocation] = 0u;
        words[destination + 16u] = words.at(source + 16u);
    }
    void SetNodeData(pal4::MonsterManagerTreeInsertResult& result,
                     std::uint32_t candidate) override {
        ++existing_updates;
        result.node = candidate;
    }
    std::uint32_t Record(std::uint32_t address, std::string name,
                         std::uint32_t effect) {
        texts[address] = std::move(name);
        words[address + 16u] = effect;
        return address;
    }
};

int Validate(const Runtime& runtime, std::uint32_t node,
             std::uint32_t parent, std::vector<std::string>& names,
             std::uint32_t nil = Runtime::kNil) {
    if (node == nil) return 1;
    Check(runtime.ReadDword(node + 4u) == parent, "parent link");
    const auto left = runtime.ReadDword(node);
    const auto right = runtime.ReadDword(node + 8u);
    const auto color = runtime.ReadDword(node + 32u);
    Check(color <= 1u, "valid node color");
    if (color == 0u) {
        Check(runtime.ReadDword(left + 32u) == 1u &&
              runtime.ReadDword(right + 32u) == 1u,
              "red node has black children");
    }
    const auto left_height = Validate(runtime, left, node, names, nil);
    names.push_back(runtime.texts.at(node + 12u));
    const auto right_height = Validate(runtime, right, node, names, nil);
    Check(left_height == right_height, "equal black height");
    return left_height + static_cast<int>(color);
}

void TestUniqueAndRebalancing() {
    Runtime runtime(false);
    const std::vector<std::string> input = {
        "m", "a", "z", "b", "y", "c", "x", "d", "w"};
    std::uint32_t first_node = 0u;
    for (std::size_t i = 0u; i < input.size(); ++i) {
        const auto record = runtime.Record(0x8000u +
            static_cast<std::uint32_t>(i) * 0x20u, input[i],
            0x6000u + static_cast<std::uint32_t>(i));
        const auto result = pal4::EffectObjectTree::FindOrCreate(
            Runtime::kTree, record, runtime);
        Check(result.inserted && result.node != Runtime::kNil &&
              runtime.ReadDword(result.node + 28u) == 0x6000u + i,
              "inserted node preserves effect identity");
        if (i == 0u) first_node = result.node;
        Check(runtime.ReadDword(Runtime::kTree + 12u) == i + 1u,
              "tree count tracks insertions");
        const auto root = runtime.ReadDword(Runtime::kHead + 4u);
        Check(runtime.ReadDword(root + 32u) == 1u, "root stays black");
        std::vector<std::string> names;
        (void)Validate(runtime, root, Runtime::kHead, names);
        Check(std::is_sorted(names.begin(), names.end()),
              "effect names remain sorted after balancing");
        Check(runtime.texts.at(runtime.ReadDword(Runtime::kHead) + 12u) ==
                  names.front() &&
              runtime.texts.at(runtime.ReadDword(Runtime::kHead + 8u) + 12u)
                  == names.back(), "header tracks both extrema");
    }
    const auto duplicate = runtime.Record(0x9000u, "m", 0x9999u);
    const auto result = pal4::EffectObjectTree::FindOrCreate(
        Runtime::kTree, duplicate, runtime);
    Check(!result.inserted && result.node == first_node &&
          runtime.ReadDword(first_node + 28u) == 0x6000u &&
          runtime.existing_updates == 1u &&
          runtime.allocations == input.size(),
          "unique mode returns existing effect without replacing it");
}

void TestDuplicateMode() {
    Runtime runtime(true);
    const auto first = runtime.Record(0x8000u, "same", 0x6000u);
    const auto second = runtime.Record(0x8020u, "same", 0x6100u);
    const auto a = pal4::EffectObjectTree::FindOrCreate(
        Runtime::kTree, first, runtime);
    const auto b = pal4::EffectObjectTree::FindOrCreate(
        Runtime::kTree, second, runtime);
    Check(a.inserted && b.inserted && a.node != b.node &&
          runtime.ReadDword(Runtime::kTree + 12u) == 2u &&
          runtime.ReadDword(a.node + 28u) == 0x6000u &&
          runtime.ReadDword(b.node + 28u) == 0x6100u,
          "duplicate mode preserves separate effect records");
    std::vector<std::string> names;
    (void)Validate(runtime, runtime.ReadDword(Runtime::kHead + 4u),
                   Runtime::kHead, names);
    Check(names == (std::vector<std::string>{"same", "same"}),
          "duplicate entries remain ordered");
}

void TestFindExact() {
    Runtime runtime(false);
    const auto absent = runtime.Record(0x8000u, "m", 0x6000u);
    Check(pal4::EffectObjectTree::FindExact(
              Runtime::kTree, absent, runtime) == Runtime::kHead,
          "empty object tree returns header");
    const auto a_key = runtime.Record(0x8020u, "a", 0x6100u);
    const auto z_key = runtime.Record(0x8040u, "z", 0x6200u);
    const auto middle = pal4::EffectObjectTree::FindOrCreate(
        Runtime::kTree, absent, runtime).node;
    const auto first = pal4::EffectObjectTree::FindOrCreate(
        Runtime::kTree, a_key, runtime).node;
    const auto last = pal4::EffectObjectTree::FindOrCreate(
        Runtime::kTree, z_key, runtime).node;
    Check(pal4::EffectObjectTree::FindExact(
              Runtime::kTree, a_key, runtime) == first &&
          pal4::EffectObjectTree::FindExact(
              Runtime::kTree, absent, runtime) == middle &&
          pal4::EffectObjectTree::FindExact(
              Runtime::kTree, z_key, runtime) == last,
          "exact lookup finds both endpoints and middle key");
    for (const auto& name : {"0", "b", "ma", "zz"}) {
        const auto key = runtime.Record(0x8100u, name, 0u);
        Check(pal4::EffectObjectTree::FindExact(
                  Runtime::kTree, key, runtime) == Runtime::kHead,
              "missing key returns header");
    }
}

void TestRemoval() {
    Runtime runtime(false);
    const std::vector<std::string> insertion = {
        "m", "a", "z", "b", "y", "c", "x", "d", "w"};
    std::map<std::string, std::uint32_t> nodes;
    for (std::size_t i = 0u; i < insertion.size(); ++i) {
        const auto record = runtime.Record(0x8000u +
            static_cast<std::uint32_t>(i) * 0x20u, insertion[i],
            0x6000u + static_cast<std::uint32_t>(i));
        const auto result = pal4::EffectObjectTree::FindOrCreate(
            Runtime::kTree, record, runtime);
        nodes[insertion[i]] = result.node;
    }
    constexpr std::uint32_t result_slot = 0x1800u;
    const std::vector<std::string> deletion = {
        "m", "a", "z", "b", "y", "c", "x", "d", "w"};
    for (const auto& name : deletion) {
        const auto node = nodes.at(name);
        const auto text = runtime.ReadDword(node + 16u);
        const auto next = nodes.upper_bound(name);
        const auto expected = next == nodes.end() ? Runtime::kHead
                                                  : next->second;
        pal4::EffectObjectTree::RemoveNode(
            Runtime::kTree, result_slot, node, Runtime::kNil, runtime);
        Check(runtime.ReadDword(result_slot) == expected,
              "erase returns in-order successor");
        Check(runtime.frees.size() >= 2u &&
              runtime.frees[runtime.frees.size() - 2u] == text - 1u &&
              runtime.frees.back() == node,
              "erase releases text before the removed node");
        nodes.erase(name);
        Check(runtime.ReadDword(Runtime::kTree + 12u) == nodes.size(),
              "erase decrements object-tree count");
        const auto root = runtime.ReadDword(Runtime::kHead + 4u);
        if (nodes.empty()) {
            Check(root == Runtime::kNil &&
                  runtime.ReadDword(Runtime::kHead) == Runtime::kHead &&
                  runtime.ReadDword(Runtime::kHead + 8u) == Runtime::kHead,
                  "empty tree restores header links");
        } else {
            Check(runtime.ReadDword(root + 32u) == 1u,
                  "erase leaves black root");
            std::vector<std::string> names;
            (void)Validate(runtime, root, Runtime::kHead, names);
            Check(std::is_sorted(names.begin(), names.end()) &&
                  names.size() == nodes.size(),
                  "erase preserves ordered red-black tree");
        }
    }
}

void TestAdvance() {
    Runtime runtime(false);
    for (const auto& [address, name] :
         {std::pair{0x8000u, "m"}, std::pair{0x8020u, "a"},
          std::pair{0x8040u, "z"}, std::pair{0x8060u, "b"}}) {
        const auto record = runtime.Record(address, name, address);
        (void)pal4::EffectObjectTree::FindOrCreate(
            Runtime::kTree, record, runtime);
    }
    auto cursor = runtime.ReadDword(Runtime::kHead);
    std::vector<std::string> names;
    while (cursor != Runtime::kHead) {
        names.push_back(runtime.texts.at(cursor + 12u));
        (void)pal4::EffectObjectTree::Advance(cursor, runtime);
    }
    Check(names == (std::vector<std::string>{"a", "b", "m", "z"}),
          "successor visits every object in name order");

    cursor = runtime.ReadDword(Runtime::kHead + 4u);
    Check(runtime.texts.at(cursor + 12u) == "m",
          "fixture root has a right subtree");
    const auto subtree_result =
        pal4::EffectObjectTree::Advance(cursor, runtime);
    Check(runtime.texts.at(cursor + 12u) == "z" &&
          subtree_result == Runtime::kNil,
          "right-subtree successor returns nil separately from cursor");

    Runtime single(false);
    const auto record = single.Record(0x8000u, "only", 0x6000u);
    auto root = pal4::EffectObjectTree::FindOrCreate(
        Runtime::kTree, record, single).node;
    cursor = root;
    const auto result = pal4::EffectObjectTree::Advance(cursor, single);
    Check(cursor == Runtime::kHead && result == root,
          "last-node successor reaches header with native return value");
}

void TestPrevious() {
    Runtime runtime(false);
    for (const auto& [address, name] :
         {std::pair{0x8000u, "m"}, std::pair{0x8020u, "a"},
          std::pair{0x8040u, "z"}, std::pair{0x8060u, "b"}}) {
        (void)pal4::EffectObjectTree::FindOrCreate(
            Runtime::kTree, runtime.Record(address, name, address), runtime);
    }
    runtime.words[Runtime::kHead + 32u] = 0u;
    auto cursor = Runtime::kHead;
    const auto header_result = pal4::EffectObjectTree::Previous(cursor, runtime);
    Check(runtime.texts.at(cursor + 12u) == "z" &&
          header_result == cursor,
          "red header moves predecessor cursor to rightmost node");
    std::vector<std::string> names;
    while (cursor != Runtime::kHead) {
        names.push_back(runtime.texts.at(cursor + 12u));
        (void)pal4::EffectObjectTree::Previous(cursor, runtime);
    }
    Check(names == (std::vector<std::string>{"z", "m", "b", "a"}),
          "predecessor visits object nodes in reverse order");

    cursor = runtime.words.at(Runtime::kHead + 4u);
    const auto subtree_result = pal4::EffectObjectTree::Previous(cursor, runtime);
    Check(runtime.texts.at(cursor + 12u) == "b" &&
          subtree_result == Runtime::kNil,
          "left-subtree predecessor returns nil separately from cursor");
}

void TestDeleteSubtree() {
    Runtime runtime(false);
    const std::vector<std::string> names = {"m", "a", "z", "b", "y"};
    std::map<std::string, std::uint32_t> nodes;
    for (std::size_t i = 0u; i < names.size(); ++i) {
        const auto record = runtime.Record(0x8000u +
            static_cast<std::uint32_t>(i) * 0x20u, names[i],
            0x6000u + static_cast<std::uint32_t>(i));
        nodes[names[i]] = pal4::EffectObjectTree::FindOrCreate(
            Runtime::kTree, record, runtime).node;
    }
    const auto root = runtime.ReadDword(Runtime::kHead + 4u);
    std::map<std::uint32_t, std::uint32_t> text_by_node;
    for (const auto& [name, node] : nodes) {
        (void)name;
        text_by_node[node] = runtime.ReadDword(node + 16u);
    }
    Check(pal4::EffectObjectTree::DeleteSubtree(
              Runtime::kNil, Runtime::kNil, runtime) == Runtime::kNil &&
          runtime.frees.empty(), "nil subtree needs no cleanup");
    const auto result = pal4::EffectObjectTree::DeleteSubtree(
        root, Runtime::kNil, runtime);
    Check(result == Runtime::kNil && runtime.frees.size() == names.size() * 2u,
          "full branch releases every text and node");
    std::vector<std::uint32_t> freed_nodes;
    for (std::size_t i = 0u; i < runtime.frees.size(); i += 2u) {
        const auto node = runtime.frees[i + 1u];
        Check(runtime.frees[i] == text_by_node.at(node) - 1u,
              "each text is released before its node");
        freed_nodes.push_back(node);
    }
    for (const auto& [name, node] : nodes) {
        (void)name;
        Check(std::count(freed_nodes.begin(), freed_nodes.end(), node) == 1,
              "each branch node is released once");
    }
    const auto position = [&](const std::string& name) {
        return std::find(freed_nodes.begin(), freed_nodes.end(),
                         nodes.at(name));
    };
    Check(position("z") < position("m") && position("m") < position("a"),
          "right branch is released before root and left branch");
}

void TestNameCountDeleteSubtree() {
    Runtime runtime(false);
    constexpr std::uint32_t nil = 0x93cdacu;
    const std::vector<std::string> names = {"m", "a", "z"};
    std::map<std::string, std::uint32_t> nodes;
    for (std::size_t i = 0u; i < names.size(); ++i) {
        const auto record = runtime.Record(0x8000u +
            static_cast<std::uint32_t>(i) * 0x20u, names[i],
            0x6000u + static_cast<std::uint32_t>(i));
        nodes[names[i]] = pal4::EffectObjectTree::FindOrCreate(
            Runtime::kTree, record, runtime).node;
    }
    for (auto& [address, value] : runtime.words) {
        (void)address;
        if (value == Runtime::kNil) value = nil;
    }
    std::map<std::uint32_t, std::uint32_t> text_by_node;
    for (const auto& [name, node] : nodes) {
        (void)name;
        text_by_node[node] = runtime.words.at(node + 16u);
    }
    Check(pal4::EffectNameCountTree::DeleteSubtree(nil, nil, runtime) == nil &&
          runtime.frees.empty(), "empty name-count branch is unchanged");
    Check(pal4::EffectNameCountTree::DeleteSubtree(
              runtime.words.at(Runtime::kHead + 4u), nil, runtime) == nil &&
          runtime.frees.size() == 6u,
          "name-count branch releases all text and nodes");
    std::vector<std::uint32_t> freed_nodes;
    for (std::size_t i = 0u; i < runtime.frees.size(); i += 2u) {
        const auto node = runtime.frees[i + 1u];
        Check(runtime.frees[i] == text_by_node.at(node) - 1u,
              "name-count text is freed before its node");
        freed_nodes.push_back(node);
    }
    Check(freed_nodes == (std::vector<std::uint32_t>{
              nodes.at("z"), nodes.at("m"), nodes.at("a")}),
          "name-count branch releases right before root before left");
}

void TestNameCountRangeAndMinimum() {
    constexpr auto nil = 0x93cdacu;
    constexpr auto result = 0x1800u;
    Runtime full(false);
    for (const auto& name : {"m", "a", "z"}) {
        const auto address = 0x8000u + full.allocations * 0x20u;
        (void)pal4::EffectObjectTree::FindOrCreate(
            Runtime::kTree, full.Record(address, name, 0x6000u), full);
    }
    for (auto& [address, value] : full.words) {
        (void)address;
        if (value == Runtime::kNil) value = nil;
    }
    const auto first = full.words.at(Runtime::kHead);
    Check(pal4::EffectNameCountTree::EraseRange(
              Runtime::kTree, result, first, Runtime::kHead, nil,
              full) == result &&
          full.words.at(result) == Runtime::kHead &&
          full.words.at(Runtime::kTree + 12u) == 0u &&
          full.words.at(Runtime::kHead + 4u) == nil &&
          full.frees.size() == 6u,
          "full name-count range releases three nodes and resets header");

    Runtime partial(false);
    const auto node = pal4::EffectObjectTree::FindOrCreate(
        Runtime::kTree, partial.Record(0x8000u, "m", 0x6000u),
        partial).node;
    for (auto& [address, value] : partial.words) {
        (void)address;
        if (value == Runtime::kNil) value = nil;
    }
    partial.words[nil + 32u] = 1u;
    Check(pal4::EffectNameCountTree::EraseRange(
              Runtime::kTree, result, node, Runtime::kHead, nil,
              partial) == result &&
          partial.words.at(result) == Runtime::kHead &&
          partial.words.at(Runtime::kTree + 12u) == 0u &&
          partial.frees.size() == 2u,
          "partial name-count range advances before node erasure");

    Runtime minimum(false);
    minimum.words[Runtime::kHead + 4u] = nil;
    Check(pal4::EffectNameCountTree::MinimumFromLink(
              Runtime::kHead + 4u, nil, minimum) ==
              Runtime::kHead + 4u,
          "empty name-count link returns its address");
    minimum.words[Runtime::kHead + 4u] = 0x4000u;
    minimum.words[0x4000u] = 0x4040u;
    minimum.words[0x4040u] = nil;
    Check(pal4::EffectNameCountTree::MinimumFromLink(
              Runtime::kHead + 4u, nil, minimum) == 0x4040u,
          "name-count minimum follows left links");
}

void TestObjectAdvanceAndCapture() {
    Runtime runtime(false);
    constexpr auto result = 0x1800u;
    auto iterator = 0x4000u;
    runtime.words[iterator + 8u] = 0x4040u;
    runtime.words[0x4040u] = Runtime::kNil;
    Check(pal4::EffectObjectTree::AdvanceAndCapture(
              iterator, result, Runtime::kNil, runtime) == result &&
          iterator == 0x4040u && runtime.words.at(result) == 0x4000u,
          "object cursor descends right and captures old node");
    runtime.words[iterator + 8u] = Runtime::kNil;
    runtime.words[iterator + 4u] = 0x4000u;
    runtime.words[0x4000u + 8u] = Runtime::kNil;
    Check(pal4::EffectObjectTree::AdvanceAndCapture(
              iterator, result, Runtime::kNil, runtime) == result &&
          iterator == 0x4000u && runtime.words.at(result) == 0x4040u,
          "object cursor ascends and captures old node");
}

void TestInsertNode() {
    Runtime runtime(false);
    const auto middle = runtime.Record(0x8000u, "m", 0x6000u);
    const auto first = pal4::EffectObjectTree::InsertNode(
        Runtime::kTree, Runtime::kNil, Runtime::kHead, middle, runtime);
    Check(runtime.ReadDword(Runtime::kHead + 4u) == first &&
          runtime.ReadDword(first + 28u) == 0x6000u &&
          runtime.ReadDword(first + 32u) == 1u,
          "direct root insertion copies identity and paints root black");
    const auto left_record = runtime.Record(0x8020u, "a", 0x6100u);
    const auto left = pal4::EffectObjectTree::InsertNode(
        Runtime::kTree, Runtime::kNil, first, left_record, runtime);
    Check(runtime.ReadDword(first) == left &&
          runtime.ReadDword(Runtime::kHead) == left &&
          runtime.ReadDword(left + 4u) == first,
          "direct left insertion updates parent and minimum");
    const auto right_record = runtime.Record(0x8040u, "z", 0x6200u);
    const auto right = pal4::EffectObjectTree::InsertNode(
        Runtime::kTree, Runtime::kNil, first, right_record, runtime);
    Check(runtime.ReadDword(first + 8u) == right &&
          runtime.ReadDword(Runtime::kHead + 8u) == right &&
          runtime.ReadDword(Runtime::kTree + 12u) == 3u,
          "direct right insertion updates maximum and count");
    std::vector<std::string> names;
    (void)Validate(runtime, runtime.ReadDword(Runtime::kHead + 4u),
                   Runtime::kHead, names);
    Check(names == (std::vector<std::string>{"a", "m", "z"}),
          "direct insertions preserve red-black ordering");
}

void TestPromoteRightChild() {
    constexpr std::uint32_t node = 0x4000u;
    constexpr std::uint32_t pivot = 0x4040u;
    constexpr std::uint32_t moved = 0x4080u;
    Runtime root(false);
    root.words[Runtime::kHead + 4u] = node;
    root.words[node + 4u] = Runtime::kHead;
    root.words[node + 8u] = pivot;
    root.words[pivot] = moved;
    root.words[pivot + 4u] = node;
    root.words[moved + 4u] = pivot;
    Check(pal4::EffectObjectTree::PromoteRightChild(
              Runtime::kTree, node, root) == pivot &&
          root.words.at(Runtime::kHead + 4u) == pivot &&
          root.words.at(pivot) == node &&
          root.words.at(node + 8u) == moved &&
          root.words.at(moved + 4u) == node &&
          root.words.at(pivot + 4u) == Runtime::kHead &&
          root.words.at(node + 4u) == pivot,
          "right-child promotion relinks root and moved subtree");

    Runtime nested(false);
    constexpr std::uint32_t parent = 0x4200u;
    nested.words[Runtime::kHead + 4u] = parent;
    nested.words[parent] = node;
    nested.words[node + 4u] = parent;
    nested.words[node + 8u] = pivot;
    nested.words[pivot] = Runtime::kNil;
    nested.words[pivot + 4u] = node;
    Check(pal4::EffectObjectTree::PromoteRightChild(
              Runtime::kTree, node, nested) == pivot &&
          nested.words.at(parent) == pivot &&
          nested.words.at(node + 8u) == Runtime::kNil &&
          nested.words.at(pivot + 4u) == parent,
          "right-child promotion updates non-root parent link");
}

void TestPromoteLeftChild() {
    constexpr std::uint32_t node = 0x4000u;
    constexpr std::uint32_t pivot = 0x4040u;
    constexpr std::uint32_t moved = 0x4080u;
    Runtime root(false);
    root.words[Runtime::kHead + 4u] = node;
    root.words[node + 4u] = Runtime::kHead;
    root.words[node] = pivot;
    root.words[pivot + 8u] = moved;
    root.words[pivot + 4u] = node;
    root.words[moved + 4u] = pivot;
    Check(pal4::EffectObjectTree::PromoteLeftChild(
              Runtime::kTree, node, root) == pivot &&
          root.words.at(Runtime::kHead + 4u) == pivot &&
          root.words.at(pivot + 8u) == node &&
          root.words.at(node) == moved &&
          root.words.at(moved + 4u) == node &&
          root.words.at(pivot + 4u) == Runtime::kHead &&
          root.words.at(node + 4u) == pivot,
          "left-child promotion relinks root and moved subtree");

    Runtime nested(false);
    constexpr std::uint32_t parent = 0x4200u;
    nested.words[Runtime::kHead + 4u] = parent;
    nested.words[parent + 8u] = node;
    nested.words[node + 4u] = parent;
    nested.words[node] = pivot;
    nested.words[pivot + 8u] = Runtime::kNil;
    nested.words[pivot + 4u] = node;
    Check(pal4::EffectObjectTree::PromoteLeftChild(
              Runtime::kTree, node, nested) == pivot &&
          nested.words.at(parent + 8u) == pivot &&
          nested.words.at(node) == Runtime::kNil &&
          nested.words.at(pivot + 4u) == parent,
          "left-child promotion updates non-root parent link");
}

void TestNameCountRemoval() {
    Runtime runtime(false);
    constexpr std::uint32_t name_nil = 0x93cdacu;
    const std::vector<std::string> insertion = {"m", "a", "z", "b", "y"};
    std::map<std::string, std::uint32_t> nodes;
    for (std::size_t i = 0u; i < insertion.size(); ++i) {
        const auto record = runtime.Record(0x8000u +
            static_cast<std::uint32_t>(i) * 0x20u, insertion[i],
            0x6000u + static_cast<std::uint32_t>(i));
        nodes[insertion[i]] = pal4::EffectObjectTree::FindOrCreate(
            Runtime::kTree, record, runtime).node;
    }
    for (auto& [address, value] : runtime.words) {
        (void)address;
        if (value == Runtime::kNil) value = name_nil;
    }
    runtime.words[name_nil + 32u] = 1u;
    constexpr std::uint32_t result_slot = 0x1800u;
    for (const auto& name : insertion) {
        const auto node = nodes.at(name);
        const auto text = runtime.ReadDword(node + 16u);
        const auto next = nodes.upper_bound(name);
        const auto expected = next == nodes.end() ? Runtime::kHead
                                                  : next->second;
        pal4::EffectNameCountTree::RemoveNode(
            Runtime::kTree, result_slot, node, name_nil, runtime);
        Check(runtime.frees.size() >= 2u &&
              runtime.ReadDword(result_slot) == expected &&
              runtime.frees[runtime.frees.size() - 2u] == text - 1u &&
              runtime.frees.back() == node,
              "count-tree erase returns successor and releases text first");
        nodes.erase(name);
        Check(runtime.ReadDword(Runtime::kTree + 12u) == nodes.size(),
              "count-tree erase decrements node count");
        const auto root = runtime.ReadDword(Runtime::kHead + 4u);
        if (nodes.empty()) {
            Check(root == name_nil &&
                  runtime.ReadDword(Runtime::kHead) == Runtime::kHead &&
                  runtime.ReadDword(Runtime::kHead + 8u) == Runtime::kHead,
                  "empty count tree restores header links");
        } else {
            Check(runtime.ReadDword(root + 32u) == 1u,
                  "count tree retains a black root");
            std::vector<std::string> ordered;
            (void)Validate(runtime, root, Runtime::kHead, ordered, name_nil);
            Check(std::is_sorted(ordered.begin(), ordered.end()) &&
                  ordered.size() == nodes.size(),
                  "count-tree erase preserves red-black ordering");
        }
    }
}

}  // namespace

int main() {
    TestUniqueAndRebalancing();
    TestDuplicateMode();
    TestFindExact();
    TestRemoval();
    TestAdvance();
    TestPrevious();
    TestDeleteSubtree();
    TestNameCountDeleteSubtree();
    TestNameCountRangeAndMinimum();
    TestObjectAdvanceAndCapture();
    TestInsertNode();
    TestPromoteRightChild();
    TestPromoteLeftChild();
    TestNameCountRemoval();
}
