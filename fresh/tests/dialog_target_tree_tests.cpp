#include "pal4/dialog_target_tree.h"
#include "pal4/arena_sequence_resources.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IDialogTargetTreeRuntime,
                       pal4::IDialogTargetTreeCursorRuntime,
                       pal4::IArenaSequenceRuntime {
    static constexpr std::uint32_t kTree = 0x1000u;
    static constexpr std::uint32_t kHead = 0x2000u;
    static constexpr std::uint32_t kNil = 0x93cda0u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> texts;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> frees;
    std::uint32_t next_node = 0x4000u;
    std::uint32_t allocations = 0u;
    std::uint32_t duplicate_updates = 0u;

    explicit Runtime(bool allow_duplicates) {
        words[kTree + 4u] = kHead;
        words[kTree + 8u] = allow_duplicates ? 1u : 0u;
        words[kTree + 12u] = 0u;
        words[kHead] = kHead;
        words[kHead + 4u] = kNil;
        words[kHead + 8u] = kHead;
        words[kNil + 40u] = 1u;
    }
    std::uint32_t SharedNil() const override { return kNil; }
    std::uint32_t ReadDword(std::uint32_t address) const override {
        const auto found = words.find(address);
        if (found == words.end()) {
            std::cerr << "FAIL: tree word is initialized at 0x"
                      << std::hex << address << '\n';
            std::exit(1);
        }
        return found->second;
    }
    std::uint8_t ReadByte(std::uint32_t address) const override {
        if (const auto found = bytes.find(address); found != bytes.end())
            return found->second;
        return static_cast<std::uint8_t>(ReadDword(address & ~3u) >>
                                         ((address & 3u) * 8u));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return static_cast<const Runtime&>(*this).ReadByte(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return ReadDword(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        WriteDword(address, value);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
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
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        Check(size == 44u, "44-byte target tree node");
        ++allocations;
        const auto node = next_node;
        next_node += 0x40u;
        return node;
    }
    std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    std::uint32_t Free(std::uint32_t address) override {
        frees.push_back(address);
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
        bytes[allocation] = 0u;
    }
    void SetNodeData(pal4::MonsterManagerTreeInsertResult& result,
                     std::uint32_t candidate) override {
        ++duplicate_updates;
        result.node = candidate;
    }
    std::uint32_t Input(std::uint32_t address, std::string text) {
        texts[address] = std::move(text);
        return address;
    }
};

int Validate(const Runtime& runtime, std::uint32_t node,
             std::uint32_t parent, std::vector<std::string>& ordered) {
    if (node == Runtime::kNil) return 1;
    Check(runtime.ReadDword(node + 4u) == parent, "parent link");
    const auto left = runtime.ReadDword(node);
    const auto right = runtime.ReadDword(node + 8u);
    const auto color = runtime.ReadDword(node + 40u);
    Check(color <= 1u, "color resides at +40");
    if (color == 0u)
        Check(runtime.ReadDword(left + 40u) == 1u &&
              runtime.ReadDword(right + 40u) == 1u,
              "red node children are black");
    const int left_height = Validate(runtime, left, node, ordered);
    ordered.push_back(runtime.texts.at(node + 12u));
    const int right_height = Validate(runtime, right, node, ordered);
    Check(left_height == right_height, "equal black height");
    return left_height + static_cast<int>(color);
}

void TestUniqueTargets() {
    Runtime runtime(false);
    const std::vector<std::string> names{
        "middle", "alpha", "zeta", "delta", "beta", "omega",
        "gamma", "sigma", "eta", "theta", "lambda"};
    std::vector<std::uint32_t> nodes;
    for (std::uint32_t i = 0u; i < names.size(); ++i) {
        const auto result = pal4::DialogTargetTree::Insert(Runtime::kTree,
            runtime.Input(0x8000u + i * 0x20u, names[i]), runtime);
        Check(result.inserted && result.node != Runtime::kNil,
              "unique insert returns new node");
        nodes.push_back(result.node);
        runtime.words[result.node + 28u] = i + 100u;
        runtime.words[result.node + 32u] = i + 200u;
        runtime.words[result.node + 36u] = i + 300u;
        Check(runtime.ReadDword(Runtime::kTree + 12u) == i + 1u,
              "tree count increments");
        const auto root = runtime.ReadDword(Runtime::kHead + 4u);
        Check(runtime.ReadDword(root + 40u) == 1u, "root is black");
        std::vector<std::string> ordered;
        (void)Validate(runtime, root, Runtime::kHead, ordered);
        Check(std::is_sorted(ordered.begin(), ordered.end()) &&
              ordered.size() == i + 1u,
              "tree remains ordered and balanced");
    }
    for (std::uint32_t i = 0u; i < nodes.size(); ++i) {
        Check(runtime.ReadDword(nodes[i] + 28u) == i + 100u &&
              runtime.ReadDword(nodes[i] + 32u) == i + 200u &&
              runtime.ReadDword(nodes[i] + 36u) == i + 300u,
              "target payload survives balancing");
    }
    const auto duplicate = pal4::DialogTargetTree::Insert(Runtime::kTree,
        runtime.Input(0x9000u, "gamma"), runtime);
    Check(!duplicate.inserted &&
          runtime.texts.at(duplicate.node + 12u) == "gamma" &&
          runtime.duplicate_updates == 1u &&
          runtime.allocations == names.size(),
          "duplicate uses existing node without allocating");
    Check(runtime.texts.at(runtime.ReadDword(Runtime::kHead) + 12u) == "alpha" &&
          runtime.texts.at(runtime.ReadDword(Runtime::kHead + 8u) + 12u) == "zeta",
          "header retains minimum and maximum");
}

void TestDuplicateMode() {
    Runtime runtime(true);
    const auto first = pal4::DialogTargetTree::Insert(Runtime::kTree,
        runtime.Input(0x8000u, "same"), runtime);
    const auto second = pal4::DialogTargetTree::Insert(Runtime::kTree,
        runtime.Input(0x8020u, "same"), runtime);
    Check(first.inserted && second.inserted && first.node != second.node &&
          runtime.allocations == 2u &&
          runtime.ReadDword(Runtime::kTree + 12u) == 2u,
          "duplicate-allowed mode allocates both 44-byte nodes");
}

void TestEraseRange() {
    Runtime runtime(false);
    const std::vector<std::string> names{
        "m", "a", "z", "c", "b", "y", "x", "d", "n"};
    for (std::uint32_t i = 0u; i < names.size(); ++i)
        (void)pal4::DialogTargetTree::Insert(Runtime::kTree,
            runtime.Input(0x8000u + i * 0x20u, names[i]), runtime);
    const auto find_node = [&](const std::string& key) {
        for (std::uint32_t node = 0x4000u; node < runtime.next_node;
             node += 0x40u)
            if (runtime.texts.at(node + 12u) == key) return node;
        return 0u;
    };
    const auto first = find_node("c");
    const auto last = find_node("y");
    Check(pal4::DialogTargetTree::EraseRange(Runtime::kTree, 0x7000u,
          first, last, Runtime::kNil, runtime) == last &&
          runtime.ReadDword(0x7000u) == last &&
          runtime.frees.size() == 10u,
          "partial range erases five nodes through reconstructed helper");
    std::vector<std::string> ordered;
    (void)Validate(runtime, runtime.ReadDword(Runtime::kHead + 4u),
                   Runtime::kHead, ordered);
    Check(ordered == std::vector<std::string>{"a", "b", "y", "z"} &&
          runtime.ReadDword(Runtime::kTree + 12u) == 4u,
          "partial range preserves outside nodes and tree invariants");

    Check(pal4::DialogTargetTree::EraseRange(Runtime::kTree, 0x7000u,
          runtime.ReadDword(Runtime::kHead), Runtime::kHead,
          Runtime::kNil, runtime) == Runtime::kHead &&
          runtime.ReadDword(0x7000u) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kHead) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kHead + 4u) == Runtime::kNil &&
          runtime.ReadDword(Runtime::kHead + 8u) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kTree + 12u) == 0u &&
          runtime.frees.size() == names.size() * 2u,
          "full range releases rest and restores header");

    Check(pal4::DialogTargetTree::EraseRange(Runtime::kTree, 0x7000u,
          Runtime::kHead, Runtime::kHead, Runtime::kNil, runtime) ==
          Runtime::kHead && runtime.frees.size() == names.size() * 2u,
          "empty range leaves resources untouched");
}

void TestAdvance() {
    Runtime runtime(false);
    const auto root = pal4::DialogTargetTree::Insert(Runtime::kTree,
        runtime.Input(0x8000u, "m"), runtime).node;
    const auto left = pal4::DialogTargetTree::Insert(Runtime::kTree,
        runtime.Input(0x8020u, "a"), runtime).node;
    const auto right = pal4::DialogTargetTree::Insert(Runtime::kTree,
        runtime.Input(0x8040u, "z"), runtime).node;
    auto node = left;
    pal4::DialogTargetTree::Advance(node, runtime);
    Check(node == root,
          "left leaf advances through parent");
    pal4::DialogTargetTree::Advance(node, runtime);
    Check(node == right,
          "root advances into right subtree");
    pal4::DialogTargetTree::Advance(node, runtime);
    Check(node == Runtime::kHead,
          "rightmost node advances to header");

    runtime = Runtime(false);
    node = pal4::DialogTargetTree::Insert(Runtime::kTree,
        runtime.Input(0x8000u, "only"), runtime).node;
    pal4::DialogTargetTree::Advance(node, runtime);
    Check(node == Runtime::kHead,
          "single-node tree advances to header");
}

void TestEraseNode() {
    Runtime runtime(false);
    const std::vector<std::string> names{
        "m", "a", "z", "c", "b", "y", "x", "d", "n"};
    std::map<std::uint32_t, std::uint32_t> payloads;
    for (std::uint32_t i = 0u; i < names.size(); ++i) {
        const auto node = pal4::DialogTargetTree::Insert(Runtime::kTree,
            runtime.Input(0x8000u + i * 0x20u, names[i]), runtime).node;
        runtime.words[node + 28u] = i + 100u;
        runtime.words[node + 32u] = i + 200u;
        runtime.words[node + 36u] = i + 300u;
        payloads[node] = i;
    }
    std::uint32_t branch = 0u;
    for (std::uint32_t node = 0x4000u; node < runtime.next_node;
         node += 0x40u) {
        if (runtime.ReadDword(node) != Runtime::kNil &&
            runtime.ReadDword(node + 8u) != Runtime::kNil) {
            branch = node;
            break;
        }
    }
    Check(branch != 0u, "target tree contains a two-child node");
    const auto erased_key = runtime.texts.at(branch + 12u);
    pal4::DialogTargetTree::EraseNode(Runtime::kTree, 0x7000u, branch,
                                      Runtime::kNil, runtime);
    std::vector<std::string> ordered;
    const auto root = runtime.ReadDword(Runtime::kHead + 4u);
    (void)Validate(runtime, root, Runtime::kHead, ordered);
    std::vector<std::string> expected = names;
    expected.erase(std::find(expected.begin(), expected.end(), erased_key));
    std::sort(expected.begin(), expected.end());
    Check(ordered == expected &&
          runtime.ReadDword(Runtime::kTree + 12u) == expected.size() &&
          runtime.frees.size() == 2u && runtime.frees.back() == branch,
          "two-child erase repairs tree and frees text then node");
    for (const auto& [node, index] : payloads) {
        if (node == branch) continue;
        Check(runtime.ReadDword(node + 28u) == index + 100u &&
              runtime.ReadDword(node + 32u) == index + 200u &&
              runtime.ReadDword(node + 36u) == index + 300u,
              "erase color repair preserves target payload");
    }

    while (!expected.empty()) {
        const auto first = runtime.ReadDword(Runtime::kHead);
        Check(runtime.texts.at(first + 12u) == expected.front(),
              "header tracks smallest remaining target");
        pal4::DialogTargetTree::EraseNode(Runtime::kTree, 0x7000u, first,
                                          Runtime::kNil, runtime);
        expected.erase(expected.begin());
        ordered.clear();
        (void)Validate(runtime, runtime.ReadDword(Runtime::kHead + 4u),
                       Runtime::kHead, ordered);
        Check(ordered == expected &&
              runtime.ReadDword(Runtime::kTree + 12u) == expected.size(),
              "successive erasure preserves order, black height and count");
    }
    Check(runtime.ReadDword(Runtime::kHead) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kHead + 4u) == Runtime::kNil &&
          runtime.ReadDword(Runtime::kHead + 8u) == Runtime::kHead &&
          runtime.frees.size() == names.size() * 2u,
          "all target nodes and text buffers are freed");
}

void TestDeleteSubtree() {
    Runtime runtime(false);
    const std::vector<std::string> names{
        "m", "a", "z", "c", "b", "y", "x"};
    for (std::uint32_t i = 0u; i < names.size(); ++i)
        (void)pal4::DialogTargetTree::Insert(Runtime::kTree,
            runtime.Input(0x8000u + i * 0x20u, names[i]), runtime);
    const auto root = runtime.ReadDword(Runtime::kHead + 4u);
    Check(pal4::DialogTargetTree::DeleteSubtree(root, Runtime::kNil,
          runtime) == Runtime::kNil &&
          runtime.frees.size() == names.size() * 2u,
          "target subtree releases each text and node");
    for (std::size_t i = 1u; i < runtime.frees.size(); i += 2u)
        Check(runtime.frees[i] >= 0x4000u &&
              runtime.frees[i] < runtime.next_node,
              "target subtree releases text before its node");
    runtime.frees.clear();
    Check(pal4::DialogTargetTree::DeleteSubtree(Runtime::kNil,
          Runtime::kNil, runtime) == Runtime::kNil &&
          runtime.frees.empty(), "empty target subtree returns nil");
}

void TestInsertNode() {
    Runtime runtime(false);
    const auto root = pal4::DialogTargetTree::InsertNode(
        Runtime::kTree, Runtime::kNil, Runtime::kHead,
        runtime.Input(0x8000u, "middle"), runtime);
    Check(root == 0x4000u &&
          runtime.ReadDword(Runtime::kHead + 4u) == root &&
          runtime.ReadDword(root + 4u) == Runtime::kHead &&
          runtime.ReadDword(root + 40u) == 1u &&
          runtime.ReadDword(Runtime::kTree + 12u) == 1u &&
          runtime.texts.at(root + 12u) == "middle",
          "44-byte helper inserts a black root");
    const auto left = pal4::DialogTargetTree::InsertNode(
        Runtime::kTree, Runtime::kNil, root,
        runtime.Input(0x8020u, "alpha"), runtime);
    Check(runtime.ReadDword(root) == left &&
          runtime.ReadDword(Runtime::kHead) == left &&
          runtime.ReadDword(Runtime::kTree + 12u) == 2u &&
          runtime.texts.at(left + 12u) == "alpha",
          "44-byte helper inserts at supplied left child");
}

void TestMinimumFromLink() {
    Runtime runtime(false);
    constexpr std::uint32_t link = 0x7000u;
    runtime.words[link] = Runtime::kNil;
    Check(pal4::DialogTargetTree::MinimumFromLink(link, Runtime::kNil,
          runtime) == link, "empty target link returns link address");
    runtime.words[link] = 0x4000u;
    runtime.words[0x4000u] = 0x4040u;
    runtime.words[0x4040u] = 0x4080u;
    runtime.words[0x4080u] = Runtime::kNil;
    Check(pal4::DialogTargetTree::MinimumFromLink(link, Runtime::kNil,
          runtime) == 0x4080u,
          "target link returns deepest left descendant");
}

void TestLowerBound() {
    Runtime runtime(false);
    Check(pal4::DialogTargetTree::LowerBound(Runtime::kTree,
          runtime.Input(0x8000u, "a"), runtime) == Runtime::kHead,
          "empty target tree returns head");
    const auto middle = pal4::DialogTargetTree::Insert(Runtime::kTree,
        runtime.Input(0x8020u, "m"), runtime).node;
    const auto first = pal4::DialogTargetTree::Insert(Runtime::kTree,
        runtime.Input(0x8040u, "a"), runtime).node;
    const auto last = pal4::DialogTargetTree::Insert(Runtime::kTree,
        runtime.Input(0x8060u, "z"), runtime).node;
    Check(pal4::DialogTargetTree::LowerBound(Runtime::kTree,
          runtime.Input(0x8080u, "a"), runtime) == first,
          "exact minimum target match");
    Check(pal4::DialogTargetTree::LowerBound(Runtime::kTree,
          runtime.Input(0x80a0u, "b"), runtime) == middle,
          "between-key target lookup chooses next larger node");
    Check(pal4::DialogTargetTree::LowerBound(Runtime::kTree,
          runtime.Input(0x80c0u, "z"), runtime) == last,
          "exact maximum target match");
    Check(pal4::DialogTargetTree::LowerBound(Runtime::kTree,
          runtime.Input(0x80e0u, "zz"), runtime) == Runtime::kHead,
          "greater than maximum returns target head");
}

void TestPromoteRightChild() {
    Runtime runtime(false);
    constexpr std::uint32_t node = 0x4000u;
    constexpr std::uint32_t promoted = 0x4040u;
    constexpr std::uint32_t moved = 0x4080u;
    runtime.words[Runtime::kHead + 4u] = node;
    runtime.words[node + 4u] = Runtime::kHead;
    runtime.words[node + 8u] = promoted;
    runtime.words[promoted] = moved;
    runtime.words[promoted + 4u] = node;
    runtime.words[moved + 4u] = promoted;
    Check(pal4::DialogTargetTree::PromoteRightChild(
          Runtime::kTree, node, runtime) == promoted &&
          runtime.ReadDword(Runtime::kHead + 4u) == promoted &&
          runtime.ReadDword(promoted + 4u) == Runtime::kHead &&
          runtime.ReadDword(promoted) == node &&
          runtime.ReadDword(node + 4u) == promoted &&
          runtime.ReadDword(node + 8u) == moved &&
          runtime.ReadDword(moved + 4u) == node,
          "target root rotation reparents moved subtree");

    runtime = Runtime(false);
    constexpr std::uint32_t parent = 0x40c0u;
    runtime.words[Runtime::kHead + 4u] = parent;
    runtime.words[parent] = node;
    runtime.words[node + 4u] = parent;
    runtime.words[node + 8u] = promoted;
    runtime.words[promoted] = Runtime::kNil;
    runtime.words[promoted + 4u] = node;
    Check(pal4::DialogTargetTree::PromoteRightChild(
          Runtime::kTree, node, runtime) == promoted &&
          runtime.ReadDword(parent) == promoted &&
          runtime.ReadDword(promoted + 4u) == parent &&
          runtime.ReadDword(promoted) == node &&
          runtime.ReadDword(node + 8u) == Runtime::kNil,
          "target non-root rotation relinks parent branch");
}

void TestPromoteLeftChild() {
    Runtime runtime(false);
    constexpr std::uint32_t node = 0x4000u;
    constexpr std::uint32_t promoted = 0x4040u;
    constexpr std::uint32_t moved = 0x4080u;
    runtime.words[Runtime::kHead + 4u] = node;
    runtime.words[node] = promoted;
    runtime.words[node + 4u] = Runtime::kHead;
    runtime.words[promoted + 4u] = node;
    runtime.words[promoted + 8u] = moved;
    runtime.words[moved + 4u] = promoted;
    Check(pal4::DialogTargetTree::PromoteLeftChild(
          Runtime::kTree, node, runtime) == promoted &&
          runtime.ReadDword(Runtime::kHead + 4u) == promoted &&
          runtime.ReadDword(promoted + 4u) == Runtime::kHead &&
          runtime.ReadDword(promoted + 8u) == node &&
          runtime.ReadDword(node + 4u) == promoted &&
          runtime.ReadDword(node) == moved &&
          runtime.ReadDword(moved + 4u) == node,
          "target root left-child rotation reparents moved subtree");

    runtime = Runtime(false);
    constexpr std::uint32_t parent = 0x40c0u;
    runtime.words[Runtime::kHead + 4u] = parent;
    runtime.words[parent + 8u] = node;
    runtime.words[node] = promoted;
    runtime.words[node + 4u] = parent;
    runtime.words[promoted + 4u] = node;
    runtime.words[promoted + 8u] = Runtime::kNil;
    Check(pal4::DialogTargetTree::PromoteLeftChild(
          Runtime::kTree, node, runtime) == promoted &&
          runtime.ReadDword(parent + 8u) == promoted &&
          runtime.ReadDword(promoted + 4u) == parent &&
          runtime.ReadDword(promoted + 8u) == node &&
          runtime.ReadDword(node) == Runtime::kNil,
          "target non-root left-child rotation relinks parent branch");
}

void TestAllocateNode() {
    Runtime runtime(false);
    const auto node = pal4::DialogTargetTree::AllocateNode(
        Runtime::kHead, 1u, runtime);
    Check(node == 0x4000u && runtime.allocations == 1u &&
          runtime.ReadDword(node + 4u) == Runtime::kHead &&
          runtime.ReadDword(node + 40u) == 1u,
          "target node allocation sets parent and color at +40");
}

void TestPrevious() {
    Runtime runtime(false);
    runtime.words[Runtime::kHead + 40u] = 0u;
    const auto middle = pal4::DialogTargetTree::Insert(Runtime::kTree,
        runtime.Input(0x8000u, "m"), runtime).node;
    const auto first = pal4::DialogTargetTree::Insert(Runtime::kTree,
        runtime.Input(0x8020u, "a"), runtime).node;
    const auto last = pal4::DialogTargetTree::Insert(Runtime::kTree,
        runtime.Input(0x8040u, "z"), runtime).node;
    Check(pal4::DialogTargetTree::Previous(last, runtime) == middle &&
          pal4::DialogTargetTree::Previous(middle, runtime) == first &&
          pal4::DialogTargetTree::Previous(first, runtime) == Runtime::kHead &&
          pal4::DialogTargetTree::Previous(Runtime::kHead, runtime) == last,
          "target predecessor traverses nodes and header");
}

}  // namespace

int main() {
    TestUniqueTargets();
    TestDuplicateMode();
    TestEraseRange();
    TestAdvance();
    TestEraseNode();
    TestDeleteSubtree();
    TestInsertNode();
    TestMinimumFromLink();
    TestLowerBound();
    TestPromoteRightChild();
    TestPromoteLeftChild();
    TestAllocateNode();
    TestPrevious();
}
