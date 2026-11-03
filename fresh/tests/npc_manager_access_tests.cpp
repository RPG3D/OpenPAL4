#include "pal4/npc_manager_access.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct SingletonRuntime final : pal4::NpcManagerSingletonRuntimeStub {
    pal4::NpcManagerAccessState* state{};
    std::uint32_t result{0x9000u};
    std::vector<std::string> calls;

    std::uint32_t SingletonManager() override {
        calls.emplace_back("manager");
        if (state != nullptr)
            Check(pal4::NpcManagerAccess::Get(*state, *this) == 0x7788u,
                  "guard precedes singleton manager lookup");
        return 0x5000u;
    }
    std::uint32_t Register(std::uint32_t manager,
        std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t type_info) override {
        calls.emplace_back("register");
        Check(manager == 0x5000u && name == "palNpcManager" &&
              id == 0x410u && factory == 0x43c310u &&
              type_info == 0x7de5d0u,
              "NPC singleton registration operands");
        return result;
    }
};

struct ObjectRuntime final : pal4::NpcManagerObjectRuntimeStub {
    std::vector<std::uint32_t> allocations;
    std::size_t next_allocation{};
    std::vector<std::uint32_t> freed;
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, std::uint32_t> next_nodes;
    std::vector<std::uint32_t> visited;
    std::vector<std::uint32_t> deleted;
    std::uint32_t cleaned_tree{};
    std::uint32_t cleaned_root{};

    std::uint32_t Allocate(std::uint32_t bytes_count) override {
        calls.push_back("allocate:" + std::to_string(bytes_count));
        if (next_allocation == allocations.size()) return 0u;
        return allocations[next_allocation++];
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed.push_back(address);
        return 0xf00du;
    }
    void InitializeTreeBase(std::uint32_t object) override {
        Check(object == 0x5000u || object == 0x8000u,
              "NPC tree base object");
        calls.emplace_back("tree-base");
    }
    void InitializeGrid(std::uint32_t grid) override {
        Check(grid == 0x5038u || grid == 0x8038u,
              "NPC grid subobject offset");
        calls.emplace_back("grid");
    }
    void CleanupGrid(std::uint32_t grid) override {
        Check(grid == 0x5038u, "NPC cleanup grid pointer");
        calls.emplace_back("cleanup-grid");
    }
    std::uint32_t CleanupTreeBase(std::uint32_t object) override {
        Check(object == 0x5000u, "NPC base cleanup pointer");
        calls.emplace_back("cleanup-base");
        return 0xbeefu;
    }
    void CleanupTreeNodes(std::uint32_t tree,
                          std::uint32_t root) override {
        calls.emplace_back("cleanup-nodes");
        cleaned_tree = tree;
        cleaned_root = root;
    }
    void ResetTreeBegin(std::uint32_t tree) override {
        Check(tree == 0x5028u, "NPC tree begin reset pointer");
        calls.emplace_back("reset-begin");
    }
    std::uint32_t NextTreeNode(std::uint32_t node) override {
        calls.emplace_back("next");
        visited.push_back(node);
        return next_nodes.at(node);
    }
    void DeleteTreeNode(std::uint32_t tree,
                        std::uint32_t node) override {
        Check(tree == 0x5028u, "NPC tree deletion pointer");
        calls.emplace_back("delete");
        deleted.push_back(node);
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        if (address == 0x5000u || address == 0x8000u)
            calls.emplace_back("vtable");
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
};

struct ActionRuntime final : pal4::NpcManagerActionRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::vector<std::vector<std::uint32_t>> calls;

    std::uint32_t ReadDword(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
    void CreateNpc(std::uint32_t manager, std::uint32_t name,
                   std::uint32_t resource, std::uint32_t position,
                   std::uint32_t rotation, std::uint32_t record,
                   std::uint32_t enabled) override {
        calls.push_back({manager, name, resource, position,
                         rotation, record, enabled});
    }
};

struct InitializeRuntime final : pal4::NpcManagerInitializeRuntimeStub {
    std::vector<std::vector<std::uint32_t>> calls;

    void Cleanup(std::uint32_t manager) override {
        calls.push_back({1u, manager});
    }
    void LoadInfo(std::uint32_t manager, std::uint32_t first,
                  std::uint32_t second) override {
        calls.push_back({2u, manager, first, second});
    }
    void InitializeGrid(std::uint32_t grid) override {
        calls.push_back({3u, grid});
    }
    void ForEachNpc(std::uint32_t manager,
                    std::uint32_t callback,
                    std::uint32_t context) override {
        calls.push_back({4u, manager, callback, context});
    }
};

struct CleanupRuntime final : pal4::NpcManagerCleanupRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> next_nodes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;

    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        words[address] = value;
    }
    void NotifyNpc(std::uint32_t npc) override {
        Check(npc == 0x9000u, "NPC notification pointer");
        calls.emplace_back("notify");
    }
    void DestroyNpc(std::uint32_t npc,
                    std::uint32_t mode) override {
        Check(npc == 0x9000u && mode == 1u,
              "NPC deleting destructor arguments");
        calls.emplace_back("destroy-npc");
    }
    std::uint32_t NextTreeNode(std::uint32_t node) override {
        calls.emplace_back("next");
        return next_nodes.at(node);
    }
    void CleanupTreeNodes(std::uint32_t tree,
                          std::uint32_t root) override {
        Check(tree == 0x5028u && root == 0x6000u,
              "cleanup right subtree before left-spine node");
        calls.emplace_back("cleanup-nodes");
    }
    void FreeTreeText(std::uint32_t text,
                      std::uint32_t mode) override {
        Check(text == 0x710cu && mode == 0u,
              "NPC tree text cleanup pointer");
        calls.emplace_back("free-text");
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed.push_back(address);
        return 0xf00du;
    }
    void DeleteTreeNode(std::uint32_t tree,
                        std::uint32_t node) override {
        Check(tree == 0x5028u && node == 0x7100u,
              "fallback tree node deletion");
        calls.emplace_back("delete-node");
    }
    void CleanupComplexTree(std::uint32_t manager) override {
        Check(manager == 0x5000u, "complex tree manager pointer");
        calls.emplace_back("cleanup-complex");
    }
    void ClearGrid(std::uint32_t grid) override {
        Check(grid == 0x5038u, "clear grid pointer");
        calls.emplace_back("clear-grid");
    }
    void GetPreviousGridElement(std::uint32_t grid) override {
        Check(grid == 0x5038u, "previous grid pointer");
        calls.emplace_back("previous-grid");
    }
    std::uint32_t ResetGrid(std::uint32_t grid) override {
        Check(grid == 0x5038u, "reset grid pointer");
        calls.emplace_back("reset-grid");
        return 0x88u;
    }
};

void SingletonColdWarmFailure() {
    Check(pal4::NpcManagerAccess::kGuardAddress == 0x8e431cu &&
          pal4::NpcManagerAccess::kInstanceAddress == 0x8e4318u,
          "NPC singleton global addresses");
    SingletonRuntime runtime;
    pal4::NpcManagerAccessState state{0x80u, 0x7788u};
    runtime.state = &state;
    Check(pal4::NpcManagerAccess::Get(state, runtime) == 0x9000u &&
          state.guard == 0x81u && state.instance == 0x9000u &&
          runtime.calls == std::vector<std::string>{"manager", "register"},
          "cold NPC singleton lookup registers and caches");
    runtime.calls.clear();
    Check(pal4::NpcManagerAccess::Get(state, runtime) == 0x9000u &&
          runtime.calls.empty(), "warm lookup only reads cached pointer");

    pal4::NpcManagerAccessState failed{};
    runtime.state = nullptr;
    runtime.result = 0u;
    Check(pal4::NpcManagerAccess::Get(failed, runtime) == 0u &&
          failed.guard == 1u, "failed registration retains guard");
    runtime.calls.clear();
    Check(pal4::NpcManagerAccess::Get(failed, runtime) == 0u &&
          runtime.calls.empty(), "failed registration is not retried");
}

void FactoryAndConstructor() {
    ObjectRuntime runtime;
    runtime.allocations = {0x5000u, 0x6000u, 0x7000u};
    pal4::NpcManagerTreeGlobals globals{};
    Check(pal4::NpcManagerObject::Create(
              globals, 0xa5u, runtime) == 0x5000u &&
          runtime.calls == std::vector<std::string>{
              "allocate:96", "tree-base", "allocate:36",
              "allocate:36", "grid", "vtable"},
          "factory allocates and constructor initializes in order");
    Check(runtime.bytes.at(0x5028u) == 0xa5u &&
          runtime.bytes.at(0x5029u) == 0xa5u &&
          runtime.bytes.at(0x5030u) == 0u &&
          runtime.words.at(0x5000u) == 0x842e64u,
          "NPC object byte fields and vtable");
    Check(globals.instance_count == 1u &&
          globals.nil_address == 0x6000u &&
          runtime.words.at(0x6000u) == 0u &&
          runtime.words.at(0x6004u) == 0u &&
          runtime.words.at(0x6008u) == 0u &&
          runtime.words.at(0x6020u) == 1u &&
          runtime.words.at(0x7000u) == 0x7000u &&
          runtime.words.at(0x7004u) == 0x6000u &&
          runtime.words.at(0x7008u) == 0x7000u &&
          runtime.words.at(0x7020u) == 0u &&
          runtime.words.at(0x502cu) == 0x7000u &&
          runtime.words.at(0x5034u) == 0u,
          "NPC shared nil and tree header layout");
}

void FactoryFailureAndSharedNilReuse() {
    ObjectRuntime failure;
    pal4::NpcManagerTreeGlobals empty{};
    Check(pal4::NpcManagerObject::Create(empty, 0u, failure) == 0u &&
          failure.calls == std::vector<std::string>{"allocate:96"} &&
          empty.instance_count == 0u,
          "failed manager allocation skips construction");

    ObjectRuntime runtime;
    runtime.allocations = {0x8100u, 0x8200u};
    pal4::NpcManagerTreeGlobals globals{3u, 0x6000u};
    Check(pal4::NpcManagerObject::Construct(
              0x8000u, globals, 0x34u, runtime) == 0x8000u &&
          globals.instance_count == 4u && globals.nil_address == 0x6000u &&
          runtime.freed == std::vector<std::uint32_t>{0x8100u} &&
          runtime.words.at(0x8104u) == 0u &&
          runtime.words.at(0x8120u) == 1u &&
          runtime.words.at(0x8204u) == 0x6000u,
          "second manager reuses shared nil after temporary node writes");
}

void DestroyNonemptyTree() {
    ObjectRuntime runtime;
    runtime.words = {{0x502cu, 0x7000u}, {0x5034u, 2u},
                     {0x7000u, 0x7000u}, {0x7004u, 0x7100u}};
    pal4::NpcManagerTreeGlobals globals{2u, 0x6000u};
    Check(pal4::NpcManagerObject::DestroyTree(
              0x5028u, globals, runtime) == 1u &&
          runtime.cleaned_tree == 0x5028u &&
          runtime.cleaned_root == 0x7100u &&
          runtime.calls == std::vector<std::string>{
              "cleanup-nodes", "reset-begin", "free"},
          "nonempty tree cleans subtree and retains shared nil");
    Check(runtime.words.at(0x7004u) == 0x6000u &&
          runtime.words.at(0x7000u) == 0x7000u &&
          runtime.words.at(0x7008u) == 0x7000u &&
          runtime.words.at(0x502cu) == 0u &&
          runtime.words.at(0x5034u) == 0u &&
          runtime.freed == std::vector<std::uint32_t>{0x7000u},
          "tree header reset and release fields");
}

void DestroyEmptyLastAndCursorFallback() {
    ObjectRuntime runtime;
    runtime.words = {{0x502cu, 0x7000u}, {0x5034u, 0u},
                     {0x7000u, 0x7000u}};
    pal4::NpcManagerTreeGlobals globals{1u, 0x6000u};
    Check(pal4::NpcManagerObject::DestroyTree(
              0x5028u, globals, runtime) == 0xf00du &&
          globals.instance_count == 0u && globals.nil_address == 0u &&
          runtime.freed == std::vector<std::uint32_t>{
              0x7000u, 0x6000u},
          "last empty tree releases header and shared nil");

    ObjectRuntime cursor;
    cursor.words = {{0x502cu, 0x7000u}, {0x5034u, 0u},
                    {0x7000u, 0x7100u}};
    cursor.next_nodes = {{0x7100u, 0x7200u},
                         {0x7200u, 0x7000u}};
    pal4::NpcManagerTreeGlobals shared{2u, 0x6000u};
    Check(pal4::NpcManagerObject::DestroyTree(
              0x5028u, shared, cursor) == 1u &&
          cursor.visited ==
              std::vector<std::uint32_t>{0x7100u, 0x7200u} &&
          cursor.deleted == cursor.visited &&
          cursor.calls == std::vector<std::string>{
              "next", "delete", "next", "delete", "free"},
          "zero-count cursor fallback deletes each node before header");
}

void VirtualDestructorOrder() {
    ObjectRuntime runtime;
    runtime.words = {{0x502cu, 0x7000u}, {0x5034u, 0u},
                     {0x7000u, 0x7000u}};
    pal4::NpcManagerTreeGlobals globals{2u, 0x6000u};
    Check(pal4::NpcManagerObject::Destroy(
              0x5000u, globals, runtime) == 0xbeefu &&
          runtime.words.at(0x5000u) == 0x842e64u &&
          runtime.calls == std::vector<std::string>{
              "vtable", "cleanup-grid", "free", "cleanup-base"} &&
          globals.instance_count == 1u &&
          globals.nil_address == 0x6000u,
          "virtual destructor composes vtable, grid, tree and base cleanup");
}

void ActionCallbackAndInitializer() {
    SingletonRuntime singleton;
    pal4::NpcManagerAccessState access{1u, 0x9000u};
    ActionRuntime action;
    action.words = {{0xa000u, 0x1100u}, {0xa00cu, 0x2200u}};
    Check(pal4::NpcManagerActionCallback::Process(
              0xa000u, access, singleton, action) == 0xa000u &&
          action.reads ==
              std::vector<std::uint32_t>{0xa00cu, 0xa000u} &&
          action.calls ==
              std::vector<std::vector<std::uint32_t>>{
                  {0x9000u, 0x1100u, 0x2200u,
                   0xa024u, 0xa030u, 0xa000u, 1u}} &&
          singleton.calls.empty(),
          "NPC record callback forwards name, resource and two transforms");

    InitializeRuntime runtime;
    Check(pal4::NpcManagerInitializer::Initialize(
              0x5000u, 0x1000u, 0x2000u, runtime) == 1u &&
          runtime.calls ==
              std::vector<std::vector<std::uint32_t>>{
                  {1u, 0x5000u},
                  {2u, 0x5000u, 0x1000u, 0x2000u},
                  {3u, 0x5038u},
                  {4u, 0x5000u, 0x43c650u, 0u}},
          "NPC scene init uses cleanup-load-grid-record order");
}

void CleanupNonemptyTree() {
    CleanupRuntime runtime;
    runtime.words = {
        {0x502cu, 0x7000u}, {0x5034u, 1u},
        {0x7000u, 0x7100u}, {0x7004u, 0x7100u},
        {0x7100u, 0x6000u}, {0x7108u, 0x6000u},
        {0x711cu, 0x9000u}};
    runtime.next_nodes[0x7100u] = 0x7000u;
    pal4::NpcManagerTreeGlobals globals{1u, 0x6000u};
    Check(pal4::NpcManagerCleanup::Clear(
              0x5000u, globals, runtime) == 0x88u &&
          runtime.calls == std::vector<std::string>{
              "notify", "destroy-npc", "next", "cleanup-nodes",
              "free-text", "free", "cleanup-complex", "clear-grid",
              "previous-grid", "reset-grid"} &&
          runtime.freed == std::vector<std::uint32_t>{0x7100u},
          "cleanup destroys NPCs before tree nodes and grid reset");
    Check(runtime.words.at(0x7000u) == 0x7000u &&
          runtime.words.at(0x7004u) == 0x6000u &&
          runtime.words.at(0x7008u) == 0x7000u &&
          runtime.words.at(0x5034u) == 0u &&
          globals.nil_address == 0x6000u,
          "cleanup retains tree header and shared nil");
}

void CleanupEmptyAndFallback() {
    CleanupRuntime empty;
    empty.words = {{0x502cu, 0x7000u}, {0x5034u, 0u},
                   {0x7000u, 0x7000u}};
    pal4::NpcManagerTreeGlobals globals{1u, 0x6000u};
    Check(pal4::NpcManagerCleanup::Clear(
              0x5000u, globals, empty) == 0x88u &&
          empty.calls == std::vector<std::string>{
              "cleanup-complex", "clear-grid",
              "previous-grid", "reset-grid"},
          "empty cleanup performs only complex tree and grid reset");

    CleanupRuntime fallback;
    fallback.words = {{0x502cu, 0x7000u}, {0x5034u, 0u},
                      {0x7000u, 0x7100u}, {0x711cu, 0x9000u}};
    fallback.next_nodes[0x7100u] = 0x7000u;
    Check(pal4::NpcManagerCleanup::Clear(
              0x5000u, globals, fallback) == 0x88u &&
          fallback.calls == std::vector<std::string>{
              "notify", "destroy-npc", "next", "next",
              "delete-node", "cleanup-complex", "clear-grid",
              "previous-grid", "reset-grid"},
          "zero-count fallback still visits and deletes header-linked node");
}

}  // namespace

int main() {
    SingletonColdWarmFailure();
    FactoryAndConstructor();
    FactoryFailureAndSharedNilReuse();
    DestroyNonemptyTree();
    DestroyEmptyLastAndCursorFallback();
    VirtualDestructorOrder();
    ActionCallbackAndInitializer();
    CleanupNonemptyTree();
    CleanupEmptyAndFallback();
    std::cout << "NPC manager access cases passed\n";
}
