#include "pal4/global_exception_handler.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Raised {
    bool integer;
    std::string text;
};

class Runtime final : public pal4::IExceptionLogRuntime,
                      public pal4::IGlobalExceptionRuntime {
public:
    std::string Format(const char* format, std::va_list args) override {
        std::va_list copy;
        va_copy(copy, args);
        const int size = std::vsnprintf(nullptr, 0, format, copy);
        va_end(copy);
        Check(size >= 0, "log format is valid");
        std::string output(static_cast<std::size_t>(size) + 1u, '\0');
        std::vsnprintf(output.data(), output.size(), format, args);
        output.resize(static_cast<std::size_t>(size));
        return output;
    }

    std::uintptr_t Open(const char* path, const char* mode) override {
        events.emplace_back(std::string("open:") + path + ":" + mode);
        return open_result;
    }

    void Print(std::uintptr_t, const char* format) override {
        printed.emplace_back(format);
    }

    int Close(std::uintptr_t) override { return 0; }

    const char* CrashReport(std::uint32_t pointer) override {
        Check(pointer == 0x1234u, "crash report receives exception pointer");
        events.emplace_back("reason");
        return "reason text";
    }

    const char* Registers(std::uint32_t pointer) override {
        Check(pointer == 0x1234u, "registers receive exception pointer");
        events.emplace_back("registers");
        return "register text";
    }

    const char* PrepareStackTrace(std::uint32_t flags,
                                  std::uint32_t pointer) override {
        Check(flags == expected_flags && pointer == 0x1234u,
              "stack prepare receives flags and exception pointer");
        events.emplace_back("prepare");
        return "first frame";
    }

    const char* NextStackTrace(std::uint32_t flags,
                               std::uint32_t pointer) override {
        Check(flags == expected_flags && pointer == 0x1234u,
              "next stack frame receives flags and exception pointer");
        events.emplace_back("next");
        ++next_count;
        return next_count == 1 ? "second frame" : nullptr;
    }

    int FirstLine() override {
        events.emplace_back("line");
        return 91;
    }

    const char* FirstFile() override {
        events.emplace_back("file");
        return "scene.cpp";
    }

    const char* FirstSymbol() override {
        events.emplace_back("symbol");
        return "UpdateScene";
    }

    [[noreturn]] void RaiseIntegerZero() override {
        events.emplace_back("raise-integer");
        throw Raised{true, "0"};
    }

    [[noreturn]] void RaiseInfo(const pal4::ExceptionInfo& info) override {
        events.emplace_back("raise-info");
        throw Raised{false, info.Text()};
    }

    void Clear() {
        events.clear();
        printed.clear();
        next_count = 0;
    }

    std::vector<std::string> events;
    std::vector<std::string> printed;
    std::uintptr_t open_result = 1u;
    std::uint32_t expected_flags = 0u;
    int next_count = 0;
};

void CheckFullReport() {
    Runtime runtime;
    runtime.expected_flags = 8u;
    pal4::ExceptionLog log;
    pal4::ExceptionHandlerOptions options;
    Check(log.Create("seh.log", runtime) == 0, "log created");
    (void)options.Set(1u, 1u, 1u, 0u, 8u);
    runtime.Clear();
    try {
        pal4::GlobalExceptionHandler::Handle(
            0xc0000005u, 0x1234u, options, log, runtime, runtime);
    } catch (const Raised& raised) {
        Check(!raised.integer && raised.text ==
                  "An Exception Catched by SEHException\n"
                  "Code:\t0xc0000005\nSee the <seh.log> Files\n"
                  "Func:  UpdateScene\nFile:  scene.cpp\nLine:  91\n",
              "full report raises formatted exception information");
        Check(runtime.printed.size() == 9u &&
                  runtime.printed[0] == std::string(95, ':') + '\n' &&
                  runtime.printed[1] ==
                      "{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{ExceptionInformation}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}\n" &&
                  runtime.printed[2] == "reason text\n\n" &&
                  runtime.printed[3] ==
                      "{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{Register Information}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}\n" &&
                  runtime.printed[4] == "register text\n\n" &&
                  runtime.printed[5] ==
                      "{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{StackInformation}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}\n" &&
                  runtime.printed[6] == "first frame\n" &&
                  runtime.printed[7] == "second frame\n" &&
                  runtime.printed[8] == std::string(95, ':') + "\n\n\n",
              "full report writes sections, both stack frames and footer in order");
        Check(runtime.events == std::vector<std::string>{
                  "open:seh.log:at", "open:seh.log:at", "reason",
                  "open:seh.log:at", "open:seh.log:at", "registers",
                  "open:seh.log:at", "open:seh.log:at", "prepare",
                  "open:seh.log:at", "next",
                  "open:seh.log:at", "next", "open:seh.log:at",
                  "line", "file", "symbol", "raise-info"},
              "report queries and append calls follow IDA control flow");
        return;
    }
    Check(false, "full report must throw");
}

void CheckDisabledReport() {
    Runtime runtime;
    pal4::ExceptionLog log;
    pal4::ExceptionHandlerOptions options;
    (void)log.Create("disabled.log", runtime);
    (void)options.Set(0u, 0u, 0u, 0u, 0u);
    runtime.Clear();
    try {
        pal4::GlobalExceptionHandler::Handle(
            1u, 0x1234u, options, log, runtime, runtime);
    } catch (const Raised& raised) {
        Check(!raised.integer && raised.text ==
                  "An Exception Catched by SEHException\n"
                  "But can't get immediate information because you turn off the StackTrace\n"
                  "Please see the <disabled.log> files for more detail\n"
                  "Func:  <Unknown>\nFile:  <Unknown>\nLine:  <Unknown>\n",
              "disabled stack trace raises the limited message");
        Check(runtime.printed.size() == 8u &&
                  runtime.printed[2] == "Not Output Reason\n\n" &&
                  runtime.printed[4] == "Not Output Register Information\n\n" &&
                  runtime.printed[6] == "Not Output StackInformation\n\n",
              "disabled sections use exact fixed messages");
        Check(runtime.events.back() == "raise-info" &&
                  runtime.next_count == 0,
              "disabled sections do not query stack frames");
        return;
    }
    Check(false, "disabled report must throw");
}

void CheckNonOneStackAndIntegerThrow() {
    Runtime runtime;
    pal4::ExceptionLog log;
    pal4::ExceptionHandlerOptions options;
    (void)log.Create("other.log", runtime);
    (void)options.Set(2u, 2u, 2u, 0u, 3u);
    runtime.Clear();
    try {
        pal4::GlobalExceptionHandler::Handle(
            0x123u, 0x1234u, options, log, runtime, runtime);
    } catch (const Raised& raised) {
        Check(!raised.integer &&
                  raised.text.find("Code:\t0x00000123") != std::string::npos &&
                  runtime.printed[6] == "Not Output StackInformation\n\n" &&
                  runtime.next_count == 0,
              "non-one stack flag skips output but nonzero flag selects detailed throw");
    }

    (void)options.Set(0u, 0u, 0u, 1u, 0u);
    runtime.Clear();
    runtime.open_result = 0u;
    try {
        pal4::GlobalExceptionHandler::Handle(
            7u, 0x1234u, options, log, runtime, runtime);
    } catch (const Raised& raised) {
        Check(raised.integer && raised.text == "0" &&
                  runtime.printed.empty() &&
                  runtime.events.back() == "raise-integer",
              "integer throw follows logging attempts even when opening fails");
        return;
    }
    Check(false, "integer mode must throw");
}

}  // namespace

int main() {
    CheckFullReport();
    CheckDisabledReport();
    CheckNonOneStackAndIntegerThrow();
    std::cout << "3 global exception handler test cases passed\n";
}
