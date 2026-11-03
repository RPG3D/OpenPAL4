#include "pal4/monster_manager_tree_lookup.h"
#include "pal4/npc_manager_tree.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
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

struct Runtime final : pal4::IMonsterManagerTreeLookupRuntime {
    static constexpr std::uint32_t kTree = 0x1000u;
    static constexpr std::uint32_t kHeader = 0x2000u;
    static constexpr std::uint32_t kNil = 0x3000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, std::string> texts;
    std::vector<std::uint32_t> compare_nodes;
    std::vector<std::uint32_t> range_nodes;

    Runtime() {
        words[kTree + 4u] = kHeader;
        words[kHeader + 4u] = kNil;
        bytes[0x9000u] = 0u;
    }
    std::uint32_t SharedNil() const override { return kNil; }
    std::uint32_t ReadDword(std::uint32_t address) const override {
        const auto found = words.find(address);
        Check(found != words.end(), "initialized word");
        return found->second;
    }
    std::uint8_t ReadByte(std::uint32_t address) const override {
        const auto found = bytes.find(address);
        Check(found != bytes.end(), "initialized byte");
        return found->second;
    }
    std::uint32_t EmptyTextData() const override { return 0x9000u; }
    std::int32_t CompareText(std::uint32_t node_text,
                             std::uint32_t input) override {
        compare_nodes.push_back(node_text - 0x0cu);
        const auto& left = texts.at(node_text);
        const auto& right = texts.at(input);
        return left < right ? -1 : (right < left ? 1 : 0);
    }
    std::int32_t CompareRange(std::uint32_t node_text,
                              std::uint32_t input) override {
        range_nodes.push_back(node_text - 0x0cu);
        const auto& left = texts.at(node_text);
        const auto& right = texts.at(input);
        return left < right ? -1 : (right < left ? 1 : 0);
    }
    void Text(std::uint32_t record, std::uint32_t data, std::string text) {
        words[record + 4u] = data;
        words[record + 8u] = static_cast<std::uint32_t>(text.size());
        for (std::uint32_t i = 0; i < text.size(); ++i)
            bytes[data + i] = static_cast<std::uint8_t>(text[i]);
        texts[record] = std::move(text);
    }
    void Node(std::uint32_t address, std::uint32_t left,
              std::uint32_t right, std::uint32_t data,
              std::string text) {
        words[address] = left;
        words[address + 8u] = right;
        Text(address + 0x0cu, data, std::move(text));
    }
    std::uint32_t Query(std::uint32_t address, std::uint32_t data,
                        std::string text) {
        Text(address, data, std::move(text));
        return address;
    }
};

void TestLookup() {
    Runtime runtime;
    Check(pal4::MonsterManagerTreeLookup::Search(Runtime::kTree,
        runtime.Query(0xa000u, 0xb000u, "m"), runtime) == Runtime::kHeader &&
        runtime.compare_nodes.empty(), "empty tree returns header");

    runtime.words[Runtime::kHeader + 4u] = 0x4000u;
    runtime.Node(0x4000u, 0x5000u, 0x6000u, 0x7000u, "m");
    runtime.Node(0x5000u, Runtime::kNil, Runtime::kNil, 0x7100u, "a");
    runtime.Node(0x6000u, Runtime::kNil, Runtime::kNil, 0x7200u, "z");
    const auto search = [&](std::uint32_t offset, const char* value) {
        const auto query = runtime.Query(0xa100u + offset * 0x20u,
                                         0xb100u + offset * 0x20u, value);
        runtime.compare_nodes.clear();
        return pal4::MonsterManagerTreeLookup::Search(Runtime::kTree,
                                                       query, runtime);
    };
    Check(search(0u, "a") == 0x5000u, "left exact match");
    Check(search(1u, "m") == 0x4000u, "root exact match");
    Check(search(2u, "z") == 0x6000u, "right exact match");
    Check(search(3u, "") == Runtime::kHeader, "shorter prefix rejected");
    Check(search(4u, "b") == Runtime::kHeader, "between keys rejected");
    Check(search(5u, "zz") == Runtime::kHeader, "past maximum rejected");
    Check(runtime.compare_nodes == std::vector<std::uint32_t>{
        0x4000u, 0x6000u}, "node-first lower-bound comparison path");
}

void TestManualComparison() {
    Runtime runtime;
    runtime.words[Runtime::kHeader + 4u] = 0x4000u;
    runtime.Node(0x4000u, Runtime::kNil, Runtime::kNil, 0x7000u, "m");
    // The lower-bound comparator and the byte record can disagree in the
    // original binary; the final byte/length check still controls output.
    runtime.texts[0x400cu] = "z";
    const auto longer = runtime.Query(0xa000u, 0xb000u, "mX");
    Check(pal4::MonsterManagerTreeLookup::Search(Runtime::kTree, longer,
                                                  runtime) == 0x4000u,
          "input longer than candidate accepted after equal prefix");
    runtime.words[0x4000u + 0x10u] = 0u;
    runtime.words[0x4000u + 0x14u] = 0u;
    runtime.texts[0x400cu] = "";
    const auto empty = runtime.Query(0xa020u, 0xb020u, "");
    Check(pal4::MonsterManagerTreeLookup::Search(Runtime::kTree, empty,
                                                  runtime) == 0x4000u,
          "null node buffer uses empty text");
}

void TestAlternativeLowerBound() {
    Runtime runtime;
    runtime.words[Runtime::kHeader + 4u] = 0x4000u;
    runtime.Node(0x4000u, 0x5000u, 0x6000u, 0x7000u, "m");
    runtime.Node(0x5000u, Runtime::kNil, Runtime::kNil, 0x7100u, "a");
    runtime.Node(0x6000u, Runtime::kNil, Runtime::kNil, 0x7200u, "z");
    const auto query = [&](std::uint32_t slot, const char* text) {
        return runtime.Query(0xa000u + slot * 0x20u,
                             0xb000u + slot * 0x20u, text);
    };
    Check(pal4::MonsterManagerTreeLookup::LowerBoundAlternative(
        Runtime::kTree, query(0u, "b"), runtime) == 0x4000u,
          "alternative returns next greater node");
    Check(runtime.range_nodes == std::vector<std::uint32_t>{0x4000u, 0x5000u} &&
          runtime.compare_nodes.empty(), "range comparator receives node first");
    Check(pal4::MonsterManagerTreeLookup::LowerBoundAlternative(
        Runtime::kTree, query(1u, "a"), runtime) == 0x5000u,
          "alternative exact left match");
    Check(pal4::MonsterManagerTreeLookup::LowerBoundAlternative(
        Runtime::kTree, query(2u, "zz"), runtime) == Runtime::kHeader,
          "alternative past maximum returns header");
}

void TestNpcTreeLookupAndCursor() {
    Runtime runtime;
    runtime.words[Runtime::kHeader + 4u] = 0x4000u;
    runtime.words[Runtime::kHeader] = 0x5000u;
    runtime.words[Runtime::kHeader + 8u] = 0x6000u;
    runtime.Node(0x4000u, 0x5000u, 0x6000u, 0x7000u, "m");
    runtime.Node(0x5000u, Runtime::kNil, Runtime::kNil, 0x7100u, "a");
    runtime.Node(0x6000u, Runtime::kNil, Runtime::kNil, 0x7200u, "z");
    runtime.words[0x4004u] = Runtime::kHeader;
    runtime.words[0x5004u] = 0x4000u;
    runtime.words[0x6004u] = 0x4000u;
    const auto name = runtime.Query(0xa000u, 0xb000u, "m");
    Check(pal4::NpcManagerTree::Search(
              Runtime::kTree, name, runtime) == 0x4000u,
          "NPC tree exact name lookup");
    const auto between = runtime.Query(0xa020u, 0xb020u, "n");
    Check(pal4::NpcManagerTree::Search(
              Runtime::kTree, between, runtime) == Runtime::kHeader &&
          pal4::NpcManagerTree::LowerBoundAlternative(
              Runtime::kTree, between, runtime) == 0x6000u,
          "NPC tree final check differs from lower-bound candidate");
    const auto next = pal4::NpcManagerTree::Successor(
        0x5000u, Runtime::kNil, runtime);
    Check(next.next == 0x4000u,
          "NPC tree cursor advances from leftmost to root");
    const auto end = pal4::NpcManagerTree::Successor(
        0x6000u, Runtime::kNil, runtime);
    Check(end.next == Runtime::kHeader,
          "NPC tree cursor reaches the header after rightmost");
}

}  // namespace

int main() {
    TestLookup();
    TestManualComparison();
    TestAlternativeLowerBound();
    TestNpcTreeLookupAndCursor();
}
