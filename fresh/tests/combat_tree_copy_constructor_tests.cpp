#include "pal4/combat_tree_copy_constructor.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kSource = 0x1000u;
constexpr std::uint32_t kDestination = 0x2000u;
constexpr std::uint32_t kNil =
    pal4::CombatTreeCopyConstructor::kSharedNilAddress;
constexpr std::uint32_t kRef =
    pal4::CombatTreeCopyConstructor::kSharedRefCountAddress;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatTreeCopyRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;
    std::vector<std::string> calls;
    std::uint32_t allocations = 0u;
    std::uint32_t fail_allocation = 0u;

    Runtime() {
        bytes[kSource] = 0x11u;
        bytes[kSource + 1u] = 0x22u;
        bytes[kSource + 8u] = 0x33u;
        words[kSource + 4u] = 0x7000u;
    }
    std::uint32_t Nil() override { return words[kNil]; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free:" + std::to_string(address));
    }
    std::uint32_t CreateNode(std::uint32_t,
                             std::uint32_t) override {
        throw std::runtime_error("CopyTree should own node creation");
    }
    void CopyData(std::uint32_t,
                  std::uint32_t) override {
        throw std::runtime_error("CopyTree copies both data words directly");
    }
    std::uint32_t Allocate(std::uint32_t bytes_requested) override {
        Check(bytes_requested == 24u, "tree nodes allocate 24 bytes");
        ++allocations;
        if (allocations == fail_allocation) throw std::bad_alloc();
        const auto address = 0x8000u + (allocations - 1u) * 0x100u;
        calls.emplace_back("allocate");
        return address;
    }
    void DestroyTree(std::uint32_t tree) override {
        Check(tree == kDestination, "destroy destination tree");
        calls.emplace_back("destroy-tree");
        Free(words[tree + 4u]);
        const auto remaining = --words[kRef];
        if (remaining == 0u) {
            const auto nil = words[kNil];
            words[kNil] = 0u;
            Free(nil);
        }
    }
};

void ConfigureThreeNodes(Runtime& runtime) {
    runtime.words[kNil] = 0x9000u;
    runtime.words[kRef] = 2u;
    runtime.words[0x7004u] = 0xa000u;
    runtime.words[kSource + 12u] = 3u;
    runtime.words[0xa000u] = 0xa020u;
    runtime.words[0xa008u] = 0xa040u;
    runtime.words[0xa014u] = 1u;
    runtime.words[0xa00cu] = 101u;
    runtime.words[0xa010u] = 1u;
    for (std::uint32_t node : {0xa020u, 0xa040u}) {
        runtime.words[node] = 0x9000u;
        runtime.words[node + 8u] = 0x9000u;
        runtime.words[node + 20u] = 0u;
    }
    runtime.words[0xa02cu] = 102u;
    runtime.words[0xa030u] = 2u;
    runtime.words[0xa04cu] = 103u;
    runtime.words[0xa050u] = 3u;
}

void PopulatedCopy() {
    Runtime runtime;
    ConfigureThreeNodes(runtime);
    Check(pal4::CombatTreeCopyConstructor::Copy(
              kDestination, kSource, runtime) == kDestination,
          "copy returns destination tree");
    Check(runtime.allocations == 5u &&
              runtime.freed == std::vector<std::uint32_t>{0x8000u} &&
              runtime.words[kNil] == 0x9000u &&
              runtime.words[kRef] == 3u,
          "existing sentinel candidate released and reference incremented");
    const auto head = runtime.words[kDestination + 4u];
    Check(head == 0x8100u &&
              runtime.words[head + 4u] == 0x8200u &&
              runtime.words[head] == 0x8400u &&
              runtime.words[head + 8u] == 0x8300u &&
              runtime.words[kDestination + 12u] == 3u,
          "root, minimum, maximum and count follow source tree");
    Check(runtime.words[0x8200u] == 0x8400u &&
              runtime.words[0x8208u] == 0x8300u &&
              runtime.words[0x820cu] == 101u &&
              runtime.words[0x8210u] == 1u &&
              runtime.words[0x840cu] == 102u &&
              runtime.words[0x8410u] == 2u &&
              runtime.words[0x830cu] == 103u &&
              runtime.words[0x8310u] == 3u,
          "left chain and right subtree copied with both data words");
    Check(runtime.bytes[kDestination] == 0x11u &&
              runtime.bytes[kDestination + 1u] == 0x22u &&
              runtime.bytes[kDestination + 8u] == 0x33u,
          "tree flags copied");
}

void EmptyAndFreshSentinel() {
    Runtime reused;
    reused.words[kNil] = 0x9000u;
    reused.words[kRef] = 2u;
    reused.words[0x7004u] = 0x9000u;
    (void)pal4::CombatTreeCopyConstructor::Copy(
        kDestination, kSource, reused);
    const auto head = reused.words[kDestination + 4u];
    Check(reused.words[head] == head &&
              reused.words[head + 4u] == 0x9000u &&
              reused.words[head + 8u] == head &&
              reused.words[kDestination + 12u] == 0u,
          "empty tree head stays circular");

    Runtime fresh;
    fresh.words[0x7004u] = 0x8000u;
    (void)pal4::CombatTreeCopyConstructor::Copy(
        kDestination, kSource, fresh);
    Check(fresh.words[kNil] == 0x8000u && fresh.words[kRef] == 1u &&
              fresh.words[0x8000u] == 0u &&
              fresh.words[0x8008u] == 0u &&
              fresh.words[0x8014u] == 1u && fresh.freed.empty(),
          "first tree creates shared sentinel");
}

void FailureCleanup() {
    for (std::uint32_t failed = 1u; failed <= 3u; ++failed) {
        Runtime runtime;
        ConfigureThreeNodes(runtime);
        runtime.fail_allocation = failed;
        bool threw = false;
        try {
            (void)pal4::CombatTreeCopyConstructor::Copy(
                kDestination, kSource, runtime);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        Check(threw && runtime.words[kRef] == 2u &&
                  runtime.words[kNil] == 0x9000u,
              "failed allocation restores existing sentinel reference count");
        if (failed == 3u)
            Check(runtime.calls.end() !=
                      std::find(runtime.calls.begin(), runtime.calls.end(),
                                "destroy-tree"),
                  "subtree failure destroys completed head");
    }
}

void AssignUninitializedTree() {
    Runtime runtime;
    ConfigureThreeNodes(runtime);
    Check(pal4::CombatTreeCopyConstructor::Assign(
              kDestination, kSource, runtime) == kDestination,
          "assign returns destination tree");
    Check(runtime.calls[0] == "byte:8192" &&
              runtime.calls[1] == "byte:8193" &&
              runtime.calls[2] == "byte:8200" &&
              runtime.freed == std::vector<std::uint32_t>{0x8000u} &&
              runtime.words[kRef] == 3u,
          "assignment copies flags then constructs a new head and reference");
    Check(runtime.words[kDestination + 4u] == 0x8100u &&
              runtime.words[0x8104u] == 0x8200u &&
              runtime.words[0x8100u] == 0x8400u &&
              runtime.words[0x8108u] == 0x8300u &&
              runtime.words[kDestination + 12u] == 3u,
          "new head points to copied root, minimum and maximum");

    Runtime empty;
    empty.words[kNil] = 0x9000u;
    empty.words[kRef] = 2u;
    empty.words[0x7004u] = 0x9000u;
    (void)pal4::CombatTreeCopyConstructor::Assign(
        kDestination, kSource, empty);
    Check(empty.words[0x8100u] == 0x8100u &&
              empty.words[0x8104u] == 0x9000u &&
              empty.words[0x8108u] == 0x8100u &&
              empty.allocations == 2u,
          "empty assignment still constructs a circular head");
}

void CopyAssignedNodesWithoutHeaderInitialization() {
    Runtime runtime;
    ConfigureThreeNodes(runtime);
    runtime.words[kDestination + 4u] = 0x7500u;
    runtime.words[0x7500u] = 0x7500u;
    runtime.words[0x7504u] = 0x9000u;
    runtime.words[0x7508u] = 0x7500u;
    runtime.bytes[kDestination] = 0xaau;
    Check(pal4::CombatTreeCopyConstructor::CopyAssignedNodes(
              kDestination, kSource, runtime) == 0x9000u,
          "node-copy helper returns shared nil");
    Check(runtime.bytes[kDestination] == 0xaau &&
              runtime.words[0x7504u] == 0x8000u &&
              runtime.words[0x7500u] == 0x8200u &&
              runtime.words[0x7508u] == 0x8100u &&
              runtime.words[kDestination + 12u] == 3u &&
              runtime.allocations == 3u,
          "helper fills nodes, count and extrema without touching tree flags");

    Runtime empty;
    empty.words[kNil] = 0x9000u;
    empty.words[0x7004u] = 0x9000u;
    empty.words[kDestination + 4u] = 0x7500u;
    Check(pal4::CombatTreeCopyConstructor::CopyAssignedNodes(
              kDestination, kSource, empty) == 0x9000u &&
              empty.words[0x7500u] == 0x7500u &&
              empty.words[0x7504u] == 0x9000u &&
              empty.words[0x7508u] == 0x7500u,
          "empty helper resets head links without allocation");
}

void InitializeTreeOwnsSentinelAndHead() {
    Runtime reused;
    reused.words[kNil] = 0x9000u;
    reused.words[kRef] = 2u;
    Check(pal4::CombatTreeCopyConstructor::InitializeTree(
              kDestination, reused) == 0x8100u &&
              reused.words[kNil] == 0x9000u &&
              reused.words[kRef] == 3u &&
              reused.freed == std::vector<std::uint32_t>{0x8000u} &&
              reused.words[kDestination + 4u] == 0x8100u &&
              reused.words[0x8100u] == 0x8100u &&
              reused.words[0x8104u] == 0x9000u &&
              reused.words[0x8108u] == 0x8100u &&
              reused.words[kDestination + 12u] == 0u,
          "standalone initializer allocates head and references existing nil");
    Runtime fresh;
    Check(pal4::CombatTreeCopyConstructor::InitializeTree(
              kDestination, fresh) == 0x8100u &&
              fresh.words[kNil] == 0x8000u &&
              fresh.words[kRef] == 1u && fresh.freed.empty(),
          "standalone initializer creates first shared nil");
}

}  // namespace

int main() {
    try {
        PopulatedCopy();
        EmptyAndFreshSentinel();
        FailureCleanup();
        AssignUninitializedTree();
        CopyAssignedNodesWithoutHeaderInitialization();
        InitializeTreeOwnsSentinelAndHead();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
