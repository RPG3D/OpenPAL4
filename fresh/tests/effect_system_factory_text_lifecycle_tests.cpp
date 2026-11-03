#include "pal4/effect_system_factory_text_tree.h"

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

struct Runtime final : pal4::IEffectSystemFactoryTextLifecycleRuntime {
    static constexpr std::uint32_t kTree = 0x2000u;
    static constexpr std::uint32_t kHead = 0x3000u;
    static constexpr std::uint32_t kNil = 0x93cdf0u;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kTree + 4u, kHead}, {kTree + 12u, 0u},
        {kHead, kHead}, {kHead + 4u, kNil},
        {kHead + 8u, kHead}};
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void DeleteSubtree(std::uint32_t root) override {
        calls.push_back("subtree:" + std::to_string(root));
    }
    void ReleaseText(std::uint32_t record) override {
        calls.push_back("release:" + std::to_string(record));
    }
    void FreeNode(std::uint32_t node) override {
        calls.push_back("free:" + std::to_string(node));
    }
    std::uint32_t Next(std::uint32_t node) override {
        calls.push_back("next:" + std::to_string(node));
        return words.at(node + 16u);
    }
    void EraseNode(std::uint32_t tree, std::uint32_t node) override {
        Check(tree == kTree, "erase receives tree address");
        calls.push_back("erase:" + std::to_string(node));
    }
};

void TestPopulatedClear() {
    Runtime runtime;
    runtime.words[Runtime::kTree + 12u] = 3u;
    runtime.words[Runtime::kHead] = 0x4100u;
    runtime.words[Runtime::kHead + 4u] = 0x4000u;
    runtime.words[Runtime::kHead + 8u] = 0x4200u;
    runtime.words[0x4000u] = 0x4100u;
    runtime.words[0x4008u] = 0x4200u;
    runtime.words[0x4100u] = Runtime::kNil;
    runtime.words[0x4108u] = Runtime::kNil;
    const auto result = pal4::EffectSystemFactoryTextTree::Clear(
        Runtime::kTree, runtime);
    Check(result == Runtime::kHead &&
              runtime.calls == std::vector<std::string>{
                  "subtree:16896", "release:16396", "free:16384",
                  "subtree:9686512", "release:16652", "free:16640"},
          "populated clear frees right subtrees before left-chain nodes");
    Check(runtime.words.at(Runtime::kHead) == Runtime::kHead &&
              runtime.words.at(Runtime::kHead + 4u) == Runtime::kNil &&
              runtime.words.at(Runtime::kHead + 8u) == Runtime::kHead &&
              runtime.words.at(Runtime::kTree + 12u) == 0u,
          "populated clear retains and resets head");
}

void TestZeroCountFallback() {
    Runtime runtime;
    runtime.words[Runtime::kHead] = 0x4000u;
    runtime.words[0x4010u] = 0x4100u;
    runtime.words[0x4110u] = Runtime::kHead;
    Check(pal4::EffectSystemFactoryTextTree::Clear(
              Runtime::kTree, runtime) == Runtime::kHead &&
              runtime.calls == std::vector<std::string>{
                  "next:16384", "erase:16384",
                  "next:16640", "erase:16640"},
          "zero-count fallback advances before erasing each node");
    Runtime empty;
    Check(pal4::EffectSystemFactoryTextTree::Clear(
              Runtime::kTree, empty) == Runtime::kHead &&
              empty.calls.empty(),
          "empty clear performs no erase");
}

}  // namespace

int main() {
    TestPopulatedClear();
    TestZeroCountFallback();
}
