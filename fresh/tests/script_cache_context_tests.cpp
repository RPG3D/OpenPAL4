#include "pal4/script_cache_context.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptCacheContextRuntime {
    std::int32_t count = 0;
    std::vector<std::string> calls;
    std::uint32_t executor_calls = 0u;

    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x1000u + executor_calls++ * 0x1000u;
    }
    std::int32_t EntryCount(std::uint32_t cache) override {
        Check(cache == 0x1054u, "first cache address");
        calls.emplace_back("count");
        return count;
    }
    std::int32_t RemoveExceptCurrent(std::uint32_t cache) override {
        Check(cache == 0x2054u, "refreshed cache address");
        calls.emplace_back("remove");
        return -7;
    }
};

void CountBranches() {
    for (auto count : {-1, 0, 1}) {
        Runtime runtime;
        runtime.count = count;
        Check(pal4::ScriptCacheContext::ClearExceptCurrent(runtime) == count &&
                  runtime.calls == std::vector<std::string>{
                      "executor", "count"},
              "zero or one cache entry returns count unchanged");
    }
    for (auto count : {2, 100}) {
        Runtime runtime;
        runtime.count = count;
        Check(pal4::ScriptCacheContext::ClearExceptCurrent(runtime) == -7 &&
                  runtime.calls == std::vector<std::string>{
                      "executor", "count", "executor", "remove"},
              "multiple entries reacquire executor and forward cleanup");
    }
}

}  // namespace

int main() {
    try {
        CountBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
