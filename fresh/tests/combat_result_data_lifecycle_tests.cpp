#include "pal4/combat_result_data_lifecycle.h"

#include <algorithm>
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kObject = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatResultDataLifecycleRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t nil = 0x9000u;
    std::uint32_t refs = 2u;

    Runtime() {
        words[kObject + 40u] = 0x2000u;
        words[kObject + 24u] = 0x2100u;
        words[0x2000u] = 0x2000u;
        words[0x2100u] = 0x2100u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    void CleanupRecord(std::uint32_t address) override {
        calls.emplace_back("record:" + std::to_string(address));
    }
    void FreeNodes(std::uint32_t tree, std::uint32_t root) override {
        calls.emplace_back("nodes:" + std::to_string(tree) + ":" +
                           std::to_string(root));
    }
    std::uint32_t SharedNil() override { return nil; }
    void FirstElement(std::uint32_t tree) override {
        calls.emplace_back("first:" + std::to_string(tree));
    }
    std::uint32_t Successor(std::uint32_t node) override {
        calls.emplace_back("next:" + std::to_string(node));
        return words[node + 4u];
    }
    void RemoveNode(std::uint32_t tree, std::uint32_t node) override {
        calls.emplace_back("erase:" + std::to_string(tree) + ":" +
                           std::to_string(node));
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
        return address == 0x9000u ? 0xbeefu : 0u;
    }
    std::uint32_t DecrementSharedRef() override {
        calls.emplace_back("decrement");
        return --refs;
    }
    std::uint32_t TakeSharedNil() override {
        calls.emplace_back("take-nil");
        const auto old = nil;
        nil = 0u;
        return old;
    }
};

struct ConstructionRuntime final : pal4::ICombatResultDataConstructionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> cleaned;
    std::vector<std::string> calls;
    std::uint32_t fail_allocation = 0u;
    std::uint32_t fail_write = 0u;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 24u, "tree nodes use 24-byte allocations");
        calls.emplace_back("allocate");
        if (allocations.size() + 1u == fail_allocation) return 0u;
        const auto address = 0x8000u +
                             static_cast<std::uint32_t>(allocations.size()) * 0x100u;
        allocations.push_back(address);
        return address;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        if (address == fail_write) throw std::runtime_error("record write");
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free:" + std::to_string(address));
    }
    void CleanupRecord(std::uint32_t address) override {
        cleaned.push_back(address);
        calls.emplace_back("record:" + std::to_string(address));
    }
};

void CheckTree(const ConstructionRuntime& runtime, std::uint32_t tree,
               std::uint32_t head, std::uint32_t nil, std::uint8_t seed) {
    Check(runtime.bytes.at(tree) == seed &&
              runtime.bytes.at(tree + 1u) == seed &&
              runtime.bytes.at(tree + 8u) == 0u,
          "tree byte flags initialized");
    Check(runtime.words.at(tree + 4u) == head &&
              runtime.words.at(tree + 12u) == 0u &&
              runtime.words.at(head) == head &&
              runtime.words.at(head + 4u) == nil &&
              runtime.words.at(head + 8u) == head &&
              runtime.words.at(head + 20u) == 0u,
          "tree head initialized and circular");
}

void ConstructFresh() {
    ConstructionRuntime runtime;
    Check(pal4::CombatResultDataLifecycle::Construct(kObject, 0x5au, 0x7bu,
                                                     runtime) == kObject,
          "constructor returns object");
    Check(runtime.allocations.size() == 4u &&
              runtime.freed == std::vector<std::uint32_t>{0x8200u},
          "second sentinel candidate is released");
    Check(runtime.words.at(pal4::CombatResultDataLifecycle::kSharedNilAddress) ==
              0x8000u &&
              runtime.words.at(pal4::CombatResultDataLifecycle::kSharedRefCountAddress) ==
                  2u &&
              runtime.words.at(0x8000u) == 0u &&
              runtime.words.at(0x8004u) == 0u &&
              runtime.words.at(0x8008u) == 0u &&
              runtime.words.at(0x8014u) == 1u,
          "first candidate becomes shared sentinel");
    CheckTree(runtime, kObject + 20u, 0x8100u, 0x8000u, 0x5au);
    CheckTree(runtime, kObject + 36u, 0x8300u, 0x8000u, 0x5au);
    for (std::uint32_t i = 0; i < 4u; ++i) {
        const auto record = kObject + 52u + i * 40u;
        Check(runtime.bytes.at(record) == 0x7bu &&
                  runtime.bytes.at(record + 36u) == 0x7bu,
              "record seed bytes");
        for (std::uint32_t offset = 4u; offset <= 32u; offset += 4u)
            Check(runtime.words.at(record + offset) == 0u,
                  "record eight words zeroed");
    }
    for (std::uint32_t offset = 0u; offset < 16u; offset += 4u)
        Check(runtime.words.at(kObject + offset) == 0u,
              "object result words cleared");
    Check(runtime.words.at(kObject + 16u) == 3u,
          "object default rating is three");
    const auto ref_write = std::find(runtime.calls.begin(), runtime.calls.end(),
        "word:" + std::to_string(pal4::CombatResultDataLifecycle::kSharedRefCountAddress));
    Check(ref_write != runtime.calls.end() &&
              std::find(ref_write, runtime.calls.end(), "free:33280") !=
                  runtime.calls.end(),
          "shared reference increment precedes candidate release");
}

void ConstructWithSharedSentinel() {
    ConstructionRuntime runtime;
    runtime.words[pal4::CombatResultDataLifecycle::kSharedNilAddress] = 0x9000u;
    runtime.words[pal4::CombatResultDataLifecycle::kSharedRefCountAddress] = 7u;
    (void)pal4::CombatResultDataLifecycle::Construct(kObject, 1u, 2u, runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{0x8000u, 0x8200u} &&
              runtime.words.at(pal4::CombatResultDataLifecycle::kSharedNilAddress) ==
                  0x9000u &&
              runtime.words.at(pal4::CombatResultDataLifecycle::kSharedRefCountAddress) ==
                  9u,
          "existing sentinel reused twice");
    CheckTree(runtime, kObject + 20u, 0x8100u, 0x9000u, 1u);
    CheckTree(runtime, kObject + 36u, 0x8300u, 0x9000u, 1u);
}

void ConstructionFailureCleanup() {
    for (std::uint32_t failed = 1u; failed <= 4u; ++failed) {
        ConstructionRuntime runtime;
        runtime.fail_allocation = failed;
        bool threw = false;
        try {
            (void)pal4::CombatResultDataLifecycle::Construct(
                kObject, 1u, 2u, runtime);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        Check(threw, "null allocation raises bad_alloc");
        Check(runtime.words[pal4::CombatResultDataLifecycle::kSharedNilAddress] ==
                  0u &&
                  runtime.words[pal4::CombatResultDataLifecycle::kSharedRefCountAddress] ==
                      0u,
              "failed construction releases sentinel references");
        Check(runtime.cleaned.empty(), "no records constructed before allocation failure");
        if (failed >= 3u)
            Check(runtime.words[kObject + 24u] == 0u,
                  "completed first tree head released");
        if (failed == 4u)
            Check(runtime.words[kObject + 40u] == 0u,
                  "second tree reference released without a head");
    }

    ConstructionRuntime runtime;
    runtime.fail_write = kObject + 52u + 2u * 40u + 4u;
    bool threw = false;
    try {
        (void)pal4::CombatResultDataLifecycle::Construct(kObject, 1u, 2u,
                                                         runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.cleaned ==
              std::vector<std::uint32_t>{kObject + 92u, kObject + 52u},
          "completed records unwind in reverse order");
    Check(runtime.words[pal4::CombatResultDataLifecycle::kSharedNilAddress] ==
              0u &&
              runtime.words[pal4::CombatResultDataLifecycle::kSharedRefCountAddress] ==
                  0u &&
              runtime.words[kObject + 24u] == 0u &&
              runtime.words[kObject + 40u] == 0u,
          "record failure releases both trees and shared sentinel");
}

void CheckRecordsFirst(const Runtime& runtime) {
    for (std::uint32_t i = 0; i < 4u; ++i) {
        Check(runtime.calls[i] ==
                  "record:" + std::to_string(kObject + 172u - 40u * i),
              "four records destruct in reverse order");
    }
}

void PopulatedTrees() {
    Runtime runtime;
    runtime.words[kObject + 48u] = 2u;
    runtime.words[kObject + 32u] = 3u;
    runtime.words[0x2004u] = 0x3000u;
    runtime.words[0x2104u] = 0x3100u;

    Check(pal4::CombatResultDataLifecycle::Destruct(kObject, runtime) ==
              0xbeefu,
          "returns final sentinel free result");
    CheckRecordsFirst(runtime);
    Check(runtime.calls[4] == "nodes:4132:12288" &&
              runtime.calls[5] == "write:8196",
          "first tree clears before second");
    Check(runtime.words[0x2004u] == 0x9000u &&
              runtime.words[0x2104u] == 0x9000u,
          "both roots reset to shared nil");
    Check(runtime.words[0x2000u] == 0x2000u &&
              runtime.words[0x2100u] == 0x2100u &&
              runtime.words[0x2008u] == 0x2000u &&
              runtime.words[0x2108u] == 0x2100u,
          "heads made circular");
    Check(runtime.words[kObject + 40u] == 0u &&
              runtime.words[kObject + 24u] == 0u &&
              runtime.words[kObject + 48u] == 0u &&
              runtime.words[kObject + 32u] == 0u,
          "both tree owners cleared");
    Check(runtime.calls.back() == "free:36864" && runtime.nil == 0u &&
              runtime.refs == 0u,
          "last shared nil reference released");
}

void EmptyCountWithNodes() {
    Runtime runtime;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x3004u] = 0x3001u;
    runtime.words[0x3001u + 4u] = 0x2000u;
    runtime.nil = 0u;

    Check(pal4::CombatResultDataLifecycle::Destruct(kObject, runtime) == 0u,
          "null shared nil return");
    CheckRecordsFirst(runtime);
    Check(runtime.calls[4] == "next:12288" &&
              runtime.calls[5] == "erase:4132:12288" &&
              runtime.calls[6] == "next:12289" &&
              runtime.calls[7] == "erase:4132:12289",
          "zero-count tree still erases reachable nodes");
    Check(runtime.calls.back() == "take-nil" && runtime.refs == 0u,
          "null shared nil does not call free");
}

void StandaloneTree() {
    Runtime runtime;
    runtime.words[kObject + 48u] = 1u;
    runtime.words[0x2004u] = 0x3000u;
    Check(pal4::CombatResultDataLifecycle::DestructTree(
              kObject + 36u, runtime) == 1u,
          "standalone tree returns remaining nil references");
    Check(runtime.calls.front() == "nodes:4132:12288" &&
              runtime.words[kObject + 40u] == 0u &&
              runtime.refs == 1u && runtime.nil == 0x9000u,
          "standalone tree releases only its own head and reference");
}

void CleanupTreeWithoutFirstElementRefresh() {
    Runtime runtime;
    runtime.words[kObject + 48u] = 1u;
    runtime.words[0x2004u] = 0x3000u;
    Check(pal4::CombatResultDataLifecycle::CleanupTree(
              kObject + 36u, runtime) == 1u,
          "cleanup leaves one shared nil reference");
    Check(runtime.calls.front() == "nodes:4132:12288" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "first:4132") == runtime.calls.end() &&
              runtime.words[kObject + 40u] == 0u &&
              runtime.refs == 1u,
          "cleanup omits destructor-only first-element refresh");
}

}  // namespace

int main() {
    try {
        ConstructFresh();
        ConstructWithSharedSentinel();
        ConstructionFailureCleanup();
        PopulatedTrees();
        EmptyCountWithNodes();
        StandaloneTree();
        CleanupTreeWithoutFirstElementRefresh();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
