#include "pal4/monster_manager_tree_insert.h"
#include "pal4/npc_manager_tree.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMonsterManagerTreeInsertRuntime {
    static constexpr std::uint32_t kTree = 0x1000u;
    static constexpr std::uint32_t kHeader = 0x2000u;
    static constexpr std::uint32_t kNil = 0x3000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::string> texts;
    std::uint32_t next_node = 0x4000u;
    unsigned raw_allocations = 0u;
    unsigned set_data_calls = 0u;

    explicit Runtime(bool direct) {
        words[kTree + 4u] = kHeader;
        words[kTree + 8u] = direct ? 1u : 0u;
        words[kTree + 12u] = 0u;
        words[kHeader] = kHeader;
        words[kHeader + 4u] = kNil;
        words[kHeader + 8u] = kHeader;
        words[kNil + 0x20u] = 1u;
    }
    std::uint32_t SharedNil() const override { return kNil; }
    std::uint32_t ReadDword(std::uint32_t address) const override {
        const auto found = words.find(address);
        Check(found != words.end(), "read initialized word");
        return found->second;
    }
    std::uint8_t ReadByte(std::uint32_t address) const override {
        const auto aligned = address & ~3u;
        return static_cast<std::uint8_t>(ReadDword(aligned) >> ((address & 3u) * 8u));
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
        const auto& left = texts.at(first);
        const auto& right = texts.at(second);
        return left < right ? -1 : (right < left ? 1 : 0);
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        Check(bytes == 0x24u, "node allocation size");
        const auto node = next_node;
        next_node += 0x40u;
        ++raw_allocations;
        return node;
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        texts[destination] = texts.at(source);
    }
    void SetNodeData(pal4::MonsterManagerTreeInsertResult& result,
                     std::uint32_t candidate) override {
        ++set_data_calls;
        result.node = candidate;
    }
    std::uint32_t AddInput(std::uint32_t address, std::string value) {
        texts[address] = std::move(value);
        return address;
    }
    std::uint32_t Word(std::uint32_t address) const { return ReadDword(address); }
};

int ValidateNode(const Runtime& runtime, std::uint32_t node,
                 std::uint32_t parent, std::vector<std::string>& keys) {
    if (node == Runtime::kNil) return 1;
    Check(runtime.Word(node + 4u) == parent, "parent link");
    const auto left = runtime.Word(node);
    const auto right = runtime.Word(node + 8u);
    const auto color = runtime.Word(node + 0x20u);
    Check(color <= 1u, "node color");
    if (color == 0u) {
        Check(runtime.Word(left + 0x20u) == 1u &&
              runtime.Word(right + 0x20u) == 1u, "red children black");
    }
    const auto left_height = ValidateNode(runtime, left, node, keys);
    keys.push_back(runtime.texts.at(node + 0x0cu));
    const auto right_height = ValidateNode(runtime, right, node, keys);
    Check(left_height == right_height, "equal black height");
    return left_height + static_cast<int>(color);
}

void TestDirectInsert() {
    Runtime runtime(true);
    std::vector<int> order(100);
    std::iota(order.begin(), order.end(), 0);
    std::mt19937 generator(0x4382c0u);
    std::shuffle(order.begin(), order.end(), generator);
    for (std::uint32_t i = 0; i < order.size(); ++i) {
        const auto input = runtime.AddInput(0x10000u + i * 0x20u,
            std::to_string(1000 + order[i]));
        const auto result = pal4::MonsterManagerTreeInsert::Insert(
            Runtime::kTree, input, runtime);
        Check(result.inserted && result.node != Runtime::kNil,
              "direct insertion result");
        Check(runtime.Word(Runtime::kTree + 12u) == i + 1u,
              "direct insertion count");
        const auto root = runtime.Word(Runtime::kHeader + 4u);
        Check(runtime.Word(root + 0x20u) == 1u, "black root");
        std::vector<std::string> keys;
        (void)ValidateNode(runtime, root, Runtime::kHeader, keys);
        Check(std::is_sorted(keys.begin(), keys.end()) && keys.size() == i + 1u,
              "in-order contents");
        Check(runtime.texts.at(runtime.Word(Runtime::kHeader) + 0x0cu) == keys.front() &&
              runtime.texts.at(runtime.Word(Runtime::kHeader + 8u) + 0x0cu) == keys.back(),
              "header extrema");
    }
    const auto duplicate = runtime.AddInput(0x20000u, "1050");
    const auto duplicate_result = pal4::MonsterManagerTreeInsert::Insert(
        Runtime::kTree, duplicate, runtime);
    Check(duplicate_result.inserted && runtime.Word(Runtime::kTree + 12u) == 101u,
          "direct mode permits duplicate keys");
    std::vector<std::string> keys;
    (void)ValidateNode(runtime, runtime.Word(Runtime::kHeader + 4u),
                       Runtime::kHeader, keys);
    Check(std::is_sorted(keys.begin(), keys.end()) && keys.size() == 101u &&
          std::count(keys.begin(), keys.end(), "1050") == 2,
          "duplicate preserves red-black ordering");
    Check(runtime.raw_allocations == 101u,
          "direct path allocates every inserted node");
}

void TestUniqueMode() {
    Runtime runtime(false);
    const auto input = runtime.AddInput(0x8000u, "m");
    auto result = pal4::MonsterManagerTreeInsert::Insert(Runtime::kTree,
                                                         input, runtime);
    const auto root = result.node;
    Check(result.inserted && root != Runtime::kNil &&
          runtime.raw_allocations == 1u &&
          runtime.Word(root + 4u) == Runtime::kHeader &&
          runtime.Word(Runtime::kTree + 12u) == 1u,
          "empty tree uses raw helper allocation");

    result = pal4::MonsterManagerTreeInsert::Insert(Runtime::kTree,
        runtime.AddInput(0x8020u, "a"), runtime);
    const auto leftmost = result.node;
    Check(result.inserted && runtime.raw_allocations == 2u &&
          runtime.Word(leftmost + 4u) == root &&
          runtime.Word(Runtime::kHeader) == leftmost,
          "leftmost shortcut");

    result = pal4::MonsterManagerTreeInsert::Insert(Runtime::kTree, input, runtime);
    Check(!result.inserted && result.node == root &&
          runtime.set_data_calls == 1u && runtime.raw_allocations == 2u,
          "duplicate selects candidate");

    const auto right_child = pal4::MonsterManagerTreeInsert::Insert(
        Runtime::kTree, runtime.AddInput(0x8080u, "t"), runtime).node;
    result = pal4::MonsterManagerTreeInsert::Insert(Runtime::kTree,
        runtime.AddInput(0x8040u, "s"), runtime);
    Check(result.inserted, "predecessor comparison insert result");
    Check(runtime.raw_allocations == 4u &&
          runtime.Word(result.node + 4u) == right_child,
          "predecessor path inserts under right child");

    std::vector<std::string> keys;
    (void)ValidateNode(runtime, runtime.Word(Runtime::kHeader + 4u),
                       Runtime::kHeader, keys);
    Check(keys == std::vector<std::string>{"a", "m", "s", "t"} &&
          runtime.Word(Runtime::kTree + 12u) == 4u,
          "helper maintains unique-key tree");

    runtime.texts[0x8060u] = "m";
    result = pal4::MonsterManagerTreeInsert::Insert(Runtime::kTree, 0x8060u,
                                                     runtime);
    Check(!result.inserted && result.node == root &&
          runtime.set_data_calls == 2u, "predecessor duplicate");
}

void TestNpcInsertHelperEntry() {
    Runtime runtime(false);
    const auto name = runtime.AddInput(0x9000u, "npc");
    const auto node = pal4::NpcManagerTree::InsertHelper(
        Runtime::kTree, Runtime::kNil, Runtime::kHeader,
        name, runtime);
    Check(node == 0x4000u &&
          runtime.Word(Runtime::kHeader + 4u) == node &&
          runtime.Word(node + 4u) == Runtime::kHeader &&
          runtime.Word(Runtime::kTree + 12u) == 1u &&
          runtime.Word(node + 0x20u) == 1u,
          "NPC raw helper links a black root and increments count");
}

}  // namespace

int main() {
    TestDirectInsert();
    TestUniqueMode();
    TestNpcInsertHelperEntry();
}
