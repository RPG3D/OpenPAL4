#include "pal4/script_execution_context.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int registration_engine_marker = 1;

int RegisterExtra(void* engine) {
    Check(engine == &registration_engine_marker,
          "callback receives the created engine");
    return 73;
}

struct Runtime final : pal4::IScriptEngineBoundary {
    std::vector<std::string> events;
    void* created_engine = &registration_engine_marker;
    void* seen_engine = nullptr;
    pal4::ScriptEngineRegistrationCallback seen_callback = nullptr;
    std::uint32_t seen_version = 0;

    void* CreateEngine(std::uint32_t version) override {
        events.push_back("create");
        seen_version = version;
        return created_engine;
    }
    void RegisterStringFunctions(void* engine) override {
        events.push_back("strings");
        seen_engine = engine;
    }
    void RegisterMathFunctions(void* engine) override {
        events.push_back("math");
        Check(engine == seen_engine, "math uses stored engine");
    }
    void RegisterGameFunctions(void* engine) override {
        events.push_back("game");
        Check(engine == seen_engine, "game registrations use stored engine");
    }
    int InvokeRegistration(
        pal4::ScriptEngineRegistrationCallback callback,
        void* engine) override {
        events.push_back("callback");
        seen_callback = callback;
        return callback(engine);
    }
    int CompleteCacheCleanup(
        pal4::ScriptExecutionContextState& context) override {
        events.push_back("cache_cleanup");
        Check(context.word_8 == 0, "callback/context word cleared first");
        return 17;
    }
    int ReleaseEngine(void* engine) override {
        events.push_back("release_engine");
        Check(engine == created_engine, "release uses current engine");
        return 23;
    }
};

}  // namespace

int main() {
    using pal4::ScriptExecutionContext;
    pal4::ScriptExecutionContextState state{};
    Runtime runtime;

    Check(ScriptExecutionContext::InitializeEngine(
              state, RegisterExtra, 0xaabbccddu, runtime) == 73 &&
              state.script_engine == &registration_engine_marker &&
              state.word_8 == 0xaabbccddu && runtime.seen_version == 20200 &&
              runtime.seen_callback == RegisterExtra &&
              ScriptExecutionContext::GetEngine(state) == state.script_engine,
          "startup stores engine/context and returns callback result");
    Check(runtime.events == std::vector<std::string>(
                                {"create", "strings", "math", "game", "callback"}),
          "startup preserves registration order");

    runtime.events.clear();
    Check(ScriptExecutionContext::CleanupEngine(state, runtime) == 23 &&
              state.word_8 == 0 && !state.script_engine &&
              runtime.events == std::vector<std::string>(
                                    {"cache_cleanup", "release_engine"}),
          "cleanup result comes from engine release after cache cleanup");

    runtime.events.clear();
    Check(ScriptExecutionContext::CleanupEngine(state, runtime) == 17 &&
              runtime.events == std::vector<std::string>({"cache_cleanup"}),
          "missing engine returns cache cleanup result");

    runtime.events.clear();
    Check(ScriptExecutionContext::InitializeEngine(
              state, nullptr, 0x1234, runtime) == 0 &&
              runtime.events == std::vector<std::string>(
                                    {"create", "strings", "math", "game"}),
          "absent callback returns zero after all base registrations");

    state.script_engine = nullptr;
    runtime.created_engine = nullptr;
    runtime.events.clear();
    Check(ScriptExecutionContext::InitializeEngine(
              state, nullptr, 0, runtime) == 0 &&
              runtime.seen_engine == nullptr &&
              runtime.events == std::vector<std::string>(
                                    {"create", "strings", "math", "game"}),
          "original wrapper forwards null engine to registration calls");
}
