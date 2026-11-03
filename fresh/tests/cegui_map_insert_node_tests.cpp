#include "pal4/cegui_map_operations.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <vector>

namespace {

constexpr std::uint32_t kMap = 0x1000u;
constexpr std::uint32_t kHeader = 0x2000u;
constexpr std::uint32_t kNil = 0x9000u;

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ICeguiMapInsertRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> allocated;

    Runtime() {
        words[kMap + 4u] = kHeader;
        words[kMap + 0x0cu] = 0u;
        words[kHeader] = kHeader;
        words[kHeader + 4u] = kNil;
        words[kHeader + 8u] = kHeader;
        words[kNil] = kNil;
        words[kNil + 4u] = kNil;
        words[kNil + 8u] = kNil;
        words[kNil + 0x14u] = 1u;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 0x18u, "map insertion allocates 24-byte node");
        const auto node = 0x3000u +
                          static_cast<std::uint32_t>(allocated.size()) * 0x100u;
        allocated.push_back(node);
        return node;
    }
    std::uint32_t ReadDword(const std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        words[address] = value;
    }

    std::uint32_t Insert(const std::uint32_t key,
                         const std::uint32_t value) {
        auto parent = kHeader;
        auto child = ReadDword(kHeader + 4u);
        while (child != kNil) {
            parent = child;
            child = ReadDword(child +
                              (key < ReadDword(child + 0x0cu) ? 0u : 8u));
        }
        const pal4::CeguiTreeGlobals globals{1u, kNil};
        return pal4::CeguiMapOperations::InsertNode(
            kMap, child, parent, {key, value}, globals, *this);
    }
};

std::uint32_t VerifySubtree(Runtime& runtime, const std::uint32_t node,
                            const std::uint32_t parent,
                            std::set<std::uint32_t>& seen,
                            std::vector<std::uint32_t>& keys) {
    if (node == kNil) return 1u;
    Check(seen.insert(node).second, "inserted tree has no cycles");
    Check(runtime.ReadDword(node + 4u) == parent,
          "inserted node has correct parent");
    const auto left = runtime.ReadDword(node);
    const auto right = runtime.ReadDword(node + 8u);
    const auto color = runtime.ReadDword(node + 0x14u);
    if (color == 0u) {
        Check(runtime.ReadDword(left + 0x14u) == 1u &&
                  runtime.ReadDword(right + 0x14u) == 1u,
              "red insertion node has black children");
    } else {
        Check(color == 1u, "inserted node color is binary");
    }
    const auto left_height = VerifySubtree(runtime, left, node, seen, keys);
    keys.push_back(runtime.ReadDword(node + 0x0cu));
    const auto right_height = VerifySubtree(runtime, right, node, seen, keys);
    Check(left_height == right_height, "insertion preserves black height");
    return left_height + (color == 1u ? 1u : 0u);
}

void VerifyTree(Runtime& runtime, std::vector<std::uint32_t> expected) {
    const auto root = runtime.ReadDword(kHeader + 4u);
    Check(root != kNil && runtime.ReadDword(root + 0x14u) == 1u,
          "inserted tree has a black root");
    std::set<std::uint32_t> seen;
    std::vector<std::uint32_t> keys;
    static_cast<void>(VerifySubtree(runtime, root, kHeader, seen, keys));
    std::sort(expected.begin(), expected.end());
    Check(keys == expected && seen.size() == expected.size() &&
              runtime.ReadDword(kMap + 0x0cu) == expected.size(),
          "insertion retains all sorted key records and count");
    const auto min_node = *std::min_element(
        seen.begin(), seen.end(), [&](const auto a, const auto b) {
            return runtime.ReadDword(a + 0x0cu) <
                   runtime.ReadDword(b + 0x0cu);
        });
    const auto max_node = *std::max_element(
        seen.begin(), seen.end(), [&](const auto a, const auto b) {
            return runtime.ReadDword(a + 0x0cu) <
                   runtime.ReadDword(b + 0x0cu);
        });
    Check(runtime.ReadDword(kHeader) == min_node &&
              runtime.ReadDword(kHeader + 8u) == max_node,
          "insertion updates header extrema");
}

void InsertSequence(const std::vector<std::uint32_t>& sequence,
                    const std::uint32_t expected_root) {
    Runtime runtime;
    std::vector<std::uint32_t> inserted;
    for (const auto key : sequence) {
        const auto node = runtime.Insert(key, key + 100u);
        Check(runtime.ReadDword(node + 0x0cu) == key &&
                  runtime.ReadDword(node + 0x10u) == key + 100u &&
                  runtime.ReadDword(node) != 0u &&
                  runtime.ReadDword(node + 8u) != 0u,
              "new node stores both mapping words and nil child links");
        inserted.push_back(key);
        VerifyTree(runtime, inserted);
    }
    Check(runtime.ReadDword(runtime.ReadDword(kHeader + 4u) + 0x0cu) ==
              expected_root,
          "rotation sequence produces expected root key");
}

void ForcedLeftBranchUsesChildArgument() {
    Runtime runtime;
    const auto root = runtime.Insert(20u, 120u);
    const pal4::CeguiTreeGlobals globals{1u, kNil};
    const auto node = pal4::CeguiMapOperations::InsertNode(
        kMap, 0xdeadbeefu, root, {30u, 130u}, globals, runtime);
    Check(runtime.ReadDword(root) == node &&
              runtime.ReadDword(root + 8u) == kNil &&
              runtime.ReadDword(node + 4u) == root &&
              runtime.ReadDword(kMap + 0x0cu) == 2u,
          "non-nil child argument forces left link regardless of key");
}

}  // namespace

int main() {
    InsertSequence({10u}, 10u);
    InsertSequence({10u, 5u, 15u}, 10u);
    InsertSequence({10u, 5u, 1u}, 5u);
    InsertSequence({10u, 1u, 5u}, 5u);
    InsertSequence({10u, 15u, 20u}, 15u);
    InsertSequence({10u, 20u, 15u}, 15u);
    InsertSequence({10u, 5u, 15u, 1u}, 10u);
    ForcedLeftBranchUsesChildArgument();
    std::cout << "8 CEGUI map insert-node test cases passed\n";
}
