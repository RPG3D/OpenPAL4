#include "pal4/script_attribute_tree_lookup.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptAttributeTreeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> visited;
    std::uint32_t last_node_data = 0u;
    std::uint32_t last_compare_length = 0u;
    bool force_node_equal = false;

    void SetText(std::uint32_t address, const char* text) {
        for (std::size_t i = 0; text[i] != '\0'; ++i)
            bytes[address + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(text[i]);
    }
    std::uint32_t NilAddress() override { return 0xdeadu; }
    std::uint32_t EmptyStringAddress() override { return 0x7000u; }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    std::int32_t CompareNodeKey(
        std::uint32_t node_key, std::uint32_t query) override {
        visited.push_back(node_key - 12u);
        if (force_node_equal) return 0;
        const auto node_data = words.at(node_key + 4u);
        const auto node_length = words.at(node_key + 8u);
        const auto query_data = words.at(query + 4u);
        const auto query_length = words.at(query + 8u);
        const auto compared = CompareBytes(
            node_data == 0u ? EmptyStringAddress() : node_data,
            query_data, std::min(node_length, query_length));
        if (compared != 0) return compared;
        return node_length < query_length ? -1 :
               node_length > query_length ? 1 : 0;
    }
    std::int32_t CompareBytes(
        std::uint32_t left, std::uint32_t right,
        std::uint32_t length) override {
        last_node_data = right;
        last_compare_length = length;
        for (std::uint32_t i = 0; i < length; ++i) {
            const auto a = bytes.at(left + i);
            const auto b = bytes.at(right + i);
            if (a != b) return a < b ? -1 : 1;
        }
        return 0;
    }
};

void SetNode(Runtime& runtime, std::uint32_t node,
             std::uint32_t left, std::uint32_t right,
             std::uint32_t data, std::uint32_t length) {
    runtime.words[node] = left;
    runtime.words[node + 8u] = right;
    runtime.words[node + 16u] = data;
    runtime.words[node + 20u] = length;
}

void TestEmptyTree() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x2004u] = runtime.NilAddress();
    std::uint32_t output = 0u;
    Check(&pal4::ScriptAttributeTreeLookup::Find(
              0x1000u, output, 0x5000u, runtime) == &output &&
              output == 0x2000u && runtime.visited.empty(),
          "empty tree returns header through output reference");
}

void TestLeftAndRightTraversal() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x2004u] = 0x3000u;
    SetNode(runtime, 0x3000u, 0x3100u, 0x3200u, 0x4000u, 1u);
    SetNode(runtime, 0x3100u, runtime.NilAddress(),
            runtime.NilAddress(), 0x4100u, 1u);
    SetNode(runtime, 0x3200u, runtime.NilAddress(),
            runtime.NilAddress(), 0x4200u, 1u);
    runtime.SetText(0x4000u, "b");
    runtime.SetText(0x4100u, "a");
    runtime.SetText(0x4200u, "c");
    runtime.words[0x5004u] = 0x6000u;
    runtime.words[0x5008u] = 1u;
    runtime.SetText(0x6000u, "a");
    std::uint32_t output = 0u;
    pal4::ScriptAttributeTreeLookup::Find(
        0x1000u, output, 0x5000u, runtime);
    Check(output == 0x3100u &&
              runtime.visited == std::vector<std::uint32_t>{
                  0x3000u, 0x3100u},
          "lower-bound traversal descends left on nonnegative comparison");

    runtime.visited.clear();
    runtime.SetText(0x6000u, "c");
    pal4::ScriptAttributeTreeLookup::Find(
        0x1000u, output, 0x5000u, runtime);
    Check(output == 0x3200u &&
              runtime.visited == std::vector<std::uint32_t>{
                  0x3000u, 0x3200u},
          "lower-bound traversal descends right on negative comparison");
}

void TestFinalComparisonAndEmptyString() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x2004u] = 0x3000u;
    SetNode(runtime, 0x3000u, runtime.NilAddress(),
            runtime.NilAddress(), 0x4000u, 4u);
    runtime.words[0x5004u] = 0x6000u;
    runtime.words[0x5008u] = 3u;
    runtime.SetText(0x4000u, "abcd");
    runtime.SetText(0x6000u, "abc");
    std::uint32_t output = 0u;
    pal4::ScriptAttributeTreeLookup::Find(
        0x1000u, output, 0x5000u, runtime);
    Check(output == 0x2000u && runtime.last_compare_length == 3u,
          "shorter query with equal prefix rejects candidate");

    runtime.words[0x3000u + 16u] = 0u;
    runtime.words[0x3000u + 20u] = 0u;
    runtime.words[0x5008u] = 0u;
    runtime.force_node_equal = true;
    pal4::ScriptAttributeTreeLookup::Find(
        0x1000u, output, 0x5000u, runtime);
    Check(output == 0x3000u && runtime.last_node_data == 0x7000u &&
              runtime.last_compare_length == 0u,
          "null node string uses global empty-string address");

    runtime.words[0x3000u + 16u] = 0x4000u;
    runtime.words[0x3000u + 20u] = 3u;
    runtime.words[0x5008u] = 4u;
    runtime.SetText(0x6000u, "abcd");
    pal4::ScriptAttributeTreeLookup::Find(
        0x1000u, output, 0x5000u, runtime);
    Check(output == 0x3000u,
          "final check accepts a longer query with equal compared prefix");
}

}  // namespace

int main() {
    TestEmptyTree();
    TestLeftAndRightTraversal();
    TestFinalComparisonAndEmptyString();
    pal4::ScriptAttributeTreeRuntimeStub stub;
    std::uint32_t output = 1u;
    Check(&pal4::ScriptAttributeTreeLookup::Find(
              0u, output, 0u, stub) == &output && output == 0u,
          "default runtime links");
    std::cout << "3 script attribute lookup cases passed\n";
}
