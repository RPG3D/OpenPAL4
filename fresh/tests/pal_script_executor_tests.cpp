#include "pal4/pal_script_executor.h"

#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalScriptExecutorRuntime {
    pal4::PalScriptExecutorState state{};
    std::vector<std::uint32_t> allocations;
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> events;
    std::size_t allocation_index = 0u;
    std::uint32_t register_result = 0x8000u;
    bool audio_object_exists = true;
    std::uint32_t previous_node = 0u;
    std::vector<std::uint32_t> insert_arguments;
    std::uint32_t lower_bound = 0u;
    std::uint32_t remove_result = 0x9999u;
    bool mutate_sequence_on_insert = false;
    std::uint32_t string_init_result = 0x12345678u;

    [[nodiscard]] std::uint32_t SingletonManager() override {
        events.push_back("manager");
        return 0x7000u;
    }
    [[nodiscard]] std::uint32_t RegisterSingleton(
        const std::uint32_t manager, const char* name,
        const std::uint32_t id, const std::uint32_t factory_callback,
        const std::uint32_t destroy_callback) override {
        Check(manager == 0x7000u && std::string(name) == "palScriptExe" &&
                  id == 1011u && factory_callback == 0x600150u &&
                  destroy_callback == 0x6001b0u,
              "singleton registers native name, id and callbacks");
        events.push_back("register");
        return register_result;
    }
    [[nodiscard]] std::uint32_t Allocate(const std::uint32_t size) override {
        Check(allocation_index < allocations.size(),
              "allocator called expected number of times");
        Check(size == (allocation_index == 0u ? 132u : 24u) || size == 24u,
              "script allocation uses 132 or 24 bytes");
        events.push_back("allocate:" + std::to_string(size));
        return allocations[allocation_index++];
    }
    [[nodiscard]] pal4::PalScriptExecutorState& StateAt(
        const std::uint32_t address) override {
        Check(address == 0x2000u, "factory and destructor address state");
        events.push_back("state");
        return state;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        memory[address] = value;
        events.push_back("write:" + std::to_string(address));
    }
    [[nodiscard]] std::uint32_t Free(const std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return address + 1u;
    }
    void InitializeString(pal4::PalScriptExecutorState& target,
                          const std::uint32_t offset) override {
        Check(offset == 72u && &target == &state,
              "string starts at native object +72");
        events.push_back("string");
        target.words[18u] = 0xaaaau;
    }
    void InitializeContext(pal4::PalScriptExecutorState& target,
                           const std::uint32_t offset) override {
        Check(offset == 84u && &target == &state,
              "context starts at native object +84");
        events.push_back("context");
        target.words[21u] = 0xbbbbu;
    }
    void DestroyContext(pal4::PalScriptExecutorState& target,
                        const std::uint32_t offset) override {
        Check(&target == &state && offset == 84u,
              "destructor destroys embedded context at +84");
        events.push_back("destroy-context");
    }
    void CleanupString(pal4::PalScriptExecutorState& target,
                       const std::uint32_t offset) override {
        Check(&target == &state && offset == 72u,
              "destructor cleans string at +72");
        events.push_back("cleanup-string");
    }
    void CleanupDataStructure(pal4::PalScriptExecutorState& target,
                              const std::uint32_t first,
                              const std::uint32_t last) override {
        Check(&target == &state && first == last &&
                  last == state.words[11u],
              "destructor cleans the full tree range");
        events.push_back("cleanup-tree");
    }
    void TidyQueuedString(const std::uint32_t address,
                      const std::uint32_t value) override {
        Check(value == 1u, "queued string tidy releases storage");
        events.push_back("tidy:" + std::to_string(address));
    }
    [[nodiscard]] bool AudioObjectExists(
        const std::uint32_t object) override {
        Check(object == 0x7777u, "audio lookup receives object argument");
        events.push_back("audio-object");
        return audio_object_exists;
    }
    [[nodiscard]] std::uint32_t TreeWord(
        const std::uint32_t address) override {
        Check(memory.contains(address), "tree read uses mapped node word");
        return memory.at(address);
    }
    [[nodiscard]] std::uint32_t PreviousTreeNode(
        const std::uint32_t node) override {
        Check(node == 0x5000u, "predecessor receives parent node");
        events.push_back("previous");
        return previous_node;
    }
    void InsertTreeNode(pal4::PalScriptExecutorState& target,
                        const std::uint32_t leaf,
                        const std::uint32_t parent,
                        const std::uint32_t key,
                        const std::uint32_t value) override {
        Check(&target == &state, "insertion receives executor tree");
        events.push_back("insert");
        insert_arguments = {leaf, parent, key, value};
        if (mutate_sequence_on_insert) target.words[14u] = 20u;
    }
    [[nodiscard]] std::uint32_t LowerBound(
        pal4::PalScriptExecutorState& target,
        const std::uint32_t key) override {
        Check(&target == &state && key == 7u,
              "lower bound receives executor tree and key");
        events.push_back("lower-bound");
        return lower_bound;
    }
    void PlayAudioObject(const std::uint32_t object) override {
        Check(object == 0x7777u, "play receives tree value");
        events.push_back("play");
    }
    [[nodiscard]] std::uint32_t RemoveTreeNode(
        pal4::PalScriptExecutorState& target,
        const std::uint32_t node) override {
        Check(&target == &state && node == 0x5000u,
              "remove receives selected node");
        events.push_back("remove");
        return remove_result;
    }
    [[nodiscard]] std::uint32_t NextTreeNode(
        const std::uint32_t node) override {
        Check(node == 0x5000u, "successor receives current node");
        events.push_back("next");
        return 0x4000u;
    }
    void FreeTreeSubtree(pal4::PalScriptExecutorState& target,
                         const std::uint32_t root) override {
        Check(&target == &state && root == 0x3000u,
              "recursive free receives right subtree");
        events.push_back("free-subtree");
    }
    [[nodiscard]] std::uint32_t InitStringBuffer(
        pal4::PalScriptExecutorState& target,
        const std::uint32_t offset, const char* text,
        const std::uint32_t length) override {
        Check(&target == &state && offset == 72u &&
                  target.words[15u] == 1u && target.words[16u] == 0u &&
                  target.words[17u] == 0xabcdu,
              "timer fields precede string initialization");
        Check(std::string(text).size() == length,
              "callback length matches native NUL scan");
        events.push_back("init-string:" + std::to_string(length));
        return string_init_result;
    }
};

void TestSingletonGuardAndCallbacks() {
    Runtime runtime;
    pal4::PalScriptExecutorGlobals globals{};
    globals.singleton_guard = 0x80u;
    Check(pal4::PalScriptExecutor::GetInstance(globals, runtime) == 0x8000u &&
              globals.singleton_guard == 0x81u &&
              globals.singleton_address == 0x8000u &&
              runtime.events == std::vector<std::string>{"manager", "register"},
          "first getter sets guard then registers singleton");
    runtime.events.clear();
    Check(pal4::PalScriptExecutor::GetInstance(globals, runtime) == 0x8000u &&
              runtime.events.empty(),
          "subsequent getter returns cached instance without registration");
    globals.singleton_guard = 0u;
    runtime.register_result = 0u;
    Check(pal4::PalScriptExecutor::GetInstance(globals, runtime) == 0u &&
              globals.singleton_guard == 1u,
          "failed registration still leaves guard set");
}

void TestConstructorInitialStateAndSharedRecord() {
    Runtime runtime;
    runtime.allocations = {0x2000u, 0x3000u, 0x4000u};
    runtime.state.words.fill(0xaaaaaaaau);
    pal4::PalScriptExecutorGlobals globals{};
    globals.instance_count = 7u;
    Check(pal4::PalScriptExecutor::Create(
              globals, 0x5au, runtime) == 0x2000u &&
              globals.shared_record == 0x3000u &&
              globals.instance_count == 8u,
          "factory creates 132-byte instance and first shared record");
    const auto& words = runtime.state.words;
    Check(words[0u] == 0xaaaaaa5au && words[4u] == 0xaaaaaa5au &&
              words[10u] == 0xaaaa5a5au && words[12u] == 0xaaaaaa00u &&
              words[1u] == 0u && words[2u] == 0u && words[3u] == 0u &&
              words[5u] == 0u && words[6u] == 0u && words[7u] == 0u &&
              words[8u] == 0u && words[9u] == 0u &&
              words[11u] == 0x4000u && words[13u] == 0u &&
              words[14u] == 0u && words[15u] == 0u &&
              words[18u] == 0xaaaau && words[21u] == 0xbbbbu &&
              words[16u] == 0xaaaaaaaau,
          "constructor writes exact byte, vector, sentinel and subobject fields");
    Check(runtime.memory[0x3000u] == 0u &&
              runtime.memory[0x3004u] == 0u &&
              runtime.memory[0x3008u] == 0u &&
              runtime.memory[0x3014u] == 1u &&
              runtime.memory[0x4000u] == 0x4000u &&
              runtime.memory[0x4004u] == 0x3000u &&
              runtime.memory[0x4008u] == 0x4000u &&
              runtime.memory[0x4014u] == 0u,
          "shared record and instance sentinel use 24-byte layouts");
    Check(runtime.events.back() == "context" &&
              runtime.events[runtime.events.size() - 2u] == "string",
          "string initializes before context");
}

void TestReuseSharedRecordAndFactoryFailure() {
    Runtime runtime;
    pal4::PalScriptExecutorGlobals globals{};
    globals.shared_record = 0x3000u;
    runtime.allocations = {0x5000u, 0x6000u};
    Check(&pal4::PalScriptExecutor::Construct(
              runtime.state, globals, 0x22u, runtime) == &runtime.state &&
              globals.shared_record == 0x3000u &&
              globals.instance_count == 1u &&
              runtime.memory[0x6004u] == 0x3000u &&
              runtime.events == std::vector<std::string>{
                  "allocate:24", "write:20484", "write:20500",
                  "free:20480", "allocate:24", "write:24580",
                  "write:24596", "write:24576", "write:24584",
                  "string", "context"},
          "later constructor releases candidate and links existing shared record");

    Runtime failed;
    failed.allocations = {0u};
    Check(pal4::PalScriptExecutor::Create(
              globals, 0x22u, failed) == 0u &&
              failed.events == std::vector<std::string>{"allocate:132"},
          "failed outer allocation returns null without constructing");
    failed.events.clear();
    failed.allocation_index = 0u;
    failed.allocations = {0x2000u, 0u};
    bool threw = false;
    try {
        (void)pal4::PalScriptExecutor::Create(
            globals, 0x22u, failed);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && failed.events == std::vector<std::string>{
              "allocate:132", "state", "allocate:24", "free:8192"},
          "factory releases outer allocation when constructor throws");
}

void TestDestroyAndReset() {
    Runtime runtime;
    pal4::PalScriptExecutorGlobals globals{};
    globals.shared_record = 0x3000u;
    globals.instance_count = 1u;
    runtime.state.words[11u] = 0x4000u;
    runtime.memory[0x4000u] = 0x4000u;
    Check(pal4::PalScriptExecutor::DestroyInstance(
              0u, 0xdeadbeefu, globals, runtime) == 0xdeadbeefu &&
              runtime.events.empty(),
          "null destroy preserves explicitly supplied undefined native return");
    Check(pal4::PalScriptExecutor::DestroyInstance(
              0x2000u, 0u, globals, runtime) == 0x2001u &&
              runtime.events == std::vector<std::string>{
                  "state", "destroy-context", "cleanup-string",
                  "cleanup-tree", "free:16384", "free:12288",
                  "free:0", "free:0", "free:8192"} &&
              globals.instance_count == 0u && globals.shared_record == 0u &&
              runtime.state.words[11u] == 0u,
          "destroy cleans instance and shared sentinel before outer allocation");

    runtime.events.clear();
    runtime.state.words[11u] = 0x4000u;
    globals.shared_record = 0x3000u;
    runtime.state.words[1u] = 0x2222u;
    runtime.state.words[2u] = 0x3333u;
    runtime.state.words[5u] = 0x1000u;
    runtime.state.words[6u] = 0x1030u;
    runtime.state.words[15u] = 0xddddddddu;
    Check(pal4::PalScriptExecutor::ResetState(
              runtime.state, 0x2000u, globals, runtime) == 0x4000u &&
              runtime.state.words[6u] == 0x1000u &&
              runtime.state.words[2u] == 0x2222u &&
              runtime.state.words[15u] == 0u &&
              runtime.events == std::vector<std::string>{
                  "tidy:4096", "tidy:4112", "tidy:4128"},
          "reset tidies 16-byte string records then resets queues and timer");
    runtime.events.clear();
    runtime.state.words[15u] = 0xddddddddu;
    Check(pal4::PalScriptExecutor::ResetState(
              runtime.state, 0x2000u, globals, runtime) == 0x4000u &&
              runtime.events.empty(),
          "reset with empty string range skips cleanup");
}

void TestCleanupInstanceWithLivePeer() {
    Runtime runtime;
    pal4::PalScriptExecutorGlobals globals{};
    globals.instance_count = 2u;
    globals.shared_record = 0x3000u;
    runtime.state.words[11u] = 0x4000u;
    runtime.state.words[13u] = 9u;
    runtime.memory[0x4000u] = 0x4000u;
    runtime.state.words[5u] = 0x1000u;
    runtime.state.words[6u] = 0x1020u;
    runtime.state.words[7u] = 0x1040u;
    runtime.state.words[1u] = 0x5000u;
    runtime.state.words[2u] = 0x5010u;
    runtime.state.words[3u] = 0x5020u;
    Check(pal4::PalScriptExecutor::CleanupInstance(
              runtime.state, globals, runtime) == 0x5001u &&
              runtime.events == std::vector<std::string>{
                  "destroy-context", "cleanup-string", "cleanup-tree",
                  "free:16384", "tidy:4096", "tidy:4112",
                  "free:4096", "free:20480"} &&
              globals.instance_count == 1u &&
              globals.shared_record == 0x3000u &&
              runtime.state.words[11u] == 0u &&
              runtime.state.words[13u] == 0u &&
              runtime.state.words[5u] == 0u &&
              runtime.state.words[6u] == 0u &&
              runtime.state.words[7u] == 0u &&
              runtime.state.words[1u] == 0u &&
              runtime.state.words[2u] == 0u &&
              runtime.state.words[3u] == 0u,
          "cleanup releases records and arrays but retains shared nil for peer");
}

void TestTreeInsertion() {
    Runtime runtime;
    pal4::PalScriptExecutorGlobals globals{};
    globals.shared_record = 0x3000u;
    runtime.state.words[11u] = 0x4000u;
    runtime.state.words[14u] = 7u;
    runtime.memory[0x4000u] = 0x4000u;
    runtime.memory[0x4004u] = 0x3000u;
    runtime.audio_object_exists = false;
    Check(pal4::PalScriptExecutor::InsertData(
              runtime.state, globals, 0x7777u, runtime) == 0xffffffffu &&
              runtime.state.words[14u] == 7u &&
              runtime.events == std::vector<std::string>{"audio-object"},
          "missing audio object skips tree and sequence counter");

    runtime.audio_object_exists = true;
    runtime.events.clear();
    Check(pal4::PalScriptExecutor::InsertData(
              runtime.state, globals, 0x7777u, runtime) == 7u &&
              runtime.state.words[14u] == 8u &&
              runtime.insert_arguments == std::vector<std::uint32_t>{
                  0x3000u, 0x4000u, 7u, 0x7777u} &&
              runtime.events == std::vector<std::string>{
                  "audio-object", "insert"},
          "empty tree inserts through header and returns old sequence");

    runtime.state.words[14u] = 7u;
    runtime.memory[0x4000u] = 0x5000u;
    runtime.memory[0x4004u] = 0x5000u;
    runtime.memory[0x5000u] = 0x3000u;
    runtime.memory[0x5008u] = 0x3000u;
    runtime.memory[0x500cu] = 7u;
    runtime.insert_arguments.clear();
    runtime.events.clear();
    Check(pal4::PalScriptExecutor::InsertData(
              runtime.state, globals, 0x7777u, runtime) == 7u &&
              runtime.state.words[14u] == 8u &&
              runtime.insert_arguments.empty() &&
              runtime.events == std::vector<std::string>{"audio-object"},
          "duplicate key skips insertion but consumes sequence");

    runtime.state.words[12u] = 1u;
    runtime.state.words[14u] = 7u;
    Check(pal4::PalScriptExecutor::InsertData(
              runtime.state, globals, 0x7777u, runtime) == 7u &&
              runtime.insert_arguments == std::vector<std::uint32_t>{
                  0x3000u, 0x5000u, 7u, 0x7777u},
          "mode byte forces insertion of an equal key");

    runtime.state.words[12u] = 0u;
    runtime.state.words[14u] = 6u;
    runtime.memory[0x500cu] = 10u;
    runtime.memory[0x4000u] = 0x6000u;
    runtime.previous_node = 0x6000u;
    runtime.memory[0x600cu] = 5u;
    runtime.insert_arguments.clear();
    runtime.events.clear();
    Check(pal4::PalScriptExecutor::InsertData(
              runtime.state, globals, 0x7777u, runtime) == 6u &&
              runtime.insert_arguments == std::vector<std::uint32_t>{
                  0x3000u, 0x5000u, 6u, 0x7777u} &&
              runtime.events == std::vector<std::string>{
                  "audio-object", "previous", "insert"},
          "left branch checks predecessor before inserting");

    runtime.state.words[14u] = 5u;
    runtime.insert_arguments.clear();
    Check(pal4::PalScriptExecutor::InsertData(
              runtime.state, globals, 0x7777u, runtime) == 5u &&
              runtime.insert_arguments.empty(),
          "equal predecessor blocks insertion");

    runtime.state.words[12u] = 1u;
    runtime.state.words[14u] = 7u;
    runtime.mutate_sequence_on_insert = true;
    Check(pal4::PalScriptExecutor::InsertData(
              runtime.state, globals, 0x7777u, runtime) == 20u &&
              runtime.state.words[14u] == 21u,
          "return and increment reread sequence after insertion callback");
}

void TestTreeRemoval() {
    Runtime runtime;
    runtime.state.words[11u] = 0x4000u;
    runtime.lower_bound = 0x4000u;
    Check(pal4::PalScriptExecutor::RemoveData(
              runtime.state, 7u, runtime) == 0x4000u &&
              runtime.events == std::vector<std::string>{"lower-bound"},
          "missing lower bound returns header without audio lookup");

    runtime.lower_bound = 0x5000u;
    runtime.memory[0x500cu] = 8u;
    runtime.events.clear();
    Check(pal4::PalScriptExecutor::RemoveData(
              runtime.state, 7u, runtime) == 0x4000u &&
              runtime.events == std::vector<std::string>{"lower-bound"},
          "greater lower bound is not an exact match");

    runtime.memory[0x500cu] = 7u;
    runtime.memory[0x5010u] = 0x7777u;
    runtime.events.clear();
    Check(pal4::PalScriptExecutor::RemoveData(
              runtime.state, 7u, runtime) == 0x9999u &&
              runtime.events == std::vector<std::string>{
                  "lower-bound", "audio-object", "play", "remove"},
          "found node plays live audio then returns remove result");

    runtime.audio_object_exists = false;
    runtime.events.clear();
    Check(pal4::PalScriptExecutor::RemoveData(
              runtime.state, 7u, runtime) == 0x9999u &&
              runtime.events == std::vector<std::string>{
                  "lower-bound", "audio-object", "remove"},
          "inactive audio skips play but still removes node");
}

void TestTreeCleanup() {
    Runtime runtime;
    pal4::PalScriptExecutorGlobals globals{};
    globals.shared_record = 0x3000u;
    runtime.state.words[11u] = 0x4000u;
    runtime.state.words[14u] = 9u;
    runtime.memory[0x4000u] = 0x4000u;
    Check(pal4::PalScriptExecutor::CleanupData(
              runtime.state, 0x2000u, globals, runtime) == 0x4000u &&
              runtime.state.words[14u] == 0u && runtime.events.empty(),
          "empty tree returns header and resets sequence");

    runtime.memory[0x4000u] = 0x5000u;
    runtime.memory[0x4004u] = 0x5000u;
    runtime.memory[0x4008u] = 0x5000u;
    runtime.memory[0x5000u] = 0x3000u;
    runtime.memory[0x5004u] = 0x4000u;
    runtime.memory[0x5008u] = 0x3000u;
    runtime.memory[0x5010u] = 0x7777u;
    runtime.state.words[14u] = 9u;
    Check(pal4::PalScriptExecutor::CleanupData(
              runtime.state, 0x2000u, globals, runtime) == 0x4000u &&
              runtime.state.words[14u] == 0u &&
              runtime.events == std::vector<std::string>{
                  "audio-object", "play", "next", "remove"},
          "zero-count cleanup plays audio then removes each node");

    runtime.events.clear();
    runtime.audio_object_exists = false;
    runtime.state.words[13u] = 1u;
    runtime.state.words[14u] = 9u;
    Check(pal4::PalScriptExecutor::CleanupData(
              runtime.state, 0x2000u, globals, runtime) == 0x2000u &&
              runtime.state.words[13u] == 0u &&
              runtime.state.words[14u] == 0u &&
              runtime.memory[0x4000u] == 0x4000u &&
              runtime.memory[0x4004u] == 0x3000u &&
              runtime.memory[0x4008u] == 0x4000u &&
              runtime.events == std::vector<std::string>{
                  "audio-object", "free-subtree", "free:20480",
                  "write:16388", "write:16384", "write:16392"},
          "nonzero-count cleanup recursively frees and resets header");

    runtime.events.clear();
    runtime.audio_object_exists = true;
    runtime.state.words[13u] = 2u;
    runtime.memory[0x4000u] = 0x6000u;
    runtime.memory[0x4004u] = 0x5000u;
    runtime.memory[0x4008u] = 0x5000u;
    runtime.memory[0x5000u] = 0x6000u;
    runtime.memory[0x6000u] = 0x3000u;
    runtime.memory[0x6004u] = 0x5000u;
    runtime.memory[0x6008u] = 0x3000u;
    runtime.memory[0x6010u] = 0x7777u;
    Check(pal4::PalScriptExecutor::CleanupData(
              runtime.state, 0x2000u, globals, runtime) == 0x2000u &&
              runtime.events == std::vector<std::string>{
                  "audio-object", "play", "audio-object", "play",
                  "free-subtree", "free:20480", "free-subtree",
                  "free:24576", "write:16388", "write:16384",
                  "write:16392"},
          "two-node cleanup traverses in order before freeing root and left");
}

void TestTimerState() {
    for (const char* text : {"", "giTimeScript"}) {
        Runtime runtime;
        runtime.state.words[15u] = 0xffffffffu;
        runtime.state.words[16u] = 0xffffffffu;
        runtime.state.words[17u] = 0xffffffffu;
        Check(pal4::PalScriptExecutor::InitializeTimerState(
                  runtime.state, 0xabcdu, text, runtime) == 0x12345678u &&
                  runtime.state.words[15u] == 1u &&
                  runtime.state.words[16u] == 0u &&
                  runtime.state.words[17u] == 0xabcdu &&
                  runtime.events == std::vector<std::string>{
                      "init-string:" + std::to_string(std::string(text).size())},
              "timer initializer forwards callback and buffer result");
    }
}

}  // namespace

int main() {
    TestSingletonGuardAndCallbacks();
    TestConstructorInitialStateAndSharedRecord();
    TestReuseSharedRecordAndFactoryFailure();
    TestDestroyAndReset();
    TestCleanupInstanceWithLivePeer();
    TestTreeInsertion();
    TestTreeRemoval();
    TestTreeCleanup();
    TestTimerState();
}
