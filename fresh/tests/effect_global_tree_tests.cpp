#include "pal4/effect_global_tree.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectGlobalTreeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t next_address = 0x100u;
    std::uint32_t nil = 0u, users = 0u;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 36u, "tree nodes allocate 36 bytes");
        const auto result = next_address;
        next_address += 0x100u;
        calls.push_back("alloc:" + std::to_string(result));
        return result;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t SharedNil() override { return nil; }
    void SetSharedNil(std::uint32_t address) override { nil = address; }
    std::uint32_t SharedNilUsers() override { return users; }
    void SetSharedNilUsers(std::uint32_t value) override { users = value; }
    void FirstElement(pal4::EffectGlobalTreeState&) override {
        calls.push_back("first");
    }
    int RegisterCleanup() override {
        calls.push_back("register");
        return -7;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::EffectGlobalTreeState state{};
    Check(pal4::EffectGlobalTree::Initialize(state, 0x1234abcdu,
                                             runtime) == -7,
          "initializer forwards atexit result");
    Check(state.tag == 0x34u && state.second == 0x12u &&
              state.flag == 0u && state.head == 0x200u &&
              state.count == 0u && runtime.nil == 0x100u &&
              runtime.users == 1u,
          "upper register bytes and fresh shared nil initialized");
    Check(runtime.words[0x100u] == 0u &&
              runtime.words[0x104u] == 0u &&
              runtime.words[0x108u] == 0u &&
              runtime.words[0x120u] == 1u &&
              runtime.words[0x200u] == 0x200u &&
              runtime.words[0x204u] == 0x100u &&
              runtime.words[0x208u] == 0x200u &&
              runtime.words[0x220u] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "alloc:256", "alloc:512", "register"},
          "shared nil and head retain native 36-byte links and flags");

    pal4::EffectGlobalTreeState other{};
    Check(pal4::EffectGlobalTree::InitializeHelper(
              other, 0x76543210u, runtime) == &other &&
              other.tag == 0x54u && other.second == 0x76u &&
              other.head == 0x400u && runtime.nil == 0x100u &&
              runtime.users == 2u &&
              runtime.calls.back() == "alloc:1024",
          "existing shared nil frees candidate and builds second head");
    Check(runtime.calls[runtime.calls.size() - 2u] == "free:768",
          "unused nil candidate released");

    runtime.calls.clear();
    other.count = 2u;
    runtime.words[other.head + 4u] = 0x500u;
    runtime.words[0x500u] = 0x600u;
    runtime.words[0x508u] = 0x700u;
    runtime.words[0x600u] = runtime.nil;
    runtime.words[0x608u] = runtime.nil;
    runtime.words[0x700u] = runtime.nil;
    runtime.words[0x708u] = runtime.nil;
    pal4::EffectGlobalTree::Cleanup(other, runtime);
    Check(other.head == 0u && other.count == 0u &&
              runtime.words[0x400u] == 0x400u &&
              runtime.words[0x404u] == 0x100u &&
              runtime.words[0x408u] == 0x400u &&
              runtime.users == 1u && runtime.nil == 0x100u &&
              runtime.calls == std::vector<std::string>{
                  "free:1792", "free:1280", "free:1536",
                  "first", "free:1024"},
          "nonempty cleanup resets links and retains shared nil");

    runtime.calls.clear();
    runtime.words[state.head] = 0x600u;
    runtime.words[state.head + 4u] = 0x600u;
    runtime.words[state.head + 8u] = 0x600u;
    runtime.words[0x600u] = runtime.nil;
    runtime.words[0x604u] = state.head;
    runtime.words[0x608u] = runtime.nil;
    runtime.words[0x620u] = 0u;
    runtime.words[runtime.nil + 32u] = 1u;
    pal4::EffectGlobalTree::Cleanup(state, runtime);
    Check(state.head == 0u && runtime.users == 0u &&
              runtime.nil == 0u &&
              runtime.calls == std::vector<std::string>{
                  "free:1536", "free:512", "free:256"},
          "zero-count cleanup walks residual nodes and frees final nil");

    Runtime leaf_runtime;
    pal4::EffectGlobalTreeState leaf_state{};
    (void)pal4::EffectGlobalTree::Construct(
        leaf_state, 1u, 2u, leaf_runtime);
    const auto leaf = 0x500u;
    leaf_state.count = 1u;
    leaf_runtime.words[leaf_state.head] = leaf;
    leaf_runtime.words[leaf_state.head + 4u] = leaf;
    leaf_runtime.words[leaf_state.head + 8u] = leaf;
    leaf_runtime.words[leaf] = leaf_runtime.nil;
    leaf_runtime.words[leaf + 4u] = leaf_state.head;
    leaf_runtime.words[leaf + 8u] = leaf_runtime.nil;
    leaf_runtime.words[leaf + 32u] = 0u;
    leaf_runtime.words[leaf_runtime.nil + 32u] = 1u;
    leaf_runtime.calls.clear();
    Check(pal4::EffectGlobalTree::Erase(
              leaf_state, leaf, leaf_runtime) == leaf_state.head &&
              leaf_state.count == 0u &&
              leaf_runtime.words[leaf_state.head] == leaf_state.head &&
              leaf_runtime.words[leaf_state.head + 4u] ==
                  leaf_runtime.nil &&
              leaf_runtime.words[leaf_state.head + 8u] ==
                  leaf_state.head &&
              leaf_runtime.calls == std::vector<std::string>{"free:1280"},
          "effect tree erase releases red leaf and returns head iterator");

    Runtime successor_runtime;
    pal4::EffectGlobalTreeState successor_state{};
    (void)pal4::EffectGlobalTree::Construct(
        successor_state, 0u, 0u, successor_runtime);
    const auto head = successor_state.head;
    const auto nil = successor_runtime.nil;
    const std::uint32_t root = 0x500u, left = 0x600u, right = 0x700u;
    successor_state.count = 3u;
    successor_runtime.words[head] = left;
    successor_runtime.words[head + 4u] = root;
    successor_runtime.words[head + 8u] = right;
    successor_runtime.words[root] = left;
    successor_runtime.words[root + 4u] = head;
    successor_runtime.words[root + 8u] = right;
    successor_runtime.words[root + 32u] = 1u;
    successor_runtime.words[left] = nil;
    successor_runtime.words[left + 4u] = root;
    successor_runtime.words[left + 8u] = nil;
    successor_runtime.words[left + 32u] = 0u;
    successor_runtime.words[right] = nil;
    successor_runtime.words[right + 4u] = root;
    successor_runtime.words[right + 8u] = nil;
    successor_runtime.words[right + 32u] = 0u;
    successor_runtime.words[nil + 32u] = 1u;
    successor_runtime.calls.clear();
    Check(pal4::EffectGlobalTree::Next(left, successor_runtime) == root &&
              pal4::EffectGlobalTree::Next(root, successor_runtime) == right &&
              pal4::EffectGlobalTree::Next(right, successor_runtime) == head,
          "effect iterator advances through parent, right subtree and end");
    Check(pal4::EffectGlobalTree::Erase(
              successor_state, root, successor_runtime) == right &&
              successor_state.count == 2u &&
              successor_runtime.words[head + 4u] == right &&
              successor_runtime.words[right] == left &&
              successor_runtime.words[right + 4u] == head &&
              successor_runtime.words[right + 32u] == 1u &&
              successor_runtime.words[left + 4u] == right &&
              successor_runtime.calls == std::vector<std::string>{"free:1280"},
          "effect tree erase transplants direct successor and repairs colour");
}
