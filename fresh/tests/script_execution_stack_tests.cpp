#include "pal4/script_execution_context.h"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptStackModuleRuntime {
    std::vector<std::string> events;
    std::vector<std::size_t> allocations;
    std::string reported;
    std::string raised;
    int function_index = 7;
    int prepare_result = 0;
    std::uint32_t next_context = 0xabc0u;
    std::uint32_t stopped_context = 0;
    std::uint32_t released_context = 0;
    void* engine = reinterpret_cast<void*>(0x5000u);

    void Report(std::uint32_t logger, const char* message) override {
        Check(logger == 0x42u, "diagnostic sink");
        events.emplace_back("report");
        reported = message;
    }
    void RaiseError(const char* message) override {
        events.emplace_back("raise");
        raised = message;
        throw std::runtime_error(message);
    }
    void CopyModuleName(char (&destination)[256],
                        const char* source) override {
        events.emplace_back("copy_name");
        Check(std::strlen(source) < sizeof(destination), "fixture name fits");
        std::strcpy(destination, source);
    }
    void AcquireContext(void* actual_engine,
                        std::uint32_t& context) override {
        Check(actual_engine == engine, "acquire engine");
        events.emplace_back("acquire");
        context = next_context;
    }
    int FindFunction(void* actual_engine, const char* module,
                     const char* function) override {
        Check(actual_engine == engine &&
                  std::string(module) == "scene" &&
                  std::string(function) == "entry", "function lookup inputs");
        events.emplace_back("find");
        return function_index;
    }
    int TranslateFunction(void* actual_engine, int index) override {
        Check(actual_engine == engine && index == function_index,
              "function index conversion inputs");
        events.emplace_back("translate");
        return 29;
    }
    std::uint32_t ResolveFunction(void* actual_engine,
                                  const char* module,
                                  int translated_index) override {
        Check(actual_engine == engine && std::string(module) == "scene" &&
                  translated_index == 29, "function resolution inputs");
        events.emplace_back("resolve");
        return 0x12345678u;
    }
    int Prepare(std::uint32_t context,
                std::uint32_t function) override {
        Check(context == next_context && function == 0x12345678u,
              "prepare inputs");
        events.emplace_back("prepare");
        return prepare_result;
    }
    void ReleaseContext(std::uint32_t context) override {
        events.emplace_back("release");
        released_context = context;
    }
    void StopPrevious(std::uint32_t context) override {
        events.emplace_back("stop_previous");
        stopped_context = context;
    }
    void CleanupHandlers(pal4::ScriptExecutionContextState&) override {
        events.emplace_back("cleanup_handlers");
    }
    void* AllocateRecords(std::size_t bytes) override {
        events.emplace_back("allocate");
        allocations.push_back(bytes);
        return new std::byte[bytes];
    }
    void FreeRecords(void* records) override {
        events.emplace_back("free");
        delete[] static_cast<std::byte*>(records);
    }
};

std::size_t Count(const pal4::ScriptExecutionContextState& state) {
    if (!state.buffer_20) return 0;
    return (static_cast<const std::byte*>(state.buffer_24) -
            static_cast<const std::byte*>(state.buffer_20)) / 276;
}

std::size_t Capacity(const pal4::ScriptExecutionContextState& state) {
    if (!state.buffer_20) return 0;
    return (static_cast<const std::byte*>(state.buffer_28) -
            static_cast<const std::byte*>(state.buffer_20)) / 276;
}

void TestInvalidInputs() {
    pal4::ScriptExecutionContextState state{};
    Runtime runtime;
    state.word_8 = 0x42u;
    Check(pal4::ScriptExecutionContext::StackEntryCount(state) == 0 &&
              !pal4::ScriptExecutionContext::HasStackModule(state, "scene") &&
              pal4::ScriptExecutionContext::FindStackModuleStatus(
                  state, "scene") == 0,
          "empty stack has zero entries and no named module");
    Check(pal4::ScriptExecutionContext::PushStackModule(
              state, "", "entry", runtime) == -1 &&
              runtime.reported ==
                  "error<executor:pushStackModule>: null ModuleName\r\n" &&
              runtime.events == std::vector<std::string>{"report"},
          "module name checked first");
    runtime.events.clear();
    Check(pal4::ScriptExecutionContext::PushStackModule(
              state, "scene", "", runtime) == -1 &&
              runtime.reported.find("null scriptFunc") != std::string::npos &&
              runtime.events == std::vector<std::string>{"report"},
          "function name checked second");
    runtime.events.clear();
    Check(pal4::ScriptExecutionContext::PushStackModule(
              state, "scene", "entry", runtime) == -1 &&
              runtime.reported.find("null scriptEngine") != std::string::npos &&
              runtime.events == std::vector<std::string>{"report"},
          "engine checked before copying module name");

    state.word_8 = 0;
    try {
        pal4::ScriptExecutionContext::PushStackModule(
            state, "", "entry", runtime);
        Check(false, "invalid module must throw without sink");
    } catch (const std::runtime_error&) {
        Check(runtime.raised.find("null ModuleName") != std::string::npos,
              "invalid module exception text");
    }
}

void TestContextAndFunctionFailures() {
    pal4::ScriptExecutionContextState state{};
    Runtime runtime;
    state.word_8 = 0x42u;
    state.script_engine = runtime.engine;
    runtime.next_context = 0;
    Check(pal4::ScriptExecutionContext::PushStackModule(
              state, "scene", "entry", runtime) == -1 &&
              runtime.reported.find("null insertContext") != std::string::npos &&
              runtime.events == std::vector<std::string>{
                                     "copy_name", "acquire", "report"} &&
              Count(state) == 0, "missing context is reported without release");
    state.word_8 = 0;
    runtime.events.clear();
    try {
        pal4::ScriptExecutionContext::PushStackModule(
            state, "scene", "entry", runtime);
        Check(false, "missing context must throw without sink");
    } catch (const std::runtime_error&) {
        Check(runtime.raised.find("null scriptContext") != std::string::npos,
              "throw path uses distinct context diagnostic");
    }

    state.word_8 = 0x42u;
    runtime.next_context = 0xabc0u;
    runtime.function_index = -6;
    runtime.events.clear();
    Check(pal4::ScriptExecutionContext::PushStackModule(
              state, "scene", "entry", runtime) == -6 &&
              runtime.released_context == 0xabc0u &&
              runtime.events == std::vector<std::string>{
                                     "copy_name", "acquire", "find",
                                     "release", "report"} && Count(state) == 0,
          "missing function releases acquired context before reporting");

    runtime.function_index = 7;
    runtime.prepare_result = 1;
    runtime.events.clear();
    Check(pal4::ScriptExecutionContext::PushStackModule(
              state, "scene", "entry", runtime) == -1 &&
              runtime.reported.find("Prepare Function Error") !=
                  std::string::npos &&
              runtime.events == std::vector<std::string>{
                                     "copy_name", "acquire", "find",
                                     "translate", "resolve", "prepare",
                                     "release", "report"} && Count(state) == 0,
          "prepare failure releases context and leaves stack unchanged");
}

void TestStackGrowthAndOwnership() {
    static_assert(offsetof(pal4::ScriptStackModuleRecord, script_context) == 256);
    static_assert(offsetof(pal4::ScriptStackModuleRecord, function_index) == 260);
    static_assert(offsetof(pal4::ScriptStackModuleRecord, execution_flag) == 264);
    pal4::ScriptExecutionContextState state{};
    Runtime runtime;
    state.script_engine = runtime.engine;
    state.word_8 = 0x42u;

    for (unsigned index = 0; index < 4; ++index) {
        runtime.next_context = 0xabc0u + index;
        runtime.events.clear();
        Check(pal4::ScriptExecutionContext::PushStackModule(
                  state, "scene", "entry", runtime) == 0 &&
                  Count(state) == index + 1 &&
                  Capacity(state) == (index == 0 ? 1u : index == 1 ? 2u : 4u),
              "successful push appends one record with expected capacity");
        auto* last = reinterpret_cast<pal4::ScriptStackModuleRecord*>(
            static_cast<std::byte*>(state.buffer_24) - 276);
        Check(std::string(last->module_name) == "scene" &&
                  last->script_context == runtime.next_context &&
                  last->function_index == -1 && last->execution_flag == 0 &&
                  last->word_268 == 0 && last->word_272 == 0 &&
                  state.current_stack_record == static_cast<std::uint32_t>(
                      reinterpret_cast<std::uintptr_t>(last)),
              "record data and current pointer transferred to stack");
        Check(pal4::ScriptExecutionContext::StackEntryCount(state) ==
                  static_cast<int>(index + 1) &&
                  pal4::ScriptExecutionContext::HasStackModule(state, "SCENE") &&
                  !pal4::ScriptExecutionContext::HasStackModule(state, "other"),
              "stack queries use live 276-byte records and case folding");
        if (index != 0) {
            Check(runtime.stopped_context == 0xabc0u + index - 1 &&
                      runtime.events[6] == "stop_previous" &&
                      runtime.events[7] == "cleanup_handlers",
                  "previous context stops after successful prepare");
        }
    }
    Check(runtime.allocations == std::vector<std::size_t>{276, 552, 1104},
          "capacity grows by 1, then doubles; spare capacity is reused");
    auto* final_record = reinterpret_cast<pal4::ScriptStackModuleRecord*>(
        static_cast<std::byte*>(state.buffer_24) - 276);
    std::strcpy(final_record->module_name, "final");
    Check(pal4::ScriptExecutionContext::HasStackModule(state, "FINAL") &&
              !pal4::ScriptExecutionContext::HasStackModule(state, "missing"),
          "module lookup scans the entire array at 276-byte stride");
    auto* first_record = static_cast<pal4::ScriptStackModuleRecord*>(
        state.buffer_20);
    first_record[0].function_index = -1;
    final_record->function_index = 7;
    Check(pal4::ScriptExecutionContext::FindStackModuleStatus(state, "scene") ==
                  -1 &&
              pal4::ScriptExecutionContext::FindStackModuleStatus(state,
                                                                  "final") == 7 &&
              pal4::ScriptExecutionContext::FindStackModuleStatus(state,
                                                                  "FINAL") == 0 &&
              pal4::ScriptExecutionContext::FindStackModuleStatus(state,
                                                                  "missing") == 0,
          "status lookup returns first matching status and is case sensitive");
    first_record[0].function_index = 0;
    Check(pal4::ScriptExecutionContext::FindStackModuleStatus(state, "scene") ==
              0,
          "finished record returns zero like a missing record");
    runtime.FreeRecords(state.buffer_20);
}

void TestDebuggerMode() {
    static_assert(offsetof(pal4::ScriptStackModuleRecord, word_268) == 268);
    static_assert(offsetof(pal4::ScriptStackModuleRecord, word_272) == 272);
    pal4::ScriptExecutionContextState state{};
    Check(pal4::ScriptExecutionContext::SetDebuggerMode(state, 1) == nullptr,
          "missing current record returns null");
    pal4::ScriptStackModuleRecord record{};
    state.current_stack_record = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&record));
    record.word_268 = 5;
    record.word_272 = 7;
    Check(pal4::ScriptExecutionContext::SetDebuggerMode(state, 1) == &record &&
              record.word_268 == 5 && record.word_272 == 7,
          "zero function index blocks mode update");
    record.function_index = -1;
    for (int invalid : {-1, 2}) {
        Check(pal4::ScriptExecutionContext::SetDebuggerMode(state, invalid) ==
                  &record && record.word_268 == 5 && record.word_272 == 7,
              "invalid mode leaves both fields unchanged");
    }
    Check(pal4::ScriptExecutionContext::SetDebuggerMode(state, 1) == &record &&
              record.word_268 == 1 && record.word_272 == 5,
          "mode one saves prior value");
    Check(pal4::ScriptExecutionContext::SetDebuggerMode(state, 0) == &record &&
              record.word_268 == 0 && record.word_272 == 1,
          "mode zero saves latest value");
}

}  // namespace

int main() {
    TestInvalidInputs();
    TestContextAndFunctionFailures();
    TestStackGrowthAndOwnership();
    TestDebuggerMode();
}
