#include "pal4/effect_system_auxiliary_tree_insert.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime : pal4::IEffectSystemAuxiliaryInsertRuntime {
    static constexpr std::uint32_t kTree = 0x2000u;
    static constexpr std::uint32_t kHead = 0x3000u;
    static constexpr std::uint32_t kNil = 0x93ce10u;
    static constexpr std::uint32_t kResult = 0x2500u;
    static constexpr std::uint32_t kInput = 0x2600u;

    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kTree + 4u, kHead}, {kTree + 12u, 0u},
        {kHead, kHead}, {kHead + 4u, kNil},
        {kHead + 8u, kHead}, {kNil, kNil},
        {kNil + 4u, 0u}, {kNil + 8u, kNil},
        {kNil + 16u, 1u}};
    std::uint32_t next_allocation = 0x4000u;
    std::uint32_t allocations = 0u;

    pal4::EffectRegistrationTree Kind() const override {
        return pal4::EffectRegistrationTree::Clumps;
    }

    std::uint32_t ReadDword(std::uint32_t address) const override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t) const override { return 0u; }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        Check(bytes == NodeSize(), "native auxiliary node size");
        const auto node = next_allocation;
        next_allocation += 0x20u;
        ++allocations;
        return node;
    }
};

std::uint32_t ParentForKey(const Runtime& runtime,
                           std::uint32_t key, std::uint32_t nil) {
    auto parent = Runtime::kHead;
    auto node = runtime.ReadDword(Runtime::kHead + 4u);
    while (node != nil) {
        parent = node;
        node = runtime.ReadDword(node +
            (key < runtime.ReadDword(node + 12u) ? 0u : 8u));
    }
    return parent;
}

int VerifyNode(const Runtime& runtime, std::uint32_t node,
               std::uint32_t parent, std::uint32_t nil,
               std::uint32_t color_offset,
               std::vector<std::uint32_t>& ordered) {
    if (node == nil)
        return 1;
    Check(runtime.ReadDword(node + 4u) == parent,
          "parent link follows rotated tree");
    const auto left = runtime.ReadDword(node);
    const auto right = runtime.ReadDword(node + 8u);
    const auto color = runtime.ReadDword(node + color_offset);
    if (color == 0u) {
        Check(runtime.ReadDword(left + color_offset) == 1u &&
                  runtime.ReadDword(right + color_offset) == 1u,
              "red node has black children");
    }
    const auto left_height = VerifyNode(runtime, left, node, nil,
                                        color_offset, ordered);
    ordered.push_back(runtime.ReadDword(node + 12u));
    const auto right_height = VerifyNode(runtime, right, node, nil,
                                         color_offset, ordered);
    Check(left_height == right_height, "red-black height is balanced");
    return left_height + (color == 1u ? 1 : 0);
}

void TestPositionedInsertion() {
    Runtime runtime;
    const std::vector<std::uint32_t> keys{
        8u, 4u, 12u, 2u, 6u, 10u, 14u,
        0u, 0xffffffffu, 1u, 13u};
    for (const auto key : keys) {
        const auto parent = ParentForKey(runtime, key, Runtime::kNil);
        runtime.words[Runtime::kInput] = key;
        Check(pal4::EffectSystemAuxiliaryTreeInsert::InsertAt(
                  Runtime::kTree, Runtime::kResult,
                  Runtime::kNil, parent, Runtime::kInput,
                  runtime) == Runtime::kResult,
              "positioned insertion returns result-slot address");
        const auto inserted = runtime.ReadDword(Runtime::kResult);
        Check(runtime.ReadDword(inserted + 12u) == key &&
                  runtime.ReadDword(Runtime::kTree + 12u) ==
                      runtime.allocations,
              "node receives unsigned key and increments tree count");
        const auto root = runtime.ReadDword(Runtime::kHead + 4u);
        Check(runtime.ReadDword(root + 16u) == 1u,
              "tree root is black after each insertion");
        std::vector<std::uint32_t> ordered;
        (void)VerifyNode(runtime, root, Runtime::kHead,
                         Runtime::kNil, 16u, ordered);
        auto expected = std::vector<std::uint32_t>(
            keys.begin(), keys.begin() + runtime.allocations);
        std::sort(expected.begin(), expected.end());
        Check(ordered == expected, "in-order keys retain unsigned ordering");
        Check(runtime.ReadDword(Runtime::kHead) != Runtime::kHead &&
                  runtime.ReadDword(Runtime::kHead + 8u) != Runtime::kHead,
              "header tracks minimum and maximum nodes");
    }
    Check(runtime.allocations == keys.size() &&
              runtime.ReadDword(Runtime::kTree + 12u) == keys.size(),
          "all 20-byte nodes remain registered");
}

struct AtomicRuntime final : Runtime {
    pal4::EffectRegistrationTree Kind() const override {
        return pal4::EffectRegistrationTree::Atomics;
    }
    AtomicRuntime() {
        words[kHead + 4u] = SharedNil();
        words[SharedNil()] = SharedNil();
        words[SharedNil() + 4u] = 0u;
        words[SharedNil() + 8u] = SharedNil();
        words[SharedNil() + 20u] = 1u;
    }
};

void TestAtomicInsertion() {
    AtomicRuntime runtime;
    const std::vector<std::uint32_t> keys{9u, 3u, 15u, 1u, 6u, 12u, 18u};
    for (const auto key : keys) {
        const auto parent = ParentForKey(runtime, key, runtime.SharedNil());
        runtime.words[Runtime::kInput] = key;
        runtime.words[Runtime::kInput + 4u] = 0xa000u + key;
        Check(pal4::EffectSystemAuxiliaryTreeInsert::InsertAt(
                  Runtime::kTree, Runtime::kResult, runtime.SharedNil(),
                  parent, Runtime::kInput, runtime) == Runtime::kResult,
              "atomic tree returns result slot");
        const auto inserted = runtime.ReadDword(Runtime::kResult);
        Check(runtime.ReadDword(inserted + 12u) == key &&
                  runtime.ReadDword(inserted + 16u) == 0xa000u + key,
              "atomic node copies key and saved callback");
        const auto root = runtime.ReadDword(Runtime::kHead + 4u);
        Check(runtime.ReadDword(root + 20u) == 1u,
              "atomic tree root is black");
        std::vector<std::uint32_t> ordered;
        (void)VerifyNode(runtime, root, Runtime::kHead,
                         runtime.SharedNil(), 20u, ordered);
        auto expected = std::vector<std::uint32_t>(
            keys.begin(), keys.begin() + runtime.allocations);
        std::sort(expected.begin(), expected.end());
        Check(ordered == expected, "atomic keys stay ordered after rotation");
    }
    Check(runtime.allocations == keys.size() && runtime.NodeSize() == 24u &&
              runtime.ReadDword(Runtime::kTree + 12u) == keys.size(),
          "all 24-byte atomic nodes are counted");
}

}  // namespace

int main() {
    TestPositionedInsertion();
    TestAtomicInsertion();
}
