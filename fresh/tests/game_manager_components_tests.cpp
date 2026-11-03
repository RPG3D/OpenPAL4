#include "pal4/game_manager_components.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
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

constexpr std::uint32_t kAddress = 0x10000u;
constexpr std::array<std::uint32_t, 5u> kGetters{
    0x7e0e40u, 0x7e0fc0u, 0x7de4b0u, 0x5a6980u, 0x59b770u};
constexpr std::array<std::uint32_t, 5u> kFactories{
    0x429c70u, 0x429a30u, 0x42a130u, 0x42a360u, 0x429ec0u};

struct TraceTreeRuntime final : pal4::IMonsterBehaviorTreeRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string>* calls{};
    std::uint32_t nil{0x7000u};
    std::uint32_t references{2u};
    std::uint32_t next_allocation{0xb000u};

    void InitializeRedBlackTree(pal4::MonsterBehaviorTreeState&,
                                std::uint32_t) override {}
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls->push_back("read:" + std::to_string(address));
        return memory.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        calls->push_back("write:" + std::to_string(address));
        memory[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "behavior node allocation size");
        const auto address = next_allocation;
        next_allocation += 0x20u;
        calls->emplace_back("allocate-node");
        return address;
    }
    void DestroySubtree(std::uint32_t tree, std::uint32_t root) override {
        Check(tree == kAddress + 0x69cu && root == 0xa100u,
              "whole-tree cleanup arguments");
        calls->emplace_back("destroy-subtree");
    }
    void GetFirstElement(std::uint32_t tree) override {
        Check(tree == kAddress + 0x69cu, "tree cursor address");
        calls->emplace_back("first-element");
    }
    void RemoveNode(std::uint32_t tree, std::uint32_t node) override {
        Check(tree == kAddress + 0x69cu && node == 0xa100u,
              "individual node removal arguments");
        calls->emplace_back("remove-node");
    }
    std::uint32_t SharedNil() override {
        calls->emplace_back("get-nil");
        return nil;
    }
    void SetSharedNil(std::uint32_t value) override {
        calls->emplace_back("set-nil");
        nil = value;
    }
    std::uint32_t SharedReferenceCount() override {
        calls->emplace_back("get-ref");
        return references;
    }
    void SetSharedReferenceCount(std::uint32_t value) override {
        calls->emplace_back("set-ref");
        references = value;
    }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        calls->push_back("free:" + std::to_string(address));
        return 0u;
    }
};

struct Runtime final : pal4::GameManagerComponentRuntimeStub {
    std::vector<std::string> calls;
    TraceTreeRuntime tree;
    std::string throw_at;
    std::uint32_t class_index = 0u;
    std::uint32_t insert_index = 0u;
    std::uint32_t copy_index = 0u;

    void Enter(const char* name) {
        calls.emplace_back(name);
        if (throw_at == name) throw std::runtime_error(name);
    }
    Runtime() { tree.calls = &calls; }
    pal4::IMonsterBehaviorTreeRuntime& TreeRuntime() override { return tree; }
    void ConstructBase(pal4::GameManagerComponentState&,
                       std::uint32_t address) override {
        Check(address == kAddress, "base address");
        Enter("base");
    }
    void DestroyBase(pal4::GameManagerComponentState&,
                     std::uint32_t address) override {
        Check(address == kAddress, "base cleanup address");
        Enter("destroy-base");
    }
    void InitializeEventProcessor(pal4::GameManagerComponentState&,
                                  std::uint32_t address) override {
        Check(address == kAddress + 0x658u, "processor address");
        Enter("processor");
    }
    void CleanupEventProcessor(pal4::GameManagerComponentState&,
                               std::uint32_t address) override {
        Check(address == kAddress + 0x658u, "processor cleanup address");
        Enter("cleanup-processor");
    }
    void InitializeScriptObject(pal4::GameManagerComponentState&,
                                std::uint32_t address) override {
        Check(address == kAddress + 0x688u, "script address");
        Enter("script");
    }
    void CleanupScriptObject(pal4::GameManagerComponentState&,
                             std::uint32_t address) override {
        Check(address == kAddress + 0x688u, "script cleanup address");
        Enter("cleanup-script");
    }
    void InitializeBehaviorTree(pal4::GameManagerComponentState& state,
                                std::uint32_t address) override {
        Check(address == kAddress + 0x69cu, "tree address");
        Check((state.words[0x69cu / 4u] & 0xffffu) == 0x5a5au &&
              (state.words[0x6a4u / 4u] & 0xffu) == 0u,
              "stack bytes and tree flag before tree initialization");
        Enter("tree");
    }
    void DestroyBehaviorTree(pal4::GameManagerComponentState&,
                             std::uint32_t address) override {
        Check(address == kAddress + 0x69cu, "tree cleanup address");
        Enter("destroy-tree");
    }
    void DestroyAuxiliary(pal4::GameManagerComponentState& state,
                          std::uint32_t address) override {
        Check(address == kAddress + 0x6dcu, "auxiliary cleanup address");
        Check(state.words[0u] == 0x842dc0u &&
              state.words[0x170u / 4u] == 0x842db0u &&
              state.words[0x1ecu / 4u] == 0x842da4u,
              "outer vtables before auxiliary cleanup");
        Enter("destroy-auxiliary");
    }
    std::uint32_t ClassIdFromGetter(std::uint32_t getter) override {
        Check(class_index < kGetters.size() &&
              getter == kGetters[class_index], "class ID getter order");
        calls.push_back("getter:" + std::to_string(class_index));
        return 10u + class_index++;
    }
    pal4::GameManagerBehaviorInsertResult InsertBehavior(
        std::uint32_t tree, std::uint32_t class_id,
        std::uint32_t factory) override {
        Check(tree == kAddress + 0x69cu && insert_index < 4u &&
              factory == kFactories[insert_index] &&
              class_id == 10u + insert_index,
              "behavior insertion arguments");
        ++insert_index;
        Enter("insert");
        return {0x9000u + insert_index, (insert_index & 1u) != 0u};
    }
    void CopyInsertionResult(
        const pal4::GameManagerBehaviorInsertResult& result) override {
        ++copy_index;
        Check(copy_index == insert_index &&
              result.node == 0x9000u + copy_index &&
              result.inserted == ((copy_index & 1u) != 0u),
              "copy receives each insertion result");
        Enter("copy");
    }
    void InsertFinalBehavior(std::uint32_t tree,
                             std::uint32_t class_id,
                             std::uint32_t factory) override {
        Check(tree == kAddress + 0x69cu && class_id == 14u &&
              factory == kFactories[4], "final behavior insertion arguments");
        Enter("final-insert");
    }
};

void TestSuccess() {
    pal4::GameManagerComponentState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(pal4::GameManagerComponents::Initialize(
        state, kAddress, 0x5au, runtime) == kAddress,
        "initializer returns object address");
    const auto& words = state.words;
    Check(words[0u] == 0x842dc0u &&
          words[0x170u / 4u] == 0x842db0u &&
          words[0x1ecu / 4u] == 0x842da4u,
          "final outer vtables");
    Check(words[0x698u / 4u] == 0x842d64u &&
          words[0x6acu / 4u] == 0x842e54u &&
          words[0x6b0u / 4u] == 0x842e48u &&
          words[0x6b8u / 4u] == 0x842e3cu &&
          words[0x6c0u / 4u] == 0x842e2cu &&
          words[0x6c8u / 4u] == 0x842e1cu &&
          words[0x6d0u / 4u] == 0x842e10u,
          "final tree vtables");
    Check(words[0x6dcu / 4u] == 0x849398u &&
          words[0x6e0u / 4u] == 0u,
          "embedded game-manager callback constructor");
    Check(words[0x204u / 4u] == 0u &&
          words[0x654u / 4u] == 0u &&
          words[0x6e4u / 4u] == 0u &&
          words[0x6e8u / 4u] == 0u &&
          words[0x6d4u / 4u] == kAddress &&
          words[0x6d8u / 4u] == 0xdeadbeefu,
          "zeroed fields, self pointer and untouched neighbor");
    Check(runtime.class_index == 5u && runtime.insert_index == 4u &&
          runtime.copy_index == 4u, "five getters and four result copies");
    Check(runtime.calls == std::vector<std::string>{
        "base", "processor", "script", "tree",
        "getter:0", "insert", "copy", "getter:1", "insert", "copy",
        "getter:2", "insert", "copy", "getter:3", "insert", "copy",
        "getter:4", "final-insert"}, "construction and registration order");
}

void TestUnwind() {
    struct Case {
        const char* failed;
        std::vector<std::string> cleanup;
    };
    const std::vector<Case> cases{
        {"base", {}},
        {"processor", {"destroy-base"}},
        {"script", {"cleanup-processor", "destroy-base"}},
        {"tree", {"cleanup-script", "cleanup-processor", "destroy-base"}},
        {"insert", {"destroy-auxiliary", "destroy-tree",
                     "cleanup-script", "cleanup-processor", "destroy-base"}},
    };
    for (const auto& test : cases) {
        pal4::GameManagerComponentState state{};
        state.words[0x69cu / 4u] = 0u;
        state.words[0x6a4u / 4u] = 0u;
        Runtime runtime;
        runtime.throw_at = test.failed;
        bool thrown = false;
        try {
            (void)pal4::GameManagerComponents::Initialize(
                state, kAddress, 0x5au, runtime);
        } catch (const std::runtime_error&) {
            thrown = true;
        }
        Check(thrown, "constructor propagates failure");
        Check(runtime.calls.size() >= test.cleanup.size() + 1u,
              "failure and cleanup were logged");
        Check(std::vector<std::string>(
                  runtime.calls.end() - static_cast<std::ptrdiff_t>(test.cleanup.size()),
                  runtime.calls.end()) == test.cleanup,
              "SEH-stage cleanup runs in reverse order");
    }
}

void TestCleanup(std::uint32_t count, std::uint32_t references) {
    pal4::GameManagerComponentState state{};
    state.words.fill(0xdeadbeefu);
    state.words[0x6a0u / 4u] = 0xa000u;
    state.words[0x6a8u / 4u] = count;
    Runtime runtime;
    runtime.tree.references = references;
    runtime.tree.memory[0xa000u] = count != 0u ? 0xa200u : 0xa100u;
    runtime.tree.memory[0xa004u] = 0xa100u;
    runtime.tree.memory[0xa008u] = 0xa000u;
    if (count == 0u) {
        runtime.tree.memory[0xa108u] = runtime.tree.nil;
        runtime.tree.memory[0xa104u] = 0xa000u;
    }
    pal4::GameManagerComponents::Cleanup(state, kAddress, runtime);
    Check(state.words[0x698u / 4u] == 0x842d64u &&
          state.words[0x6a0u / 4u] == 0u &&
          state.words[0x6a8u / 4u] == 0u &&
          state.words[0x6acu / 4u] == 0xdeadbeefu,
          "tree release writes only its observed fields");
    Check(runtime.tree.references == references - 1u &&
          runtime.tree.nil == (references == 1u ? 0u : 0x7000u),
          "shared nil lifetime");
    const auto& calls = runtime.calls;
    Check(!calls.empty() && calls.front() == "destroy-auxiliary" &&
          calls[calls.size() - 3u] == "cleanup-script" &&
          calls[calls.size() - 2u] == "cleanup-processor" &&
          calls.back() == "destroy-base",
          "auxiliary, tree, script, processor and base cleanup order");
    const auto contains = [&](const std::string& name) {
        return std::find(calls.begin(), calls.end(), name) != calls.end();
    };
    Check(contains("destroy-subtree") == (count != 0u) &&
          contains("remove-node") == (count == 0u) &&
          contains("first-element") == (count != 0u),
          "count selects whole-tree or individual-node release");
    Check(contains("free:40960") &&
          contains("free:28672") == (references == 1u),
          "header and final shared nil release");
}

void TestBehaviorNodeInsertion() {
    pal4::GameManagerComponentState state{};
    state.words[0x6a0u / 4u] = 0xa000u;
    state.words[0x6acu / 4u] = 0xdeadbeefu;
    Runtime runtime;
    auto& memory = runtime.tree.memory;
    memory[0xa000u] = 0xa000u;
    memory[0xa004u] = runtime.tree.nil;
    memory[0xa008u] = 0xa000u;
    memory[runtime.tree.nil] = 0u;
    memory[runtime.tree.nil + 4u] = 0u;
    memory[runtime.tree.nil + 8u] = 0u;
    memory[runtime.tree.nil + 20u] = 1u;
    pal4::GameManagerBehaviorInsertResult result{};
    auto* returned = &pal4::GameManagerComponents::InsertBehaviorNode(
        state, 3, 0x9000u, result, runtime);
    Check(returned == &result && result.node == 0xb000u && result.inserted &&
          state.words[0x6a8u / 4u] == 1u &&
          state.words[0x6acu / 4u] == 0xdeadbeefu &&
          memory.at(0xb00cu) == 3u && memory.at(0xb010u) == 0x9000u &&
          memory.at(0xa004u) == 0xb000u,
          "first insertion stores key, factory, root and output");

    (void)pal4::GameManagerComponents::InsertBehaviorNode(
        state, 3, 0x9111u, result, runtime);
    Check(result.node == 0xb000u && !result.inserted &&
          state.words[0x6a8u / 4u] == 1u &&
          runtime.tree.next_allocation == 0xb020u,
          "unique mode returns existing node without allocating");

    (void)pal4::GameManagerComponents::InsertBehaviorNode(
        state, 1, 0x9222u, result, runtime);
    (void)pal4::GameManagerComponents::InsertBehaviorNode(
        state, 5, 0x9333u, result, runtime);
    Check(result.node == 0xb040u && result.inserted &&
          state.words[0x6a8u / 4u] == 3u &&
          memory.at(0xa000u) == 0xb020u &&
          memory.at(0xa008u) == 0xb040u &&
          memory.at(0xb000u) == 0xb020u &&
          memory.at(0xb008u) == 0xb040u,
          "ordered insertions update header extrema and node links");

    state.words[0x6a4u / 4u] = 1u;
    (void)pal4::GameManagerComponents::InsertBehaviorNode(
        state, 3, 0x9444u, result, runtime);
    Check(result.node == 0xb060u && result.inserted &&
          state.words[0x6a8u / 4u] == 4u &&
          memory.at(0xb06cu) == 3u && memory.at(0xb070u) == 0x9444u,
          "duplicate-allowed mode inserts a second key");
}

}  // namespace

int main() {
    TestSuccess();
    TestUnwind();
    TestCleanup(2u, 2u);
    TestCleanup(0u, 1u);
    TestBehaviorNodeInsertion();
}
