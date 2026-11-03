#include "pal4/stack_trace_formatter.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IStackTraceRuntime {
public:
    [[nodiscard]] bool IsBadReadPointer(
        const std::uint32_t address,
        const std::uint32_t bytes) override {
        Check(address == 0x1000u && bytes == 8u,
              "stack trace validates two-pointer exception record");
        ++pointer_checks;
        return bad_pointer;
    }

    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t address) const override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }

    void InitializeSymbolHandler() override {
        ++initialize_calls;
    }

    [[nodiscard]] std::uint32_t GetCurrentThread() override {
        ++thread_calls;
        return 0x9000u;
    }

    [[nodiscard]] std::uint32_t GetCurrentProcessId() override {
        ++process_id_calls;
        return 0x1234u;
    }

    [[nodiscard]] std::uint32_t GetCurrentProcess() override {
        ++process_calls;
        return 0xa000u;
    }

    [[nodiscard]] std::uint32_t WalkStack(
        const std::uint32_t machine, const std::uint32_t process_id,
        const std::uint32_t thread, const std::uint32_t frame,
        const std::uint32_t context,
        const std::uint32_t read_callback) override {
        Check(machine == 0x14cu && process_id == 0x1234u &&
                  thread == 0x9000u && frame ==
                      pal4::StackTraceFormatter::kFrameAddress &&
                  context == 0x3000u && read_callback ==
                      pal4::StackTraceFormatter::kReadCallback,
              "StackWalk receives x86 machine, global frame and read callback");
        ++walk_calls;
        if (throw_on_walk) {
            throw std::runtime_error("synthetic stack-walk exception");
        }
        words[frame] = 0x401234u;
        words[frame + 24u] = frame_offset;
        words[frame + 52u] = 1u;
        words[frame + 56u] = 2u;
        words[frame + 60u] = 3u;
        words[frame + 64u] = 4u;
        return walk_result;
    }

    [[nodiscard]] std::uint32_t SymGetModuleBase(
        const std::uint32_t process_id,
        const std::uint32_t address) override {
        Check(process_id == 0x1234u && address == 0x401234u,
              "module lookup uses process ID and walked PC");
        ++module_base_calls;
        return module_base;
    }

    [[nodiscard]] std::string GetModuleFileName(
        const std::uint32_t process, const std::uint32_t module,
        const std::uint32_t max_chars) override {
        Check(process == 0xa000u && module == 0x400000u &&
                  max_chars == 1024u,
              "module filename wrapper receives current process and base");
        ++module_name_calls;
        return module_name;
    }

    [[nodiscard]] int TryGetSymbol(
        const std::uint32_t process_id, const std::uint32_t address,
        std::uint32_t& displacement,
        pal4::CrashReportSymbol& symbol) override {
        Check(process_id == 0x1234u && address == 0x401234u &&
                  symbol.size_of_struct == 24u &&
                  symbol.max_name_length == 488u,
              "symbol record is initialized before DbgHelp lookup");
        ++symbol_calls;
        displacement = symbol_displacement;
        symbol.name = symbol_name;
        return symbol_result;
    }

    [[nodiscard]] int TryGetLine(
        const std::uint32_t process_id, const std::uint32_t address,
        std::uint32_t& displacement,
        pal4::CrashReportLine& line) override {
        Check(process_id == 0x1234u && address == 0x401234u &&
                  line.size_of_struct == 20u && line.key == 0u &&
                  line.line_number == 0u && line.file_name.empty() &&
                  line.address == 0u,
              "source line record is zeroed and assigned its size");
        ++line_calls;
        displacement = line_displacement;
        line.file_name = file_name;
        line.line_number = line_number;
        return line_result;
    }

    void SeedException() {
        words[0x1004u] = 0x3000u;
        words[0x3000u + 188u] = 0x23u;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::string module_name = "engine.dll";
    std::string symbol_name = "RunGame";
    std::string file_name = "main.cpp";
    std::uint32_t frame_offset = 0x7000u;
    std::uint32_t walk_result = 1u;
    std::uint32_t module_base = 0x400000u;
    std::uint32_t symbol_displacement = 12u;
    std::uint32_t line_displacement = 6u;
    std::uint32_t line_number = 42u;
    int symbol_result = 1;
    int line_result = 1;
    int pointer_checks = 0;
    int initialize_calls = 0;
    int thread_calls = 0;
    int process_id_calls = 0;
    int process_calls = 0;
    int walk_calls = 0;
    int module_base_calls = 0;
    int module_name_calls = 0;
    int symbol_calls = 0;
    int line_calls = 0;
    bool bad_pointer = false;
    bool throw_on_walk = false;
};

void CheckInvalidAndUnavailableFrames() {
    Runtime runtime;
    runtime.SeedException();
    pal4::StackTraceFormatter formatter;
    runtime.bad_pointer = true;
    Check(formatter.Generate(0u, 0x1000u, runtime) == nullptr &&
              runtime.initialize_calls == 0 && formatter.FrameCount() == 0u,
          "bad input exits before symbol initialization");
    runtime.bad_pointer = false;
    runtime.walk_result = 0u;
    Check(formatter.Generate(0u, 0x1000u, runtime) == nullptr &&
              formatter.FrameCount() == 0u &&
              runtime.module_base_calls == 0,
          "failed StackWalk returns null and clears count");
    runtime.walk_result = 1u;
    runtime.frame_offset = 0u;
    Check(formatter.Generate(0u, 0x1000u, runtime) == nullptr &&
              formatter.FrameCount() == 0u,
          "zero walked frame offset returns null");
    runtime.frame_offset = 0x7000u;
    runtime.module_base = 0u;
    Check(formatter.Generate(0u, 0x1000u, runtime) == nullptr &&
              formatter.FrameCount() == 0u,
          "unknown walked module returns null");
}

void CheckBasicFrameAndOptionalFields() {
    Runtime runtime;
    runtime.SeedException();
    pal4::StackTraceFormatter formatter;
    Check(formatter.Generate(0u, 0x1000u, runtime) != nullptr &&
              formatter.Report() == "0023:00401234" &&
              formatter.FrameCount() == 1u &&
              runtime.module_name_calls == 0 &&
              runtime.symbol_calls == 0 && runtime.line_calls == 0,
          "unflagged frame contains only selector and walked PC");
    Check(formatter.Generate(0u, 0x1000u, runtime) != nullptr &&
              formatter.FrameCount() == 2u,
          "each complete frame increments global count");

    pal4::StackTraceFormatter full;
    Check(full.Generate(15u, 0x1000u, runtime) != nullptr &&
              full.Report() ==
                  "0023:00401234 (0x00000001 0x00000002 0x00000003 0x00000004)"
                  "\n           engine.dll\n"
                  "           RunGame()+0012 byte(s)\n"
                  "           main.cpp\n           line 0042+0006 byte(s)\n\n" &&
              full.FrameCount() == 1u && full.FirstSymbol() == "RunGame" &&
              full.FirstFile() == "main.cpp" && full.FirstLine() == 42u,
          "all four options format parameters, module, symbol and line");
    runtime.symbol_name = "Next";
    runtime.file_name = "other.cpp";
    runtime.line_number = 99u;
    Check(full.Generate(15u, 0x1000u, runtime) != nullptr &&
              full.FrameCount() == 2u && full.FirstSymbol() == "RunGame" &&
              full.FirstFile() == "main.cpp" && full.FirstLine() == 42u,
          "first-frame symbol and file cache are not overwritten later");
}

void CheckSymbolAndLineFailureBranches() {
    Runtime runtime;
    runtime.SeedException();
    pal4::StackTraceFormatter formatter;
    runtime.symbol_result = 2;
    Check(formatter.Generate(12u, 0x1000u, runtime) != nullptr &&
              formatter.Report() == "0023:00401234" &&
              formatter.FrameCount() == 0u && runtime.line_calls == 0,
          "symbol result other than one returns partial report and resets count");
    runtime.symbol_result = 1;
    runtime.line_result = 2;
    runtime.symbol_displacement = 0u;
    Check(formatter.Generate(12u, 0x1000u, runtime) != nullptr &&
              formatter.Report() ==
                  "0023:00401234\n           RunGame\n" &&
              formatter.FrameCount() == 1u && formatter.FirstFile().empty(),
          "line result other than one still counts frame");

    pal4::StackTraceFormatter line_only;
    runtime.line_result = 1;
    Check(line_only.Generate(8u, 0x1000u, runtime) != nullptr &&
              line_only.Report() ==
                  "0023:00401234           main.cpp\n"
                  "           line 0042+0006 byte(s)\n\n" &&
              line_only.FirstSymbol().empty() &&
              line_only.FirstFile() == "main.cpp",
          "line option can run without symbol option");
}

void CheckTruncationAndExceptionCleanup() {
    Runtime runtime;
    runtime.SeedException();
    pal4::StackTraceFormatter formatter;
    runtime.symbol_name.assign(2000u, 'S');
    Check(formatter.Generate(12u, 0x1000u, runtime) != nullptr &&
              formatter.Report().size() == 1022u &&
              formatter.FrameCount() == 0u && runtime.line_calls == 0,
          "long symbol truncates report and returns before line query");
    runtime.symbol_name = "RunGame";
    runtime.file_name.assign(2000u, 'F');
    Check(formatter.Generate(12u, 0x1000u, runtime) != nullptr &&
              formatter.Report().size() == 1022u &&
              formatter.FrameCount() == 0u,
          "long source path truncates report and clears frame count");

    runtime.file_name = "main.cpp";
    Check(formatter.Generate(12u, 0x1000u, runtime) != nullptr &&
              formatter.FirstSymbol() == "RunGame" &&
              formatter.FirstFile() == "main.cpp" &&
              formatter.FirstLine() == 42u,
          "successful frame populates first-frame caches");
    const std::string previous = formatter.Report();
    runtime.bad_pointer = true;
    Check(formatter.Generate(12u, 0x1000u, runtime) == nullptr &&
              formatter.Report() == previous && formatter.FrameCount() == 1u,
          "bad pointer leaves existing report and count intact");
    runtime.bad_pointer = false;
    runtime.throw_on_walk = true;
    Check(formatter.Generate(12u, 0x1000u, runtime) == nullptr &&
              formatter.FrameCount() == 0u &&
              formatter.FirstSymbol().empty() &&
              formatter.FirstFile().empty() &&
              formatter.FirstLine() == 0u,
          "exception cleanup clears count, first names and first line");
}

}  // namespace

int main() {
    CheckInvalidAndUnavailableFrames();
    CheckBasicFrameAndOptionalFields();
    CheckSymbolAndLineFailureBranches();
    CheckTruncationAndExceptionCleanup();
    std::cout << "4 stack trace formatter test cases passed\n";
}
