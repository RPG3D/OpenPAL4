#include "pal4/dialog_object.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IDialogObjectPropertyRuntime {
    std::vector<std::string> calls;
    std::uint32_t first = 0x1100u;
    std::uint32_t second = 0x2100u;
    std::map<std::uint32_t, int> comparisons{{0x1100u, 0}, {0x2100u, 0}};
    std::uint32_t FirstTreeCandidate(const pal4::DialogObjectState& state,
                                     std::string_view key) override {
        Check(state.words[36u] == 0x1000u && key == "hero",
              "first tree query");
        calls.emplace_back("first"); return first;
    }
    std::uint32_t SecondTreeCandidate(const pal4::DialogObjectState& state,
                                      std::string_view key) override {
        Check(state.words[40u] == 0x2000u && key == "hero",
              "second tree query");
        calls.emplace_back("second"); return second;
    }
    int CompareKeyToNode(std::string_view key,
                         std::uint32_t node) override {
        Check(key == "hero" && comparisons.contains(node),
              "compare query to candidate key");
        calls.emplace_back("compare:" + std::to_string(node));
        return comparisons.at(node);
    }
    std::array<float, 3u> ReadStoredVector(std::uint32_t node) override {
        Check(node == 0x2100u, "stored vector node");
        calls.emplace_back("stored"); return {1.0f, 2.0f, 3.0f};
    }
    std::uint32_t ReadFacetHandle(std::uint32_t node) override {
        Check(node == 0x1100u, "facet node");
        calls.emplace_back("facet-handle"); return 0x3000u;
    }
    void ReadFacetVector(std::uint32_t facet,
                         std::array<float, 3u>& output) override {
        Check(facet == 0x3000u, "facet vector dispatch");
        calls.emplace_back("facet-vector"); output = {4.0f, 5.0f, 6.0f};
    }
    void WriteFacetHandle(std::uint32_t node,
                          std::uint32_t facet) override {
        Check(node == 0x1100u && facet == 0x3000u, "facet update");
        calls.emplace_back("facet-update");
    }
    void InsertFacet(pal4::DialogObjectState& state, std::string_view key,
                     std::uint32_t facet) override {
        Check(state.words[36u] == 0x1000u && key == "hero" &&
              facet == 0x3000u, "facet insert");
        calls.emplace_back("facet-insert");
    }
};
struct CleanupRuntime final : pal4::IDialogObjectFacetCleanupRuntime {
    std::vector<std::string> calls;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> words;
    std::uint32_t SharedFirstTreeSentinel() override {
        calls.emplace_back("shared"); return 0x9000u;
    }
    std::uint32_t ReadNodeWord(std::uint32_t node,
                               std::uint32_t offset) override {
        calls.emplace_back("read:" + std::to_string(node) + ":" +
                           std::to_string(offset));
        return words.at({node, offset});
    }
    void WriteNodeWord(std::uint32_t node, std::uint32_t offset,
                       std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(node) + ":" +
                           std::to_string(offset));
        words[{node, offset}] = value;
    }
    void DeleteFirstTreeSubtree(std::uint32_t root) override {
        calls.emplace_back("branch:" + std::to_string(root));
    }
    void ReleaseNodeText(std::uint32_t address) override {
        calls.emplace_back("text:" + std::to_string(address));
    }
    void FreeNode(std::uint32_t node) override {
        calls.emplace_back("free:" + std::to_string(node));
    }
    std::uint32_t NextFirstTreeNode(std::uint32_t node) override {
        calls.emplace_back("next:" + std::to_string(node));
        return words.at({node, 0u});
    }
    void EraseFirstTreeNode(std::uint32_t node) override {
        calls.emplace_back("erase:" + std::to_string(node));
    }
};
}  // namespace

int main() {
    pal4::DialogObjectState state{};
    state.words[36u] = 0x1000u;
    state.words[40u] = 0x2000u;
    Runtime runtime;
    std::array<float, 3u> output{9.0f, 8.0f, 7.0f};
    Check(pal4::DialogObject::GetStoredVector(
              state, "hero", output, runtime) &&
          output == std::array<float, 3u>{1.0f, 2.0f, 3.0f} &&
          runtime.calls == std::vector<std::string>{
              "second", "compare:8448", "stored"},
          "second tree exact match reads 12-byte stored vector");
    runtime.calls.clear();
    runtime.second = 0x2000u;
    output = {9.0f, 8.0f, 7.0f};
    Check(!pal4::DialogObject::GetStoredVector(
              state, "hero", output, runtime) &&
          output == std::array<float, 3u>{9.0f, 8.0f, 7.0f} &&
          runtime.calls == std::vector<std::string>{"second"},
          "second tree sentinel misses without comparing or writing");
    runtime.second = 0x2100u;
    runtime.comparisons[0x2100u] = -1;
    runtime.calls.clear();
    Check(!pal4::DialogObject::GetStoredVector(
              state, "hero", output, runtime) &&
          output == std::array<float, 3u>{9.0f, 8.0f, 7.0f} &&
          runtime.calls == std::vector<std::string>{
              "second", "compare:8448"},
          "lower-bound candidate beyond key is rejected");
    runtime.comparisons[0x2100u] = 0;
    runtime.calls.clear();
    Check(pal4::DialogObject::GetVector(state, "hero", output, runtime) &&
          output == std::array<float, 3u>{4.0f, 5.0f, 6.0f} &&
          runtime.calls == std::vector<std::string>{
              "first", "compare:4352", "facet-handle", "facet-vector"},
          "first tree facet takes priority over stored vector");
    runtime.first = 0x1000u;
    runtime.calls.clear();
    Check(pal4::DialogObject::GetVector(state, "hero", output, runtime) &&
          output == std::array<float, 3u>{1.0f, 2.0f, 3.0f} &&
          runtime.calls == std::vector<std::string>{
              "first", "second", "compare:8448", "stored"},
          "first tree sentinel falls back to second tree");
    runtime.first = 0x1100u;
    runtime.comparisons[0x1100u] = -1;
    runtime.calls.clear();
    Check(pal4::DialogObject::GetVector(state, "hero", output, runtime) &&
          runtime.calls == std::vector<std::string>{
              "first", "compare:4352", "second", "compare:8448", "stored"},
          "first tree greater candidate also falls back");
    runtime.first = 0x1000u;
    runtime.second = 0x2000u;
    output = {9.0f, 8.0f, 7.0f};
    runtime.calls.clear();
    Check(!pal4::DialogObject::GetVector(state, "hero", output, runtime) &&
          output == std::array<float, 3u>{9.0f, 8.0f, 7.0f} &&
          runtime.calls == std::vector<std::string>{"first", "second"},
          "complete miss preserves output");
    runtime.calls.clear();
    pal4::DialogObject::SetFacet(state, "hero", 0u, runtime);
    Check(runtime.calls.empty(), "null facet has no side effects");
    runtime.first = 0x1100u;
    runtime.comparisons[0x1100u] = 0;
    pal4::DialogObject::SetFacet(state, "hero", 0x3000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "first", "compare:4352", "facet-update"},
          "existing key replaces facet handle");
    runtime.first = 0x1000u;
    runtime.calls.clear();
    pal4::DialogObject::SetFacet(state, "hero", 0x3000u, runtime);
    Check(runtime.calls == std::vector<std::string>{"first", "facet-insert"},
          "sentinel candidate inserts facet");
    runtime.first = 0x1100u;
    runtime.comparisons[0x1100u] = -1;
    runtime.calls.clear();
    pal4::DialogObject::SetFacet(state, "hero", 0x3000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "first", "compare:4352", "facet-insert"},
          "greater candidate inserts facet");

    CleanupRuntime cleanup;
    state.words[38u] = 2u;
    cleanup.words[{0x1000u, 4u}] = 0x1100u;
    cleanup.words[{0x1100u, 0u}] = 0x1200u;
    cleanup.words[{0x1100u, 8u}] = 0x3000u;
    cleanup.words[{0x1200u, 0u}] = 0x9000u;
    cleanup.words[{0x1200u, 8u}] = 0x4000u;
    pal4::DialogObject::ClearFacets(state, cleanup);
    Check(state.words[38u] == 0u &&
          cleanup.words.at({0x1000u, 4u}) == 0x9000u &&
          cleanup.words.at({0x1000u, 0u}) == 0x1000u &&
          cleanup.words.at({0x1000u, 8u}) == 0x1000u &&
          cleanup.calls == std::vector<std::string>{
              "shared", "read:4096:4", "read:4352:0", "read:4352:8",
              "branch:12288", "text:4364", "free:4352", "read:4608:0",
              "read:4608:8", "branch:16384", "text:4620", "free:4608",
              "write:4096:4", "write:4096:0", "write:4096:8"},
          "nonempty facet tree releases nodes and resets head");
    cleanup.calls.clear();
    cleanup.words[{0x1000u, 0u}] = 0x1100u;
    cleanup.words[{0x1100u, 0u}] = 0x1200u;
    cleanup.words[{0x1200u, 0u}] = 0x1000u;
    pal4::DialogObject::ClearFacets(state, cleanup);
    Check(cleanup.calls == std::vector<std::string>{
              "read:4096:0", "next:4352", "erase:4352",
              "next:4608", "erase:4608"},
          "zero-count linked fallback erases by iterator");
    cleanup.calls.clear();
    cleanup.words[{0x1000u, 0u}] = 0x1000u;
    pal4::DialogObject::ClearFacets(state, cleanup);
    Check(cleanup.calls == std::vector<std::string>{"read:4096:0"},
          "empty facet tree has no node cleanup");
}
