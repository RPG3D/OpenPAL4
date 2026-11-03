#include "pal4/symbol_handler.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ISymbolHandlerRuntime {
public:
    [[nodiscard]] std::uint32_t SymGetOptions() override {
        events.emplace_back("get-options");
        return options;
    }

    void SymSetOptions(const std::uint32_t value) override {
        events.emplace_back("set-options");
        set_options = value;
    }

    [[nodiscard]] std::uint32_t GetCurrentProcessId() override {
        events.emplace_back("process-id");
        return 0x1234u;
    }

    [[nodiscard]] std::uint32_t InitializeAndLoadModules(
        const std::uint32_t process_id,
        const std::uint32_t process_handle,
        const std::uint32_t search_path,
        const std::uint32_t invade_process) override {
        events.emplace_back("initialize");
        Check(process_id == 0x1234u && process_handle == 0x1234u &&
                  search_path == 0u && invade_process == 1u,
              "symbol loading uses process ID twice, null path and invade flag");
        return initialize_result;
    }

    [[nodiscard]] int SymGetLineFromAddr(
        const std::uint32_t process, const std::uint32_t address,
        std::uint32_t& displacement,
        pal4::CrashReportLine& line) override {
        events.emplace_back("line");
        Check(process == 0x9000u && address == 0x401234u &&
                  line.size_of_struct == 20u,
              "line wrapper forwards process, address and line structure");
        displacement = 6u;
        line.line_number = 42u;
        return line_result;
    }

    std::vector<std::string> events;
    std::uint32_t options = 0x80000002u;
    std::uint32_t set_options = 0u;
    std::uint32_t initialize_result = 7u;
    int line_result = 2;
};

void CheckColdAndWarmInitialization() {
    std::uint32_t guard = 0u;
    Runtime runtime;
    pal4::SymbolHandler handler(guard);
    Check(handler.Initialize(runtime) == 7u && guard == 1u &&
              runtime.set_options == 0x80000016u &&
              runtime.events == std::vector<std::string>{
                  "get-options", "set-options", "process-id", "initialize"},
          "cold initialization sets options and returns loader result");
    runtime.events.clear();
    Check(handler.Initialize(runtime) == 1u && runtime.events.empty(),
          "warm path returns cached guard without external calls");

    guard = 9u;
    Check(handler.Initialize(runtime) == 9u && runtime.events.empty(),
          "non-one nonzero guard value is returned unchanged");
}

void CheckFailedLoadStillSetsGuard() {
    std::uint32_t guard = 0u;
    Runtime runtime;
    runtime.initialize_result = 0u;
    pal4::SymbolHandler handler(guard);
    Check(handler.Initialize(runtime) == 0u && guard == 1u,
          "failed symbol load still marks initialization guard");
    runtime.events.clear();
    Check(handler.Initialize(runtime) == 1u && runtime.events.empty(),
          "failed first load is not retried");
}

void CheckLineForwarding() {
    Runtime runtime;
    pal4::CrashReportLine line;
    std::uint32_t displacement = 0u;
    Check(pal4::SymbolHandler::GetLineInfo(
              0x9000u, 0x401234u, displacement, line, runtime) == 2 &&
              displacement == 6u && line.line_number == 42u &&
              runtime.events == std::vector<std::string>{"line"},
          "line wrapper returns raw API result and passes output references");
}

}  // namespace

int main() {
    CheckColdAndWarmInitialization();
    CheckFailedLoadStillSetsGuard();
    CheckLineForwarding();
    std::cout << "3 symbol handler test cases passed\n";
}
