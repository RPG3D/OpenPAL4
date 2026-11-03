#include "pal4/logger_object.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class FakeRuntime final : public pal4::ILoggerAppendRuntime {
public:
    std::uint32_t ReadDword(const std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return filename;
    }
    std::uint32_t OpenFile(const std::uint32_t filename_address,
                           const std::string_view mode) override {
        events.push_back("open:" + std::to_string(filename_address) + ":" +
                         std::string(mode));
        return stream;
    }
    std::string Vsprintf(const std::uint32_t format_address,
                         const std::uint32_t arguments_address) override {
        events.push_back("vsprintf:" + std::to_string(format_address) + ":" +
                         std::to_string(arguments_address));
        return formatted;
    }
    void Fprintf(const std::uint32_t handle,
                 const std::string_view format) override {
        events.push_back("fprintf:" + std::to_string(handle) + ":" +
                         std::string(format));
    }
    int CloseFile(const std::uint32_t handle) override {
        events.push_back("close:" + std::to_string(handle));
        return close_result;
    }

    std::vector<std::string> events;
    std::uint32_t filename = 0x5000;
    std::uint32_t stream = 0x6000;
    std::string formatted = "status=7";
    int close_result = 19;
};

void SuccessFormatsWritesAndReturnsCloseResult() {
    FakeRuntime runtime;
    Check(pal4::LoggerObject::AppendFormatted(0x4000, 0x7000, 0x8000,
                                               runtime) == 19,
          "logger append returns fclose result");
    Check(runtime.events == std::vector<std::string>{
              "read:16384", "open:20480:at", "vsprintf:28672:32768",
              "fprintf:24576:status=7", "close:24576"},
          "append follows fopen, vsprintf, fprintf, fclose order");
}

void OpenFailureStillFormatsAndClosesNullStream() {
    FakeRuntime runtime;
    runtime.stream = 0;
    runtime.close_result = -1;
    Check(pal4::LoggerObject::AppendFormatted(0x4000, 0x7000, 0x8000,
                                               runtime) == -1,
          "failed open propagates the null-stream close result");
    Check(runtime.events == std::vector<std::string>{
              "read:16384", "open:20480:at", "vsprintf:28672:32768",
              "close:0"},
          "failed fopen skips fprintf but still formats and closes null");
}

void FormattedPercentTextIsPassedAsFprintfFormat() {
    FakeRuntime runtime;
    runtime.formatted = "%s is literal input";
    static_cast<void>(pal4::LoggerObject::AppendFormatted(
        0x4000, 0x7000, 0x8000, runtime));
    Check(runtime.events[3] == "fprintf:24576:%s is literal input",
          "formatted output is passed as the fprintf format argument");
}

}  // namespace

int main() {
    SuccessFormatsWritesAndReturnsCloseResult();
    OpenFailureStillFormatsAndClosesNullStream();
    FormattedPercentTextIsPassedAsFprintfFormat();
    std::cout << "3 logger-append test cases passed\n";
}
