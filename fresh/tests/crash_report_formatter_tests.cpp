#include "pal4/crash_report_formatter.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
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

class Runtime final : public pal4::ICrashReportRuntime {
public:
    [[nodiscard]] bool IsBadReadPointer(
        const std::uint32_t address,
        const std::uint32_t bytes) override {
        Check(address == 0x1000u && bytes == 8u,
              "IsBadReadPtr validates the two-pointer record");
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

    [[nodiscard]] std::uint32_t GetCurrentProcess() override {
        ++process_calls;
        return 0x9000u;
    }

    [[nodiscard]] std::uint32_t GetCurrentProcessId() override {
        ++process_id_calls;
        return 0x1234u;
    }

    [[nodiscard]] std::string GetModuleFileName(
        const std::uint32_t process, const std::uint32_t module,
        const std::uint32_t max_chars) override {
        Check(process == 0x9000u && max_chars != 0u,
              "module path uses process handle and nonzero size");
        module_queries.push_back(module);
        return module == 0u ? executable_name : module_name;
    }

    [[nodiscard]] std::uint32_t GetModuleHandle(
        const char* module_name_arg) override {
        Check(std::string(module_name_arg) == "NTDLL.DLL",
              "fallback loads NTDLL message source");
        ++module_handle_calls;
        return 0x8000u;
    }

    [[nodiscard]] std::string FormatMessage(
        const std::uint32_t flags, const std::uint32_t module,
        const std::uint32_t code, const std::uint32_t language,
        const std::uint32_t max_chars) override {
        Check(flags == 0xa00u && module == 0x8000u &&
                  code == 0xe1234567u && language == 0u &&
                  max_chars == 1024u,
              "FormatMessage uses observed flags, source and size");
        ++format_message_calls;
        return fallback_message;
    }

    [[nodiscard]] std::uint32_t SymGetModuleBase(
        const std::uint32_t process_id,
        const std::uint32_t address) override {
        Check(process_id == 0x1234u && address == 0x401234u,
              "SymGetModuleBase receives process ID and exception address");
        return module_base;
    }

    [[nodiscard]] int TryGetSymbol(
        const std::uint32_t process_id, const std::uint32_t address,
        std::uint32_t& displacement,
        pal4::CrashReportSymbol& symbol) override {
        Check(process_id == 0x1234u && address == 0x401234u &&
                  symbol.size_of_struct == 24u &&
                  symbol.max_name_length == 488u,
              "symbol query initializes observed IMAGEHLP_SYMBOL fields");
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
              "line query initializes observed IMAGEHLP_LINE fields");
        ++line_calls;
        displacement = line_displacement;
        line.file_name = file_name;
        line.line_number = line_number;
        return line_result;
    }

    void SeedException() {
        words[0x1000u] = 0x2000u;
        words[0x1004u] = 0x3000u;
        words[0x2000u] = 0xc0000005u;
        words[0x200cu] = 0x401234u;
        words[0x3000u + 188u] = 0x23u;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> module_queries;
    std::string executable_name = "game.exe";
    std::string module_name = "engine.dll";
    std::string fallback_message = "unknown error";
    std::string symbol_name = "RunGame";
    std::string file_name = "main.cpp";
    std::uint32_t module_base = 0u;
    std::uint32_t symbol_displacement = 0u;
    std::uint32_t line_displacement = 0u;
    std::uint32_t line_number = 42u;
    int symbol_result = 0;
    int line_result = 0;
    int pointer_checks = 0;
    int initialize_calls = 0;
    int process_calls = 0;
    int process_id_calls = 0;
    int module_handle_calls = 0;
    int format_message_calls = 0;
    int symbol_calls = 0;
    int line_calls = 0;
    bool bad_pointer = false;
};

void CheckInvalidPointerPreservesPreviousReport() {
    Runtime runtime;
    runtime.SeedException();
    pal4::CrashReportFormatter formatter;
    Check(formatter.Generate(0x1000u, runtime) != nullptr,
          "valid input produces a report");
    const std::string previous = formatter.Report();
    runtime.bad_pointer = true;
    Check(formatter.Generate(0x1000u, runtime) == nullptr &&
              formatter.Report() == previous &&
              runtime.initialize_calls == 1,
          "bad read pointer exits before initialization without clearing buffer");
}

void CheckKnownDescriptionAndUnknownModule() {
    Runtime runtime;
    runtime.SeedException();
    pal4::CrashReportFormatter formatter;
    Check(formatter.Generate(0x1000u, runtime) != nullptr &&
              formatter.Report() ==
                  "Reason:    game.exe caused an "
                  "{**EXCEPTION_ACCESS_VIOLATION**}\n"
                  "Location:  in module <UNKNOWN> at 0023:00401234",
          "known exception and missing module produce observed report text");
    Check(runtime.module_queries == std::vector<std::uint32_t>{0u} &&
              runtime.module_handle_calls == 0 &&
              runtime.format_message_calls == 0 &&
              runtime.symbol_calls == 1 && runtime.line_calls == 0,
          "known description bypasses FormatMessage and failed symbol bypasses line");
}

void CheckMessageFallbackAndNamedModule() {
    Runtime runtime;
    runtime.SeedException();
    runtime.words[0x2000u] = 0xe1234567u;
    runtime.module_base = 0x400000u;
    runtime.symbol_result = 2;
    pal4::CrashReportFormatter formatter;
    Check(formatter.Generate(0x1000u, runtime) != nullptr &&
              formatter.Report() ==
                  "Reason:    game.exe caused an unknown error\n"
                  "Location:  in module engine.dll at 0023:00401234",
          "fallback message and resolved module path are appended");
    Check(runtime.module_queries ==
              std::vector<std::uint32_t>{0u, 0x400000u} &&
              runtime.process_calls == 2 &&
              runtime.module_handle_calls == 1 &&
              runtime.format_message_calls == 1 &&
              runtime.symbol_calls == 1 && runtime.line_calls == 0,
          "symbol API result other than exactly one stops optional sections");
}

void CheckSymbolAndLineVariants() {
    Runtime runtime;
    runtime.SeedException();
    runtime.symbol_result = 1;
    runtime.line_result = 1;
    runtime.symbol_displacement = 12u;
    runtime.line_displacement = 6u;
    pal4::CrashReportFormatter formatter;
    Check(formatter.Generate(0x1000u, runtime) != nullptr &&
              formatter.Report() ==
                  "Reason:    game.exe caused an "
                  "{**EXCEPTION_ACCESS_VIOLATION**}\n"
                  "Location:  in module <UNKNOWN> at 0023:00401234\n"
                  "Function:  RunGame()+0012 byte(s)\n"
                  "File:      main.cpp\nLine:      000042+000006 byte(s)",
          "symbol and source line use their respective displacements");
    Check(runtime.process_id_calls == 3 && runtime.line_calls == 1,
          "module, symbol and line each request current process ID");

    runtime.symbol_displacement = 0u;
    runtime.line_displacement = 0u;
    Check(formatter.Generate(0x1000u, runtime) != nullptr &&
              formatter.Report().find(
                  "\nFunction:  RunGame \nFile:      main.cpp\nLine:      000042") !=
                  std::string::npos,
          "zero displacements use plain function and line formats");
    runtime.line_result = 2;
    Check(formatter.Generate(0x1000u, runtime) != nullptr &&
              formatter.Report().find("\nFile:") == std::string::npos,
          "line API result other than exactly one ends after function");
}

void CheckLongSymbolAndFilenameTruncation() {
    Runtime runtime;
    runtime.SeedException();
    runtime.symbol_result = 1;
    runtime.line_result = 1;
    runtime.symbol_name.assign(2000u, 'S');
    pal4::CrashReportFormatter formatter;
    Check(formatter.Generate(0x1000u, runtime) != nullptr &&
              formatter.Report().size() == 1022u &&
              formatter.Report().back() == 'S' && runtime.line_calls == 0,
          "long symbol is copied to original buffer limit and skips line lookup");

    runtime.symbol_name = "RunGame";
    runtime.file_name.assign(2000u, 'F');
    Check(formatter.Generate(0x1000u, runtime) != nullptr &&
              formatter.Report().size() == 1022u &&
              formatter.Report().back() == 'F' &&
              formatter.Report().find("\nLine:") == std::string::npos,
          "long file name is copied to original buffer limit and skips line");
}

}  // namespace

int main() {
    CheckInvalidPointerPreservesPreviousReport();
    CheckKnownDescriptionAndUnknownModule();
    CheckMessageFallbackAndNamedModule();
    CheckSymbolAndLineVariants();
    CheckLongSymbolAndFilenameTruncation();
    std::cout << "5 crash report formatter test cases passed\n";
}
