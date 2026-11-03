#include "pal4/monster_behavior_manager.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct ProcessorRuntimeProbe final : pal4::IScriptEventProcessorRuntime {
    explicit ProcessorRuntimeProbe(std::vector<std::string>& log) : calls(log) {}
    std::vector<std::string>& calls;
    bool throw_on_document{};

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("processor-allocate:" + std::to_string(bytes));
        return 0x9000u;
    }
    std::uint32_t InitializeDocument(std::uint32_t address,
                                     const char* name,
                                     std::uint32_t option) override {
        Check(address == 0x9000u &&
                  std::string(name) == "behAIAtomicMgr-memory" &&
                  option == 0u,
              "processor document arguments");
        calls.push_back("processor-document");
        if (throw_on_document)
            throw std::runtime_error("document");
        return 0x9100u;
    }
    void CleanupDocument(std::uint32_t address) override {
        Check(address == 0x9100u, "processor cleanup document address");
        calls.push_back("processor-cleanup-document");
    }
    void DestroyDocument(std::uint32_t address) override {
        Check(address == 0x9100u, "processor destroy document address");
        calls.push_back("processor-destroy-document");
    }
    std::uint32_t ReadDword(std::uint32_t) override {
        Check(false, "no event pointer read during constructor cleanup");
        return 0u;
    }
    void CleanupEventData(std::uint32_t) override {
        Check(false, "no event data during constructor cleanup");
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("processor-free:" + std::to_string(address));
        return 0u;
    }
};

struct ScriptRuntimeProbe final : pal4::IScriptObjectRuntime {
    explicit ScriptRuntimeProbe(std::vector<std::string>& log) : calls(log) {}
    std::vector<std::string>& calls;
    void InitializeTaskContainer(std::uint32_t address) override {
        Check(address == 0x5688u, "script object offset");
        calls.push_back("script-init");
    }
    std::uint32_t DestroyTaskContainer(std::uint32_t address) override {
        Check(address == 0x5688u, "script object cleanup offset");
        calls.push_back("script-destroy");
        return 0u;
    }
};

struct SchedulerRuntimeProbe final : pal4::IScriptEventSchedulerRuntime {
    explicit SchedulerRuntimeProbe(std::vector<std::string>& log) : calls(log) {}
    std::vector<std::string>& calls;
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x5694u, "script task context byte");
        calls.push_back("scheduler-read-byte");
        return 0u;
    }
    std::uint32_t ReadDword(std::uint32_t) override { return 0u; }
    std::int32_t Random() override { return 0; }
    std::uint32_t GetConditionDocument(std::uint32_t) override { return 0u; }
    std::uint32_t CallCondition(std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    void InsertCandidate(pal4::ScriptEventProcessorState&,
                         std::uint32_t, std::uint32_t) override {}
    std::uint32_t DispatchEvent(std::uint32_t, std::uint32_t,
                                std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    std::uint32_t UpdateContainer(std::uint32_t address,
                                  float elapsed) override {
        Check(address == 0x5688u && elapsed == 0.5f,
              "script scheduler context and elapsed time");
        calls.push_back("scheduler-update");
        return 0x55u;
    }
};

struct Runtime final : pal4::IMonsterBehaviorManagerRuntime {
    std::vector<std::string> calls;
    ProcessorRuntimeProbe processor{calls};
    ScriptRuntimeProbe script{calls};
    SchedulerRuntimeProbe scheduler{calls};
    int throw_on_insert{-1};
    bool throw_on_tree{};
    std::uint32_t state_gate{1u};
    std::uint32_t config_attribute{};
    std::int32_t config_value{};
    std::uint32_t action_result{};
    std::uint32_t animation_z_result{};
    std::uint32_t animation_c_result{88u};
    std::uint32_t collision_result{23u};
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t first_node{0x8100u};
    std::uint32_t next_node{0x8000u};

    pal4::IScriptEventProcessorRuntime& ProcessorRuntime() override {
        return processor;
    }
    pal4::IScriptObjectRuntime& ScriptRuntime() override { return script; }
    std::uint8_t ProcessorIncomingByte() override { return 0x44u; }
    pal4::MonsterBehaviorTreeBytes TreeInputBytes() override {
        calls.push_back("tree-inputs");
        return {0x12u, 0x34u};
    }
    void ConstructBase(pal4::MonsterBehaviorManagerState& state,
                       std::uint32_t address) override {
        Check(address == 0x5000u, "manager object address");
        calls.push_back("base-init");
        state.words[10] = 0x12345678u;
    }
    void DestructBase(pal4::MonsterBehaviorManagerState&,
                      std::uint32_t address) override {
        Check(address == 0x5000u, "base cleanup address");
        calls.push_back("base-destroy");
    }
    void InitializeTree(pal4::MonsterBehaviorManagerState& state,
                        std::uint32_t address) override {
        Check(address == 0x569cu &&
                  state.words[0x69cu / 4u] == 0xa5a53412u &&
                  state.words[0x6a4u / 4u] == 0xa5a5a500u,
              "tree input bytes are written before tree initialization");
        calls.push_back("tree-init");
        if (throw_on_tree)
            throw std::runtime_error("tree");
    }
    void CleanupTree(pal4::MonsterBehaviorManagerState&,
                     std::uint32_t address) override {
        Check(address == 0x5698u, "tree cleanup subobject address");
        calls.push_back("tree-cleanup");
    }
    void DestroyAuxiliary(pal4::MonsterBehaviorManagerState&,
                          std::uint32_t address) override {
        Check(address == 0x56d4u, "auxiliary cleanup address");
        calls.push_back("aux-destroy");
    }
    void CleanupWordArray(pal4::MonsterBehaviorManagerState&,
                          std::uint32_t address) override {
        Check(address == 0x5710u, "first trailing cleanup address");
        calls.push_back("word-cleanup");
    }
    void CleanupHistory(pal4::MonsterBehaviorManagerState&,
                        std::uint32_t address) override {
        Check(address == 0x5720u, "second trailing cleanup address");
        calls.push_back("history-cleanup");
    }
    void InsertBehavior(std::uint32_t tree_address,
                        std::uint32_t class_id,
                        std::uint32_t factory_address) override {
        Check(tree_address == 0x569cu && class_id < 4u &&
                  factory_address ==
                      pal4::MonsterBehaviorManager::kFactories[class_id],
              "behavior descriptor address, id and factory");
        calls.push_back("insert:" + std::to_string(class_id));
        if (static_cast<int>(class_id) == throw_on_insert)
            throw std::runtime_error("insert");
    }
    void SetRecordErrorFlag(std::uint32_t address,
                            std::uint8_t value) override {
        Check(value == 1u, "record cleanup flag");
        calls.push_back("record:" + std::to_string(address));
    }
    void FreeMemory(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return memory.at(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        memory[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    std::uint32_t TreeFirst(std::uint32_t address) override {
        Check(address == 0x569cu, "behavior tree address");
        calls.push_back("tree-first");
        return first_node;
    }
    std::uint32_t TreeNext(std::uint32_t address) override {
        calls.push_back("tree-next:" + std::to_string(address));
        return next_node;
    }
    std::uint32_t TreeNilAddress() override { return 0x8200u; }
    void TreeEraseSubtree(std::uint32_t address,
                          std::uint32_t root) override {
        Check(address == 0x569cu, "erase tree address");
        calls.push_back("tree-erase:" + std::to_string(root));
    }
    void TreeRemoveNode(std::uint32_t address,
                        std::uint32_t node) override {
        Check(address == 0x569cu, "remove tree address");
        calls.push_back("tree-remove:" + std::to_string(node));
    }
    void ReleaseSharedTreeNil() override {
        calls.push_back("tree-release-nil");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte-write:" + std::to_string(address));
    }
    void FreeStringStorage(std::uint32_t record,
                           std::uint32_t allocation,
                           std::uint32_t size) override {
        calls.push_back("string-free:" + std::to_string(record) + ":" +
                        std::to_string(allocation) + ":" +
                        std::to_string(size));
    }
    std::uint32_t ResetBaseObject(pal4::MonsterBehaviorManagerState&,
                                  std::uint32_t address) override {
        Check(address == 0x5000u, "base reset address");
        calls.push_back("base-reset");
        return 0x1234u;
    }
    std::uint32_t ProcessManagerState(
        pal4::MonsterBehaviorManagerState& state,
        std::uint32_t address, float elapsed) override {
        Check(address == 0x5000u && elapsed == 0.5f &&
                  state.words[0x1f0u / 4u] != 0u,
              "manager state receives owner and elapsed time");
        calls.push_back("state-gate");
        return state_gate;
    }
    void CheckStateAndProcessCollision(pal4::MonsterBehaviorManagerState& state,
                                       std::uint32_t address, float elapsed) override {
        Check(elapsed == 0.5f && address == 0x5000u &&
                  state.words[0x1f0u / 4u] != 0u,
              "collision callback owner and elapsed time");
        calls.push_back("collision");
    }
    pal4::IScriptEventSchedulerRuntime& SchedulerRuntime() override {
        return scheduler;
    }
    std::uint32_t UpdatePosition(std::uint32_t address, float elapsed,
                                 std::uint32_t argument) override {
        Check(address == 0x5000u && elapsed == 0.5f && argument == 0x99u,
              "position update arguments");
        calls.push_back("position");
        return 0x7777u;
    }
    void CleanupScriptObjectCollection(std::uint32_t address) override {
        Check(address == 0x5688u, "script collection cleanup address");
        calls.push_back("script-collection-cleanup");
    }
    std::uint32_t XmlChildByIndex(std::uint32_t parent,
                                  std::uint32_t index) override {
        Check(parent == 0x7000u && index == 0u,
              "first XML behavior child lookup");
        calls.push_back("xml-first-child");
        return 0x7100u;
    }
    void LoadFuzzyBehaviorConfig(std::uint32_t address,
                                 std::uint32_t child) override {
        Check(address == 0x5000u && child == 0x7100u,
              "MST fuzzy behavior configuration arguments");
        calls.push_back("load-fuzzy");
    }
    void DispatchComponentField(std::uint32_t address,
                                std::uint32_t slot,
                                std::uint32_t field) override {
        Check(address == 0x5000u &&
                  ((slot == 4u && field == 0x6018u) ||
                   (slot == 8u && field == 0x6024u)),
              "component virtual field dispatch arguments");
        calls.push_back("field:" + std::to_string(slot));
    }
    std::uint32_t GetConfigAttribute(std::uint32_t config,
                                     const char* name) override {
        Check(config == 0x7000u &&
                  std::string(name) == "PAL4_MST_NEED_STORAGE",
              "storage action config key");
        calls.push_back("config-attribute");
        return config_attribute;
    }
    std::int32_t ParseConfigInt(std::uint32_t attribute,
                                std::int32_t fallback) override {
        Check(attribute == 0x7100u && fallback == 0,
              "storage action integer default");
        calls.push_back("config-int");
        return config_value;
    }
    std::uint32_t CallActionSlot4(std::uint32_t action) override {
        Check(action == 0x8000u, "storage action address");
        calls.push_back("action-slot-4");
        return action_result;
    }
    std::uint32_t SelectAnimation(std::uint32_t address,
                                  const char* name) override {
        Check(address == 0x5170u, "animation subobject address");
        calls.push_back(std::string("animation:") + name);
        if (std::string(name) == "Z02") return animation_z_result;
        Check(std::string(name) == "C03", "fallback animation name");
        return animation_c_result;
    }
    std::uint32_t ProcessCollisionAndConfig(
        std::uint32_t address, float incoming_st0,
        float constant_one) override {
        Check(address == 0x5000u && incoming_st0 == 0.25f &&
                  constant_one == 1.0f,
              "stop-pursuit collision arguments");
        calls.push_back("stop-collision");
        return collision_result;
    }
};

using Manager = pal4::MonsterBehaviorManager;
using State = pal4::MonsterBehaviorManagerState;

State FilledState() {
    State state;
    state.words.fill(0xa5a5a5a5u);
    return state;
}

void TestConstruct() {
    auto state = FilledState();
    Runtime runtime;
    Check(Manager::Construct(state, 0x5000u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "base-init", "processor-allocate:60", "processor-document",
                  "script-init", "tree-inputs", "tree-init",
                  "insert:0", "insert:1", "insert:2", "insert:3"},
          "constructor composes subobjects and registers exactly classIDs 0..3");
    Check(state.words[0] == 0x842c98u &&
              state.words[0x170u / 4u] == 0x842c8cu &&
              state.words[0x1ecu / 4u] == 0x842c80u &&
              state.words[0x204u / 4u] == 0u &&
              state.words[0x654u / 4u] == 0u &&
              state.words[0x658u / 4u] == 0xa5a5a544u &&
              state.words[(0x658u + 16u) / 4u] == 0x9100u &&
              state.words[0x69cu / 4u] == 0xa5a53412u &&
              state.words[0x6acu / 4u] == 0x842d28u &&
              state.words[0x6b0u / 4u] == 0x842d1cu &&
              state.words[0x6b8u / 4u] == 0x842d10u &&
              state.words[0x6c0u / 4u] == 0x842d00u &&
              state.words[0x6c8u / 4u] == 0x842cf0u &&
              state.words[0x6ccu / 4u] == 0x5000u &&
              state.words[0x6d4u / 4u] == 0x849384u &&
              state.words[0x6d8u / 4u] == 0u &&
              state.words[0x6dcu / 4u] == 0x5000u &&
              state.words[0x710u / 4u] == 0xa5a5a534u &&
              state.words[0x720u / 4u] == 0xa5a5a534u &&
              state.words[0x6f8u / 4u] == 0x3f800000u &&
              state.words[0x6fcu / 4u] == 0x3f800000u &&
              state.words[0x738u / 4u] == 0u &&
              state.words[10] == 0x12345678u,
          "constructor writes exact vtables, subobject fields and scalar tail");
}

void TestTreeFailureUnwindsPriorStages() {
    auto state = FilledState();
    Runtime runtime;
    runtime.throw_on_tree = true;
    try {
        (void)Manager::Construct(state, 0x5000u, runtime);
        Check(false, "tree exception must escape");
    } catch (const std::runtime_error&) {
    }
    Check(runtime.calls == std::vector<std::string>{
              "base-init", "processor-allocate:60", "processor-document",
              "script-init", "tree-inputs", "tree-init", "script-destroy",
              "processor-cleanup-document", "processor-destroy-document",
              "processor-free:37120", "processor-free:0", "processor-free:0",
              "base-destroy"},
          "tree failure unwinds script, processor and base without tree cleanup");
}

void TestRegistrationFailureUnwindsAllStages() {
    auto state = FilledState();
    Runtime runtime;
    runtime.throw_on_insert = 2;
    try {
        (void)Manager::Construct(state, 0x5000u, runtime);
        Check(false, "registration exception must escape");
    } catch (const std::runtime_error&) {
    }
    const auto history = std::find(runtime.calls.begin(), runtime.calls.end(),
                                   "history-cleanup");
    Check(history != runtime.calls.end() &&
              std::vector<std::string>(history, runtime.calls.end()) ==
                  std::vector<std::string>{
                      "history-cleanup", "word-cleanup", "aux-destroy",
                      "tree-cleanup", "script-destroy",
                      "processor-cleanup-document", "processor-destroy-document",
                      "processor-free:37120", "processor-free:0",
                      "processor-free:0", "base-destroy"} &&
              runtime.calls[runtime.calls.size() - 12u] == "insert:2",
          "registration failure unwinds trailing fields and subobjects in reverse");
}

void TestProcessorFailureUnwindsBase() {
    auto state = FilledState();
    Runtime runtime;
    runtime.processor.throw_on_document = true;
    try {
        (void)Manager::Construct(state, 0x5000u, runtime);
        Check(false, "processor exception must escape");
    } catch (const std::runtime_error&) {
    }
    Check(runtime.calls == std::vector<std::string>{
              "base-init", "processor-allocate:60", "processor-document",
              "processor-free:36864", "base-destroy"},
          "processor failure releases its allocation then unwinds the base");
}

void PrepareForDestroy(State& state, Runtime& runtime,
                       std::uint32_t count) {
    (void)Manager::Construct(state, 0x5000u, runtime);
    runtime.calls.clear();
    state.words[0x724u / 4u] = 0xa000u;
    state.words[0x728u / 4u] = 0xa020u;
    state.words[0x72cu / 4u] = 0xa040u;
    state.words[0x714u / 4u] = 0xb000u;
    state.words[0x718u / 4u] = 0xb010u;
    state.words[0x71cu / 4u] = 0xb020u;
    state.words[0x6a0u / 4u] = 0x8000u;
    state.words[0x6a8u / 4u] = count;
    runtime.memory[0x8000u] = count ? 0x8100u : 0x8000u;
    runtime.memory[0x8004u] = 0x8100u;
}

void TestDestroyPopulatedTree() {
    auto state = FilledState();
    Runtime runtime;
    PrepareForDestroy(state, runtime, 2u);
    Manager::Destroy(state, 0x5000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "record:40960", "record:40976", "free:40960",
              "free:45056", "aux-destroy", "tree-first",
              "tree-erase:33024", "write:32772", "write:32768",
              "write:32776", "tree-first", "free:32768",
              "tree-release-nil", "script-destroy",
              "processor-cleanup-document", "processor-destroy-document",
              "processor-free:37120", "processor-free:0",
              "processor-free:0", "base-destroy"} &&
              runtime.memory[0x8000u] == 0x8000u &&
              runtime.memory[0x8004u] == 0x8200u &&
              runtime.memory[0x8008u] == 0x8000u &&
              state.words[0x6a8u / 4u] == 0u &&
              state.words[0x6a0u / 4u] == 0u &&
              state.words[0x724u / 4u] == 0u &&
              state.words[0x714u / 4u] == 0u,
          "destructor releases arrays, populated tree and subobjects in order");
}

void TestDestroyEmptyTree() {
    auto state = FilledState();
    Runtime runtime;
    PrepareForDestroy(state, runtime, 0u);
    state.words[0x724u / 4u] = 0u;
    state.words[0x728u / 4u] = 0u;
    Manager::Destroy(state, 0x5000u, runtime);
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "tree-first") == runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                    "tree-erase:33024") == runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                    "free:0") != runtime.calls.end(),
          "empty tree skips node teardown but still releases header");
}

void TestDestroyTreeFallback() {
    auto state = FilledState();
    Runtime runtime;
    PrepareForDestroy(state, runtime, 1u);
    runtime.first_node = 0x8300u;
    Manager::Destroy(state, 0x5000u, runtime);
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "tree-next:33024") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                    "tree-remove:33024") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                    "tree-erase:33024") == runtime.calls.end(),
          "fallback advances iterator before removing its old node");
}

void TestResetReleasesStringReferences() {
    auto state = FilledState();
    Runtime runtime;
    state.words[0x654u / 4u] = 0u;
    state.words[0x714u / 4u] = 0x9000u;
    state.words[0x718u / 4u] = 0x9010u;
    state.words[0x724u / 4u] = 0xa000u;
    state.words[0x728u / 4u] = 0xa040u;
    runtime.memory[0xa004u] = 0xb001u;
    runtime.memory[0xa00cu] = 5u;
    runtime.memory[0xa014u] = 0xb101u;
    runtime.memory[0xa01cu] = 6u;
    runtime.memory[0xa024u] = 0xb201u;
    runtime.memory[0xa02cu] = 7u;
    runtime.memory[0xa034u] = 0u;
    runtime.bytes[0xb000u] = 0u;
    runtime.bytes[0xb100u] = 0xffu;
    runtime.bytes[0xb200u] = 2u;

    Check(Manager::Reset(state, 0x5000u, runtime) == 0x1234u &&
              runtime.calls.front() ==
                  "string-free:40960:45056:7" &&
              runtime.calls.back() == "base-reset" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "string-free:40960:45056:7") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "string-free:40976:45312:8") != runtime.calls.end() &&
              runtime.bytes[0xb200u] == 1u &&
              runtime.memory[0xa004u] == 0u &&
              runtime.memory[0xa014u] == 0u &&
              runtime.memory[0xa024u] == 0u &&
              runtime.memory[0xa034u] == 0u &&
              state.words[0x718u / 4u] == 0x9000u &&
              state.words[0x728u / 4u] == 0xa000u,
          "reset releases zero, sentinel and shared string references");
}

void TestResetEmptyArrays() {
    State state{};
    Runtime runtime;
    state.words[0x714u / 4u] = 0x9000u;
    state.words[0x724u / 4u] = 0xa000u;
    state.words[0x728u / 4u] = 0xa000u;
    Check(Manager::Reset(state, 0x5000u, runtime) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "base-reset"},
          "empty reset forwards only to base cleanup");
}

void TestCleanupBuffersWithActiveSource() {
    for (bool notify : {false, true}) {
        State state{};
        Runtime runtime;
        state.words[0x654u / 4u] = 0x6000u;
        state.words[(0x658u + 5u * 4u) / 4u] = 9u;
        state.words[(0x658u + 6u * 4u) / 4u] = 1u;
        state.words[0x704u / 4u] = 0x55u;
        state.words[0x6d0u / 4u] = 0x7100u;
        state.words[0x6e0u / 4u] = 1u;
        state.words[0x730u / 4u] = 1u;
        state.words[0x734u / 4u] = 2u;
        state.words[0x738u / 4u] = 3u;
        state.words[0x714u / 4u] = 0x9000u;
        state.words[0x718u / 4u] = 0x9010u;
        state.words[0x724u / 4u] = 0xa000u;
        state.words[0x728u / 4u] = 0xa000u;
        Check(Manager::CleanupBuffers(
                  state, 0x5000u, notify, runtime) == 0xa000u &&
                  runtime.calls == (notify ?
                      std::vector<std::string>{
                          "script-collection-cleanup", "field:4"} :
                      std::vector<std::string>{
                          "script-collection-cleanup"}) &&
                  state.words[(0x658u + 5u * 4u) / 4u] == 0u &&
                  state.words[(0x658u + 6u * 4u) / 4u] == 0u &&
                  state.words[0x704u / 4u] == 0u &&
                  state.words[0x6d0u / 4u] == 0u &&
                  state.words[0x6e0u / 4u] == 0u &&
                  state.words[0x730u / 4u] == 0u &&
                  state.words[0x734u / 4u] == 0u &&
                  state.words[0x738u / 4u] == 0u &&
                  state.words[0x718u / 4u] == 0x9000u,
              "active cleanup clears events, script flag and transient fields");
    }
}

void TestCleanupBuffersWithoutSource() {
    State state{};
    Runtime runtime;
    state.words[0x704u / 4u] = 0x55u;
    state.words[0x6d0u / 4u] = 0x7100u;
    state.words[0x724u / 4u] = 0xa000u;
    state.words[0x728u / 4u] = 0xa000u;
    Check(Manager::CleanupBuffers(state, 0x5000u, true, runtime) ==
              0xa000u &&
              runtime.calls.empty() &&
              state.words[0x704u / 4u] == 0x55u &&
              state.words[0x6d0u / 4u] == 0x7100u,
          "missing source skips conditional script cleanup");
}

void TestResetUsesCleanupBuffers() {
    State state{};
    Runtime runtime;
    state.words[0x654u / 4u] = 0x6000u;
    state.words[0x724u / 4u] = 0xa000u;
    state.words[0x728u / 4u] = 0xa000u;
    Check(Manager::Reset(state, 0x5000u, runtime) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "script-collection-cleanup", "field:4", "base-reset"},
          "reset composes cleanup with notify flag before base reset");
}

void TestUpdateInactive() {
    State state{};
    Runtime runtime;
    Check(Manager::Update(state, 0x5000u, 0.5f, 0x99u, runtime) == 0u &&
              runtime.calls.empty(),
          "inactive manager returns its zero flag without forwarding");
}

void TestUpdateTimerThreshold() {
    for (const auto [initial, limit, completed] :
         {std::tuple{0.25f, 1.0f, false},
          std::tuple{0.5f, 1.0f, false},
          std::tuple{0.75f, 1.0f, true}}) {
        State state{};
        Runtime runtime;
        state.words[0x1f0u / 4u] = 1u;
        state.words[0x730u / 4u] = 1u;
        state.words[0x734u / 4u] = std::bit_cast<std::uint32_t>(limit);
        state.words[0x738u / 4u] = std::bit_cast<std::uint32_t>(initial);
        Check(Manager::Update(state, 0x5000u, 0.5f, 0x99u, runtime) ==
                  0x7777u &&
                  runtime.calls == (completed ?
                      std::vector<std::string>{"collision", "position"} :
                      std::vector<std::string>{"position"}) &&
                  state.words[0x730u / 4u] == (completed ? 0u : 1u) &&
                  std::bit_cast<float>(state.words[0x738u / 4u]) ==
                      (completed ? limit : initial + 0.5f),
              "timer completes only when accumulated value exceeds limit");
    }
}

void TestUpdateNaNDoesNotCompleteTimer() {
    State state{};
    Runtime runtime;
    state.words[0x1f0u / 4u] = 1u;
    state.words[0x730u / 4u] = 1u;
    state.words[0x734u / 4u] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[0x738u / 4u] = std::bit_cast<std::uint32_t>(
        std::numeric_limits<float>::quiet_NaN());
    (void)Manager::Update(state, 0x5000u, 0.5f, 0x99u, runtime);
    Check(state.words[0x730u / 4u] == 1u &&
              std::isnan(std::bit_cast<float>(state.words[0x738u / 4u])) &&
              runtime.calls == std::vector<std::string>{"position"},
          "unordered floating-point comparison skips timer completion");
}

void TestUpdateScriptGate() {
    for (auto gate : {0u, 1u}) {
        State state{};
        Runtime runtime;
        runtime.state_gate = gate;
        state.words[0x1f0u / 4u] = 1u;
        state.words[(0x658u + 24u) / 4u] = 1u;
        Check(Manager::Update(state, 0x5000u, 0.5f, 0x99u, runtime) ==
                  0x7777u &&
                  runtime.calls == (gate == 0u ?
                      std::vector<std::string>{
                          "state-gate", "scheduler-read-byte",
                          "scheduler-update", "position"} :
                      std::vector<std::string>{"state-gate", "position"}),
              "normal update dispatches script only when state gate is zero");
    }
}

void TestComponentWithoutXml() {
    for (auto source : {0u, 0x6000u}) {
        State state{};
        Runtime runtime;
        state.words[0x704u / 4u] = 0x55u;
        state.words[0x6e0u / 4u] = 0x77u;
        state.words[0x730u / 4u] = 1u;
        state.words[0x714u / 4u] = 0x9000u;
        state.words[0x718u / 4u] = 0x9010u;
        state.words[0x724u / 4u] = 0xa000u;
        state.words[0x728u / 4u] = 0xa000u;
        if (source != 0u)
            runtime.memory[source + 0x48u] = 0u;
        Check(Manager::InitializeComponent(
                  state, 0x5000u, source, true, runtime) == 0xa000u &&
                  runtime.calls.empty() &&
                  state.words[0x654u / 4u] == source &&
                  state.words[0x704u / 4u] == 0x55u &&
                  state.words[0x6e0u / 4u] == 0u &&
                  state.words[0x730u / 4u] == 0u &&
                  state.words[0x718u / 4u] == 0x9000u,
              "missing source or XML child skips event setup but clears tail");
    }
}

void TestInitializeComponentWithXml() {
    for (bool dispatch : {false, true}) {
        State state{};
        Runtime runtime;
        runtime.memory[0x6048u] = 0x7000u;
        state.words[0x704u / 4u] = 0x55u;
        state.words[0x724u / 4u] = 0xa000u;
        state.words[0x728u / 4u] = 0xa000u;
        Check(Manager::InitializeComponent(
                  state, 0x5000u, 0x6000u, dispatch, runtime) ==
                  0xa000u &&
                  runtime.calls == (dispatch ?
                      std::vector<std::string>{
                          "xml-first-child", "load-fuzzy", "field:4",
                          "field:8"} :
                      std::vector<std::string>{
                          "xml-first-child", "load-fuzzy"}) &&
                  state.words[0x654u / 4u] == 0x6000u &&
                  state.words[0x6d0u / 4u] == 0x7100u &&
                  state.words[0x704u / 4u] == 0u &&
                  state.words[(0x658u + 28u) / 4u] == 0x594e00u &&
                  state.words[(0x658u + 24u) / 4u] == 1u,
              "XML initialization sets script callback, flag and optional fields");
    }
}

void TestReplaceComponentClearsOldScriptState() {
    State state{};
    Runtime runtime;
    runtime.memory[0x6048u] = 0x7000u;
    state.words[0x724u / 4u] = 0xa000u;
    state.words[0x728u / 4u] = 0xa000u;
    state.words[(0x658u + 5u * 4u) / 4u] = 9u;
    state.words[(0x658u + 7u * 4u) / 4u] = 0x4444u;
    Check(Manager::ReplaceComponent(
              state, 0x5000u, 0x6000u, false, runtime) == 0xa000u &&
              runtime.calls == std::vector<std::string>{
                  "script-collection-cleanup", "xml-first-child",
                  "load-fuzzy"} &&
              state.words[(0x658u + 5u * 4u) / 4u] == 0u &&
              state.words[(0x658u + 7u * 4u) / 4u] == 0x4444u,
          "replacement clears old events but retains existing callback");
}

void TestReplaceComponentWithoutXmlSkipsOldCleanup() {
    State state{};
    Runtime runtime;
    state.words[0x724u / 4u] = 0xa000u;
    state.words[0x728u / 4u] = 0xa000u;
    Check(Manager::ReplaceComponent(
              state, 0x5000u, 0u, true, runtime) == 0xa000u &&
              runtime.calls.empty(),
          "replacement requires non-null source and XML child");
}

void TestStorageActionWithoutSourceOrAttribute() {
    State state{};
    Runtime runtime;
    Check(Manager::ProcessStorageAction(state, 0x8000u, runtime) == 0u &&
              runtime.calls.empty(),
          "storage action without source returns zero before reads");
    state.words[0x654u / 4u] = 0x6000u;
    runtime.memory[0x604cu] = 0x7000u;
    Check(Manager::ProcessStorageAction(state, 0x8000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "config-attribute"},
          "missing storage attribute skips action callback");
}

void TestStorageActionNonpositiveConfig() {
    for (auto value : {0, -2}) {
        State state{};
        Runtime runtime;
        state.words[0x654u / 4u] = 0x6000u;
        runtime.memory[0x604cu] = 0x7000u;
        runtime.config_attribute = 0x7100u;
        runtime.config_value = value;
        Check(Manager::ProcessStorageAction(
                  state, 0x8000u, runtime) ==
                  static_cast<std::uint32_t>(value) &&
                  runtime.calls == std::vector<std::string>{
                      "config-attribute", "config-int"},
              "nonpositive storage setting returns parsed value");
    }
}

void TestStorageActionUpdatesStateOnlyOnEleven() {
    for (auto action_result : {7u, 11u}) {
        State state{};
        Runtime runtime;
        state.words[0x654u / 4u] = 0x6000u;
        state.words[0x1f0u / 4u] = 0x99u;
        runtime.memory[0x604cu] = 0x7000u;
        runtime.memory[0x8014u] = 0x55u;
        runtime.config_attribute = 0x7100u;
        runtime.config_value = 1;
        runtime.action_result = action_result;
        Check(Manager::ProcessStorageAction(
                  state, 0x8000u, runtime) ==
                  (action_result == 11u ? 0x55u : action_result) &&
                  runtime.calls == std::vector<std::string>{
                      "config-attribute", "config-int", "action-slot-4"} &&
                  state.words[0x1f0u / 4u] ==
                      (action_result == 11u ? 0x55u : 0x99u),
              "virtual action result eleven copies field into state flag");
    }
}

void TestStateTransitionSkipAndCleanup() {
    for (auto active : {1u, 2u}) {
        State state{};
        Runtime runtime;
        state.words[0x6e0u / 4u] = active;
        Check(Manager::SetStateAndCleanup(
                  state, 0x5000u, 0x1234u, runtime) == active &&
                  state.words[0x6e4u / 4u] == 0u &&
                  runtime.calls.empty(),
              "active states skip transition and preserve fields");
    }

    State state{};
    Runtime runtime;
    state.words[0x6e0u / 4u] = 3u;
    state.words[0xacu / 4u] = 11u;
    state.words[0xb0u / 4u] = 22u;
    state.words[0xb4u / 4u] = 33u;
    state.words[0x6f8u / 4u] = 44u;
    state.words[0x714u / 4u] = 0xa000u;
    state.words[0x724u / 4u] = 0xb000u;
    state.words[0x728u / 4u] = 0xb010u;
    runtime.memory[0xb004u] = 0xc000u;
    runtime.memory[0xb00cu] = 6u;
    runtime.bytes[0xbfffu] = 0u;
    Check(Manager::SetStateAndCleanup(
              state, 0x5000u, 0x1234u, runtime) == 88u &&
              state.words[0x6e0u / 4u] == 1u &&
              state.words[0x6e4u / 4u] == 0x1234u &&
              state.words[0x6e8u / 4u] == 11u &&
              state.words[0x6ecu / 4u] == 22u &&
              state.words[0x6f0u / 4u] == 33u &&
              state.words[0x6fcu / 4u] == 44u &&
              state.words[0x718u / 4u] == 0xa000u &&
              state.words[0x728u / 4u] == 0xb000u &&
              runtime.memory[0xb004u] == 0u &&
              runtime.memory[0xb008u] == 0u &&
              runtime.memory[0xb00cu] == 0u,
          "transition snapshots position and clears transient record");
    Check(runtime.calls == std::vector<std::string>{
              "string-free:45056:49151:8", "write:45060",
              "write:45064", "write:45068",
              "script-collection-cleanup", "animation:Z02", "animation:C03"},
          "record cleanup precedes script cleanup and animation fallback");
    runtime.calls.clear();
    Check(Manager::SetStateAndCleanup(
              state, 0x5000u, 9u, runtime) == 1u && runtime.calls.empty(),
          "repeated transition does not clean or select again");

    State direct{};
    Runtime first;
    first.animation_z_result = 17u;
    Check(Manager::SetStateAndCleanup(
              direct, 0x5000u, 0u, first) == 17u &&
              first.calls == std::vector<std::string>{
                  "script-collection-cleanup", "animation:Z02"},
          "successful first animation skips fallback");
}

void TestStopPursuitFlags() {
    for (const auto [timer, mode] :
         std::vector<std::pair<std::uint32_t, std::uint32_t>>{
             {1u, 1u}, {0u, 0u}, {2u, 2u}}) {
        State state{};
        Runtime runtime;
        state.words[0x730u / 4u] = timer;
        state.words[0x6e0u / 4u] = mode;
        Check(Manager::SetStopPursuitFlag(
                  state, 0x5000u, 0.25f, runtime) == 1u &&
                  state.words[0x704u / 4u] == 1u &&
                  runtime.calls.empty(),
              "stop flag is set without callback outside active gate");
        Manager::ClearStopPursuitFlag(state);
        Check(state.words[0x704u / 4u] == 0u,
              "clear callback resets stop flag");
    }
    State state{};
    Runtime runtime;
    state.words[0x6e0u / 4u] = 1u;
    state.words[0x730u / 4u] = 0u;
    Check(Manager::SetStopPursuitFlag(
              state, 0x5000u, 0.25f, runtime) == 23u &&
              state.words[0x704u / 4u] == 1u &&
              runtime.calls == std::vector<std::string>{"stop-collision"},
          "active pursuit invokes collision and forwards result");
}

}  // namespace

int main() {
    TestConstruct();
    TestTreeFailureUnwindsPriorStages();
    TestRegistrationFailureUnwindsAllStages();
    TestProcessorFailureUnwindsBase();
    TestDestroyPopulatedTree();
    TestDestroyEmptyTree();
    TestDestroyTreeFallback();
    TestResetReleasesStringReferences();
    TestResetEmptyArrays();
    TestCleanupBuffersWithActiveSource();
    TestCleanupBuffersWithoutSource();
    TestResetUsesCleanupBuffers();
    TestUpdateInactive();
    TestUpdateTimerThreshold();
    TestUpdateNaNDoesNotCompleteTimer();
    TestUpdateScriptGate();
    TestComponentWithoutXml();
    TestInitializeComponentWithXml();
    TestReplaceComponentClearsOldScriptState();
    TestReplaceComponentWithoutXmlSkipsOldCleanup();
    TestStorageActionWithoutSourceOrAttribute();
    TestStorageActionNonpositiveConfig();
    TestStorageActionUpdatesStateOnlyOnEleven();
    TestStateTransitionSkipAndCleanup();
    TestStopPursuitFlags();
    State state{};
    pal4::MonsterBehaviorManagerRuntimeStub stub;
    Check(Manager::Construct(state, 0x5000u, stub) == 0x5000u,
          "default monster behavior manager runtime links");
    std::cout << "26 monster behavior manager cases passed\n";
}
