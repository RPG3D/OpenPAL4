#include "pal4/monster_manager_tree_cleanup.h"
#include "pal4/npc_manager_tree.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMonsterManagerTreeCleanupRuntime {
    static constexpr std::uint32_t kNil = 0x3000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t SharedNil() const override { return kNil; }
    std::uint32_t ReadDword(std::uint32_t address) const override {
        return words.at(address);
    }
    void ReleaseTextRecord(std::uint32_t record) override {
        calls.push_back("text:" + std::to_string(record - 0x0cu));
    }
    void FreeMemory(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void Node(std::uint32_t address, std::uint32_t left,
              std::uint32_t right) {
        words[address] = left;
        words[address + 8u] = right;
    }
};

void TestCleanupOrder() {
    Runtime runtime;
    constexpr std::uint32_t a = 0x4000u;
    constexpr std::uint32_t b = 0x5000u;
    constexpr std::uint32_t c = 0x6000u;
    constexpr std::uint32_t d = 0x7000u;
    constexpr std::uint32_t e = 0x8000u;
    Check(pal4::MonsterManagerTreeCleanup::DestroySubtree(
        Runtime::kNil, runtime) == Runtime::kNil && runtime.calls.empty(),
        "nil subtree is untouched");
    runtime.Node(a, b, c);
    runtime.Node(b, Runtime::kNil, e);
    runtime.Node(c, d, Runtime::kNil);
    runtime.Node(d, Runtime::kNil, Runtime::kNil);
    runtime.Node(e, Runtime::kNil, Runtime::kNil);
    Check(pal4::MonsterManagerTreeCleanup::DestroySubtree(a, runtime)
              == Runtime::kNil, "cleanup raw nil return");
    std::vector<std::string> expected;
    for (const auto node : {c, d, a, e, b}) {
        expected.push_back("text:" + std::to_string(node));
        expected.push_back("free:" + std::to_string(node));
    }
    Check(runtime.calls == expected, "right recursion then left iteration");
}

void TestNpcCleanupEntry() {
    Runtime runtime;
    runtime.Node(0x4000u, 0x5000u, 0x6000u);
    runtime.Node(0x5000u, Runtime::kNil, Runtime::kNil);
    runtime.Node(0x6000u, Runtime::kNil, Runtime::kNil);
    Check(pal4::NpcManagerTree::DestroySubtree(
              0x4000u, runtime) == Runtime::kNil &&
          runtime.calls == std::vector<std::string>{
              "text:24576", "free:24576", "text:16384", "free:16384",
              "text:20480", "free:20480"},
          "NPC subtree frees right, root, then left");
}

}  // namespace

int main() {
    TestCleanupOrder();
    TestNpcCleanupEntry();
}
