#include "pal4/monster_manager_tree_insert.h"
#include "pal4/monster_manager_tree_remove.h"
#include "pal4/npc_manager_tree.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <set>
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

struct Runtime final : pal4::IMonsterManagerTreeInsertRuntime,
                       pal4::IMonsterManagerTreeRemoveRuntime {
    static constexpr std::uint32_t kTree = 0x1000u;
    static constexpr std::uint32_t kHeader = 0x2000u;
    static constexpr std::uint32_t kNil = 0x3000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, std::string> texts;
    std::vector<std::uint32_t> freed;
    std::uint32_t next_node = 0x4000u;

    Runtime() {
        words[kTree + 4u] = kHeader;
        words[kTree + 8u] = 1u;
        words[kTree + 12u] = 0u;
        words[kHeader] = kHeader;
        words[kHeader + 4u] = kNil;
        words[kHeader + 8u] = kHeader;
        words[kNil] = kNil;
        words[kNil + 8u] = kNil;
        words[kNil + 0x20u] = 1u;
    }
    std::uint32_t SharedNil() const override { return kNil; }
    std::uint32_t ReadDword(std::uint32_t address) const override {
        const auto found = words.find(address);
        Check(found != words.end(), "read initialized word");
        return found->second;
    }
    std::uint8_t ReadByte(std::uint32_t address) const override {
        const auto found = bytes.find(address);
        if (found != bytes.end()) return found->second;
        const auto value = ReadDword(address & ~3u);
        return static_cast<std::uint8_t>(value >> ((address & 3u) * 8u));
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
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
        return node;
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        texts[destination] = texts.at(source);
        words[destination + 4u] = 0u;
        words[destination + 8u] = 0u;
        words[destination + 12u] = 0u;
    }
    void SetNodeData(pal4::MonsterManagerTreeInsertResult&,
                     std::uint32_t) override {
        Check(false, "direct insert does not request duplicate result");
    }
    void FreeMemory(std::uint32_t address) override {
        if (address >= 0x4000u && address < next_node) {
            Check(ReadDword(address + 0x10u) == 0u &&
                  ReadDword(address + 0x14u) == 0u &&
                  ReadDword(address + 0x18u) == 0u,
                  "name record cleared before node free");
        }
        freed.push_back(address);
    }
};

int ValidateNode(const Runtime& runtime, std::uint32_t node,
                 std::uint32_t parent, std::set<std::uint32_t>& seen,
                 std::vector<std::string>& keys) {
    if (node == Runtime::kNil) return 1;
    Check(seen.insert(node).second, "no node cycle");
    Check(runtime.ReadDword(node + 4u) == parent, "parent link");
    const auto left = runtime.ReadDword(node);
    const auto right = runtime.ReadDword(node + 8u);
    const auto color = runtime.ReadDword(node + 0x20u);
    Check(color <= 1u, "valid color");
    if (color == 0u)
        Check(runtime.ReadDword(left + 0x20u) == 1u &&
              runtime.ReadDword(right + 0x20u) == 1u,
              "red node children black");
    const auto left_height = ValidateNode(runtime, left, node, seen, keys);
    keys.push_back(runtime.texts.at(node + 0x0cu));
    const auto right_height = ValidateNode(runtime, right, node, seen, keys);
    Check(left_height == right_height, "black height");
    return left_height + static_cast<int>(color);
}

void Validate(const Runtime& runtime, const std::map<int, std::uint32_t>& active) {
    const auto root = runtime.ReadDword(Runtime::kHeader + 4u);
    Check(runtime.ReadDword(Runtime::kTree + 12u) == active.size(),
          "tree node count");
    if (active.empty()) {
        Check(root == Runtime::kNil &&
              runtime.ReadDword(Runtime::kHeader) == Runtime::kHeader &&
              runtime.ReadDword(Runtime::kHeader + 8u) == Runtime::kHeader,
              "empty tree header");
        return;
    }
    Check(runtime.ReadDword(root + 0x20u) == 1u, "black root");
    std::set<std::uint32_t> seen;
    std::vector<std::string> keys;
    (void)ValidateNode(runtime, root, Runtime::kHeader, seen, keys);
    std::vector<std::string> expected;
    for (const auto& [key, node] : active) {
        (void)node;
        expected.push_back(std::to_string(1000 + key));
    }
    Check(keys == expected && seen.size() == active.size(), "ordered active nodes");
    Check(runtime.ReadDword(Runtime::kHeader) == active.begin()->second &&
          runtime.ReadDword(Runtime::kHeader + 8u) == active.rbegin()->second,
          "header extrema");
}

void TestContinuousDeletion() {
    Runtime runtime;
    std::vector<int> order(100);
    std::iota(order.begin(), order.end(), 0);
    std::mt19937 generator(0x438520u);
    std::shuffle(order.begin(), order.end(), generator);
    std::map<int, std::uint32_t> active;
    for (std::uint32_t i = 0; i < order.size(); ++i) {
        const auto input = 0x10000u + i * 0x20u;
        runtime.texts[input] = std::to_string(1000 + order[i]);
        const auto result = pal4::MonsterManagerTreeInsert::Insert(
            Runtime::kTree, input, runtime);
        Check(result.inserted, "seed insertion");
        active[order[i]] = result.node;
    }
    Validate(runtime, active);
    std::shuffle(order.begin(), order.end(), generator);
    for (const auto key : order) {
        const auto current = active.find(key);
        const auto node = current->second;
        const auto following = std::next(current);
        const auto expected_successor = following == active.end()
            ? Runtime::kHeader : following->second;
        const auto old_frees = runtime.freed.size();
        const auto next = pal4::MonsterManagerTreeRemove::Remove(
            Runtime::kTree, node, runtime);
        Check(next == expected_successor, "saved successor return");
        Check(runtime.freed.size() == old_frees + 1u &&
              runtime.freed.back() == node, "original node freed");
        active.erase(current);
        Validate(runtime, active);
    }
}

void TestReferenceHeader() {
    for (const std::uint8_t count : {std::uint8_t{0u},
                                    std::uint8_t{1u}, std::uint8_t{0xffu}}) {
        Runtime runtime;
        runtime.texts[0x10000u] = "1000";
        const auto node = pal4::MonsterManagerTreeInsert::Insert(
            Runtime::kTree, 0x10000u, runtime).node;
        constexpr std::uint32_t data = 0x50001u;
        runtime.words[node + 0x10u] = data;
        runtime.words[node + 0x14u] = 4u;
        runtime.words[node + 0x18u] = 8u;
        runtime.bytes[data - 1u] = count;
        Check(pal4::MonsterManagerTreeRemove::Remove(Runtime::kTree, node,
                                                     runtime) == Runtime::kHeader,
              "single-node successor");
        if (count == 1u) {
            Check(runtime.freed == std::vector<std::uint32_t>{node} &&
                  runtime.bytes[data - 1u] == 0u,
                  "shared header decremented");
        } else {
            Check(runtime.freed ==
                      (std::vector<std::uint32_t>{data - 1u, node}),
                  "unshared header freed before node");
        }
    }
}

void TestNpcTreeComposition() {
    Runtime runtime;
    std::map<int, std::uint32_t> active;
    for (const int key : {2, 1, 3}) {
        const auto address = 0x20000u + static_cast<std::uint32_t>(key) * 0x20u;
        runtime.texts[address] = std::to_string(1000 + key);
        const auto result = pal4::NpcManagerTree::Insert(
            Runtime::kTree, address, runtime);
        Check(result.inserted, "NPC tree inserted unique name");
        active[key] = result.node;
        Validate(runtime, active);
    }
    Check(pal4::NpcManagerTree::Remove(
              Runtime::kTree, active[2], runtime) == active[3],
          "NPC tree removal returns the saved successor");
    active.erase(2);
    Validate(runtime, active);
    Check(runtime.freed ==
              std::vector<std::uint32_t>{0x4000u},
          "NPC tree frees the original node after transplant");
}

}  // namespace

int main() {
    TestContinuousDeletion();
    TestReferenceHeader();
    TestNpcTreeComposition();
}
