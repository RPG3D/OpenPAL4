#include "pal4/effect_object_pool.h"
#include "pal4/effect_name_count_tree.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IEffectObjectPoolReturnRuntime {
    static constexpr std::uint32_t kPool = 0x1000u;
    static constexpr std::uint32_t kHead = 0x2000u;
    static constexpr std::uint32_t kCountHead = 0x3800u;
    static constexpr std::uint32_t kNameNil = 0x93cdacu;
    static constexpr std::uint32_t kEffect = 0x6000u;
    static constexpr std::uint32_t kName = 0x7300u;
    static constexpr std::uint32_t kText = 0x8001u;
    static constexpr std::uint32_t kObjectNil = 0x93cdb0u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> texts;
    std::map<std::uint32_t, std::uint32_t> successors;
    std::vector<std::string> calls;
    std::uint32_t found = kHead;
    bool throw_on_insert = false;
    bool throw_on_query = false;
    std::vector<std::uint32_t> counter_nodes;
    std::size_t counter_index = 0u;
    std::uint32_t next_count_node = 0x5900u;

    Runtime() {
        bytes[kPool] = 1u;
        bytes[kPool + 1u] = 1u;
        bytes[kPool + 48u] = 0u;
        bytes[kText - 1u] = 1u;
        bytes[kText] = 'a';
        bytes[kText + 1u] = 'b';
        bytes[kText + 2u] = 'c';
        words[kPool + 28u] = kHead;
        words[kPool + 4u] = 0u;
        words[kPool + 12u] = 0x3000u;
        words[kPool + 20u] = 0u;
        words[0x3000u] = 0x3000u;
        words[kPool + 44u] = kCountHead;
        words[kPool + 52u] = 0u;
        words[kCountHead] = kCountHead;
        words[kCountHead + 4u] = kNameNil;
        words[kCountHead + 8u] = kCountHead;
        words[kNameNil + 32u] = 1u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        if (const auto it = texts.find(address); it != texts.end())
            texts[0x7600u] = it->second;
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t EmptyTextAddress() override { return 0x9001u; }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t flag) override {
        calls.push_back("flag " + std::to_string(record) + " " +
                        std::to_string(flag));
        if (flag == 0u || flag == 1u) {
            words[record + 4u] = 0u;
            words[record + 8u] = 0u;
            words[record + 12u] = 0u;
        }
    }
    bool AllocateText(std::uint32_t record,
                      std::uint32_t, bool) override {
        calls.push_back("allocate text");
        words[record + 4u] = 0xa001u;
        return true;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        calls.push_back("copy bytes");
        for (std::uint32_t i = 0u; i < length; ++i)
            bytes[destination + i] = bytes.at(source + i);
    }
    void DetachAndReset(std::uint32_t record) override {
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t size) override {
        words[record + 8u] = size;
    }
    pal4::EffectPoolTemporaryRecords TemporaryRecords() override {
        calls.push_back("temp records");
        return {0x7000u, 0x7100u, 0x7200u};
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        Check((destination == 0x7100u && source == 0x7000u) ||
              (destination == 0x7400u && source == kName) ||
              (destination == 0x7500u && source >= 0x4000u) ||
              (destination >= 0x590cu &&
               (source == 0x7400u || source == kName)),
              "text assignment uses event or name record");
        if (source != 0x7400u)
            calls.push_back(destination == 0x7400u ? "assign count" :
                            destination == 0x7500u ? "assign trim name" :
                            "assign");
        for (std::uint32_t offset = 4u; offset <= 12u; offset += 4u)
            words[destination + offset] = words.at(source + offset);
        if (const auto it = texts.find(source); it != texts.end())
            texts[destination] = it->second;
    }
    void InsertCached(std::uint32_t tree,
                      std::uint32_t record) override {
        Check(tree == kPool + 8u && record == 0x7100u &&
              words.at(record + 16u) == kEffect,
              "returned effect enters cache tree");
        calls.push_back("insert");
        if (throw_on_insert) throw std::runtime_error("insert failed");
    }
    std::uint32_t TemporaryNameRecord() override {
        calls.push_back("temp name");
        return kName;
    }
    void ReadName(std::uint32_t xml_node,
                  std::uint32_t record) override {
        Check(xml_node == kEffect + 20u && record == kName,
              "name comes from effect XML node");
        calls.push_back("read name");
        bytes[record] = 7u;
        words[record + 4u] = kText;
        words[record + 8u] = 3u;
        words[record + 12u] = 8u;
        texts[record] = "a";
    }
    std::uint32_t FindActive(std::uint32_t tree,
                             std::uint32_t name) override {
        Check(tree == kPool + 24u &&
              (name == kName || name == 0x7500u),
              "active lookup receives tree and name");
        calls.push_back("find active");
        return found;
    }
    std::uint32_t NextActive(std::uint32_t node) override {
        calls.push_back("next active");
        return successors.at(node);
    }
    void RemoveActive(std::uint32_t tree,
                      std::uint32_t node) override {
        Check(tree == kPool + 24u && node == 0x4040u,
              "matching active node is removed");
        calls.push_back("remove active");
    }
    void RemoveFromParentFrame(std::uint32_t effect) override {
        Check(effect == kEffect, "parent detach receives effect");
        calls.push_back("detach parent");
    }
    void DestroyEffect(std::uint32_t effect) override {
        Check(effect == kEffect, "destroy receives effect");
        calls.push_back("destroy effect");
    }
    std::uint32_t SharedNameNil() const override { return kNameNil; }
    std::int32_t CompareText(std::uint32_t first,
                             std::uint32_t second) override {
        if (first == 0x7600u && throw_on_query)
            throw std::runtime_error("query failed");
        const auto& a = texts.at(first);
        const auto& b = texts.at(second);
        return a < b ? -1 : (b < a ? 1 : 0);
    }
    std::uint32_t TemporaryCountRecord() override {
        calls.push_back("temp count");
        return 0x7400u;
    }
    std::uint32_t AllocateNameCountNode(
        std::uint32_t size) override {
        Check(size == 36u, "name-count node is 36 bytes");
        calls.push_back("insert count");
        const auto node = next_count_node;
        next_count_node += 0x40u;
        return node;
    }
    std::uint32_t SharedObjectNil() const override { return kObjectNil; }
    std::uint32_t TemporaryTrimQuery() override {
        ++counter_index;
        return 0x7600u;
    }
    std::uint32_t TemporaryTrimName() override { return 0x7500u; }
    std::uint32_t TemporaryTrimZero() override { return 0x7700u; }
    void RemoveCached(std::uint32_t tree,
                      std::uint32_t node) override {
        Check(tree == kPool + 8u, "trim removes from cache tree");
        calls.push_back("remove cached " + std::to_string(node));
    }
    void ReleaseTrimName(std::uint32_t name) override {
        Check(name == 0x7500u, "trim releases selected name");
        calls.push_back("release trim name");
    }
};

void TestInactiveAndCacheDisabled() {
    Runtime inactive;
    inactive.bytes[Runtime::kPool] = 0u;
    pal4::EffectObjectPool::ReturnEffect(
        Runtime::kPool, Runtime::kEffect, 0u, inactive);
    Check(inactive.calls.empty(), "inactive pool ignores returned effect");

    Runtime uncached;
    uncached.bytes[Runtime::kPool + 1u] = 0u;
    pal4::EffectObjectPool::ReturnEffect(
        Runtime::kPool, Runtime::kEffect, 1u, uncached);
    Check(uncached.calls == (std::vector<std::string>{
              "detach parent", "destroy effect"}),
          "cache-disabled pool destroys effect without reading its name");
}

void TestRetireMatchingAndMissingEffect() {
    Runtime matching;
    matching.found = 0x4000u;
    matching.words[0x4000u + 28u] = 0x5000u;
    matching.successors[0x4000u] = 0x4040u;
    matching.words[0x4040u + 28u] = Runtime::kEffect;
    pal4::EffectObjectPool::ReturnEffect(
        Runtime::kPool, Runtime::kEffect, 1u, matching);
    Check(matching.calls == (std::vector<std::string>{
              "temp name", "read name", "find active", "next active",
              "remove active", "detach parent", "destroy effect",
              "temp count", "assign count", "insert count",
              "flag 29696 1", "flag 29440 1"}),
          "retire mode finds effect identity before removing and destroying");

    Runtime missing;
    missing.found = 0x4000u;
    missing.words[0x4000u + 28u] = 0x5000u;
    missing.successors[0x4000u] = Runtime::kHead;
    pal4::EffectObjectPool::ReturnEffect(
        Runtime::kPool, Runtime::kEffect, 1u, missing);
    Check(missing.calls == (std::vector<std::string>{
              "temp name", "read name", "find active", "next active",
              "flag 29440 1"}),
          "retire mode leaves unmatched effects untouched");
}

void TestReturnToCacheAndFailureCleanup() {
    Runtime returned;
    pal4::EffectObjectPool::ReturnEffect(
        Runtime::kPool, Runtime::kEffect, 0u, returned);
    Check(returned.bytes.at(Runtime::kEffect + 4u) == 1u &&
          returned.bytes.at(Runtime::kEffect + 80u) == 0u &&
          returned.words.at(0x7100u + 16u) == Runtime::kEffect &&
          returned.calls == (std::vector<std::string>{
              "temp name", "read name", "detach parent", "temp records",
              "flag 28672 1", "flag 28928 0", "assign", "insert",
              "flag 28928 1", "flag 28672 1", "temp count",
              "assign count", "insert count", "flag 29696 1",
              "flag 29440 1"}),
          "return mode inserts effect before updating counts and releasing name");

    Runtime throwing;
    throwing.throw_on_insert = true;
    bool caught = false;
    try {
        pal4::EffectObjectPool::ReturnEffect(
            Runtime::kPool, Runtime::kEffect, 0u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.calls == (std::vector<std::string>{
              "temp name", "read name", "detach parent", "temp records",
              "flag 28672 1", "flag 28928 0", "assign", "insert",
              "flag 28928 1", "flag 28672 1", "flag 29440 1"}),
          "failed cache insertion releases both copies and the name");
}

void TestTrimThresholds() {
    Runtime runtime;
    pal4::EffectObjectPool::TrimCache(Runtime::kPool, runtime);
    Check(runtime.calls.empty(), "empty cache does not trim");
    runtime.words[Runtime::kPool + 20u] = 1u;
    pal4::EffectObjectPool::TrimCache(Runtime::kPool, runtime);
    Check(runtime.calls.empty(), "zero limit does not trim");
    runtime.words[Runtime::kPool + 4u] = 2u;
    pal4::EffectObjectPool::TrimCache(Runtime::kPool, runtime);
    Check(runtime.calls.empty(), "cache below limit does not trim");
    runtime.words[Runtime::kPool + 4u] = 1u;
    pal4::EffectObjectPool::TrimCache(Runtime::kPool, runtime);
    Check(runtime.calls.empty(), "empty tree does not select a node");
}

void AddCachedNode(Runtime& runtime, std::uint32_t node,
                   std::uint32_t parent, std::uint32_t right,
                   std::uint32_t count_node, std::uint32_t count) {
    runtime.words[node] = Runtime::kObjectNil;
    runtime.words[node + 4u] = parent;
    runtime.words[node + 8u] = right;
    runtime.bytes[node + 12u] = 7u;
    runtime.words[node + 16u] = Runtime::kText;
    runtime.words[node + 20u] = 0u;
    runtime.words[node + 24u] = 8u;
    runtime.words[node + 28u] = Runtime::kEffect;
    const auto key = runtime.counter_nodes.empty() ? "a" : "b";
    runtime.texts[node + 12u] = key;
    runtime.texts[count_node + 12u] = key;
    runtime.words[count_node] = Runtime::kNameNil;
    runtime.words[count_node + 8u] = Runtime::kNameNil;
    runtime.words[count_node + 32u] = 1u;
    if (runtime.counter_nodes.empty()) {
        runtime.words[Runtime::kCountHead] = count_node;
        runtime.words[Runtime::kCountHead + 4u] = count_node;
        runtime.words[Runtime::kCountHead + 8u] = count_node;
        runtime.words[count_node + 4u] = Runtime::kCountHead;
    } else {
        const auto previous = runtime.counter_nodes.back();
        runtime.words[previous + 8u] = count_node;
        runtime.words[count_node + 4u] = previous;
        runtime.words[Runtime::kCountHead + 8u] = count_node;
    }
    runtime.words[count_node + 28u] = count;
    runtime.counter_nodes.push_back(count_node);
}

void TestTrimSelectionAndActiveMatch() {
    Runtime runtime;
    constexpr std::uint32_t first = 0x4100u;
    constexpr std::uint32_t second = 0x4200u;
    runtime.words[Runtime::kPool + 20u] = 2u;
    runtime.words[Runtime::kPool + 4u] = 2u;
    runtime.words[0x3000u] = first;
    runtime.words[0x3000u + 4u] = first;
    runtime.words[0x3000u + 8u] = second;
    runtime.words[first] = Runtime::kObjectNil;
    runtime.words[first + 4u] = 0x3000u;
    runtime.words[first + 8u] = second;
    AddCachedNode(runtime, first, 0x3000u, second, 0x5100u, 9u);
    AddCachedNode(runtime, second, first, Runtime::kObjectNil,
                  0x5200u, 3u);
    runtime.found = 0x4000u;
    runtime.words[0x4000u + 8u] = 0x4040u;
    runtime.words[0x4000u + 28u] = 0x5000u;
    runtime.words[0x4040u] = Runtime::kObjectNil;
    runtime.words[0x4040u + 4u] = 0x4000u;
    runtime.words[0x4040u + 28u] = Runtime::kEffect;
    pal4::EffectObjectPool::TrimCache(Runtime::kPool, runtime);
    Check(runtime.counter_index == 2u &&
          runtime.calls.end() != std::find(runtime.calls.begin(),
              runtime.calls.end(), "remove cached 16896") &&
          runtime.calls.end() != std::find(runtime.calls.begin(),
              runtime.calls.end(), "remove active"),
          "least-used cached effect is also removed from active tree");
    const auto cached = std::find(runtime.calls.begin(), runtime.calls.end(),
                                  "remove cached 16896");
    const auto active = std::find(runtime.calls.begin(), runtime.calls.end(),
                                  "remove active");
    const auto detach = std::find(runtime.calls.begin(), runtime.calls.end(),
                                  "detach parent");
    const auto destroy = std::find(runtime.calls.begin(), runtime.calls.end(),
                                   "destroy effect");
    const auto release = std::find(runtime.calls.begin(), runtime.calls.end(),
                                   "release trim name");
    Check(cached < active && active < detach && detach < destroy &&
          destroy < release, "trim preserves removal and release order");
}

void TestTrimEarlierMinimum() {
    Runtime runtime;
    constexpr std::uint32_t first = 0x4100u;
    constexpr std::uint32_t second = 0x4200u;
    runtime.words[Runtime::kPool + 20u] = 2u;
    runtime.words[Runtime::kPool + 4u] = 2u;
    runtime.words[0x3000u] = first;
    runtime.words[0x3000u + 4u] = first;
    runtime.words[0x3000u + 8u] = second;
    AddCachedNode(runtime, first, 0x3000u, second, 0x5100u, 1u);
    AddCachedNode(runtime, second, first, Runtime::kObjectNil,
                  0x5200u, 9u);
    runtime.found = Runtime::kHead;
    pal4::EffectObjectPool::TrimCache(Runtime::kPool, runtime);
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "remove cached 16640") != runtime.calls.end() &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "remove cached 16896") == runtime.calls.end(),
          "earlier minimum stays selected after later higher count");
}

void TestTrimQueryFailure() {
    Runtime runtime;
    constexpr std::uint32_t node = 0x4100u;
    runtime.words[Runtime::kPool + 20u] = 1u;
    runtime.words[Runtime::kPool + 4u] = 1u;
    runtime.words[0x3000u] = node;
    runtime.words[0x3000u + 4u] = node;
    runtime.words[0x3000u + 8u] = node;
    AddCachedNode(runtime, node, 0x3000u, Runtime::kObjectNil,
                  0x5100u, 1u);
    runtime.throw_on_query = true;
    bool caught = false;
    try {
        pal4::EffectObjectPool::TrimCache(Runtime::kPool, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "flag 30208 1") != runtime.calls.end() &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "remove cached 16640") == runtime.calls.end(),
          "query failure releases temporary copy without evicting");
}

void TestTrimTieAndMissingActive() {
    Runtime runtime;
    constexpr std::uint32_t first = 0x4100u;
    constexpr std::uint32_t second = 0x4200u;
    runtime.words[Runtime::kPool + 20u] = 2u;
    runtime.words[Runtime::kPool + 4u] = 2u;
    runtime.words[0x3000u] = first;
    runtime.words[0x3000u + 4u] = first;
    runtime.words[0x3000u + 8u] = second;
    AddCachedNode(runtime, first, 0x3000u, second, 0x5100u, 3u);
    AddCachedNode(runtime, second, first, Runtime::kObjectNil,
                  0x5200u, 3u);
    runtime.found = Runtime::kHead;
    pal4::EffectObjectPool::TrimCache(Runtime::kPool, runtime);
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "remove cached 16896") != runtime.calls.end() &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "remove active") == runtime.calls.end() &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "destroy effect") != runtime.calls.end(),
          "tie selects later node and destroys effect without active match");
}

void TestIncrementExistingNameCount() {
    Runtime existing;
    existing.ReadName(Runtime::kEffect + 20u, Runtime::kName);
    existing.calls.clear();
    constexpr std::uint32_t node = 0x4800u;
    existing.words[Runtime::kCountHead + 4u] = node;
    existing.words[Runtime::kCountHead] = node;
    existing.words[Runtime::kCountHead + 8u] = node;
    existing.words[node] = Runtime::kNameNil;
    existing.words[node + 4u] = Runtime::kCountHead;
    existing.words[node + 8u] = Runtime::kNameNil;
    existing.words[node + 32u] = 1u;
    existing.words[node + 28u] = 5u;
    existing.texts[node + 12u] = "a";
    pal4::EffectObjectPool::IncrementNameCount(
        Runtime::kPool, Runtime::kName, existing);
    Check(existing.words.at(node + 28u) == 6u &&
          existing.calls.empty(),
          "existing name increments its count without allocating a record");

    Runtime missing;
    missing.ReadName(Runtime::kEffect + 20u, Runtime::kName);
    missing.calls.clear();
    missing.words[Runtime::kCountHead + 4u] = node;
    missing.words[Runtime::kCountHead] = node;
    missing.words[Runtime::kCountHead + 8u] = node;
    missing.words[node] = Runtime::kNameNil;
    missing.words[node + 4u] = Runtime::kCountHead;
    missing.words[node + 8u] = Runtime::kNameNil;
    missing.words[node + 32u] = 1u;
    missing.texts[node + 12u] = "z";
    pal4::EffectObjectPool::IncrementNameCount(
        Runtime::kPool, Runtime::kName, missing);
    Check(missing.calls == (std::vector<std::string>{
              "temp count", "assign count", "insert count",
              "flag 29696 1"}) &&
          missing.words.at(Runtime::kPool + 52u) == 1u &&
          missing.words.at(0x5900u + 28u) == 1u,
          "missing key before a greater node inserts a new count record");
}

void TestNameCountTree() {
    Runtime unique;
    unique.ReadName(Runtime::kEffect + 20u, Runtime::kName);
    unique.words[Runtime::kName + 16u] = 7u;
    const auto first = pal4::EffectNameCountTree::FindOrCreate(
        Runtime::kPool + 40u, Runtime::kName, unique);
    Check(first.inserted && first.node == 0x5900u &&
          unique.words.at(first.node + 28u) == 7u &&
          unique.words.at(first.node + 32u) == 1u &&
          unique.words.at(Runtime::kCountHead + 4u) == first.node &&
          unique.words.at(Runtime::kPool + 52u) == 1u,
          "empty name-count tree copies count and installs black root");
    const auto duplicate = pal4::EffectNameCountTree::FindOrCreate(
        Runtime::kPool + 40u, Runtime::kName, unique);
    Check(!duplicate.inserted && duplicate.node == first.node &&
          unique.words.at(Runtime::kPool + 52u) == 1u &&
          unique.words.at(first.node + 28u) == 7u,
          "unique mode returns existing count without replacement");

    Runtime repeated;
    repeated.ReadName(Runtime::kEffect + 20u, Runtime::kName);
    repeated.words[Runtime::kName + 16u] = 2u;
    repeated.bytes[Runtime::kPool + 48u] = 1u;
    const auto a = pal4::EffectNameCountTree::FindOrCreate(
        Runtime::kPool + 40u, Runtime::kName, repeated);
    repeated.words[Runtime::kName + 16u] = 5u;
    const auto b = pal4::EffectNameCountTree::FindOrCreate(
        Runtime::kPool + 40u, Runtime::kName, repeated);
    Check(a.inserted && b.inserted && a.node != b.node &&
          repeated.words.at(Runtime::kPool + 52u) == 2u &&
          repeated.words.at(a.node + 28u) == 2u &&
          repeated.words.at(b.node + 28u) == 5u &&
          repeated.words.at(Runtime::kNameNil + 32u) == 1u,
          "insertion mode keeps separate same-name count nodes");
}

void TestNameCountLowerBound() {
    Runtime runtime;
    runtime.ReadName(Runtime::kEffect + 20u, Runtime::kName);
    const auto tree = Runtime::kPool + 40u;
    Check(pal4::EffectNameCountTree::LowerBound(
              tree, Runtime::kName, runtime) == Runtime::kCountHead,
          "empty name-count lower bound returns header");
    constexpr std::uint32_t a = 0x5100u;
    constexpr std::uint32_t m = 0x5200u;
    constexpr std::uint32_t z = 0x5300u;
    runtime.words[Runtime::kCountHead + 4u] = m;
    runtime.words[m] = a;
    runtime.words[m + 8u] = z;
    runtime.words[a] = Runtime::kNameNil;
    runtime.words[a + 8u] = Runtime::kNameNil;
    runtime.words[z] = Runtime::kNameNil;
    runtime.words[z + 8u] = Runtime::kNameNil;
    runtime.texts[a + 12u] = "a";
    runtime.texts[m + 12u] = "m";
    runtime.texts[z + 12u] = "z";
    for (const auto& [key, expected] :
         {std::pair{"0", a}, std::pair{"a", a},
          std::pair{"b", m}, std::pair{"m", m},
          std::pair{"q", z}, std::pair{"z", z},
          std::pair{"zz", Runtime::kCountHead}}) {
        runtime.texts[Runtime::kName] = key;
        Check(pal4::EffectNameCountTree::LowerBound(
                  tree, Runtime::kName, runtime) == expected,
              "name-count lower bound handles endpoints and gaps");
    }
}

void TestFindOrCreateNameCount() {
    Runtime tree;
    tree.ReadName(Runtime::kEffect + 20u, Runtime::kName);
    tree.words[Runtime::kName + 16u] = 7u;
    const auto insert = [&](const char* name) {
        tree.texts[Runtime::kName] = name;
        return pal4::EffectObjectPool::FindOrCreateNameCount(
            Runtime::kPool + 40u, Runtime::kName, tree);
    };
    const auto middle = insert("m");
    Check(middle.inserted && middle.node == 0x5900u &&
          tree.words.at(Runtime::kCountHead + 4u) == middle.node &&
          tree.words.at(middle.node + 4u) == Runtime::kCountHead &&
          tree.words.at(middle.node + 28u) == 7u &&
          tree.words.at(middle.node + 32u) == 1u,
          "empty count tree inserts a black root with copied count");
    Check(insert("m").node == middle.node &&
          tree.words.at(Runtime::kPool + 52u) == 1u,
          "existing name returns the root without insertion");
    const auto left = insert("a");
    const auto right = insert("z");
    Check(left.inserted && right.inserted &&
          tree.words.at(middle.node) == left.node &&
          tree.words.at(middle.node + 8u) == right.node &&
          tree.words.at(Runtime::kCountHead) == left.node &&
          tree.words.at(Runtime::kCountHead + 8u) == right.node,
          "new minimum and maximum update both header extrema");
    Check(insert("a").node == left.node &&
          insert("z").node == right.node &&
          tree.words.at(Runtime::kPool + 52u) == 3u,
          "existing endpoint keys preserve the tree");
    const auto between = insert("n");
    Check(between.inserted &&
          tree.words.at(Runtime::kPool + 52u) == 4u &&
          pal4::EffectNameCountTree::LowerBound(
              Runtime::kPool + 40u, Runtime::kName, tree) == between.node &&
          tree.words.at(Runtime::kCountHead + 4u) != Runtime::kNameNil,
          "between-key insertion restores a searchable tree");

    tree.bytes[Runtime::kPool + 48u] = 1u;
    const auto duplicate = insert("z");
    Check(duplicate.inserted && duplicate.node != right.node &&
          tree.words.at(Runtime::kPool + 52u) == 5u &&
          tree.words.at(duplicate.node + 28u) == 7u &&
          tree.words.at(Runtime::kNameNil + 32u) == 1u,
          "insertion mode keeps distinct duplicate nodes");
}

void TestNameCountInsertNode() {
    Runtime tree;
    tree.ReadName(Runtime::kEffect + 20u, Runtime::kName);
    tree.words[Runtime::kName + 16u] = 3u;
    const auto address = Runtime::kPool + 40u;
    const auto root = pal4::EffectNameCountTree::InsertNode(
        address, Runtime::kNameNil, Runtime::kCountHead,
        Runtime::kName, tree);
    Check(root == 0x5900u &&
          tree.words.at(Runtime::kCountHead + 4u) == root &&
          tree.words.at(root + 28u) == 3u &&
          tree.words.at(root + 32u) == 1u,
          "direct name-count insert links and colors an empty root");
    tree.texts[Runtime::kName] = "0";
    const auto left = pal4::EffectNameCountTree::InsertNode(
        address, Runtime::kNameNil, root, Runtime::kName, tree);
    tree.texts[Runtime::kName] = "z";
    const auto right = pal4::EffectNameCountTree::InsertNode(
        address, Runtime::kNameNil, root, Runtime::kName, tree);
    Check(tree.words.at(root) == left &&
          tree.words.at(root + 8u) == right &&
          tree.words.at(Runtime::kCountHead) == left &&
          tree.words.at(Runtime::kCountHead + 8u) == right &&
          tree.words.at(Runtime::kPool + 52u) == 3u,
          "direct name-count insertion links both child directions");
}

void TestNameCountPromoteRightChild() {
    constexpr std::uint32_t node = 0x5100u;
    constexpr std::uint32_t pivot = 0x5200u;
    constexpr std::uint32_t moved = 0x5300u;
    Runtime root;
    root.words[Runtime::kCountHead + 4u] = node;
    root.words[node + 4u] = Runtime::kCountHead;
    root.words[node + 8u] = pivot;
    root.words[pivot] = moved;
    root.words[pivot + 4u] = node;
    root.words[moved + 4u] = pivot;
    Check(pal4::EffectNameCountTree::PromoteRightChild(
              Runtime::kPool + 40u, node, root) == pivot &&
          root.words.at(Runtime::kCountHead + 4u) == pivot &&
          root.words.at(pivot) == node &&
          root.words.at(node + 8u) == moved &&
          root.words.at(moved + 4u) == node &&
          root.words.at(pivot + 4u) == Runtime::kCountHead &&
          root.words.at(node + 4u) == pivot,
          "name-count right-child promotion relinks root and moved subtree");

    Runtime nested;
    constexpr std::uint32_t parent = 0x5400u;
    nested.words[Runtime::kCountHead + 4u] = parent;
    nested.words[parent] = node;
    nested.words[node + 4u] = parent;
    nested.words[node + 8u] = pivot;
    nested.words[pivot] = Runtime::kNameNil;
    nested.words[pivot + 4u] = node;
    Check(pal4::EffectNameCountTree::PromoteRightChild(
              Runtime::kPool + 40u, node, nested) == pivot &&
          nested.words.at(parent) == pivot &&
          nested.words.at(node + 8u) == Runtime::kNameNil &&
          nested.words.at(pivot + 4u) == parent,
          "name-count right-child promotion relinks non-root parent");
}

void TestNameCountPromoteLeftChild() {
    constexpr std::uint32_t node = 0x5100u;
    constexpr std::uint32_t pivot = 0x5200u;
    constexpr std::uint32_t moved = 0x5300u;
    Runtime root;
    root.words[Runtime::kCountHead + 4u] = node;
    root.words[node + 4u] = Runtime::kCountHead;
    root.words[node] = pivot;
    root.words[pivot + 8u] = moved;
    root.words[pivot + 4u] = node;
    root.words[moved + 4u] = pivot;
    Check(pal4::EffectNameCountTree::PromoteLeftChild(
              Runtime::kPool + 40u, node, root) == pivot &&
          root.words.at(Runtime::kCountHead + 4u) == pivot &&
          root.words.at(pivot + 8u) == node &&
          root.words.at(node) == moved &&
          root.words.at(moved + 4u) == node &&
          root.words.at(pivot + 4u) == Runtime::kCountHead &&
          root.words.at(node + 4u) == pivot,
          "name-count left-child promotion relinks root and moved subtree");

    Runtime nested;
    constexpr std::uint32_t parent = 0x5400u;
    nested.words[Runtime::kCountHead + 4u] = parent;
    nested.words[parent + 8u] = node;
    nested.words[node + 4u] = parent;
    nested.words[node] = pivot;
    nested.words[pivot + 8u] = Runtime::kNameNil;
    nested.words[pivot + 4u] = node;
    Check(pal4::EffectNameCountTree::PromoteLeftChild(
              Runtime::kPool + 40u, node, nested) == pivot &&
          nested.words.at(parent + 8u) == pivot &&
          nested.words.at(node) == Runtime::kNameNil &&
          nested.words.at(pivot + 4u) == parent,
          "name-count left-child promotion relinks non-root parent");
}

void TestNameCountPrevious() {
    Runtime runtime;
    runtime.ReadName(Runtime::kEffect + 20u, Runtime::kName);
    runtime.words[Runtime::kName + 16u] = 1u;
    for (const char* name : {"m", "a", "z", "b"}) {
        runtime.texts[Runtime::kName] = name;
        (void)pal4::EffectNameCountTree::FindOrCreate(
            Runtime::kPool + 40u, Runtime::kName, runtime);
    }
    runtime.words[Runtime::kCountHead + 32u] = 0u;
    auto cursor = Runtime::kCountHead;
    const auto header_result = pal4::EffectNameCountTree::Previous(
        cursor, runtime);
    Check(runtime.texts.at(cursor + 12u) == "z" &&
          header_result == cursor,
          "name-count red header moves to rightmost node");
    std::vector<std::string> names;
    while (cursor != Runtime::kCountHead) {
        names.push_back(runtime.texts.at(cursor + 12u));
        (void)pal4::EffectNameCountTree::Previous(cursor, runtime);
    }
    Check(names == (std::vector<std::string>{"z", "m", "b", "a"}),
          "name-count predecessor visits every node in reverse order");
    cursor = runtime.words.at(Runtime::kCountHead + 4u);
    Check(pal4::EffectNameCountTree::Previous(cursor, runtime) ==
              Runtime::kNameNil &&
          runtime.texts.at(cursor + 12u) == "b",
          "name-count left-subtree result differs from updated cursor");
}

void TestNameCountAdvance() {
    Runtime runtime;
    runtime.ReadName(Runtime::kEffect + 20u, Runtime::kName);
    runtime.words[Runtime::kName + 16u] = 1u;
    for (const char* name : {"m", "a", "z", "b"}) {
        runtime.texts[Runtime::kName] = name;
        (void)pal4::EffectNameCountTree::FindOrCreate(
            Runtime::kPool + 40u, Runtime::kName, runtime);
    }
    auto cursor = runtime.words.at(Runtime::kCountHead);
    std::vector<std::string> names;
    while (cursor != Runtime::kCountHead) {
        names.push_back(runtime.texts.at(cursor + 12u));
        (void)pal4::EffectNameCountTree::Advance(cursor, runtime);
    }
    Check(names == (std::vector<std::string>{"a", "b", "m", "z"}),
          "name-count successor visits nodes in order");
    cursor = runtime.words.at(Runtime::kCountHead + 4u);
    Check(pal4::EffectNameCountTree::Advance(cursor, runtime) ==
              Runtime::kNameNil &&
          runtime.texts.at(cursor + 12u) == "z",
          "name-count right-subtree result differs from updated cursor");
    Check(pal4::EffectNameCountTree::Advance(cursor, runtime) ==
              Runtime::kCountHead &&
          cursor == Runtime::kCountHead,
          "name-count final successor reaches header");
}

}  // namespace

int main() {
    TestInactiveAndCacheDisabled();
    TestRetireMatchingAndMissingEffect();
    TestReturnToCacheAndFailureCleanup();
    TestIncrementExistingNameCount();
    TestNameCountTree();
    TestNameCountLowerBound();
    TestFindOrCreateNameCount();
    TestNameCountInsertNode();
    TestNameCountPromoteRightChild();
    TestNameCountPromoteLeftChild();
    TestNameCountPrevious();
    TestNameCountAdvance();
    TestTrimThresholds();
    TestTrimSelectionAndActiveMatch();
    TestTrimEarlierMinimum();
    TestTrimQueryFailure();
    TestTrimTieAndMissingActive();
}
