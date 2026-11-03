#include "pal4/effect_system_cleanup.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
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

struct Runtime final : pal4::IEffectSystemCleanupEffectsRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kFirstNode = 0x2110u;
    static constexpr std::uint32_t kSecondNode = 0x2210u;
    static constexpr std::uint32_t kXmlNode = 0x2310u;
    static constexpr std::uint32_t kTimedNode = 0x2410u;
    static constexpr std::uint32_t kInlineNode = 0x2510u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool throw_on_notify = false;

    Runtime() {
        EmptyTree(12u, 0x2100u);
        EmptyTree(28u, 0x2200u);
        EmptyTree(228u, 0x2300u);
        EmptyTree(244u, 0x2400u);
        EmptyTree(212u, 0x2500u);
        words[kSystem + 244u + 12u] = 0u;
    }
    void EmptyTree(std::uint32_t offset, std::uint32_t head) {
        words[kSystem + offset + 4u] = head;
        words[head] = head;
        words[head + 4u] = Nil(kSystem + offset);
        words[head + 8u] = head;
    }
    static std::uint32_t Nil(std::uint32_t tree) {
        return 0x9000u + tree;
    }
    void SingleNode(std::uint32_t offset, std::uint32_t node,
                    std::uint32_t object_offset,
                    std::uint32_t object) {
        const auto tree = kSystem + offset;
        const auto head = words.at(tree + 4u);
        words[head] = node;
        words[head + 4u] = node;
        words[head + 8u] = node;
        words[node] = Nil(tree);
        words[node + 4u] = head;
        words[node + 8u] = Nil(tree);
        words[node + object_offset] = object;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto it = words.find(address);
        Check(it != words.end(), "unexpected effect tree read");
        return it->second;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        const auto it = bytes.find(address);
        Check(it != bytes.end(), "unexpected inline active byte");
        calls.emplace_back(it->second ? "inline active" : "inline inactive");
        return it->second;
    }
    std::uint32_t SharedNil(std::uint32_t tree) override {
        return Nil(tree);
    }
    std::uint32_t RemoveInactiveInline(std::uint32_t tree,
                                       std::uint32_t node) override {
        Check(tree == kSystem + 212u, "inline prune tree");
        calls.emplace_back("remove inline");
        const auto next = words.at(node + 8u);
        const auto head = words.at(tree + 4u);
        if (next == Nil(tree)) {
            words[head] = head;
            words[head + 8u] = head;
            return head;
        }
        words[head] = next;
        words[head + 4u] = next;
        words[next + 4u] = head;
        return next;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        if (address == kFirstNode + 16u)
            calls.emplace_back("zero first effect");
    }
    void BeforeDestroyEffect(std::uint32_t effect) override {
        calls.emplace_back("before " + std::to_string(effect));
    }
    void DestroyEffect(std::uint32_t effect,
                       std::uint32_t flag) override {
        Check(flag == 1u, "object deleting destructor flag");
        calls.emplace_back("destroy " + std::to_string(effect));
    }
    void ResetDialog(std::uint32_t effect) override {
        calls.emplace_back("reset " + std::to_string(effect));
    }
    pal4::EffectSystemHandle CopyHandle(
        std::uint32_t id, std::uint32_t system) override {
        Check(id == 7u && system == kSystem, "cleanup handle source");
        calls.emplace_back("copy handle");
        return {id, system};
    }
    void NotifyRemoved(std::uint32_t system,
                       const pal4::EffectSystemHandle& handle,
                       std::uint32_t context) override {
        Check(system == kSystem &&
              handle == pal4::EffectSystemHandle{7u, kSystem} &&
              context == 0x5555u, "cleanup callback arguments");
        calls.emplace_back("notify");
        if (throw_on_notify) throw std::runtime_error("callback");
    }
    void ResetTemporaryHandle(pal4::EffectSystemHandle& handle) override {
        Check(handle == pal4::EffectSystemHandle{7u, kSystem},
              "cleanup temporary handle");
        calls.emplace_back("reset handle");
        handle = {};
    }
    void ReturnEffect(std::uint32_t effect,
                      std::uint32_t mode) override {
        Check(mode == 0u, "cleanup pool return mode");
        calls.emplace_back("return " + std::to_string(effect));
    }
    void ClearTree(std::uint32_t tree, std::uint32_t first,
                   std::uint32_t head) override {
        Check(head == words.at(tree + 4u) && first == words.at(head),
              "clear tree bounds");
        calls.emplace_back("clear " + std::to_string(tree - kSystem));
        words[head] = head;
        words[head + 4u] = Nil(tree);
        words[head + 8u] = head;
    }
    void DestroyTreeBranch(std::uint32_t root) override {
        calls.emplace_back("branch " + std::to_string(root));
    }
    void FreeNode(std::uint32_t node) override {
        calls.emplace_back("free " + std::to_string(node));
    }
    void EraseNode(std::uint32_t tree,
                   std::uint32_t node) override {
        calls.emplace_back("erase " + std::to_string(node));
        const auto head = words.at(tree + 4u);
        words[head] = head;
        words[head + 4u] = Nil(tree);
        words[head + 8u] = head;
    }
};

void PopulateActive(Runtime& runtime) {
    runtime.SingleNode(228u, Runtime::kXmlNode, 16u, 0x4000u);
    runtime.SingleNode(12u, Runtime::kFirstNode, 16u, 0x4100u);
    runtime.words[Runtime::kFirstNode + 12u] = 7u;
    runtime.words[Runtime::kFirstNode + 20u] = 0x5555u;
    runtime.SingleNode(28u, Runtime::kSecondNode, 12u, 0x4200u);
    runtime.SingleNode(244u, Runtime::kTimedNode, 12u, 0x4300u);
    runtime.words[Runtime::kSystem + 244u + 12u] = 1u;
    runtime.SingleNode(212u, Runtime::kInlineNode, 16u, 1u);
    runtime.bytes[Runtime::kInlineNode + 16u] = 1u;
}

void TestEmptyAndActiveCleanup() {
    Runtime empty;
    Check(pal4::EffectSystem::CleanupEffects(
              Runtime::kSystem, empty) == 0x2400u &&
          empty.calls == (std::vector<std::string>{
              "clear 228", "clear 12", "clear 28"}),
          "empty cleanup clears three trees and returns timed head");

    Runtime active;
    PopulateActive(active);
    Check(pal4::EffectSystem::CleanupEffects(
              Runtime::kSystem, active) == 0x2400u &&
          active.calls == (std::vector<std::string>{
              "before 16384", "destroy 16384", "clear 228",
              "reset 16640", "copy handle", "notify", "inline active",
              "return 16640", "zero first effect", "reset handle",
              "clear 12", "reset 16896", "inline active",
              "return 16896", "clear 28", "before 17152",
              "destroy 17152", "branch 41204", "free 9232"}) &&
          active.words.at(Runtime::kFirstNode + 16u) == 0u &&
          active.words.at(Runtime::kSystem + 244u + 12u) == 0u &&
          active.words.at(0x2400u + 4u) ==
              Runtime::Nil(Runtime::kSystem + 244u),
          "cleanup releases all four trees in native order");
}

void TestTimedFallbackAndCallbackException() {
    Runtime fallback;
    fallback.SingleNode(244u, Runtime::kTimedNode, 12u, 0x4300u);
    Check(pal4::EffectSystem::CleanupEffects(
              Runtime::kSystem, fallback) == 0x2400u &&
          fallback.calls == (std::vector<std::string>{
              "clear 228", "clear 12", "clear 28",
              "before 17152", "destroy 17152", "erase 9232"}),
          "zero-count timed tree erases nodes individually");

    Runtime thrown;
    PopulateActive(thrown);
    thrown.throw_on_notify = true;
    bool caught = false;
    try {
        (void)pal4::EffectSystem::CleanupEffects(Runtime::kSystem, thrown);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && thrown.calls == (std::vector<std::string>{
              "before 16384", "destroy 16384", "clear 228",
              "reset 16640", "copy handle", "notify", "reset handle"}),
          "callback exception releases the temporary handle");
}

void TestInlinePrune() {
    Runtime empty;
    Check(pal4::EffectSystem::ProcessEffectList(
              Runtime::kSystem, empty) == 0x2500u && empty.calls.empty(),
          "empty inline tree returns its head");

    Runtime active;
    active.SingleNode(212u, Runtime::kInlineNode, 16u, 1u);
    active.bytes[Runtime::kInlineNode + 16u] = 1u;
    Check(pal4::EffectSystem::ProcessEffectList(
              Runtime::kSystem, active) == 0x2500u &&
          active.calls == std::vector<std::string>{"inline active"},
          "active inline node advances without removal");

    Runtime inactive;
    inactive.SingleNode(212u, Runtime::kInlineNode, 16u, 0u);
    inactive.bytes[Runtime::kInlineNode + 16u] = 0u;
    Check(pal4::EffectSystem::ProcessEffectList(
              Runtime::kSystem, inactive) == 0x2500u &&
          inactive.calls == (std::vector<std::string>{
              "inline inactive", "remove inline"}) &&
          inactive.words.at(0x2500u) == 0x2500u,
          "inactive inline node is removed and traversal stops at head");

    Runtime mixed;
    constexpr auto second = 0x2530u;
    const auto tree = Runtime::kSystem + 212u;
    mixed.SingleNode(212u, Runtime::kInlineNode, 16u, 0u);
    mixed.words[Runtime::kInlineNode + 8u] = second;
    mixed.words[0x2500u + 8u] = second;
    mixed.words[second] = Runtime::Nil(tree);
    mixed.words[second + 4u] = Runtime::kInlineNode;
    mixed.words[second + 8u] = Runtime::Nil(tree);
    mixed.words[second + 16u] = 1u;
    mixed.bytes[Runtime::kInlineNode + 16u] = 0u;
    mixed.bytes[second + 16u] = 1u;
    Check(pal4::EffectSystem::ProcessEffectList(
              Runtime::kSystem, mixed) == 0x2500u &&
          mixed.calls == (std::vector<std::string>{
              "inline inactive", "remove inline", "inline active"}) &&
          mixed.words.at(0x2500u) == second,
          "prune advances to the surviving node after erasure");
}

void TestTimedLeftChain() {
    Runtime runtime;
    constexpr auto child = 0x2430u;
    const auto tree = Runtime::kSystem + 244u;
    runtime.SingleNode(244u, Runtime::kTimedNode, 12u, 0x4300u);
    runtime.words[Runtime::kTimedNode] = child;
    runtime.words[child] = Runtime::Nil(tree);
    runtime.words[child + 4u] = Runtime::kTimedNode;
    runtime.words[child + 8u] = Runtime::Nil(tree);
    runtime.words[child + 12u] = 0x4400u;
    runtime.words[0x2400u] = child;
    runtime.words[tree + 12u] = 2u;
    Check(pal4::EffectSystem::CleanupEffects(
              Runtime::kSystem, runtime) == 0x2400u &&
          runtime.calls == (std::vector<std::string>{
              "clear 228", "clear 12", "clear 28",
              "before 17408", "destroy 17408",
              "before 17152", "destroy 17152",
              "branch 41204", "free 9232",
              "branch 41204", "free 9264"}) &&
          runtime.words.at(tree + 12u) == 0u,
          "timed cleanup visits both nodes, then frees the left chain");
}

}  // namespace

int main() {
    TestEmptyAndActiveCleanup();
    TestTimedFallbackAndCallbackException();
    TestInlinePrune();
    TestTimedLeftChain();
}
