#include "pal4/dialog_text_tree.h"
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

struct Runtime final : pal4::IDialogTextTreeRuntime,
                       pal4::IArenaSequenceRuntime {
    static constexpr std::uint32_t kTree = 0x1000u;
    static constexpr std::uint32_t kHead = 0x2000u;
    static constexpr std::uint32_t kNil = 0x93cda8u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> texts;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> frees;
    std::uint32_t next_node = 0x4000u;
    std::uint32_t allocations = 0u;
    std::uint32_t duplicate_updates = 0u;
    std::vector<std::uint32_t> assigned_sources;

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
        const auto found = words.find(address);
        Check(found != words.end(), "tree word was initialized");
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
        Check(size == 36u, "36-byte tree node allocation");
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
        assigned_sources.push_back(source);
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
    std::uint32_t Input(std::uint32_t address, std::string value) {
        texts[address] = std::move(value);
        return address;
    }
};

void Validate(const Runtime& runtime, std::uint32_t node,
              std::uint32_t parent, std::vector<std::string>& ordered,
              int& black_height) {
    if (node == Runtime::kNil) {
        black_height = 1;
        return;
    }
    Check(runtime.ReadDword(node + 4u) == parent, "parent link");
    const auto left = runtime.ReadDword(node);
    const auto right = runtime.ReadDword(node + 8u);
    const auto color = runtime.ReadDword(node + 32u);
    Check(color <= 1u, "node color");
    if (color == 0u) {
        Check(runtime.ReadDword(left + 32u) == 1u &&
              runtime.ReadDword(right + 32u) == 1u,
              "red node has black children");
    }
    int left_height = 0;
    int right_height = 0;
    Validate(runtime, left, node, ordered, left_height);
    ordered.push_back(runtime.texts.at(node + 12u));
    Validate(runtime, right, node, ordered, right_height);
    Check(left_height == right_height, "balanced black height");
    black_height = left_height + static_cast<int>(color);
}

void TestUnique() {
    Runtime runtime(false);
    const std::vector<std::string> input{"m", "a", "z", "c", "b", "y", "x"};
    for (std::uint32_t i = 0; i < input.size(); ++i) {
        const auto address = runtime.Input(0x8000u + i * 0x20u, input[i]);
        const auto result = pal4::DialogTextTree::Insert(
            Runtime::kTree, address, runtime);
        Check(result.inserted && result.node != Runtime::kNil,
              "unique insertion result");
        Check(runtime.ReadDword(Runtime::kTree + 12u) == i + 1u,
              "unique count");
        const auto root = runtime.ReadDword(Runtime::kHead + 4u);
        Check(runtime.ReadDword(root + 32u) == 1u, "black root");
        std::vector<std::string> ordered;
        int height = 0;
        Validate(runtime, root, Runtime::kHead, ordered, height);
        Check(std::is_sorted(ordered.begin(), ordered.end()) &&
              ordered.size() == i + 1u, "ordered tree after repair");
    }
    const auto existing = pal4::DialogTextTree::Insert(
        Runtime::kTree, runtime.Input(0x9000u, "c"), runtime);
    Check(!existing.inserted && runtime.texts.at(existing.node + 12u) == "c" &&
          runtime.allocations == input.size() &&
          runtime.duplicate_updates == 1u,
          "duplicate reuses candidate without allocating");
    Check(runtime.assigned_sources.size() == input.size(),
          "each inserted node copies text once");
    Check(runtime.texts.at(runtime.ReadDword(Runtime::kHead) + 12u) == "a" &&
          runtime.texts.at(runtime.ReadDword(Runtime::kHead + 8u) + 12u) == "z",
          "header tracks extrema");
}

void TestDuplicates() {
    Runtime runtime(true);
    const auto first = pal4::DialogTextTree::Insert(Runtime::kTree,
        runtime.Input(0x8000u, "same"), runtime);
    const auto second = pal4::DialogTextTree::Insert(Runtime::kTree,
        runtime.Input(0x8020u, "same"), runtime);
    Check(first.inserted && second.inserted && first.node != second.node &&
          runtime.ReadDword(Runtime::kTree + 12u) == 2u &&
          runtime.allocations == 2u && runtime.duplicate_updates == 0u,
          "direct mode permits duplicate keys");
}

void TestErase() {
    Runtime runtime(false);
    const std::vector<std::string> input{
        "m", "a", "z", "c", "b", "y", "x", "d", "n"};
    for (std::uint32_t i = 0; i < input.size(); ++i) {
        (void)pal4::DialogTextTree::Insert(Runtime::kTree,
            runtime.Input(0x8000u + i * 0x20u, input[i]), runtime);
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
    Check(branch != 0u, "find a node with two children");
    const auto removed_key = runtime.texts.at(branch + 12u);
    std::vector<std::string> expected = input;
    std::sort(expected.begin(), expected.end());
    const auto position = std::find(expected.begin(), expected.end(), removed_key);
    Check(position != expected.end(), "selected node occurs in tree");
    const auto next_key = position + 1 == expected.end() ?
        std::string{} : *(position + 1);
    expected.erase(position);

    pal4::DialogTextTree::EraseNode(Runtime::kTree, 0x7000u, branch,
                                    Runtime::kNil, runtime);
    const auto successor = runtime.ReadDword(0x7000u);
    Check((next_key.empty() && successor == Runtime::kHead) ||
          (!next_key.empty() && runtime.texts.at(successor + 12u) == next_key),
          "erase returns original successor");
    Check(runtime.ReadDword(Runtime::kTree + 12u) == input.size() - 1u &&
          runtime.frees.size() == 2u && runtime.frees.back() == branch,
          "erase releases text and node then decrements count");
    std::vector<std::string> ordered;
    int height = 0;
    const auto root = runtime.ReadDword(Runtime::kHead + 4u);
    Validate(runtime, root, Runtime::kHead, ordered, height);
    Check(ordered == expected, "two-child erase repairs ordered tree");

    while (!expected.empty()) {
        const auto first = runtime.ReadDword(Runtime::kHead);
        Check(runtime.texts.at(first + 12u) == expected.front(),
              "header leftmost before erase");
        pal4::DialogTextTree::EraseNode(Runtime::kTree, 0x7000u, first,
                                        Runtime::kNil, runtime);
        expected.erase(expected.begin());
        ordered.clear();
        height = 0;
        Validate(runtime, runtime.ReadDword(Runtime::kHead + 4u),
                 Runtime::kHead, ordered, height);
        Check(ordered == expected &&
              runtime.ReadDword(Runtime::kTree + 12u) == expected.size(),
              "each erase preserves red-black ordering and count");
    }
    Check(runtime.ReadDword(Runtime::kHead) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kHead + 8u) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kHead + 4u) == Runtime::kNil &&
          runtime.frees.size() == input.size() * 2u,
          "empty tree restores header and releases all nodes and text");
}

void TestEraseRange() {
    Runtime runtime(false);
    const std::vector<std::string> input{
        "m", "a", "z", "c", "b", "y", "x", "d", "n"};
    for (std::uint32_t i = 0; i < input.size(); ++i)
        (void)pal4::DialogTextTree::Insert(Runtime::kTree,
            runtime.Input(0x8000u + i * 0x20u, input[i]), runtime);
    const auto find_node = [&](const std::string& key) {
        for (std::uint32_t node = 0x4000u; node < runtime.next_node;
             node += 0x40u) {
            if (runtime.texts.at(node + 12u) == key) return node;
        }
        return 0u;
    };
    const auto first = find_node("c");
    const auto last = find_node("y");
    Check(first != 0u && last != 0u, "range boundary nodes exist");
    pal4::DialogTextTree::EraseRange(Runtime::kTree, 0x7000u, first, last,
                                     Runtime::kNil, runtime);
    Check(runtime.ReadDword(0x7000u) == last &&
          runtime.ReadDword(Runtime::kTree + 12u) == 4u &&
          runtime.frees.size() == 10u,
          "partial range erases five nodes and returns last");
    std::vector<std::string> ordered;
    int height = 0;
    Validate(runtime, runtime.ReadDword(Runtime::kHead + 4u),
             Runtime::kHead, ordered, height);
    Check(ordered == std::vector<std::string>{"a", "b", "y", "z"},
          "partial range preserves outside nodes");

    pal4::DialogTextTree::EraseRange(Runtime::kTree, 0x7000u,
        runtime.ReadDword(Runtime::kHead), Runtime::kHead,
        Runtime::kNil, runtime);
    Check(runtime.ReadDword(0x7000u) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kHead) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kHead + 4u) == Runtime::kNil &&
          runtime.ReadDword(Runtime::kHead + 8u) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kTree + 12u) == 0u &&
          runtime.frees.size() == input.size() * 2u,
          "full range clears tree and releases all records");

    runtime.frees.clear();
    pal4::DialogTextTree::EraseRange(Runtime::kTree, 0x7000u,
        Runtime::kHead, Runtime::kHead, Runtime::kNil, runtime);
    Check(runtime.ReadDword(0x7000u) == Runtime::kHead &&
          runtime.frees.empty(), "empty range does not release records");
}

void TestLowerBound() {
    Runtime runtime(false);
    Check(pal4::DialogTextTree::LowerBound(Runtime::kTree,
          runtime.Input(0x8000u, "a"), runtime) == Runtime::kHead,
          "empty tree returns header");
    const auto middle = pal4::DialogTextTree::Insert(Runtime::kTree,
        runtime.Input(0x8020u, "m"), runtime).node;
    const auto first = pal4::DialogTextTree::Insert(Runtime::kTree,
        runtime.Input(0x8040u, "a"), runtime).node;
    const auto last = pal4::DialogTextTree::Insert(Runtime::kTree,
        runtime.Input(0x8060u, "z"), runtime).node;
    Check(pal4::DialogTextTree::LowerBound(Runtime::kTree,
          runtime.Input(0x8080u, "a"), runtime) == first,
          "exact minimum match");
    Check(pal4::DialogTextTree::LowerBound(Runtime::kTree,
          runtime.Input(0x80a0u, "b"), runtime) == middle,
          "between two keys chooses next larger");
    Check(pal4::DialogTextTree::LowerBound(Runtime::kTree,
          runtime.Input(0x80c0u, "z"), runtime) == last,
          "exact maximum match");
    Check(pal4::DialogTextTree::LowerBound(Runtime::kTree,
          runtime.Input(0x80e0u, "zz"), runtime) == Runtime::kHead,
          "greater than maximum returns header");
}

void TestDeleteSubtree() {
    Runtime runtime(false);
    const std::vector<std::string> names{
        "m", "a", "z", "c", "b", "y", "x"};
    for (std::uint32_t i = 0; i < names.size(); ++i)
        (void)pal4::DialogTextTree::Insert(Runtime::kTree,
            runtime.Input(0x8000u + i * 0x20u, names[i]), runtime);
    const auto root = runtime.ReadDword(Runtime::kHead + 4u);
    Check(pal4::DialogTextTree::DeleteSubtree(root, Runtime::kNil,
          runtime) == Runtime::kNil &&
          runtime.frees.size() == names.size() * 2u,
          "subtree deletion releases all node texts and nodes");
    for (std::size_t i = 1u; i < runtime.frees.size(); i += 2u)
        Check(runtime.frees[i] >= 0x4000u &&
              runtime.frees[i] < runtime.next_node,
              "each text release precedes its node free");
    runtime.frees.clear();
    Check(pal4::DialogTextTree::DeleteSubtree(Runtime::kNil,
          Runtime::kNil, runtime) == Runtime::kNil &&
          runtime.frees.empty(), "empty subtree returns nil without release");
}

void TestInsertNode() {
    Runtime runtime(false);
    const auto root = pal4::DialogTextTree::InsertNode(
        Runtime::kTree, Runtime::kNil, Runtime::kHead,
        runtime.Input(0x8000u, "middle"), runtime);
    Check(root == 0x4000u &&
          runtime.ReadDword(Runtime::kHead + 4u) == root &&
          runtime.ReadDword(root + 4u) == Runtime::kHead &&
          runtime.ReadDword(root + 32u) == 1u &&
          runtime.ReadDword(Runtime::kTree + 12u) == 1u &&
          runtime.texts.at(root + 12u) == "middle",
          "helper inserts a black root with copied text");
    const auto left = pal4::DialogTextTree::InsertNode(
        Runtime::kTree, Runtime::kNil, root,
        runtime.Input(0x8020u, "alpha"), runtime);
    Check(runtime.ReadDword(root) == left &&
          runtime.ReadDword(Runtime::kHead) == left &&
          runtime.ReadDword(Runtime::kTree + 12u) == 2u &&
          runtime.texts.at(left + 12u) == "alpha",
          "helper inserts at supplied left child and updates minimum");
}

void TestPromoteRightChild() {
    Runtime runtime(false);
    constexpr std::uint32_t a = 0x4000u;
    constexpr std::uint32_t b = 0x4040u;
    constexpr std::uint32_t c = 0x4080u;
    runtime.words[Runtime::kHead + 4u] = a;
    runtime.words[a] = Runtime::kNil;
    runtime.words[a + 4u] = Runtime::kHead;
    runtime.words[a + 8u] = b;
    runtime.words[b] = c;
    runtime.words[b + 4u] = a;
    runtime.words[b + 8u] = Runtime::kNil;
    runtime.words[c + 4u] = b;
    Check(pal4::DialogTextTree::PromoteRightChild(
          Runtime::kTree, a, runtime) == b &&
          runtime.ReadDword(Runtime::kHead + 4u) == b &&
          runtime.ReadDword(b + 4u) == Runtime::kHead &&
          runtime.ReadDword(b) == a &&
          runtime.ReadDword(a + 4u) == b &&
          runtime.ReadDword(a + 8u) == c &&
          runtime.ReadDword(c + 4u) == a,
          "root rotation promotes right child and reparents middle subtree");

    runtime = Runtime(false);
    constexpr std::uint32_t parent = 0x40c0u;
    runtime.words[Runtime::kHead + 4u] = parent;
    runtime.words[parent] = a;
    runtime.words[a + 4u] = parent;
    runtime.words[a + 8u] = b;
    runtime.words[b] = Runtime::kNil;
    runtime.words[b + 4u] = a;
    Check(pal4::DialogTextTree::PromoteRightChild(
          Runtime::kTree, a, runtime) == b &&
          runtime.ReadDword(parent) == b &&
          runtime.ReadDword(b + 4u) == parent &&
          runtime.ReadDword(b) == a &&
          runtime.ReadDword(a + 8u) == Runtime::kNil,
          "non-root rotation relinks left branch");
}

void TestMinimumFromLink() {
    Runtime runtime(false);
    constexpr std::uint32_t link = 0x7000u;
    runtime.words[link] = Runtime::kNil;
    Check(pal4::DialogTextTree::MinimumFromLink(link, Runtime::kNil,
          runtime) == link, "empty link returns its address");
    runtime.words[link] = 0x4000u;
    runtime.words[0x4000u] = 0x4040u;
    runtime.words[0x4040u] = 0x4080u;
    runtime.words[0x4080u] = Runtime::kNil;
    Check(pal4::DialogTextTree::MinimumFromLink(link, Runtime::kNil,
          runtime) == 0x4080u, "left spine returns deepest node");
}

void TestPromoteLeftChild() {
    Runtime runtime(false);
    constexpr std::uint32_t a = 0x4000u;
    constexpr std::uint32_t b = 0x4040u;
    constexpr std::uint32_t c = 0x4080u;
    runtime.words[Runtime::kHead + 4u] = a;
    runtime.words[a] = b;
    runtime.words[a + 4u] = Runtime::kHead;
    runtime.words[a + 8u] = Runtime::kNil;
    runtime.words[b] = Runtime::kNil;
    runtime.words[b + 4u] = a;
    runtime.words[b + 8u] = c;
    runtime.words[c + 4u] = b;
    Check(pal4::DialogTextTree::PromoteLeftChild(
          Runtime::kTree, a, runtime) == b &&
          runtime.ReadDword(Runtime::kHead + 4u) == b &&
          runtime.ReadDword(b + 4u) == Runtime::kHead &&
          runtime.ReadDword(b + 8u) == a &&
          runtime.ReadDword(a + 4u) == b &&
          runtime.ReadDword(a) == c &&
          runtime.ReadDword(c + 4u) == a,
          "root rotation promotes left child and reparents middle subtree");

    runtime = Runtime(false);
    constexpr std::uint32_t parent = 0x40c0u;
    runtime.words[Runtime::kHead + 4u] = parent;
    runtime.words[parent + 8u] = a;
    runtime.words[a] = b;
    runtime.words[a + 4u] = parent;
    runtime.words[b + 4u] = a;
    runtime.words[b + 8u] = Runtime::kNil;
    Check(pal4::DialogTextTree::PromoteLeftChild(
          Runtime::kTree, a, runtime) == b &&
          runtime.ReadDword(parent + 8u) == b &&
          runtime.ReadDword(b + 4u) == parent &&
          runtime.ReadDword(b + 8u) == a &&
          runtime.ReadDword(a) == Runtime::kNil,
          "non-root rotation relinks right branch");
}

void TestPrevious() {
    Runtime runtime(false);
    runtime.words[Runtime::kHead + 32u] = 0u;
    const auto middle = pal4::DialogTextTree::Insert(Runtime::kTree,
        runtime.Input(0x8000u, "m"), runtime).node;
    const auto first = pal4::DialogTextTree::Insert(Runtime::kTree,
        runtime.Input(0x8020u, "a"), runtime).node;
    const auto last = pal4::DialogTextTree::Insert(Runtime::kTree,
        runtime.Input(0x8040u, "z"), runtime).node;
    Check(pal4::DialogTextTree::Previous(last, runtime) == middle &&
          pal4::DialogTextTree::Previous(middle, runtime) == first &&
          pal4::DialogTextTree::Previous(first, runtime) == Runtime::kHead &&
          pal4::DialogTextTree::Previous(Runtime::kHead, runtime) == last,
          "predecessor traverses nodes and header in both directions");
}

void TestNext() {
    Runtime runtime(false);
    const auto middle = pal4::DialogTextTree::Insert(Runtime::kTree,
        runtime.Input(0x8000u, "m"), runtime).node;
    const auto first = pal4::DialogTextTree::Insert(Runtime::kTree,
        runtime.Input(0x8020u, "a"), runtime).node;
    const auto last = pal4::DialogTextTree::Insert(Runtime::kTree,
        runtime.Input(0x8040u, "z"), runtime).node;
    Check(pal4::DialogTextTree::Next(first, runtime) == middle &&
          pal4::DialogTextTree::Next(middle, runtime) == last &&
          pal4::DialogTextTree::Next(last, runtime) == Runtime::kHead,
          "text successor traverses nodes and reaches header");
}

}  // namespace

int main() {
    TestUnique();
    TestDuplicates();
    TestErase();
    TestEraseRange();
    TestLowerBound();
    TestDeleteSubtree();
    TestInsertNode();
    TestPromoteRightChild();
    TestMinimumFromLink();
    TestPromoteLeftChild();
    TestPrevious();
    TestNext();
}
