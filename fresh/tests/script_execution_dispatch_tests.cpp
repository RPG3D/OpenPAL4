#include "pal4/script_execution_context.h"
#include "pal4/script_wait_handler_base.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptHandlerDispatchRuntime {
    std::vector<std::string> events;
    std::vector<pal4::ScriptWaitHandlerBaseState*> processed;
    std::vector<pal4::ScriptWaitHandlerBaseState*> deleted;
    std::vector<pal4::ScriptWaitHandlerBaseState*> close_on_process;
    pal4::ScriptStackModuleRecord* expected_current = nullptr;

    void Process(pal4::ScriptWaitHandlerBaseState& handler,
                 int first, int second) override {
        Check(first == 17 && second == 23,
              "process forwards both arguments in order");
        events.emplace_back("process");
        processed.push_back(&handler);
        for (auto* candidate : close_on_process) {
            if (candidate == &handler) handler.closed = 1;
        }
    }
    void Delete(pal4::ScriptWaitHandlerBaseState& handler,
                unsigned flags) override {
        Check(flags == 1, "closed handler deleting-destructor flag");
        events.emplace_back("delete");
        deleted.push_back(&handler);
    }
    int ExecuteCurrentModule(pal4::ScriptExecutionContextState&) override {
        if (expected_current) {
            Check(expected_current->execution_flag == 0,
                  "execution flag cleared before module execution");
        }
        events.emplace_back("execute");
        return 91;
    }
};

void TestEmptyAndOpen() {
    pal4::ScriptExecutionContextState state{};
    Runtime runtime;
    Check(pal4::ScriptExecutionContext::ProcessWaitHandlers(
              state, 17, 23, runtime) == 91 &&
              runtime.events == std::vector<std::string>{"execute"},
          "empty handler array only executes current module");

    pal4::ScriptWaitHandlerBaseState open{};
    pal4::ScriptWaitHandlerBaseState* handlers[] = {&open};
    state.buffer_36 = handlers;
    state.buffer_40 = handlers + 1;
    runtime.events.clear();
    Check(pal4::ScriptExecutionContext::ProcessWaitHandlers(
              state, 17, 23, runtime) == 91 &&
              state.buffer_40 == handlers + 1 &&
              runtime.processed == std::vector{&open} &&
              runtime.deleted.empty() &&
              runtime.events == std::vector<std::string>{
                                    "process", "execute"},
          "open handler remains and does not clear execution flag");
}

void TestConsecutiveClosed() {
    pal4::ScriptExecutionContextState state{};
    Runtime runtime;
    pal4::ScriptWaitHandlerBaseState first{};
    pal4::ScriptWaitHandlerBaseState second{};
    pal4::ScriptWaitHandlerBaseState third{};
    pal4::ScriptWaitHandlerBaseState* handlers[] = {&first, &second, &third};
    state.buffer_36 = handlers;
    state.buffer_40 = handlers + 3;
    runtime.close_on_process = {&first, &second};
    Check(pal4::ScriptExecutionContext::ProcessWaitHandlers(
              state, 17, 23, runtime) == 91 &&
              state.buffer_40 == handlers + 1 && handlers[0] == &third &&
              runtime.processed == std::vector{&first, &second, &third} &&
              runtime.deleted == std::vector{&first, &second} &&
              runtime.events == std::vector<std::string>{
                                    "process", "delete", "process", "delete",
                                    "process", "execute"},
          "consecutive closed handlers are removed without skipping successor");
}

void TestLastRemovedEarlyExit() {
    pal4::ScriptExecutionContextState state{};
    Runtime runtime;
    pal4::ScriptWaitHandlerBaseState open{};
    pal4::ScriptWaitHandlerBaseState closed{};
    pal4::ScriptWaitHandlerBaseState* handlers[] = {&open, &closed};
    state.buffer_36 = handlers;
    state.buffer_40 = handlers + 2;
    pal4::ScriptStackModuleRecord current{};
    current.function_index = 2;
    current.execution_flag = 1;
    state.current_stack_record = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&current));
    runtime.expected_current = &current;
    runtime.close_on_process = {&closed};
    Check(pal4::ScriptExecutionContext::ProcessWaitHandlers(
              state, 17, 23, runtime) == 91 &&
              state.buffer_40 == handlers + 1 && handlers[0] == &open &&
              runtime.events == std::vector<std::string>{
                                    "process", "process", "delete",
                                    "execute"},
          "removing final cursor clears flag even with earlier open handler");

    pal4::ScriptWaitHandlerBaseState* only[] = {&closed};
    closed.closed = 1;
    state.buffer_36 = only;
    state.buffer_40 = only + 1;
    current.execution_flag = 1;
    runtime.close_on_process.clear();
    runtime.events.clear();
    Check(pal4::ScriptExecutionContext::ProcessWaitHandlers(
              state, 17, 23, runtime) == 91 &&
              state.buffer_40 == only &&
              runtime.events == std::vector<std::string>{
                                    "process", "delete", "execute"},
          "single closed handler clears flag before module execution");
}

}  // namespace

int main() {
    TestEmptyAndOpen();
    TestConsecutiveClosed();
    TestLastRemovedEarlyExit();
}
