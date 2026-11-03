#include "pal4/effect_system_factory_text_tree.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemFactoryTreeRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x2004u, 0x3000u}, {0x3000u, 0x4020u},
        {0x3004u, 0x4000u}, {0x3008u, 0x4040u},
        {0x4000u, 0x4020u}, {0x4008u, 0x4040u},
        {0x4020u, 0x93cdf0u}, {0x4028u, 0x93cdf0u},
        {0x4040u, 0x93cdf0u}, {0x4048u, 0x93cdf0u}};
    std::unordered_map<std::uint32_t, std::string> names{
        {0x4000u, "M"}, {0x4020u, "F"}, {0x4040u, "T"}};
    std::uint8_t duplicate_flag = 0u;
    std::uint32_t inserted_parent = 0u;
    std::uint32_t inserted_child = 0u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x2008u, "tree duplicate-flag offset");
        return duplicate_flag;
    }
    int Compare(std::string_view input,
                std::uint32_t node) override {
        Check(names.contains(node), "comparison uses a real text-tree node");
        calls.push_back("compare");
        return input < names.at(node)
                   ? -1 : (input > names.at(node) ? 1 : 0);
    }
    std::uint32_t Previous(std::uint32_t node) override {
        Check(node == 0x4040u, "predecessor requested for rightmost node");
        calls.push_back("previous");
        return 0x4000u;
    }
    std::uint32_t InsertAt(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent,
        pal4::EffectFactoryTreeEntry entry) override {
        Check(tree == 0x2000u && child == 0x93cdf0u &&
                  entry.creator == 0x9000u,
              "insert helper receives leaf and copied creator");
        inserted_child = child;
        inserted_parent = parent;
        calls.push_back("insert");
        return 0x5000u;
    }
};

void TestUniqueKeys() {
    Runtime runtime;
    auto result = pal4::EffectSystemFactoryTextTree::SearchAndInsert(
        0x2000u, {"A", 0x9000u}, runtime);
    Check(result.node == 0x5000u && result.inserted &&
              runtime.inserted_parent == 0x4020u,
          "key below minimum inserts on leftmost branch");
    runtime.calls.clear();
    result = pal4::EffectSystemFactoryTextTree::SearchAndInsert(
        0x2000u, {"H", 0x9000u}, runtime);
    Check(result.node == 0x5000u && result.inserted &&
              runtime.inserted_parent == 0x4020u,
          "key between left and root inserts at right leaf");
    runtime.calls.clear();
    result = pal4::EffectSystemFactoryTextTree::SearchAndInsert(
        0x2000u, {"M", 0x9000u}, runtime);
    Check(result.node == 0x4000u && !result.inserted &&
              runtime.calls.back() == "compare",
          "exact duplicate returns predecessor without insertion");
    runtime.calls.clear();
    result = pal4::EffectSystemFactoryTextTree::SearchAndInsert(
        0x2000u, {"Z", 0x9000u}, runtime);
    Check(result.node == 0x5000u && result.inserted &&
              runtime.inserted_parent == 0x4040u,
          "key above maximum inserts at rightmost leaf");
}

void TestDuplicateFlag() {
    Runtime runtime;
    runtime.duplicate_flag = 1u;
    const auto result =
        pal4::EffectSystemFactoryTextTree::SearchAndInsert(
            0x2000u, {"M", 0x9000u}, runtime);
    Check(result.node == 0x5000u && result.inserted &&
              runtime.calls.back() == "insert",
          "duplicate-allowed tree inserts without predecessor check");
}

}  // namespace

int main() {
    TestUniqueKeys();
    TestDuplicateFlag();
}
