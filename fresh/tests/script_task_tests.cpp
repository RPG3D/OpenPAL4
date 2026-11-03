#include "pal4/script_task.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::size_t IndexOf(const std::vector<std::string>& calls,
                    const std::string& name) {
    const auto found = std::find(calls.begin(), calls.end(), name);
    if (found == calls.end()) throw std::runtime_error("missing " + name);
    return static_cast<std::size_t>(found - calls.begin());
}

struct Runtime : virtual pal4::IScriptTaskRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t owner = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> lengths;
    std::map<std::uint32_t, std::string> strings;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint8_t> reset_flags;
    std::vector<std::uint32_t> reallocated;
    std::vector<std::uint32_t> copied;
    bool actor_active = false;
    bool system_busy = false;
    bool cache_contains = false;
    bool csb_mode = true;
    bool execute_result = false;
    std::uint32_t interpreter = 0x9000u;
    std::uint32_t cached_script = 0x9000u;
    unsigned transient_count = 0u;

    void BaseConstruct(std::uint32_t address) override {
        Check(address == object, "script base constructor address");
        calls.emplace_back("base construct");
    }
    std::uint32_t BaseDestruct(std::uint32_t address) override {
        Check(address == object, "script base destructor address");
        calls.emplace_back("base destruct");
        return 0x76543210u;
    }
    std::uint8_t TransientInitialByte() override {
        ++transient_count;
        return 0x5au;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("write byte");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
        calls.emplace_back("free");
    }
    std::uint32_t CStringLength(std::uint32_t source) override {
        return lengths.at(source);
    }
    [[noreturn]] void ThrowLength() override {
        throw std::length_error("script name too long");
    }
    std::uint32_t SetErrorFlag(std::uint32_t record,
                               std::uint8_t flag) override {
        reset_flags.push_back(flag);
        calls.emplace_back("reset text");
        for (const auto offset : {4u, 8u, 12u})
            words[record + offset] = 0u;
        return 0u;
    }
    void ReallocateText(std::uint32_t record,
                        std::uint32_t length) override {
        Check(record == object + 0x40u, "script text record");
        reallocated.push_back(length);
        words[record + 4u] = 0xc000u;
        words[record + 12u] = length;
        calls.emplace_back("reallocate text");
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        Check(length == lengths.at(source), "script copied length");
        copied.push_back(destination);
        calls.emplace_back("copy text");
    }
    std::string ReadCString(std::uint32_t source) override {
        calls.emplace_back("read function");
        return source == 0u ? std::string{} : strings.at(source);
    }
    bool ActorControlActive() override {
        calls.emplace_back("actor control");
        return actor_active;
    }
    bool ScriptSystemBusy() override {
        calls.emplace_back("script busy");
        return system_busy;
    }
    void InitializeScript() override {
        calls.emplace_back("initialize script");
    }
    bool ScriptCacheContains(std::string_view full_name) override {
        Check(full_name == "palGameOMTask_script[Test]",
              "script cache key");
        calls.emplace_back("cache contains");
        return cache_contains;
    }
    bool IsCsbMode() override {
        calls.emplace_back("csb mode");
        return csb_mode;
    }
    std::uint32_t ArenaContext() override {
        calls.emplace_back("arena context");
        return 0x7000u;
    }
    std::uint32_t CreateInterpreter(
        bool mode, std::string_view path,
        std::string_view full_name, std::string_view function_name,
        std::uint32_t arena_context) override {
        Check(mode == csb_mode &&
                  path == "palGameOMTask_script" &&
                  full_name == "palGameOMTask_script[Test]" &&
                  function_name == "Test" &&
                  arena_context == 0x7000u,
              "script interpreter arguments");
        calls.emplace_back("create interpreter");
        return interpreter;
    }
    bool AddScriptModule(std::uint32_t handle) override {
        Check(handle == interpreter, "script execution handle");
        calls.emplace_back("add script module");
        return execute_result;
    }
    void PushStackModule(std::uint32_t handle) override {
        Check(handle == interpreter, "script report handle");
        calls.emplace_back("push stack module");
    }
    void ReleaseInterpreter(std::uint32_t handle) override {
        Check(handle == interpreter, "script release handle");
        calls.emplace_back("release interpreter");
    }
    void ShowMissingScriptError() override {
        calls.emplace_back("missing script");
    }
    void CleanupScriptFiles() override {
        calls.emplace_back("cleanup scripts");
    }
    std::uint32_t FindCachedScript(std::string_view path) override {
        Check(path == "palGameOMTask_script", "script update cache key");
        calls.emplace_back("find cached script");
        return cached_script;
    }
};

struct SpawnRuntime final : Runtime, pal4::IScriptTaskSpawnRuntime {
    static constexpr std::uint32_t context = 0x7000u;
    static constexpr std::uint32_t helper = 0xb000u;
    static constexpr std::uint32_t collection = owner + 0x2a0u;
    std::deque<std::uint32_t> allocations{helper};
    std::vector<std::uint32_t> allocation_sizes;
    std::uint32_t added_helper = 0u;
    std::uint32_t processed = 0u;

    SpawnRuntime() {
        words[object + 0x58u] = context;
        words[object + 0x54u] = owner;
        words[object + 0x30u] = 0u;
        words[collection + 4u] = 0xa000u;
        words[collection + 8u] = 0xa004u;
        words[collection + 12u] = 0xa008u;
        words[0xa000u] = 0xaaaa0000u;
    }
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        allocation_sizes.push_back(size);
        calls.emplace_back("allocate");
        const auto address = allocations.front();
        allocations.pop_front();
        return address;
    }
    std::uint32_t ConstructHelperTask(std::uint32_t memory) override {
        Check(memory == helper, "script helper memory");
        calls.emplace_back("construct helper");
        return memory;
    }
    void AddHelperTask(std::uint32_t task_context,
                       std::uint32_t task,
                       std::uint32_t first,
                       std::uint32_t second,
                       std::uint32_t third) override {
        Check(task_context == context && task == helper &&
                  first == 1u && second == 0u && third == 0u,
              "script helper timing");
        calls.emplace_back("add helper");
        added_helper = task;
    }
    void SetHelperOwner(std::uint32_t task,
                        std::uint32_t task_owner) override {
        Check((task == helper || task == 0u) && task_owner == owner,
              "script helper owner");
        calls.emplace_back("set helper owner");
    }
    void NotifyPreviousTask(std::uint32_t previous) override {
        Check(previous == 0x6000u, "script previous task");
        calls.emplace_back("previous");
    }
    void MarkProcessed(std::uint32_t task_context) override {
        Check(task_context == context, "script processed context");
        calls.emplace_back("processed");
        processed = task_context;
    }
};

void SeedEntry(Runtime& runtime) {
    runtime.words[Runtime::object + 0x54u] = Runtime::owner;
    runtime.words[Runtime::object + 0x48u] = 4u;
    runtime.words[Runtime::object + 0x44u] = 0x3000u;
    runtime.words[Runtime::object + 0x50u] = 1u;
    runtime.bytes[Runtime::object + 0x3cu] = 0u;
    runtime.strings[0x3000u] = "Test";
}

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::ScriptTask::Construct(Runtime::object, runtime) ==
              Runtime::object && runtime.transient_count == 1u &&
              runtime.bytes.at(Runtime::object + 0x40u) == 0x5au &&
              runtime.words.at(Runtime::object + 0x44u) == 0u &&
              runtime.words.at(Runtime::object + 0x54u) == 0u &&
              runtime.words.at(Runtime::object) == 0x848940u &&
              runtime.calls.front() == "base construct",
          "script constructor layout");
    for (const auto count : {std::uint8_t{0u},
                             std::uint8_t{2u},
                             std::uint8_t{0xffu}}) {
        Runtime destroying;
        destroying.words[Runtime::object + 0x44u] = 0x3000u;
        destroying.bytes[0x2fffu] = count;
        Check(pal4::ScriptTask::Destruct(Runtime::object, destroying) ==
                  0x76543210u &&
                  destroying.words.at(Runtime::object) == 0x8486fcu &&
                  destroying.words.at(Runtime::object + 0x44u) == 0u &&
                  (count == 2u ?
                       destroying.bytes.at(0x2fffu) == 1u :
                       destroying.freed ==
                           (std::vector<std::uint32_t>{0x2fffu})) &&
                  destroying.calls.back() == "base destruct",
              "script destructor reference count");
    }
}

void CheckInitialize() {
    Runtime fresh;
    fresh.words[Runtime::object + 0x44u] = 0u;
    fresh.words[Runtime::object + 0x4cu] = 0u;
    fresh.lengths[0x9000u] = 4u;
    Check(pal4::ScriptTask::Initialize(
              Runtime::object, Runtime::owner, 0x9000u,
              1u, 0x7000u, fresh) == 0x7000u &&
              fresh.words.at(Runtime::object + 0x54u) ==
                  Runtime::owner &&
              fresh.bytes.at(Runtime::object + 0x3cu) == 0u &&
              fresh.words.at(Runtime::object + 0x48u) == 4u &&
              fresh.words.at(Runtime::object + 0x50u) == 1u &&
              fresh.words.at(Runtime::object + 0x58u) == 0x7000u &&
              fresh.reallocated ==
                  (std::vector<std::uint32_t>{4u}) &&
              fresh.copied ==
                  (std::vector<std::uint32_t>{0xc000u}),
          "script initializer allocates name and stores arguments");
    Runtime shared_empty;
    shared_empty.words[Runtime::object + 0x44u] = 0x3000u;
    shared_empty.bytes[0x2fffu] = 2u;
    shared_empty.lengths[0xd000u] = 0u;
    Check(pal4::ScriptTask::Initialize(
              Runtime::object, Runtime::owner, 0xd000u,
              1u, 0x7000u, shared_empty) == 0x3000u &&
              shared_empty.bytes.at(0x2fffu) == 1u &&
              shared_empty.words.at(Runtime::object + 0x44u) == 0u &&
              shared_empty.reset_flags.empty(),
          "script shared empty name returns old buffer");
    Runtime empty;
    empty.words[Runtime::object + 0x44u] = 0u;
    empty.lengths[0xd000u] = 0u;
    Check(pal4::ScriptTask::Initialize(
              Runtime::object, Runtime::owner, 0xd000u,
              17u, 0x7000u, empty) == 17u &&
              empty.reset_flags ==
                  (std::vector<std::uint8_t>{1u}),
          "script empty name returns wait word");
    Runtime reused;
    reused.words[Runtime::object + 0x44u] = 0x3000u;
    reused.words[Runtime::object + 0x4cu] = 10u;
    reused.bytes[0x2fffu] = 0u;
    reused.lengths[0x9000u] = 4u;
    (void)pal4::ScriptTask::Initialize(
        Runtime::object, Runtime::owner, 0x9000u,
        1u, 0x7000u, reused);
    Check(reused.reallocated.empty() &&
              reused.copied ==
                  (std::vector<std::uint32_t>{0x3000u}),
          "script reuses exclusive name buffer");
    Runtime shared_nonempty;
    shared_nonempty.words[Runtime::object + 0x44u] = 0x3000u;
    shared_nonempty.bytes[0x2fffu] = 2u;
    shared_nonempty.lengths[0x9000u] = 4u;
    (void)pal4::ScriptTask::Initialize(
        Runtime::object, Runtime::owner, 0x9000u,
        1u, 0x7000u, shared_nonempty);
    Check(shared_nonempty.reallocated ==
              (std::vector<std::uint32_t>{4u}) &&
              shared_nonempty.reset_flags.empty(),
          "script shared nonempty name reallocates");
    Runtime too_long;
    too_long.lengths[0x9000u] = 0xfffffffeu;
    bool thrown = false;
    try {
        (void)pal4::ScriptTask::Initialize(
            Runtime::object, Runtime::owner, 0x9000u,
            1u, 0x7000u, too_long);
    } catch (const std::length_error&) {
        thrown = true;
    }
    Check(thrown, "script oversized name rejected");
}

void CheckEnter() {
    Runtime missing_owner;
    missing_owner.words[Runtime::object + 0x54u] = 0u;
    Check(pal4::ScriptTask::Enter(
              Runtime::object, missing_owner) == 0u &&
              missing_owner.calls.empty(), "script owner gate");
    Runtime empty_name;
    SeedEntry(empty_name);
    empty_name.words[Runtime::object + 0x48u] = 0u;
    Check(pal4::ScriptTask::Enter(
              Runtime::object, empty_name) == 0u &&
              empty_name.bytes.at(Runtime::object + 0x3cu) == 1u,
          "script empty name completes");
    Runtime busy;
    SeedEntry(busy);
    busy.system_busy = true;
    Check(pal4::ScriptTask::Enter(Runtime::object, busy) == 0u &&
              busy.bytes.at(Runtime::object + 0x3cu) == 1u &&
              IndexOf(busy.calls, "actor control") <
                  IndexOf(busy.calls, "script busy"),
          "script busy system gate");
    Runtime actor;
    SeedEntry(actor);
    actor.actor_active = true;
    actor.system_busy = true;
    actor.cache_contains = true;
    Check(pal4::ScriptTask::Enter(Runtime::object, actor) == 1u &&
              actor.bytes.at(Runtime::object + 0x3cu) == 0u &&
              std::find(actor.calls.begin(), actor.calls.end(),
                        "script busy") == actor.calls.end() &&
              std::find(actor.calls.begin(), actor.calls.end(),
                        "create interpreter") == actor.calls.end() &&
              actor.calls.back() == "cleanup scripts",
          "script actor route and cache hit");
    Runtime failed;
    SeedEntry(failed);
    failed.interpreter = 0u;
    Check(pal4::ScriptTask::Enter(Runtime::object, failed) == 1u &&
              IndexOf(failed.calls, "create interpreter") <
                  IndexOf(failed.calls, "missing script") &&
              IndexOf(failed.calls, "missing script") <
                  IndexOf(failed.calls, "cleanup scripts") &&
              failed.bytes.at(Runtime::object + 0x3cu) == 0u,
          "script missing interpreter error path");
    Runtime no_exception;
    SeedEntry(no_exception);
    no_exception.words[Runtime::object + 0x50u] = 0u;
    Check(pal4::ScriptTask::Enter(Runtime::object,
                                   no_exception) == 1u &&
              IndexOf(no_exception.calls, "add script module") <
                  IndexOf(no_exception.calls, "missing script") &&
              no_exception.bytes.at(Runtime::object + 0x3cu) == 1u,
          "script execution false branch and immediate completion");
    Runtime exception;
    SeedEntry(exception);
    exception.execute_result = true;
    Check(pal4::ScriptTask::Enter(Runtime::object,
                                   exception) == 1u &&
              IndexOf(exception.calls, "push stack module") <
                  IndexOf(exception.calls, "release interpreter") &&
              IndexOf(exception.calls, "release interpreter") <
                  IndexOf(exception.calls, "cleanup scripts") &&
              std::find(exception.calls.begin(), exception.calls.end(),
                        "missing script") == exception.calls.end(),
          "script exception handling branch");
}

void CheckUpdate() {
    Runtime missing;
    missing.words[Runtime::object + 0x54u] = 0u;
    missing.bytes[Runtime::object + 0x3cu] = 1u;
    Check(pal4::ScriptTask::Update(Runtime::object, missing) ==
              0u && missing.bytes.at(Runtime::object + 0x3cu) == 0u,
          "script update owner gate");
    Runtime completed;
    completed.words[Runtime::object + 0x54u] = Runtime::owner;
    completed.bytes[Runtime::object + 0x3cu] = 1u;
    Check(pal4::ScriptTask::Update(Runtime::object, completed) ==
              0u && completed.calls.empty(),
          "script update completed gate");
    for (const auto cached : {0u, 0xffffffffu, 0x9000u}) {
        Runtime running;
        running.words[Runtime::object + 0x54u] = Runtime::owner;
        running.bytes[Runtime::object + 0x3cu] = 0u;
        running.cached_script = cached;
        Check(pal4::ScriptTask::Update(Runtime::object, running) ==
                  1u &&
                  running.bytes.at(Runtime::object + 0x3cu) ==
                      (cached == 0x9000u ? 0u : 1u) &&
                  IndexOf(running.calls, "find cached script") == 0u,
              "script update cache state");
    }
}

void CheckFollowup() {
    SpawnRuntime missing_context;
    missing_context.words[Runtime::object + 0x58u] = 0u;
    Check(pal4::ScriptTask::SpawnFollowup(
              Runtime::object, missing_context) == 0u &&
              missing_context.calls.empty(), "script context gate");
    SpawnRuntime missing_owner;
    missing_owner.words[Runtime::object + 0x54u] = 0u;
    Check(pal4::ScriptTask::SpawnFollowup(
              Runtime::object, missing_owner) == 0u &&
              missing_owner.calls.empty(), "script followup owner gate");
    SpawnRuntime previous;
    previous.words[Runtime::object + 0x30u] = 0x6000u;
    Check(pal4::ScriptTask::SpawnFollowup(
              Runtime::object, previous) == 1u &&
              previous.calls ==
                  (std::vector<std::string>{"previous"}),
          "script previous callback shortcut");
    SpawnRuntime helper;
    Check(pal4::ScriptTask::SpawnFollowup(
              Runtime::object, helper) == 1u &&
              helper.added_helper == SpawnRuntime::helper &&
              helper.words.at(0xa004u) == SpawnRuntime::helper &&
              helper.processed == SpawnRuntime::context &&
              helper.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x40u}),
          "script appends helper");
    SpawnRuntime growth;
    growth.words[SpawnRuntime::collection + 12u] = 0xa004u;
    growth.allocations.push_back(0xe000u);
    Check(pal4::ScriptTask::SpawnFollowup(
              Runtime::object, growth) == 1u &&
              growth.allocation_sizes ==
                  (std::vector<std::uint32_t>{0x40u, 8u}) &&
              growth.words.at(0xe000u) == 0xaaaa0000u &&
              growth.words.at(0xe004u) == SpawnRuntime::helper &&
              growth.processed == SpawnRuntime::context,
          "script grows helper collection");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckInitialize();
        CheckEnter();
        CheckUpdate();
        CheckFollowup();
        std::cout << "script_task_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
