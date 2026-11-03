#include "pal4/effect_system_tree_update.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, std::string_view message) {
    if (condition) return;
    std::cerr << message << '\n';
    std::exit(1);
}

struct Runtime final : pal4::IEffectSystemTreeUpdateRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kXmlTree = kSystem + 228u;
    static constexpr std::uint32_t kInlineTree = kSystem + 212u;
    static constexpr std::uint32_t kXmlHead = 0x2300u;
    static constexpr std::uint32_t kXmlFirst = 0x2310u;
    static constexpr std::uint32_t kXmlSecond = 0x2330u;
    static constexpr std::uint32_t kInlineHead = 0x2500u;
    static constexpr std::uint32_t kInlineFirst = 0x2510u;
    static constexpr std::uint32_t kInlineSecond = 0x2530u;
    bool enabled = true;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    Runtime() {
        EmptyTree(kXmlTree, kXmlHead);
        EmptyTree(kInlineTree, kInlineHead);
    }
    static std::uint32_t Nil(std::uint32_t tree) {
        return 0x9000u + tree;
    }
    void EmptyTree(std::uint32_t tree, std::uint32_t head) {
        words[tree + 4u] = head;
        words[head] = head;
        words[head + 4u] = Nil(tree);
        words[head + 8u] = head;
    }
    void TwoNodes(std::uint32_t tree, std::uint32_t head,
                  std::uint32_t first, std::uint32_t second) {
        words[head] = first;
        words[head + 4u] = first;
        words[head + 8u] = second;
        words[first] = Nil(tree);
        words[first + 4u] = head;
        words[first + 8u] = second;
        words[second] = Nil(tree);
        words[second + 4u] = first;
        words[second + 8u] = Nil(tree);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        if (address == kSystem + 4u) {
            calls.emplace_back("enabled");
            return enabled ? 1u : 0u;
        }
        const auto it = bytes.find(address);
        Check(it != bytes.end(), "unexpected inline flag address");
        calls.emplace_back(it->second ? "active" : "inactive");
        return it->second;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto it = words.find(address);
        Check(it != words.end(), "unexpected effect tree address");
        if (address == kInlineTree + 4u)
            calls.emplace_back("prune");
        return it->second;
    }
    std::uint32_t SharedNil(std::uint32_t tree) override {
        Check(tree == kXmlTree || tree == kInlineTree,
              "effect tree nil source");
        return Nil(tree);
    }
    std::uint32_t RemoveInactiveInline(std::uint32_t tree,
                                       std::uint32_t node) override {
        Check(tree == kInlineTree && node == kInlineSecond,
              "inactive inline node removal");
        calls.emplace_back("remove");
        words[kInlineHead + 8u] = kInlineFirst;
        words[kInlineFirst + 8u] = Nil(tree);
        return kInlineHead;
    }
    void UpdateXmlObject(std::uint32_t object,
                         std::uint32_t first,
                         std::uint32_t second) override {
        Check(first == 0xaaaau && second == 0xbbbbu,
              "XML update arguments");
        calls.emplace_back("xml " + std::to_string(object));
    }
    void UpdateInlineObject(std::uint32_t object,
                            std::uint32_t first,
                            std::uint32_t second) override {
        Check(first == 0xaaaau && second == 0xbbbbu,
              "inline update arguments");
        calls.emplace_back("inline " + std::to_string(object));
    }
};

void TestXmlTraversal() {
    Runtime disabled;
    disabled.enabled = false;
    pal4::EffectSystem::UpdateXmlEffects(
        Runtime::kSystem, 0xaaaau, 0xbbbbu, disabled);
    Check(disabled.calls == std::vector<std::string>{"enabled"},
          "disabled XML effects do not traverse tree");

    Runtime empty;
    pal4::EffectSystem::UpdateXmlEffects(
        Runtime::kSystem, 0xaaaau, 0xbbbbu, empty);
    Check(empty.calls == std::vector<std::string>{"enabled"},
          "empty XML tree has no object callbacks");

    Runtime populated;
    populated.TwoNodes(Runtime::kXmlTree, Runtime::kXmlHead,
                       Runtime::kXmlFirst, Runtime::kXmlSecond);
    populated.words[Runtime::kXmlFirst + 16u] = 0x4000u;
    populated.words[Runtime::kXmlSecond + 16u] = 0x4100u;
    pal4::EffectSystem::UpdateXmlEffects(
        Runtime::kSystem, 0xaaaau, 0xbbbbu, populated);
    Check(populated.calls == (std::vector<std::string>{
              "enabled", "xml 16384", "xml 16640"}),
          "XML effect tree updates both objects in key order");
}

void TestInlineTraversalAndPrune() {
    Runtime disabled;
    disabled.enabled = false;
    pal4::EffectSystem::UpdateInlineEffects(
        Runtime::kSystem, 0xaaaau, 0xbbbbu, disabled);
    Check(disabled.calls == std::vector<std::string>{"enabled"},
          "disabled inline effects do not prune");

    Runtime empty;
    pal4::EffectSystem::UpdateInlineEffects(
        Runtime::kSystem, 0xaaaau, 0xbbbbu, empty);
    Check(empty.calls == (std::vector<std::string>{
              "enabled", "prune", "prune"}),
          "empty inline tree still invokes the final prune pass");

    Runtime mixed;
    mixed.TwoNodes(Runtime::kInlineTree, Runtime::kInlineHead,
                   Runtime::kInlineFirst, Runtime::kInlineSecond);
    mixed.words[Runtime::kInlineFirst + 12u] = 0x4200u;
    mixed.words[Runtime::kInlineSecond + 12u] = 0x4300u;
    mixed.bytes[Runtime::kInlineFirst + 16u] = 1u;
    mixed.bytes[Runtime::kInlineSecond + 16u] = 0u;
    pal4::EffectSystem::UpdateInlineEffects(
        Runtime::kSystem, 0xaaaau, 0xbbbbu, mixed);
    Check(mixed.calls == (std::vector<std::string>{
              "enabled", "prune", "active", "inline 16896",
              "inactive", "prune", "active", "inactive", "remove"}),
          "inline update skips inactive object then prunes its node");
}

}  // namespace

int main() {
    TestXmlTraversal();
    TestInlineTraversalAndPrune();
}
