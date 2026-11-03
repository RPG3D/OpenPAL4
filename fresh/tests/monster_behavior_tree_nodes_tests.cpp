#include "pal4/monster_behavior_tree.h"

#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kNil = 0x7000u;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMonsterBehaviorTreeRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::uint32_t nil{};
    std::uint32_t references{};

    void InitializeRedBlackTree(pal4::MonsterBehaviorTreeState&,
                                std::uint32_t) override {}
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        Check(bytes == 0x18u && !allocations.empty(),
              "node allocation uses 24 bytes");
        const auto address = allocations.front();
        allocations.erase(allocations.begin());
        return address;
    }
    void DestroySubtree(std::uint32_t, std::uint32_t) override {}
    void GetFirstElement(std::uint32_t) override {}
    void RemoveNode(std::uint32_t, std::uint32_t) override {}
    std::uint32_t SharedNil() override { return nil; }
    void SetSharedNil(std::uint32_t value) override { nil = value; }
    std::uint32_t SharedReferenceCount() override { return references; }
    void SetSharedReferenceCount(std::uint32_t value) override {
        references = value;
    }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
        return 0u;
    }
};

void FirstTreeAndSharedNil() {
    Runtime runtime;
    runtime.allocations = {0x7000u, 0x8000u};
    pal4::MonsterBehaviorTreeState first{};
    first.words[4] = 99u;
    const auto header = pal4::MonsterBehaviorTree::InitializeNodes(first, runtime);
    Check(header == 0x8000u && first.words[2] == header &&
              first.words[4] == 0u && runtime.nil == 0x7000u &&
              runtime.references == 1u && runtime.freed.empty(),
          "first tree creates shared nil and resets count");
    Check(runtime.words.at(0x7000u) == 0u &&
              runtime.words.at(0x7004u) == 0u &&
              runtime.words.at(0x7008u) == 0u &&
              runtime.words.at(0x7014u) == 1u &&
              runtime.words.at(0x8000u) == header &&
              runtime.words.at(0x8004u) == runtime.nil &&
              runtime.words.at(0x8008u) == header &&
              runtime.words.at(0x8014u) == 0u,
          "nil and header fields follow observed offsets");

    runtime.allocations = {0x9000u, 0xa000u};
    pal4::MonsterBehaviorTreeState second{};
    Check(pal4::MonsterBehaviorTree::InitializeNodes(second, runtime)
              == 0xa000u && runtime.nil == 0x7000u &&
              runtime.references == 2u &&
              runtime.freed == std::vector<std::uint32_t>{0x9000u} &&
              runtime.words.at(0xa004u) == runtime.nil,
          "second tree frees unused candidate and shares nil");
}

void SubtreeReleaseOrder() {
    Runtime runtime;
    runtime.nil = 0x7000u;
    runtime.words[0x1000u] = 0x0500u;
    runtime.words[0x1008u] = 0x2000u;
    runtime.words[0x0500u] = 0x0200u;
    runtime.words[0x0508u] = kNil;
    runtime.words[0x0200u] = kNil;
    runtime.words[0x0208u] = kNil;
    runtime.words[0x2000u] = kNil;
    runtime.words[0x2008u] = 0x2500u;
    runtime.words[0x2500u] = kNil;
    runtime.words[0x2508u] = kNil;
    Check(pal4::MonsterBehaviorTree::DestroySubtreeNodes(
              0x1000u, runtime) == kNil &&
              runtime.freed == std::vector<std::uint32_t>{
                  0x2500u, 0x2000u, 0x1000u, 0x0500u, 0x0200u},
          "right recursion and left-chain free order");
    runtime.freed.clear();
    Check(pal4::MonsterBehaviorTree::DestroySubtreeNodes(kNil, runtime)
              == kNil && runtime.freed.empty(),
          "empty subtree returns nil without freeing nodes");
}

void SuccessorCursors() {
    Runtime runtime;
    runtime.nil = kNil;
    const auto header = 0x8000u;
    const auto root = 0x1000u;
    const auto left = 0x0500u;
    const auto right = 0x2000u;
    const auto inner = 0x1500u;
    runtime.words[header + 4u] = root;
    runtime.words[header + 8u] = right;
    runtime.words[root] = left;
    runtime.words[root + 4u] = header;
    runtime.words[root + 8u] = right;
    runtime.words[left + 4u] = root;
    runtime.words[left + 8u] = kNil;
    runtime.words[right] = inner;
    runtime.words[right + 4u] = root;
    runtime.words[right + 8u] = kNil;
    runtime.words[inner] = kNil;
    runtime.words[inner + 4u] = right;
    runtime.words[inner + 8u] = kNil;

    auto cursor = root;
    Check(pal4::MonsterBehaviorTree::AdvanceSuccessor(cursor, runtime)
              == kNil && cursor == inner,
          "right-subtree descent returns nil register and updates cursor");
    std::uint32_t previous{};
    auto& output = pal4::MonsterBehaviorTree::SuccessorCursor(
        cursor, previous, runtime);
    Check(&output == &previous && previous == inner && cursor == right,
          "output helper returns original cursor and ascends parent");
    Check(pal4::MonsterBehaviorTree::AdvanceSuccessor(cursor, runtime)
              == header && cursor == header,
          "rightmost node advances to header");
    cursor = left;
    Check(pal4::MonsterBehaviorTree::AdvanceSuccessor(cursor, runtime)
              == root && cursor == root,
          "left child ascends directly to parent");
}

}  // namespace

int main() {
    FirstTreeAndSharedNil();
    SubtreeReleaseOrder();
    SuccessorCursors();
}
