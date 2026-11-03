#include "pal4/effect_system_tree_cleanup.h"

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

struct Runtime final : pal4::IEffectSystemTreeCleanupRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x2010u, 0x4008u}, {0x2020u, 0x5017u}};
    std::unordered_map<std::uint32_t, std::uint32_t> next;
    std::uint8_t active = 1u;
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x2004u, "active byte offset");
        return active;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == 0x2004u, "active byte write offset");
        calls.push_back("disable");
        active = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        if (address == 0x2008u)
            calls.push_back("radius");
        else
            calls.push_back("reset");
        words[address] = value;
    }
    std::uint32_t Nil(pal4::EffectRegistrationTree kind) override {
        calls.push_back("nil");
        return kind == pal4::EffectRegistrationTree::Clumps
                   ? 0x6010u : 0x6020u;
    }
    void DeleteSubtree(pal4::EffectRegistrationTree kind,
                       std::uint32_t root) override {
        Check(root == (kind == pal4::EffectRegistrationTree::Clumps
                           ? 0x6010u : 0x6020u),
              "subtree cleanup receives the right child");
        calls.push_back("subtree");
    }
    void Free(std::uint32_t node) override {
        Check(node == 0x4100u || node == 0x5100u,
              "left-chain node freed");
        calls.push_back("free");
    }
    std::uint32_t Advance(pal4::EffectRegistrationTree kind,
                          std::uint32_t cursor) override {
        Check(cursor == (kind == pal4::EffectRegistrationTree::Clumps
                             ? 0x4100u : 0x5100u),
              "zero-count path advances current node");
        calls.push_back("advance");
        return next.at(cursor);
    }
    void Erase(pal4::EffectRegistrationTree kind,
               std::uint32_t tree, std::uint32_t node) override {
        Check(tree == (kind == pal4::EffectRegistrationTree::Clumps
                           ? 0x200cu : 0x201cu) &&
                  node == (kind == pal4::EffectRegistrationTree::Clumps
                               ? 0x4100u : 0x5100u),
              "zero-count path erases selected node");
        calls.push_back("erase");
    }
    std::uint32_t FirstElement(std::uint32_t tree) override {
        Check(tree == 0x200cu || tree == 0x201cu,
              "first element uses current tree");
        calls.push_back("first");
        return words.at(tree + 4u);
    }
};

void TestInactive() {
    Runtime runtime;
    runtime.active = 0u;
    Check(pal4::EffectSystemTreeCleanup::CleanupEffects(
              0x2000u, runtime) == 0u && runtime.calls.empty(),
          "inactive system leaves both trees intact");
}

void TestPopulatedTrees() {
    Runtime runtime;
    runtime.words.insert({{0x2018u, 1u}, {0x2028u, 1u},
                          {0x4008u, 0x4100u}, {0x400cu, 0x4100u},
                          {0x4100u, 0x6010u}, {0x4108u, 0x6010u},
                          {0x5017u, 0x5100u}, {0x501bu, 0x5100u},
                          {0x5100u, 0x6020u}, {0x5108u, 0x6020u}});
    Check(pal4::EffectSystemTreeCleanup::CleanupEffects(
              0x2000u, runtime) == 0x17u &&
              runtime.active == 0u && runtime.words[0x2008u] == 0u &&
              runtime.words[0x2018u] == 0u &&
              runtime.words[0x2028u] == 0u &&
              runtime.words[0x4008u] == 0x4008u &&
              runtime.words[0x400cu] == 0x6010u &&
              runtime.words[0x5017u] == 0x5017u &&
              runtime.words[0x501bu] == 0x6020u,
          "populated trees free both left chains and restore separate nils");
    Check(runtime.calls == std::vector<std::string>{
                "disable", "radius", "nil", "subtree", "free",
                "reset", "reset", "reset", "reset", "first",
                "nil", "subtree", "free", "reset", "reset",
                "reset", "reset", "first"},
          "tree cleanup order follows native first-then-second sequence");
}

void TestZeroCountWithNodes() {
    Runtime runtime;
    runtime.words.insert({{0x2018u, 0u}, {0x2028u, 0u},
                          {0x4008u, 0x4100u}, {0x5017u, 0x5100u}});
    runtime.next = {{0x4100u, 0x4008u}, {0x5100u, 0x5017u}};
    Check(pal4::EffectSystemTreeCleanup::CleanupEffects(
              0x2000u, runtime) == 0x17u &&
              runtime.calls ==
                  std::vector<std::string>{"disable", "radius",
                                           "advance", "erase",
                                           "advance", "erase"},
          "zero-count fallback advances before erasing both nodes");
}

}  // namespace

int main() {
    TestInactive();
    TestPopulatedTrees();
    TestZeroCountWithNodes();
}
