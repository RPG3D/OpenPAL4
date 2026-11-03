#include "pal4/exception_log.h"

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

class Runtime final : public pal4::IExceptionLogRuntime {
public:
    std::string Format(const char* format, std::va_list args) override {
        events.emplace_back("format");
        std::va_list copy;
        va_copy(copy, args);
        const int length = std::vsnprintf(nullptr, 0, format, copy);
        va_end(copy);
        Check(length >= 0, "valid formatted input");
        std::string output(static_cast<std::size_t>(length) + 1u, '\0');
        std::vsnprintf(output.data(), output.size(), format, args);
        output.resize(static_cast<std::size_t>(length));
        return output;
    }

    std::uintptr_t Open(const char* path, const char* mode) override {
        events.emplace_back(std::string("open:") + path + ":" + mode);
        return opened_stream;
    }

    void Print(std::uintptr_t stream, const char* format) override {
        Check(stream == opened_stream, "print uses opened stream");
        events.emplace_back("print");
        printed.emplace_back(format);
    }

    int Close(std::uintptr_t stream) override {
        Check(stream == opened_stream, "close uses opened stream");
        events.emplace_back("close");
        return close_result;
    }

    std::vector<std::string> events;
    std::vector<std::string> printed;
    std::uintptr_t opened_stream = 0x1234u;
    int close_result = 17;
};

void CheckCreate() {
    pal4::ExceptionLog log;
    Runtime runtime;
    Check(log.Create("exception.log", runtime) == 17 &&
              log.Path() == "exception.log" &&
              runtime.events == std::vector<std::string>{
                  "open:exception.log:wt", "print", "print", "print", "close"},
          "create opens in truncate mode, writes header and returns close result");
    Check(runtime.printed.size() == 3u &&
              runtime.printed[0] == std::string(95, ':') + "\n" &&
              runtime.printed[1] ==
                  "{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{SEHException}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}\n" &&
              runtime.printed[2] == std::string(95, ':') + "\n\n\n",
          "header bytes match the three IDA string literals");
}

void CheckOpenFailure() {
    pal4::ExceptionLog log;
    Runtime runtime;
    runtime.opened_stream = 0u;
    Check(log.Create("failed.log", runtime) == 0 &&
              log.Path() == "failed.log" &&
              runtime.events == std::vector<std::string>{"open:failed.log:wt"},
          "failed create retains copied path and skips writes and close");
    runtime.events.clear();
    Check(log.Append(runtime, "value=%d", 42) == 0 &&
              log.Buffer() == "value=42" &&
              runtime.events == std::vector<std::string>{
                  "format", "open:failed.log:at"},
          "append formats before attempting to open the file");
}

void CheckAppend() {
    pal4::ExceptionLog log;
    Runtime runtime;
    Check(log.Create("trace.log", runtime) == 17, "create succeeds");
    runtime.events.clear();
    runtime.printed.clear();
    runtime.close_result = -3;
    Check(log.Append(runtime, "name=%s number=%d", "Ada", 7) == -3 &&
              log.Buffer() == "name=Ada number=7" &&
              runtime.printed == std::vector<std::string>{"name=Ada number=7"} &&
              runtime.events == std::vector<std::string>{
                  "format", "open:trace.log:at", "print", "close"},
          "append forwards formatted buffer as print format and returns close result");
}

}  // namespace

int main() {
    CheckCreate();
    CheckOpenFailure();
    CheckAppend();
    std::cout << "3 exception log test cases passed\n";
}
