#include "pal4/effect_system_group_draw.h"

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

struct Runtime final : pal4::IEffectSystemGroupDrawRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kActiveTree = kSystem + 12u;
    static constexpr std::uint32_t kActiveHead = 0x2000u;
    static constexpr std::uint32_t kFirst = 0x2110u;
    static constexpr std::uint32_t kSecond = 0x2130u;
    static constexpr std::uint32_t kNil = 0x9000u;
    static constexpr std::uint32_t kXmlTree = kSystem + 228u;
    static constexpr std::uint32_t kInlineTree = kSystem + 212u;
    bool enabled = true;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t state_gets{};
    std::uint32_t state_sets{};

    Runtime() {
        words[kActiveTree + 4u] = kActiveHead;
        words[kActiveHead] = kActiveHead;
        words[kActiveHead + 4u] = kNil;
        words[kActiveHead + 8u] = kActiveHead;
        words[kSystem + 172u] = 0x7000u;
        EmptyTree(kXmlTree, 0x2300u);
        EmptyTree(kInlineTree, 0x2500u);
        for (std::uint32_t offset = 68u; offset <= 200u; offset += 12u) {
            const auto head = 0x3000u + offset;
            words[kSystem + offset + 4u] = head;
            words[head] = head;
        }
    }
    void EmptyTree(std::uint32_t tree, std::uint32_t head) {
        words[tree + 4u] = head;
        words[head] = head;
        words[head + 4u] = kNil;
        words[head + 8u] = head;
    }
    void AddTreeObjects(std::uint32_t tree, std::uint32_t head,
                        std::uint32_t first, std::uint32_t second,
                        std::uint32_t object_offset) {
        words[head] = first;
        words[head + 4u] = first;
        words[head + 8u] = second;
        words[first] = kNil;
        words[first + 4u] = head;
        words[first + 8u] = second;
        words[first + object_offset] = 0x6000u;
        words[second] = kNil;
        words[second + 4u] = first;
        words[second + 8u] = kNil;
        words[second + object_offset] = 0x6100u;
        Check(tree == kXmlTree || tree == kInlineTree,
              "draw tree fixture kind");
    }
    void ActiveTwo() {
        words[kActiveHead] = kFirst;
        words[kActiveHead + 4u] = kFirst;
        words[kActiveHead + 8u] = kSecond;
        words[kFirst] = kNil;
        words[kFirst + 4u] = kActiveHead;
        words[kFirst + 8u] = kSecond;
        words[kSecond] = kNil;
        words[kSecond + 4u] = kFirst;
        words[kSecond + 8u] = kNil;
    }
    void AddObject(std::uint32_t offset) {
        const auto head = words.at(kSystem + offset + 4u);
        const auto node = 0x4000u + offset;
        words[head] = node;
        words[node] = head;
        words[node + 8u] = 0x5000u + offset;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kSystem + 4u, "group draw enabled address");
        calls.emplace_back("enabled");
        return enabled ? 1u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto it = words.find(address);
        Check(it != words.end(), "unexpected draw tree/list read");
        return it->second;
    }
    std::uint32_t SharedNil(std::uint32_t tree) override {
        Check(tree == kActiveTree || tree == kXmlTree ||
              tree == kInlineTree, "draw tree nil source");
        return kNil;
    }
    void NoOpDraw(std::uint32_t context) override {
        Check(context == 0xaaaau, "placeholder draw context");
        calls.emplace_back("nullsub");
    }
    void DrawObject(std::uint32_t object,
                    std::uint32_t context) override {
        Check(context == 0xaaaau, "group draw context");
        calls.emplace_back("draw " + std::to_string(object));
    }
    void DrawXmlObject(std::uint32_t object,
                       std::uint32_t context) override {
        Check(context == 0xaaaau, "XML draw context");
        calls.emplace_back("xml " + std::to_string(object));
    }
    std::uint32_t GetRenderState(std::uint32_t state) override {
        ++state_gets;
        return state + 100u;
    }
    void SetRenderState(std::uint32_t,
                        std::uint32_t) override {
        ++state_sets;
    }
    void SortAtomicGeometries(std::uint32_t geometry_data) override {
        Check(geometry_data == kSystem + 52u,
              "group render-state geometry data");
        calls.emplace_back("render states");
    }
};

void TestActivePlaceholder() {
    Runtime disabled;
    disabled.enabled = false;
    pal4::EffectSystem::DrawActivePlaceholder(
        Runtime::kSystem, 0xaaaau, disabled);
    Check(disabled.calls == std::vector<std::string>{"enabled"},
          "disabled active draw skips placeholder calls");

    Runtime empty;
    pal4::EffectSystem::DrawActivePlaceholder(
        Runtime::kSystem, 0xaaaau, empty);
    Check(empty.calls == std::vector<std::string>{"enabled"},
          "empty active tree has no placeholder calls");

    Runtime populated;
    populated.ActiveTwo();
    pal4::EffectSystem::DrawActivePlaceholder(
        Runtime::kSystem, 0xaaaau, populated);
    Check(populated.calls == (std::vector<std::string>{
              "enabled", "nullsub", "nullsub"}),
          "placeholder visits both active nodes without drawing objects");
}

void TestObjectGroupDrawOrder() {
    Runtime disabled;
    disabled.enabled = false;
    pal4::EffectSystem::DrawObjectGroups(
        Runtime::kSystem, 0xaaaau, disabled);
    Check(disabled.calls == std::vector<std::string>{"enabled"},
          "disabled group draw skips lists and render states");

    Runtime empty;
    pal4::EffectSystem::DrawObjectGroups(
        Runtime::kSystem, 0xaaaau, empty);
    Check(empty.calls == (std::vector<std::string>{
              "enabled", "enabled", "render states"}) &&
          empty.state_gets == 5u && empty.state_sets == 10u,
          "empty groups still apply the central render-state change");

    constexpr std::uint32_t before[] = {
        164u, 68u, 92u, 116u, 128u, 152u, 104u, 140u};
    constexpr std::uint32_t after[] = {80u, 176u, 188u, 200u};
    Runtime populated;
    std::vector<std::string> expected{"enabled"};
    for (const auto offset : before) {
        populated.AddObject(offset);
        expected.push_back("draw " + std::to_string(0x5000u + offset));
    }
    expected.emplace_back("enabled");
    expected.emplace_back("render states");
    for (const auto offset : after) {
        populated.AddObject(offset);
        expected.push_back("draw " + std::to_string(0x5000u + offset));
    }
    pal4::EffectSystem::DrawObjectGroups(
        Runtime::kSystem, 0xaaaau, populated);
    Check(populated.calls == expected &&
          populated.state_gets == 5u && populated.state_sets == 10u,
          "all twelve groups draw in IDA order around state change");
}

void TestXmlAndInlineDraw() {
    Runtime disabled;
    disabled.enabled = false;
    pal4::EffectSystem::DrawXmlEffects(
        Runtime::kSystem, 0xaaaau, disabled);
    pal4::EffectSystem::DrawInlineEffects(
        Runtime::kSystem, 0xaaaau, disabled);
    Check(disabled.calls == (std::vector<std::string>{
              "enabled", "enabled"}),
          "disabled XML and inline draw skip both trees");

    Runtime empty;
    pal4::EffectSystem::DrawXmlEffects(
        Runtime::kSystem, 0xaaaau, empty);
    pal4::EffectSystem::DrawInlineEffects(
        Runtime::kSystem, 0xaaaau, empty);
    Check(empty.calls == (std::vector<std::string>{
              "enabled", "enabled"}),
          "empty XML and inline trees have no object calls");

    Runtime populated;
    populated.AddTreeObjects(Runtime::kXmlTree, 0x2300u,
                             0x2310u, 0x2330u, 16u);
    populated.AddTreeObjects(Runtime::kInlineTree, 0x2500u,
                             0x2510u, 0x2530u, 12u);
    pal4::EffectSystem::DrawXmlEffects(
        Runtime::kSystem, 0xaaaau, populated);
    pal4::EffectSystem::DrawInlineEffects(
        Runtime::kSystem, 0xaaaau, populated);
    Check(populated.calls == (std::vector<std::string>{
              "enabled", "xml 24576", "xml 24832", "enabled",
              "draw 24576", "draw 24832"}),
          "XML slot +16 and inline slot +76 draw both tree nodes");
}

}  // namespace

int main() {
    TestActivePlaceholder();
    TestObjectGroupDrawOrder();
    TestXmlAndInlineDraw();
}
