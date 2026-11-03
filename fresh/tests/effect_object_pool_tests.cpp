#include "pal4/effect_object_pool.h"

#include <algorithm>
#include <array>
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

struct Runtime final : pal4::IEffectObjectPoolDestructorRuntime {
    static constexpr std::uint32_t kPool = 0x1000u;
    static constexpr std::uint32_t kHead = 0x2000u;
    static constexpr std::uint32_t kCachedHead = 0x3000u;
    static constexpr std::uint32_t kActiveHead = 0x3400u;
    static constexpr std::uint32_t kNil = 0x93cdacu;
    static constexpr std::uint32_t kObjectNil = 0x93cdb0u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> successors;
    std::vector<std::string> calls;
    std::uint32_t name_nil_references = 1u;
    std::uint32_t object_nil_references = 2u;
    bool name_nil_cleared = false;
    bool object_nil_cleared = false;

    Runtime() {
        bytes[kPool] = 1u;
        bytes[kPool + 1u] = 1u;
        words[kPool + 4u] = 32u;
        words[kPool + 44u] = kHead;
        words[kPool + 52u] = 0u;
        words[kHead] = kHead;
        words[kHead + 4u] = kNil;
        words[kHead + 8u] = kHead;
        words[kPool + 12u] = kCachedHead;
        words[kPool + 20u] = 0u;
        words[kPool + 28u] = kActiveHead;
        words[kPool + 36u] = 0u;
        for (const auto head : {kCachedHead, kActiveHead}) {
            words[head] = head;
            words[head + 4u] = kObjectNil;
            words[head + 8u] = head;
        }
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("byte " + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("word " + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t SharedNil() const override { return kNil; }
    std::uint32_t SharedObjectNil() const override {
        return kObjectNil;
    }
    void DestroyEffectObject(std::uint32_t pool,
                             std::uint32_t scratch,
                             std::uint32_t effect) override {
        Check(pool == kPool && scratch == 0xaaaau,
              "object destructor receives pool and scratch");
        calls.push_back("effect " + std::to_string(effect));
    }
    void DestroyObjectBranch(std::uint32_t root) override {
        calls.push_back("object branch " + std::to_string(root));
    }
    void EraseObjectNode(std::uint32_t tree,
                         std::uint32_t node) override {
        Check(tree == kPool + 8u || tree == kPool + 24u,
              "object erase receives one of the two trees");
        calls.push_back("object erase " + std::to_string(node));
    }
    void DestroySubtree(std::uint32_t node) override {
        calls.push_back("destroy " + std::to_string(node));
    }
    void ReleaseText(std::uint32_t text) override {
        calls.push_back("text " + std::to_string(text));
    }
    void FreeNode(std::uint32_t node) override {
        calls.push_back("free " + std::to_string(node));
    }
    std::uint32_t Next(std::uint32_t node) override {
        calls.push_back("next " + std::to_string(node));
        return successors.at(node);
    }
    void EraseNode(std::uint32_t tree, std::uint32_t node) override {
        Check(tree == kPool + 40u, "erase receives the pool tree");
        calls.push_back("erase " + std::to_string(node));
    }
    void RemoveNameCountRange(std::uint32_t tree, std::uint32_t first,
                              std::uint32_t last) override {
        Check(tree == kPool + 40u && last == kHead &&
              first == words.at(kHead), "name range spans its tree");
        calls.emplace_back("remove names");
    }
    void EraseEffectObjectRange(std::uint32_t tree, std::uint32_t first,
                                std::uint32_t last) override {
        Check(tree == kPool + 24u && last == kActiveHead &&
              first == words.at(kActiveHead), "effect range spans its tree");
        calls.emplace_back("remove effects");
    }
    std::uint32_t FirstElement(std::uint32_t tree) override {
        Check(tree == kPool + 8u, "first-element query uses object tree");
        calls.emplace_back("first object");
        return words.at(kCachedHead);
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
        return address + 1u;
    }
    std::uint32_t SharedNilReferences(
        pal4::EffectObjectPoolNilKind kind) override {
        return kind == pal4::EffectObjectPoolNilKind::NameCount
            ? name_nil_references : object_nil_references;
    }
    void SetSharedNilReferences(pal4::EffectObjectPoolNilKind kind,
                                std::uint32_t references) override {
        if (kind == pal4::EffectObjectPoolNilKind::NameCount)
            name_nil_references = references;
        else
            object_nil_references = references;
        calls.push_back("references " + std::to_string(references));
    }
    void ClearSharedNil(pal4::EffectObjectPoolNilKind kind) override {
        if (kind == pal4::EffectObjectPoolNilKind::NameCount)
            name_nil_cleared = true;
        else
            object_nil_cleared = true;
        calls.emplace_back("clear nil");
    }
};

void TestInactiveAndEmpty() {
    Runtime inactive;
    inactive.bytes[Runtime::kPool] = 0u;
    pal4::EffectObjectPool::Deinitialize(Runtime::kPool, 0xaaaau, inactive);
    Check(inactive.calls.empty(), "inactive pool performs no cleanup");

    Runtime empty;
    pal4::EffectObjectPool::Deinitialize(Runtime::kPool, 0xaaaau, empty);
    Check(empty.bytes.at(Runtime::kPool) == 0u &&
          empty.bytes.at(Runtime::kPool + 1u) == 0u &&
          empty.words.at(Runtime::kPool + 4u) == 0u &&
          empty.calls == (std::vector<std::string>{
              "byte 4096", "byte 4097", "word 4100"}),
          "empty pool clears fields before object cleanup");
}

void TestTreeAndFallback() {
    Runtime tree;
    constexpr std::uint32_t root = 0x4000u;
    constexpr std::uint32_t left = 0x4040u;
    tree.words[Runtime::kPool + 52u] = 3u;
    tree.words[Runtime::kHead] = left;
    tree.words[Runtime::kHead + 4u] = root;
    tree.words[Runtime::kHead + 8u] = 0x4080u;
    tree.words[root] = left;
    tree.words[root + 8u] = 0x4080u;
    tree.words[left] = Runtime::kNil;
    tree.words[left + 8u] = Runtime::kNil;
    pal4::EffectObjectPool::Deinitialize(Runtime::kPool, 0xaaaau, tree);
    Check(tree.calls == (std::vector<std::string>{
              "byte 4096", "byte 4097", "word 4100",
              "destroy 16512", "text 16396", "free 16384",
              "destroy 9686444", "text 16460", "free 16448",
              "word 8196", "word 4148", "word 8192", "word 8200"}) &&
          tree.words.at(Runtime::kHead) == Runtime::kHead &&
          tree.words.at(Runtime::kHead + 4u) == Runtime::kNil &&
          tree.words.at(Runtime::kHead + 8u) == Runtime::kHead &&
          tree.words.at(Runtime::kPool + 52u) == 0u,
          "populated pool releases right branches before left spine");

    Runtime fallback;
    fallback.words[Runtime::kHead] = 0x4000u;
    fallback.successors[0x4000u] = 0x4040u;
    fallback.successors[0x4040u] = Runtime::kHead;
    pal4::EffectObjectPool::Deinitialize(Runtime::kPool, 0xaaaau, fallback);
    Check(fallback.calls == (std::vector<std::string>{
              "byte 4096", "byte 4097", "word 4100",
              "next 16384", "erase 16384", "next 16448",
              "erase 16448"}),
          "zero-count fallback advances before erasing each node");
}

void TestResetAndEnable() {
    Runtime runtime;
    Check(pal4::EffectObjectPool::ResetAndEnable(
              Runtime::kPool, 0xaaaau, 64u, runtime) == 1u &&
          runtime.bytes.at(Runtime::kPool) == 1u &&
          runtime.bytes.at(Runtime::kPool + 1u) == 1u &&
          runtime.words.at(Runtime::kPool + 4u) == 64u &&
          runtime.calls == (std::vector<std::string>{
              "byte 4096", "byte 4097", "word 4100",
              "byte 4096", "byte 4097", "word 4100"}),
          "reset completes cleanup before enabling new parameter");
}

void TestDestruct() {
    Runtime empty;
    Check(pal4::EffectObjectPool::Destruct(
              Runtime::kPool, 0xaaaau, empty) == Runtime::kObjectNil + 1u &&
          empty.calls == (std::vector<std::string>{
              "byte 4096", "byte 4097", "word 4100",
              "remove names", "free 8192", "word 4140", "word 4148",
              "references 0", "clear nil", "free " +
                  std::to_string(Runtime::kNil),
              "remove effects", "free 13312", "word 4124", "word 4132",
              "references 1", "free 12288", "word 4108", "word 4116",
              "references 0", "clear nil", "free " +
                  std::to_string(Runtime::kObjectNil)}) &&
          empty.name_nil_cleared && empty.object_nil_cleared &&
          empty.words.at(Runtime::kPool + 12u) == 0u &&
          empty.words.at(Runtime::kPool + 28u) == 0u &&
          empty.words.at(Runtime::kPool + 44u) == 0u,
          "pool destructor frees all three heads and both shared nils");

    Runtime populated;
    populated.bytes[Runtime::kPool] = 0u;
    populated.words[Runtime::kPool + 20u] = 1u;
    populated.words[Runtime::kCachedHead] = 0x4000u;
    populated.words[Runtime::kCachedHead + 4u] = 0x4000u;
    populated.words[Runtime::kCachedHead + 8u] = 0x4000u;
    populated.object_nil_references = 3u;
    Check(pal4::EffectObjectPool::Destruct(
              Runtime::kPool, 0xaaaau, populated) == 1u &&
          populated.calls.end() != std::find(populated.calls.begin(),
              populated.calls.end(), "object branch 16384") &&
          populated.words.at(Runtime::kCachedHead + 4u) ==
              Runtime::kObjectNil &&
          populated.object_nil_references == 1u &&
          !populated.object_nil_cleared,
          "populated object tree resets head and retains shared nil");

    Runtime fallback;
    fallback.bytes[Runtime::kPool] = 0u;
    fallback.words[Runtime::kCachedHead] = 0x4000u;
    fallback.words[0x4000u + 4u] = Runtime::kCachedHead;
    fallback.words[0x4000u + 8u] = Runtime::kObjectNil;
    Check(pal4::EffectObjectPool::Destruct(
              Runtime::kPool, 0xaaaau, fallback) ==
              Runtime::kObjectNil + 1u &&
          std::find(fallback.calls.begin(), fallback.calls.end(),
                    "object erase 16384") != fallback.calls.end(),
          "zero-count object fallback advances before erasure");
}

void TestCleanupAllObjects() {
    Runtime guarded;
    pal4::EffectObjectPool::CleanupAllObjects(
        Runtime::kPool, 0xaaaau, guarded);
    Check(guarded.calls.empty(),
          "enabled cache defers object cleanup");

    Runtime populated;
    populated.bytes[Runtime::kPool + 1u] = 0u;
    constexpr std::uint32_t left = 0x5040u;
    constexpr std::uint32_t root = 0x5000u;
    constexpr std::uint32_t right = 0x5080u;
    populated.words[Runtime::kCachedHead] = 0x4000u;
    populated.words[Runtime::kCachedHead + 4u] = 0x4000u;
    populated.words[Runtime::kCachedHead + 8u] = 0x4000u;
    populated.words[Runtime::kPool + 20u] = 1u;
    populated.words[Runtime::kActiveHead] = left;
    populated.words[Runtime::kActiveHead + 4u] = root;
    populated.words[Runtime::kActiveHead + 8u] = right;
    populated.words[Runtime::kPool + 36u] = 3u;
    populated.words[root] = left;
    populated.words[root + 4u] = Runtime::kActiveHead;
    populated.words[root + 8u] = right;
    populated.words[root + 28u] = 0x6000u;
    populated.words[left] = Runtime::kObjectNil;
    populated.words[left + 4u] = root;
    populated.words[left + 8u] = Runtime::kObjectNil;
    populated.words[left + 28u] = 0x6040u;
    populated.words[right] = Runtime::kObjectNil;
    populated.words[right + 4u] = root;
    populated.words[right + 8u] = Runtime::kObjectNil;
    populated.words[right + 28u] = 0x6080u;
    pal4::EffectObjectPool::CleanupAllObjects(
        Runtime::kPool, 0xaaaau, populated);
    Check(populated.calls == (std::vector<std::string>{
              "effect 24640", "effect 24576", "effect 24704",
              "object branch 16384", "word 12292", "word 4116",
              "word 12288", "word 12296", "object branch 20480",
              "word 13316", "word 4132", "word 13312", "word 13320"}) &&
          populated.words.at(Runtime::kCachedHead) == Runtime::kCachedHead &&
          populated.words.at(Runtime::kCachedHead + 4u) ==
              Runtime::kObjectNil &&
          populated.words.at(Runtime::kActiveHead) == Runtime::kActiveHead &&
          populated.words.at(Runtime::kActiveHead + 4u) ==
              Runtime::kObjectNil &&
          populated.words.at(Runtime::kPool + 20u) == 0u &&
          populated.words.at(Runtime::kPool + 36u) == 0u,
          "active objects are destroyed in order before both trees reset");

    Runtime fallback;
    fallback.bytes[Runtime::kPool + 1u] = 0u;
    fallback.words[Runtime::kCachedHead] = 0x4000u;
    fallback.words[Runtime::kCachedHead + 4u] = 0x4000u;
    fallback.words[Runtime::kCachedHead + 8u] = 0x4000u;
    fallback.words[0x4000u + 4u] = Runtime::kCachedHead;
    fallback.words[0x4000u + 8u] = Runtime::kObjectNil;
    fallback.words[Runtime::kActiveHead] = 0x5000u;
    fallback.words[Runtime::kActiveHead + 4u] = 0x5000u;
    fallback.words[Runtime::kActiveHead + 8u] = 0x5000u;
    fallback.words[0x5000u + 4u] = Runtime::kActiveHead;
    fallback.words[0x5000u + 8u] = Runtime::kObjectNil;
    fallback.words[0x5000u + 28u] = 0x6000u;
    pal4::EffectObjectPool::CleanupAllObjects(
        Runtime::kPool, 0xaaaau, fallback);
    Check(fallback.calls == (std::vector<std::string>{
              "effect 24576", "object erase 16384",
              "object erase 20480"}),
          "zero-count trees erase each node after successor lookup");
}

struct AcquireRuntime final : pal4::IEffectObjectPoolAcquireRuntime {
    static constexpr std::uint32_t kPool = 0x1000u;
    static constexpr std::uint32_t kHead = 0x2000u;
    static constexpr std::uint32_t kKey = 0x3000u;
    static constexpr std::uint32_t kNode = 0x4000u;
    static constexpr std::uint32_t kEffect = 0x6000u;
    static constexpr std::uint32_t kText = 0x8001u;
    static constexpr std::uint32_t kEmpty = 0x9001u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t found = kHead;
    std::uint32_t loaded = kEffect;
    bool throw_on_insert = false;

    AcquireRuntime() {
        bytes[kPool] = 1u;
        bytes[kPool + 1u] = 1u;
        bytes[kKey] = 7u;
        bytes[kText - 1u] = 1u;
        bytes[kText] = 'a';
        bytes[kText + 1u] = 'b';
        bytes[kText + 2u] = 'c';
        words[kPool + 12u] = kHead;
        words[kKey + 4u] = kText;
        words[kKey + 8u] = 3u;
        words[kKey + 12u] = 8u;
        words[kNode + 28u] = kEffect;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
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
    std::uint32_t EmptyTextAddress() override { return kEmpty; }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t flag) override {
        calls.push_back("flag " + std::to_string(record) + " " +
                        std::to_string(flag));
        if (flag == 0u) {
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
        calls.push_back("detach");
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t size) override {
        words[record + 8u] = size;
    }
    std::uint32_t FindCached(std::uint32_t tree,
                             std::uint32_t key) override {
        Check(tree == kPool + 8u && key == kKey,
              "cache lookup receives native tree and key");
        calls.push_back("find");
        return found;
    }
    void RemoveCached(std::uint32_t tree,
                      std::uint32_t node) override {
        Check(tree == kPool + 8u && node == kNode,
              "cache removal receives found node");
        calls.push_back("remove");
    }
    std::uint32_t InitializeEffectSystem() override {
        calls.push_back("initialize system");
        return 0x5000u;
    }
    std::uint32_t LoadEffectFromFile(std::uint32_t manager,
                                     std::uint32_t text) override {
        Check(manager == 0x5044u && (text == kText || text == kEmpty),
              "loader receives manager at +68 and resolved text");
        calls.push_back("load");
        return loaded;
    }
    pal4::EffectPoolTemporaryRecords TemporaryRecords() override {
        calls.push_back("temporaries");
        return {0x7000u, 0x7100u, 0x7200u};
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        Check(destination == 0x7100u && source == 0x7000u,
              "temporary key copies event text");
        calls.push_back("assign text");
        for (std::uint32_t offset = 4u; offset <= 12u; offset += 4u)
            words[destination + offset] = words.at(source + offset);
    }
    void InsertCached(std::uint32_t tree,
                      std::uint32_t record) override {
        Check(tree == kPool + 24u && record == 0x7100u &&
              words.at(record + 16u) == loaded,
              "insertion receives text and loaded effect");
        calls.push_back("insert");
        if (throw_on_insert) throw std::runtime_error("insert failed");
    }
    void SetRotation(std::uint32_t effect,
                     const std::array<float, 3u>& value) override {
        Check(effect == kEffect &&
              value == std::array<float, 3u>{0.0f, 0.0f, 0.0f},
              "default rotation");
        calls.push_back("rotation");
    }
    void SetScale(std::uint32_t effect,
                  const std::array<float, 3u>& value) override {
        Check(effect == kEffect &&
              value == std::array<float, 3u>{1.0f, 1.0f, 1.0f},
              "default scale");
        calls.push_back("scale");
    }
    void SetPosition(std::uint32_t effect,
                     const std::array<float, 3u>& value) override {
        Check(effect == kEffect &&
              value == std::array<float, 3u>{0.0f, 1.0f, 1.0f},
              "default position");
        calls.push_back("position");
    }
    void ClearFacets(std::uint32_t effect) override {
        Check(effect == kEffect, "facet cleanup receives effect");
        calls.push_back("facets");
    }
    void CleanupXmlNode(std::uint32_t node) override {
        Check(node == kEffect + 20u, "XML node is effect +20");
        calls.push_back("xml");
    }
    void RemoveFromParentFrame(std::uint32_t effect) override {
        Check(effect == kEffect, "frame cleanup receives effect");
        calls.push_back("parent frame");
    }
    void ResetVisibility(std::uint32_t effect) override {
        Check(effect == kEffect, "visibility reset receives effect");
        calls.push_back("visibility");
    }
};

void TestAcquireDisabledAndUncached() {
    AcquireRuntime disabled;
    disabled.bytes[AcquireRuntime::kPool] = 0u;
    Check(pal4::EffectObjectPool::AcquireEffect(
              AcquireRuntime::kPool, AcquireRuntime::kKey, disabled) == 0u &&
          disabled.calls.empty(), "disabled pool never loads effects");

    AcquireRuntime direct;
    direct.bytes[AcquireRuntime::kPool + 1u] = 0u;
    direct.words[AcquireRuntime::kKey + 4u] = 0u;
    Check(pal4::EffectObjectPool::AcquireEffect(
              AcquireRuntime::kPool, AcquireRuntime::kKey, direct) ==
              AcquireRuntime::kEffect &&
          direct.calls == (std::vector<std::string>{
              "initialize system", "load", "rotation", "scale",
              "position", "facets", "xml", "parent frame",
              "visibility"}),
          "uncached effect uses empty-text fallback and prepares result");
}

void TestAcquireCacheHitAndLoadFailure() {
    AcquireRuntime hit;
    hit.found = AcquireRuntime::kNode;
    Check(pal4::EffectObjectPool::AcquireEffect(
              AcquireRuntime::kPool, AcquireRuntime::kKey, hit) ==
              AcquireRuntime::kEffect &&
          hit.calls == (std::vector<std::string>{
              "find", "remove", "rotation", "scale", "position",
              "facets", "xml", "parent frame", "visibility"}),
          "cache hit removes node before preparing effect");

    AcquireRuntime empty_hit;
    empty_hit.found = AcquireRuntime::kNode;
    empty_hit.words[AcquireRuntime::kNode + 28u] = 0u;
    Check(pal4::EffectObjectPool::AcquireEffect(
              AcquireRuntime::kPool, AcquireRuntime::kKey,
              empty_hit) == 0u &&
          empty_hit.calls == (std::vector<std::string>{"find", "remove"}),
          "empty cached effect returns after removing node");

    AcquireRuntime failed;
    failed.loaded = 0u;
    Check(pal4::EffectObjectPool::AcquireEffect(
              AcquireRuntime::kPool, AcquireRuntime::kKey, failed) == 0u &&
          failed.calls == (std::vector<std::string>{
              "find", "initialize system", "load"}),
          "load failure skips cache insertion and effect preparation");
}

void TestAcquireCacheMissAndException() {
    AcquireRuntime miss;
    Check(pal4::EffectObjectPool::AcquireEffect(
              AcquireRuntime::kPool, AcquireRuntime::kKey, miss) ==
              AcquireRuntime::kEffect &&
          miss.words.at(0x7000u + 16u) == AcquireRuntime::kEffect &&
          miss.words.at(0x7100u + 16u) == AcquireRuntime::kEffect &&
          miss.bytes.at(AcquireRuntime::kText - 1u) == 2u &&
          miss.calls == (std::vector<std::string>{
              "find", "initialize system", "load", "temporaries",
              "flag 28672 1", "flag 28928 0", "assign text", "insert",
              "flag 28928 1", "flag 28672 1", "rotation", "scale",
              "position", "facets", "xml", "parent frame",
              "visibility"}),
          "cache miss copies event data, inserts and releases temporaries");

    AcquireRuntime saturated;
    saturated.bytes[AcquireRuntime::kText - 1u] = 0xfeu;
    Check(pal4::EffectObjectPool::AcquireEffect(
              AcquireRuntime::kPool, AcquireRuntime::kKey, saturated) ==
              AcquireRuntime::kEffect &&
          saturated.bytes.at(0xa001u) == 'a' &&
          saturated.bytes.at(0xa003u) == 'c' &&
          saturated.bytes.at(0xa004u) == 0u &&
          saturated.bytes.at(AcquireRuntime::kText - 1u) == 0xfeu &&
          saturated.calls[4] == "allocate text" &&
          saturated.calls[5] == "copy bytes",
          "saturated cache key receives an owned text copy");

    AcquireRuntime throwing;
    throwing.throw_on_insert = true;
    bool caught = false;
    try {
        (void)pal4::EffectObjectPool::AcquireEffect(
            AcquireRuntime::kPool, AcquireRuntime::kKey, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.calls == (std::vector<std::string>{
              "find", "initialize system", "load", "temporaries",
              "flag 28672 1", "flag 28928 0", "assign text", "insert",
              "flag 28928 1", "flag 28672 1"}),
          "failed insertion releases both temporaries without preparing effect");
}

}  // namespace

int main() {
    TestInactiveAndEmpty();
    TestTreeAndFallback();
    TestResetAndEnable();
    TestDestruct();
    TestCleanupAllObjects();
    TestAcquireDisabledAndUncached();
    TestAcquireCacheHitAndLoadFailure();
    TestAcquireCacheMissAndException();
}
