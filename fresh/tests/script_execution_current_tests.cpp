#include "pal4/script_execution_context.h"

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

struct Runtime final : pal4::IScriptCurrentModuleRuntime {
    std::vector<std::string> events;
    std::vector<int> results;
    std::size_t result_index = 0;
    std::string reported;
    std::string raised;
    pal4::ScriptStackModuleRecord* expected_record = nullptr;
    void* expected_engine = reinterpret_cast<void*>(0x5000u);
    std::uint32_t expected_context = 0x7000u;

    void StopContext(std::uint32_t context) override {
        Check(context == expected_context, "stop context identity");
        events.emplace_back("stop");
    }
    void ReleaseContext(std::uint32_t context) override {
        Check(context == expected_context, "release context identity");
        events.emplace_back("release");
    }
    void RemoveModule(void* engine,
                      pal4::ScriptStackModuleRecord& record) override {
        Check(engine == expected_engine && &record == expected_record &&
                  record.script_context == 0,
              "remove module after context release");
        events.emplace_back("remove");
    }
    void CompleteCleanup(pal4::ScriptExecutionContextState&) override {
        events.emplace_back("complete_cleanup");
    }
    void AbortContext(std::uint32_t context) override {
        Check(context == expected_context, "abort context identity");
        events.emplace_back("abort");
    }
    int ExecuteContext(std::uint32_t context, int option) override {
        Check(context == expected_context && option == 0 &&
                  result_index < results.size(),
              "execute context and zero option");
        events.emplace_back("execute");
        return results[result_index++];
    }
    const char* ExceptionFunctionName(std::uint32_t context) override {
        Check(context == expected_context, "exception function context");
        events.emplace_back("function_name");
        return "main";
    }
    int ExceptionLine(std::uint32_t context, int option) override {
        Check(context == expected_context && option == 0,
              "exception line context and option");
        events.emplace_back("line");
        return 42;
    }
    const char* ExceptionReason(std::uint32_t context,
                                int option) override {
        Check(context == expected_context && option == 0,
              "exception reason context and option");
        events.emplace_back("reason");
        return "division by zero";
    }
    const char* FunctionDeclaration(void* engine,
                                    const char* function_name,
                                    int option) override {
        Check(engine == expected_engine &&
                  std::string(function_name) == "main" && option == 0,
              "declaration uses engine and context function name");
        events.emplace_back("declaration");
        return "void ";
    }
    void ReportException(std::uint32_t logger,
                         std::string_view text) override {
        Check(logger == 0x42u, "exception logger identity");
        events.emplace_back("report");
        reported = text;
    }
    void RaiseException(std::string_view text) override {
        events.emplace_back("raise");
        raised = text;
        throw std::runtime_error(raised);
    }
};

pal4::ScriptExecutionContextState StateFor(
    pal4::ScriptStackModuleRecord& record, Runtime& runtime) {
    pal4::ScriptExecutionContextState state{};
    state.script_engine = runtime.expected_engine;
    state.word_8 = 0x42u;
    state.current_stack_record = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&record));
    record.script_context = runtime.expected_context;
    return state;
}

void TestMissingAndFinished() {
    pal4::ScriptExecutionContextState empty{};
    Runtime runtime;
    Check(pal4::ScriptExecutionContext::ExecuteCurrentModule(empty, runtime) ==
              -1 && runtime.events.empty(),
          "missing current record returns minus one");

    pal4::ScriptStackModuleRecord record{};
    auto state = StateFor(record, runtime);
    record.function_index = 0;
    Check(pal4::ScriptExecutionContext::ExecuteCurrentModule(state, runtime) ==
              0 &&
              runtime.events == std::vector<std::string>{"complete_cleanup"},
          "finished record with no backing array completes cleanup");

    pal4::ScriptStackModuleRecord records[2]{};
    runtime.events.clear();
    runtime.expected_record = &records[1];
    records[0].function_index = 2;
    records[0].script_context = runtime.expected_context;
    records[1].function_index = 0;
    records[1].script_context = runtime.expected_context;
    state = StateFor(records[1], runtime);
    state.buffer_20 = records;
    state.buffer_24 = records + 2;
    state.buffer_28 = records + 2;
    runtime.results = {4};
    Check(pal4::ScriptExecutionContext::ExecuteCurrentModule(state, runtime) ==
              4 && state.buffer_24 == records + 1 &&
              state.current_stack_record == static_cast<std::uint32_t>(
                  reinterpret_cast<std::uintptr_t>(&records[0])) &&
              runtime.events == std::vector<std::string>{
                                    "stop", "release", "remove", "execute"},
          "finished top record is released, popped and previous executes");

    runtime.events.clear();
    runtime.result_index = 0;
    records[0].function_index = 0;
    runtime.expected_record = &records[0];
    Check(pal4::ScriptExecutionContext::ExecuteCurrentModule(state, runtime) ==
              0 && state.buffer_24 == records &&
              runtime.events == std::vector<std::string>{
                                    "stop", "release", "remove",
                                    "complete_cleanup"},
          "last finished record completes cleanup and returns zero");

    records[0].function_index = 0;
    records[0].script_context = runtime.expected_context;
    records[1].function_index = 2;
    state.current_stack_record = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&records[0]));
    state.buffer_24 = records + 2;
    runtime.results = {6};
    runtime.result_index = 0;
    runtime.events.clear();
    Check(pal4::ScriptExecutionContext::ExecuteCurrentModule(state, runtime) ==
              6 && state.buffer_24 == records + 2 &&
              runtime.events == std::vector<std::string>{"execute"},
          "finished non-top record is executed without popping the stack");
}

void TestAbortAndExecutionModes() {
    Runtime runtime;
    pal4::ScriptStackModuleRecord record{};
    auto state = StateFor(record, runtime);
    record.function_index = 2;
    record.execution_flag = 1;
    runtime.results = {5};
    Check(pal4::ScriptExecutionContext::ExecuteCurrentModule(state, runtime) ==
              1 && record.function_index == 1 &&
              runtime.events == std::vector<std::string>{"abort"} &&
              runtime.result_index == 0,
          "pause flag aborts context before any execute call");

    record.execution_flag = 0;
    runtime.events.clear();
    record.word_268 = 0;
    Check(pal4::ScriptExecutionContext::ExecuteCurrentModule(state, runtime) ==
              5 && record.function_index == 5 && runtime.result_index == 1 &&
              runtime.events == std::vector<std::string>{"execute"},
          "ordinary mode executes exactly once");

    runtime.events.clear();
    runtime.results = {2, 0};
    runtime.result_index = 0;
    record.function_index = 1;
    record.word_268 = 1;
    Check(pal4::ScriptExecutionContext::ExecuteCurrentModule(state, runtime) ==
              0 && record.function_index == 0 && runtime.result_index == 2 &&
              runtime.events == std::vector<std::string>{"execute", "execute"},
          "debug mode one executes until status becomes zero");

    runtime.events.clear();
    runtime.results = {-1};
    runtime.result_index = 0;
    record.function_index = 2;
    record.word_268 = 0;
    Check(pal4::ScriptExecutionContext::ExecuteCurrentModule(state, runtime) ==
              -1 && runtime.events == std::vector<std::string>{"execute"},
          "minus-one execution status exits before exception lookup");
}

void TestExceptionReporting() {
    Runtime runtime;
    pal4::ScriptStackModuleRecord record{};
    auto state = StateFor(record, runtime);
    record.function_index = 2;
    runtime.results = {3};
    const std::string expected =
        "< catched an exception >\r\nreason:  division by zero\r\n"
        "func:    void main\r\nline:    42\r\n";
    Check(pal4::ScriptExecutionContext::ExecuteCurrentModule(state, runtime) ==
              3 && runtime.reported == expected &&
              runtime.events == std::vector<std::string>{
                                    "execute", "function_name", "line",
                                    "reason", "declaration", "report"},
          "exception status assembles diagnostic and reports through logger");

    state.word_8 = 0;
    runtime.events.clear();
    runtime.result_index = 0;
    try {
        pal4::ScriptExecutionContext::ExecuteCurrentModule(state, runtime);
        Check(false, "missing logger must raise exception");
    } catch (const std::runtime_error&) {
        Check(runtime.raised == expected &&
                  runtime.events.back() == "raise",
              "exception text is thrown when logger is absent");
    }
}

}  // namespace

int main() {
    TestMissingAndFinished();
    TestAbortAndExecutionModes();
    TestExceptionReporting();
}
