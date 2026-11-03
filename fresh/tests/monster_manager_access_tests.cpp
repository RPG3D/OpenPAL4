#include "pal4/monster_manager_access.h"

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

struct SingletonRuntime final : pal4::MonsterManagerSingletonRuntimeStub {
    std::uint32_t SingletonManager() override {
        events.emplace_back("manager");
        if (state != nullptr) {
            Check(pal4::MonsterManagerAccess::Get(*state, *this) == 0x7788u,
                  "guard is set before manager lookup");
        }
        return 0x5000u;
    }
    std::uint32_t Register(std::uint32_t manager, std::string_view name,
                           std::uint32_t id, std::uint32_t factory,
                           std::uint32_t type_info) override {
        events.emplace_back("register");
        Check(manager == 0x5000u && name == "palMonsterManager" &&
                  id == 0x411u && factory == 0x4377b0u &&
                  type_info == 0x7de5d0u,
              "registration preserves all IDA operands");
        return result;
    }
    pal4::MonsterManagerAccessState* state{};
    std::uint32_t result{0x9000u};
    std::vector<std::string> events;
};

struct ObjectRuntime final : pal4::MonsterManagerObjectRuntimeStub {
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.emplace_back("allocate:" + std::to_string(bytes));
        if (next_allocation == allocations.size()) return 0u;
        return allocations[next_allocation++];
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.emplace_back("free");
        freed.push_back(address);
        return 0xf00du;
    }
    void InitializeTreeBase(std::uint32_t object) override {
        events.emplace_back("tree");
        tree_objects.push_back(object);
    }
    void InitializeGrid(std::uint32_t grid) override {
        events.emplace_back("grid");
        grid_objects.push_back(grid);
    }
    void CleanupGrid(std::uint32_t grid) override {
        events.emplace_back("cleanup-grid");
        grid_objects.push_back(grid);
    }
    std::uint32_t CleanupTreeBase(std::uint32_t object) override {
        events.emplace_back("cleanup-base");
        tree_objects.push_back(object);
        return 0xbeefu;
    }
    void CleanupTreeNodes(std::uint32_t tree,
                          std::uint32_t root) override {
        events.emplace_back("cleanup-nodes");
        cleaned_tree = tree;
        cleaned_root = root;
    }
    void ResetTreeBegin(std::uint32_t tree) override {
        events.emplace_back("reset-begin");
        reset_tree = tree;
    }
    std::uint32_t NextTreeNode(std::uint32_t node) override {
        events.emplace_back("next");
        visited.push_back(node);
        return next_nodes.at(node);
    }
    void DeleteTreeNode(std::uint32_t tree,
                        std::uint32_t node) override {
        events.emplace_back("delete");
        deleted_tree = tree;
        deleted.push_back(node);
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        if (address == 0x5000u) events.emplace_back("vtable");
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::vector<std::uint32_t> allocations;
    std::size_t next_allocation{};
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> tree_objects;
    std::vector<std::uint32_t> grid_objects;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, std::uint32_t> next_nodes;
    std::vector<std::uint32_t> visited;
    std::vector<std::uint32_t> deleted;
    std::uint32_t cleaned_tree{};
    std::uint32_t cleaned_root{};
    std::uint32_t reset_tree{};
    std::uint32_t deleted_tree{};
    std::vector<std::string> events;
};

struct ActionRuntime final : pal4::MonsterManagerActionRuntimeStub {
    std::uint32_t ReadDword(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
    void CreateMonster(std::uint32_t manager, std::uint32_t name,
                       std::uint32_t secondary_name,
                       std::uint32_t position, std::uint32_t facing,
                       std::uint32_t record, std::uint32_t enabled) override {
        calls.push_back({manager, name, secondary_name, position,
                         facing, record, enabled});
    }
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::vector<std::vector<std::uint32_t>> calls;
};

struct InitializeRuntime final : pal4::MonsterManagerInitializeRuntimeStub {
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
    void ForEachMonster(std::uint32_t manager,
                        std::uint32_t callback,
                        std::uint32_t context) override {
        calls.push_back({4u, manager, callback, context});
    }
    std::vector<std::vector<std::uint32_t>> calls;
};

void TestSingletonColdWarmAndFailure() {
    SingletonRuntime runtime;
    pal4::MonsterManagerAccessState state{0x80u, 0x7788u};
    runtime.state = &state;
    Check(pal4::MonsterManagerAccess::Get(state, runtime) == 0x9000u &&
              state.guard == 0x81u && state.instance == 0x9000u &&
              runtime.events == std::vector<std::string>{"manager", "register"},
          "first lookup registers and caches with other guard bits intact");
    runtime.events.clear();
    Check(pal4::MonsterManagerAccess::Get(state, runtime) == 0x9000u &&
              runtime.events.empty(), "warm lookup reads the cached instance");

    pal4::MonsterManagerAccessState failed{};
    runtime.state = nullptr;
    runtime.result = 0u;
    Check(pal4::MonsterManagerAccess::Get(failed, runtime) == 0u &&
              failed.guard == 1u && failed.instance == 0u,
          "failed registration still marks the guard");
    runtime.events.clear();
    Check(pal4::MonsterManagerAccess::Get(failed, runtime) == 0u &&
              runtime.events.empty(), "failed registration is not retried");
}

void TestFactoryAndFirstConstructor() {
    ObjectRuntime runtime;
    runtime.allocations = {0x5000u, 0x6000u, 0x7000u};
    pal4::MonsterManagerTreeGlobals globals{};
    Check(pal4::MonsterManagerObject::Create(globals, 0xa5u, runtime) ==
              0x5000u, "factory returns constructed storage");
    Check(runtime.events == std::vector<std::string>{
              "allocate:96", "tree", "allocate:36", "allocate:36",
              "grid", "vtable"},
          "constructor preserves allocation and initialization order");
    Check(globals.instance_count == 1u && globals.nil_address == 0x6000u &&
              runtime.tree_objects == std::vector<std::uint32_t>{0x5000u} &&
              runtime.grid_objects == std::vector<std::uint32_t>{0x5038u},
          "constructor creates first shared nil and grid subobject");
    Check(runtime.bytes.at(0x5028u) == 0xa5u &&
              runtime.bytes.at(0x5029u) == 0xa5u &&
              runtime.bytes.at(0x5030u) == 0u,
          "observed stack byte and reset flag occupy exact offsets");
    Check(runtime.words.at(0x6000u) == 0u &&
              runtime.words.at(0x6004u) == 0u &&
              runtime.words.at(0x6008u) == 0u &&
              runtime.words.at(0x6020u) == 1u,
          "first shared nil has observed field values");
    Check(runtime.words.at(0x7000u) == 0x7000u &&
              runtime.words.at(0x7004u) == 0x6000u &&
              runtime.words.at(0x7008u) == 0x7000u &&
              runtime.words.at(0x7020u) == 0u &&
              runtime.words.at(0x502cu) == 0x7000u &&
              runtime.words.at(0x5034u) == 0u &&
              runtime.words.at(0x5000u) == 0x842da0u,
          "tree header, object fields and final vtable match IDA");
}

void TestSharedNilReuse() {
    ObjectRuntime runtime;
    runtime.allocations = {0x8100u, 0x8200u};
    pal4::MonsterManagerTreeGlobals globals{3u, 0x6000u};
    Check(pal4::MonsterManagerObject::Construct(
              0x8000u, globals, 0x34u, runtime) == 0x8000u &&
              globals.instance_count == 4u && globals.nil_address == 0x6000u,
          "existing shared nil is retained and counted");
    Check(runtime.freed == std::vector<std::uint32_t>{0x8100u} &&
              runtime.words.at(0x8104u) == 0u &&
              runtime.words.at(0x8120u) == 1u &&
              runtime.words.find(0x8100u) == runtime.words.end() &&
              runtime.words.at(0x8204u) == 0x6000u,
          "temporary nil receives two writes before being freed");
}

void TestFactoryAllocationFailure() {
    ObjectRuntime runtime;
    pal4::MonsterManagerTreeGlobals globals{};
    Check(pal4::MonsterManagerObject::Create(globals, 0u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"allocate:96"} &&
              globals.instance_count == 0u,
          "failed object allocation skips construction");
}

void TestTreeDestructorWithNodes() {
    ObjectRuntime runtime;
    runtime.words = {
        {0x502cu, 0x7000u}, {0x5034u, 2u},
        {0x7000u, 0x7000u}, {0x7004u, 0x7100u}};
    pal4::MonsterManagerTreeGlobals globals{2u, 0x6000u};
    Check(pal4::MonsterManagerObject::DestroyTree(
              0x5028u, globals, runtime) == 1u,
          "direct tree destructor returns remaining shared count");
    Check(runtime.cleaned_tree == 0x5028u &&
              runtime.cleaned_root == 0x7100u &&
              runtime.reset_tree == 0x5028u &&
              runtime.events == std::vector<std::string>{
                  "cleanup-nodes", "reset-begin", "free"},
          "nonempty tree cleans nodes and resets begin before header free");
    Check(runtime.words.at(0x7004u) == 0x6000u &&
              runtime.words.at(0x7000u) == 0x7000u &&
              runtime.words.at(0x7008u) == 0x7000u &&
              runtime.words.at(0x502cu) == 0u &&
              runtime.words.at(0x5034u) == 0u &&
              runtime.freed == std::vector<std::uint32_t>{0x7000u} &&
              globals.nil_address == 0x6000u,
          "tree header and shared nil state match direct destructor");
}

void TestEmptyTreeLastInstance() {
    ObjectRuntime runtime;
    runtime.words = {
        {0x502cu, 0x7000u}, {0x5034u, 0u}, {0x7000u, 0x7000u}};
    pal4::MonsterManagerTreeGlobals globals{1u, 0x6000u};
    Check(pal4::MonsterManagerObject::DestroyTree(
              0x5028u, globals, runtime) == 0xf00du &&
              globals.instance_count == 0u && globals.nil_address == 0u &&
              runtime.freed == std::vector<std::uint32_t>{0x7000u, 0x6000u},
          "last tree releases the header and shared nil");
    Check(runtime.events == std::vector<std::string>{"free", "free"},
          "empty tree has no node cleanup calls");
}

void TestCursorDeletionAndVirtualDestructor() {
    ObjectRuntime runtime;
    runtime.words = {
        {0x502cu, 0x7000u}, {0x5034u, 0u}, {0x7000u, 0x7100u}};
    runtime.next_nodes = {{0x7100u, 0x7200u}, {0x7200u, 0x7000u}};
    pal4::MonsterManagerTreeGlobals globals{2u, 0x6000u};
    Check(pal4::MonsterManagerObject::Destroy(
              0x5000u, globals, runtime) == 0xbeefu,
          "virtual destructor returns base cleanup result");
    Check(runtime.visited == std::vector<std::uint32_t>{0x7100u, 0x7200u} &&
              runtime.deleted == runtime.visited &&
              runtime.deleted_tree == 0x5028u,
          "inconsistent empty-count tree walks and deletes each node");
    Check(runtime.events == std::vector<std::string>{
              "vtable", "cleanup-grid", "next", "delete", "next",
              "delete", "free", "cleanup-base"} &&
              runtime.grid_objects == std::vector<std::uint32_t>{0x5038u},
          "virtual destructor preserves grid-tree-base cleanup order");
}

void TestActionCallback() {
    SingletonRuntime singleton;
    pal4::MonsterManagerAccessState access{1u, 0x9000u};
    ActionRuntime action;
    action.words = {{0xa000u, 0x1100u}, {0xa00cu, 0x2200u}};
    Check(pal4::MonsterManagerActionCallback::Process(
              0xa000u, access, singleton, action) == 0xa000u &&
              action.reads == std::vector<std::uint32_t>{0xa00cu, 0xa000u} &&
              action.calls == std::vector<std::vector<std::uint32_t>>{
                  {0x9000u, 0x1100u, 0x2200u, 0xa018u,
                   0xa024u, 0xa000u, 1u}} && singleton.events.empty(),
          "action callback forwards record fields and preserves return");
}

void TestManagerInitialize() {
    InitializeRuntime runtime;
    Check(pal4::MonsterManagerInitializer::Initialize(
              0x5000u, 0x1000u, 0x2000u, runtime) == 1u &&
              runtime.calls == std::vector<std::vector<std::uint32_t>>{
                  {1u, 0x5000u},
                  {2u, 0x5000u, 0x1000u, 0x2000u},
                  {3u, 0x5038u},
                  {4u, 0x5000u, 0x437af0u, 0u}},
          "manager initialization uses cleanup-load-grid-walk order");
}

}  // namespace

int main() {
    TestSingletonColdWarmAndFailure();
    TestFactoryAndFirstConstructor();
    TestSharedNilReuse();
    TestFactoryAllocationFailure();
    TestTreeDestructorWithNodes();
    TestEmptyTreeLastInstance();
    TestCursorDeletionAndVirtualDestructor();
    TestActionCallback();
    TestManagerInitialize();
}
