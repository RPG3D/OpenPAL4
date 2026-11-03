#include "pal4/effect_system_factory_text_tree.h"

#include <algorithm>
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

struct Runtime final : pal4::IEffectSystemFactoryTextInsertRuntime {
    static constexpr std::uint32_t kTree = 0x2000u;
    static constexpr std::uint32_t kHead = 0x3000u;
    static constexpr std::uint32_t kNil = 0x93cdf0u;
    static constexpr std::uint32_t kInput = 0x2600u;

    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kTree + 4u, kHead}, {kTree + 12u, 0u},
        {kHead, kHead}, {kHead + 4u, kNil},
        {kHead + 8u, kHead}, {kNil, kNil},
        {kNil + 4u, 0u}, {kNil + 8u, kNil},
        {kNil + 32u, 1u}};
    std::unordered_map<std::uint32_t, std::string> names;
    std::uint32_t next_allocation = 0x4000u;
    std::uint32_t allocations = 0u;
    std::uint8_t duplicate_flag = 0u;

    std::uint32_t ReadDword(std::uint32_t address) const override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) const override {
        Check(address == kTree + 8u, "duplicate flag offset");
        return duplicate_flag;
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        Check(bytes == 36u, "factory node allocation size");
        const auto node = next_allocation;
        next_allocation += 0x40u;
        ++allocations;
        return node;
    }
    std::int32_t CompareNames(std::uint32_t first,
                              std::uint32_t second) override {
        const auto& a = names.at(first);
        const auto& b = names.at(second);
        return a < b ? -1 : (a > b ? 1 : 0);
    }
    void CopyFactoryRecord(std::uint32_t destination,
                           std::uint32_t source) override {
        names[destination] = names.at(source);
        words[destination + 16u] = words.at(source + 16u);
    }
};

int Verify(const Runtime& runtime, std::uint32_t node,
           std::uint32_t parent,
           std::vector<std::string>& ordered) {
    if (node == Runtime::kNil) return 1;
    Check(runtime.ReadDword(node + 4u) == parent,
          "red-black parent link");
    const auto left = runtime.ReadDword(node);
    const auto right = runtime.ReadDword(node + 8u);
    const auto color = runtime.ReadDword(node + 32u);
    if (color == 0u)
        Check(runtime.ReadDword(left + 32u) == 1u &&
                  runtime.ReadDword(right + 32u) == 1u,
              "red node has black children");
    const auto left_height = Verify(runtime, left, node, ordered);
    ordered.push_back(runtime.names.at(node + 12u));
    const auto right_height = Verify(runtime, right, node, ordered);
    Check(left_height == right_height, "red-black heights match");
    return left_height + (color == 1u ? 1 : 0);
}

void TestInsertion() {
    Runtime runtime;
    const std::vector<std::string> names{
        "M", "F", "T", "A", "H", "P", "Z", "B", "G"};
    for (std::size_t i = 0u; i < names.size(); ++i) {
        runtime.names[Runtime::kInput] = names[i];
        runtime.words[Runtime::kInput + 16u] =
            0x6000u + static_cast<std::uint32_t>(i);
        const auto result =
            pal4::EffectSystemFactoryTextTree::InsertTextNode(
                Runtime::kTree, Runtime::kInput, runtime);
        Check(result.inserted &&
                  runtime.names.at(result.node + 12u) == names[i] &&
                  runtime.ReadDword(result.node + 28u) ==
                      0x6000u + static_cast<std::uint32_t>(i) &&
                  runtime.ReadDword(Runtime::kTree + 12u) ==
                      runtime.allocations,
              "insert copies name and factory callback into 36-byte node");
        const auto root = runtime.ReadDword(Runtime::kHead + 4u);
        Check(runtime.ReadDword(root + 32u) == 1u,
              "factory tree root remains black");
        std::vector<std::string> ordered;
        (void)Verify(runtime, root, Runtime::kHead, ordered);
        auto expected = std::vector<std::string>(
            names.begin(), names.begin() + runtime.allocations);
        std::sort(expected.begin(), expected.end());
        Check(ordered == expected, "factory names remain ordered");
    }
    runtime.names[Runtime::kInput] = "M";
    runtime.words[Runtime::kInput + 16u] = 0x9999u;
    const auto duplicate =
        pal4::EffectSystemFactoryTextTree::InsertTextNode(
            Runtime::kTree, Runtime::kInput, runtime);
    Check(!duplicate.inserted && runtime.allocations == names.size() &&
              runtime.ReadDword(duplicate.node + 28u) != 0x9999u,
          "unique tree returns old node without overwriting factory");
    runtime.duplicate_flag = 1u;
    const auto allowed =
        pal4::EffectSystemFactoryTextTree::InsertTextNode(
            Runtime::kTree, Runtime::kInput, runtime);
    Check(allowed.inserted && runtime.allocations == names.size() + 1u &&
              runtime.ReadDword(allowed.node + 28u) == 0x9999u,
          "duplicate-enabled tree allocates another callback node");
}

void TestPositionedInsert() {
    Runtime runtime;
    runtime.names[Runtime::kInput] = "Factory";
    runtime.words[Runtime::kInput + 16u] = 0x7777u;
    const auto node = pal4::EffectSystemFactoryTextTree::InsertAt(
        Runtime::kTree, Runtime::kNil, Runtime::kHead,
        Runtime::kInput, runtime);
    Check(node == 0x4000u && runtime.allocations == 1u &&
              runtime.ReadDword(Runtime::kHead + 4u) == node &&
              runtime.ReadDword(node + 4u) == Runtime::kHead &&
              runtime.ReadDword(node + 28u) == 0x7777u &&
              runtime.ReadDword(node + 32u) == 1u &&
              runtime.ReadDword(Runtime::kTree + 12u) == 1u,
          "positioned helper inserts and balances 36-byte root");
}

void TestRotations() {
    Runtime left;
    left.words[Runtime::kHead + 4u] = 0x4000u;
    left.words[0x4000u] = Runtime::kNil;
    left.words[0x4004u] = Runtime::kHead;
    left.words[0x4008u] = 0x4040u;
    left.words[0x4040u] = 0x4080u;
    left.words[0x4044u] = 0x4000u;
    left.words[0x4048u] = Runtime::kNil;
    left.words[0x4084u] = 0x4040u;
    Check(pal4::EffectSystemFactoryTextTree::RotateLeft(
              Runtime::kTree, 0x4000u, left) == 0x4040u &&
              left.words.at(Runtime::kHead + 4u) == 0x4040u &&
              left.words.at(0x4040u) == 0x4000u &&
              left.words.at(0x4008u) == 0x4080u &&
              left.words.at(0x4084u) == 0x4000u &&
              left.words.at(0x4044u) == Runtime::kHead,
          "left rotation promotes right child and reattaches middle subtree");

    Runtime right;
    right.words[Runtime::kHead + 4u] = 0x4000u;
    right.words[0x4000u] = 0x4040u;
    right.words[0x4004u] = Runtime::kHead;
    right.words[0x4008u] = Runtime::kNil;
    right.words[0x4040u] = Runtime::kNil;
    right.words[0x4044u] = 0x4000u;
    right.words[0x4048u] = 0x4080u;
    right.words[0x4084u] = 0x4040u;
    Check(pal4::EffectSystemFactoryTextTree::RotateRight(
              Runtime::kTree, 0x4000u, right) == 0x4040u &&
              right.words.at(Runtime::kHead + 4u) == 0x4040u &&
              right.words.at(0x4048u) == 0x4000u &&
              right.words.at(0x4000u) == 0x4080u &&
              right.words.at(0x4084u) == 0x4000u &&
              right.words.at(0x4044u) == Runtime::kHead,
          "right rotation promotes left child and reattaches middle subtree");
}

}  // namespace

int main() {
    TestInsertion();
    TestPositionedInsert();
    TestRotations();
}
