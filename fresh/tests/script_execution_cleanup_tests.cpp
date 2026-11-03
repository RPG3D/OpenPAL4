#include "pal4/script_execution_context.h"

#include <cstdlib>
#include <cstring>
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

struct Runtime final : pal4::IScriptExecutionCleanupRuntime {
    std::vector<std::string> events;
    std::vector<std::uint32_t> stopped;
    std::vector<std::uint32_t> released;
    std::vector<void*> reset_handlers;
    std::vector<void*> deleted_handlers;

    void StopScriptContext(std::uint32_t context) override {
        events.emplace_back("stop_context");
        stopped.push_back(context);
    }
    void ReleaseScriptContext(std::uint32_t context) override {
        events.emplace_back("release_context");
        released.push_back(context);
    }
    void ResetHandler(void* handler) override {
        events.emplace_back("reset_handler");
        reset_handlers.push_back(handler);
    }
    void DeleteHandler(void* handler, unsigned flags) override {
        Check(flags == 1, "deleting destructor flag");
        events.emplace_back("delete_handler");
        deleted_handlers.push_back(handler);
    }
};

void TestHandlerCleanup() {
    pal4::ScriptExecutionContextState state{};
    Runtime runtime;
    void* handlers[] = {reinterpret_cast<void*>(0x1000u),
                        reinterpret_cast<void*>(0x2000u)};
    state.buffer_36 = handlers;
    state.buffer_40 = handlers + 2;
    Check(pal4::ScriptExecutionContext::CleanupHandlers(state, runtime) ==
              0 && state.buffer_40 == handlers &&
              runtime.reset_handlers == std::vector<void*>{handlers[0],
                                                            handlers[1]} &&
              runtime.deleted_handlers == runtime.reset_handlers &&
              runtime.events == std::vector<std::string>{
                                    "reset_handler", "delete_handler",
                                    "reset_handler", "delete_handler"},
          "handler cleanup preserves virtual-call and reset order");
    runtime.events.clear();
    Check(pal4::ScriptExecutionContext::CleanupHandlers(state, runtime) ==
              0 && runtime.events.empty(),
          "empty handler span still clears execution flag");

    pal4::ScriptStackModuleRecord current{};
    current.function_index = 2;
    current.execution_flag = 1;
    state.current_stack_record = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&current));
    Check(pal4::ScriptExecutionContext::CleanupHandlers(state, runtime) ==
              static_cast<int>(reinterpret_cast<std::uintptr_t>(&current)) &&
              current.execution_flag == 0,
          "handler cleanup invokes concrete execution-flag clear");
}

void TestRemoveOtherModules() {
    pal4::ScriptExecutionContextState state{};
    Runtime runtime;
    auto* records = new pal4::ScriptStackModuleRecord[4]{};
    const char* names[] = {"old-a", "KeEp", "old-b", "keep"};
    for (unsigned i = 0; i < 4; ++i) {
        std::strcpy(records[i].module_name, names[i]);
        records[i].script_context = 0x100u + i;
    }
    state.buffer_20 = records;
    state.buffer_24 = records + 4;
    state.buffer_28 = records + 4;
    state.current_stack_record = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&records[3]));
    const int expected_return = static_cast<int>(
        reinterpret_cast<std::uintptr_t>(&records[3]));
    Check(pal4::ScriptExecutionContext::RemoveOtherStackModules(
              state, runtime) == expected_return &&
              state.buffer_24 == records + 2 &&
              std::string(records[0].module_name) == "KeEp" &&
              std::string(records[1].module_name) == "keep" &&
              records[0].script_context == 0x101u &&
              records[1].script_context == 0x103u &&
              runtime.stopped == std::vector<std::uint32_t>{0x100u, 0x102u} &&
              runtime.released == runtime.stopped &&
              runtime.events == std::vector<std::string>{
                                    "stop_context", "release_context",
                                    "stop_context", "release_context"},
          "filter releases mismatches and compacts repeated removals");
    delete[] records;
}

void TestCompleteCleanupOrder() {
    pal4::ScriptExecutionContextState state{};
    Runtime runtime;
    pal4::ScriptStackModuleRecord records[2]{};
    records[0].script_context = 0x100u;
    records[1].script_context = 0x101u;
    state.buffer_20 = records;
    state.buffer_24 = records + 2;
    state.buffer_28 = records + 2;
    void* handler = reinterpret_cast<void*>(0x1234u);
    state.buffer_36 = &handler;
    state.buffer_40 = &handler + 1;
    state.current_stack_record = 0x4321u;
    Check(pal4::ScriptExecutionContext::CompleteStackCleanup(
              state, runtime) == 0 &&
              state.current_stack_record == 0 &&
              state.buffer_24 == records &&
              records[0].script_context == 0 &&
              records[1].script_context == 0 &&
              runtime.stopped == std::vector<std::uint32_t>{0x101u, 0x100u} &&
              runtime.released == runtime.stopped &&
              runtime.events == std::vector<std::string>{
                                    "stop_context", "release_context",
                                    "stop_context", "release_context",
                                    "reset_handler", "delete_handler"},
          "complete cleanup clears stack before handlers and flag");

    runtime.events.clear();
    state.current_stack_record = 0x4321u;
    Check(pal4::ScriptExecutionContext::ClearStack(state, runtime) == 0 &&
              state.current_stack_record == 0 &&
              state.buffer_24 == records && runtime.events.empty(),
          "empty allocated stack only resets current record");

    pal4::ScriptExecutionContextState null_state{};
    null_state.current_stack_record = 0x4321u;
    Check(pal4::ScriptExecutionContext::ClearStack(null_state, runtime) == 0 &&
              null_state.current_stack_record == 0 &&
              null_state.buffer_20 == nullptr,
          "null stack also resets current record");
}

}  // namespace

int main() {
    TestHandlerCleanup();
    TestRemoveOtherModules();
    TestCompleteCleanupOrder();
}
