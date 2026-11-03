#include "pal4/monster_behavior_tree.h"

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

struct Runtime final : pal4::IMonsterBehaviorTreeRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> calls;
    std::uint32_t nil{0x7000u};
    std::uint32_t references{2u};

    void InitializeRedBlackTree(pal4::MonsterBehaviorTreeState& state,
                                std::uint32_t address) override {
        Check(address == 0x169cu && state.words[3] == 0x123456ccu,
              "initializer receives the embedded tree address");
        calls.emplace_back("initialize");
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return memory.at(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        memory[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t) override { return 0u; }
    void DestroySubtree(std::uint32_t address,
                        std::uint32_t root) override {
        Check(address == 0x169cu && root == 0xa100u,
              "subtree root is header +4");
        calls.emplace_back("destroy-subtree");
    }
    void GetFirstElement(std::uint32_t address) override {
        Check(address == 0x169cu, "first-element tree address");
        calls.emplace_back("first-element");
    }
    void RemoveNode(std::uint32_t address,
                    std::uint32_t node) override {
        Check(address == 0x169cu, "remove tree address");
        calls.push_back("remove:" + std::to_string(node));
    }
    std::uint32_t SharedNil() override {
        calls.emplace_back("get-nil");
        return nil;
    }
    void SetSharedNil(std::uint32_t address) override {
        calls.emplace_back("set-nil");
        nil = address;
    }
    std::uint32_t SharedReferenceCount() override {
        calls.emplace_back("get-ref");
        return references;
    }
    void SetSharedReferenceCount(std::uint32_t value) override {
        calls.emplace_back("set-ref");
        references = value;
    }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return address + 0x10u;
    }
};

pal4::MonsterBehaviorTreeState State(std::uint32_t count) {
    pal4::MonsterBehaviorTreeState state{};
    state.words[0] = 0xdeadbeefu;
    state.words[2] = 0xa000u;
    state.words[4] = count;
    return state;
}

}  // namespace

int main() {
    Runtime runtime;
    auto state = State(0u);
    state.words[1] = 0x87654321u;
    state.words[3] = 0x12345678u;
    Check(pal4::MonsterBehaviorTree::Initialize(
              state, 0x1698u, 0xaau, 0xbbu, 0xccu, runtime) == 0x169cu &&
              state.words[1] == 0x8765bbaau &&
              state.words[3] == 0x123456ccu &&
              state.words[0] == 0xdeadbeefu &&
              runtime.calls == std::vector<std::string>{"initialize"},
          "constructor writes three bytes and preserves adjacent fields");

    runtime.calls.clear();
    state = State(2u);
    runtime.memory[0xa000u] = 0xa200u;
    runtime.memory[0xa004u] = 0xa100u;
    runtime.memory[0xa008u] = 0xa300u;
    Check(pal4::MonsterBehaviorTree::Destroy(state, 0x1698u, runtime)
              == 1u && state.words[0] == 0x842d64u &&
              state.words[2] == 0u && state.words[4] == 0u &&
              runtime.memory[0xa000u] == 0xa000u &&
              runtime.memory[0xa004u] == 0x7000u &&
              runtime.memory[0xa008u] == 0xa000u &&
              runtime.nil == 0x7000u && runtime.references == 1u,
          "nonempty tree resets header and retains shared nil");
    Check(runtime.calls == std::vector<std::string>{
              "read:40960", "read:40964", "destroy-subtree", "get-nil",
              "write:40964", "write:40960", "write:40968",
              "first-element", "free:40960", "get-ref", "set-ref"},
          "nonempty destructor call order");

    runtime.calls.clear();
    state = State(0u);
    runtime.memory[0xa000u] = 0xa200u;
    runtime.memory[0xa004u] = 0xa200u;
    runtime.memory[0xa008u] = 0xa200u;
    runtime.memory[0xa208u] = runtime.nil;
    runtime.memory[0xa204u] = 0xa000u;
    Check(pal4::MonsterBehaviorTree::Destroy(state, 0x1698u, runtime)
              == 0x7010u && runtime.references == 0u &&
              runtime.nil == 0u,
          "zero-count path removes node and releases last shared nil");
    Check(runtime.calls == std::vector<std::string>{
              "read:40960", "get-nil", "read:41480", "read:41476",
              "read:40968", "read:40964", "read:41480",
              "read:40968",
              "remove:41472",
              "free:40960", "get-ref", "set-ref", "get-nil",
              "set-nil", "free:28672"},
          "zero-count traversal and nil release order");

    runtime.calls.clear();
    state = State(0u);
    runtime.memory[0xa000u] = 0xa000u;
    runtime.nil = 0u;
    runtime.references = 1u;
    Check(pal4::MonsterBehaviorTree::Destroy(state, 0x1698u, runtime)
              == 0u && runtime.calls.back() == "set-nil",
          "empty tree and null shared nil");

    runtime.calls.clear();
    state = State(0u);
    runtime.memory[0xa000u] = 0xa000u;
    runtime.references = 2u;
    Check(pal4::MonsterBehaviorTree::Delete(
              state, 0x1698u, 2u, runtime) == 0x1698u &&
              runtime.calls.back() == "set-ref",
          "deleting destructor ignores non-low flag bits");

    runtime.calls.clear();
    state = State(0u);
    runtime.memory[0xa000u] = 0xa000u;
    runtime.references = 2u;
    Check(pal4::MonsterBehaviorTree::Delete(
              state, 0x1698u, 3u, runtime) == 0x1698u &&
              runtime.calls.back() == "free:5784",
          "deleting destructor conditionally frees owner");
}
