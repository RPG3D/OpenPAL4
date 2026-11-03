#include "pal4/script_engine_error_reporter.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Sink final : pal4::IScriptEngineErrorSink {
    int calls{};
    std::string received;
    void Write(const char* text) override {
        ++calls;
        received = text;
    }
};

}  // namespace

int main() {
    constexpr std::array<const char*, 27> expected = {
        "lower array dimension not registered", "can't bind all functions",
        "invalid interface", "invalid configuration", "no global var",
        "no module", "multiple functions", "already registered",
        "invalid type", "invalid object", "invalid declaration",
        "name taken", "invalid name", "not supported", "no function",
        "invalid arg", "context not prepared", "context not finished",
        "context active", "error", "success", "execution suspended",
        "execution aborted", "execution exception", "execution prepared",
        "execution uninitialized", "execution active"};
    for (int result = -20; result <= 6; ++result) {
        std::string text = "module: ";
        text += expected[static_cast<unsigned>(result + 20)];
        text += "\r\n";
        if (result == 3) {
            text += "module: execution prepared\r\n";
        }
        Check(pal4::ScriptEngineErrorReporter::Format(result, "module: ") == text,
              "all observed result codes preserve exact text and line endings");
    }
    Check(pal4::ScriptEngineErrorReporter::Format(-21, "module: ").empty() &&
              pal4::ScriptEngineErrorReporter::Format(7, "module: ").empty(),
          "out-of-range result produces an empty stream");
    Check(pal4::ScriptEngineErrorReporter::Format(-5, nullptr) ==
              "invalid arg\r\n",
          "null message uses empty prefix");

    Sink sink;
    pal4::ScriptEngineErrorReporter::Report(3, "read: ", &sink);
    Check(sink.calls == 1 && sink.received ==
              "read: execution exception\r\nread: execution prepared\r\n",
          "exception result falls through to prepared result and invokes sink once");
    pal4::ScriptEngineErrorReporter::Report(99, "read: ", &sink);
    Check(sink.calls == 2 && sink.received.empty(),
          "unknown result still calls sink with empty text");
    pal4::ScriptEngineErrorReporter::Report(-1, "ignored", nullptr);
    Check(sink.calls == 2, "null sink performs no callback");
}
