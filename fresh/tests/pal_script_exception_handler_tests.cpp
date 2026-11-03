#include "pal4/pal_script_exception_handler.h"

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

struct Runtime final : pal4::IPalScriptExceptionRuntime {
    std::uint32_t script_object = 0x4000u;
    std::uint32_t recovery_object = 0x5000u;
    std::uint32_t report_result = 0x1234abcd;
    std::int32_t classification = 2;
    std::uint32_t try_result = 0u;
    std::vector<std::int32_t> steps;
    std::size_t step_index = 0u;
    std::vector<std::string> events;

    [[nodiscard]] std::uint32_t ScriptObject(
        const std::uint32_t context) override {
        Check(context == 0x2054u,
              "script object lookup receives executor +84");
        events.push_back("script");
        return script_object;
    }
    [[nodiscard]] std::uint32_t RecoveryObject(
        const std::uint32_t script) override {
        Check(script == script_object,
              "recovery lookup uses returned script object");
        events.push_back("recovery");
        return recovery_object;
    }
    [[nodiscard]] const char* What(
        const std::uint32_t exception_address) override {
        Check(exception_address == 0x3000u,
              "exception message uses native exception address");
        events.push_back("what");
        return "failure";
    }
    [[nodiscard]] const char* FunctionName(
        const std::uint32_t exception_address) override {
        Check(exception_address == 0x3000u,
              "function name uses native exception address");
        events.push_back("function");
        return "main";
    }
    [[nodiscard]] std::uint32_t Report(
        const std::uint32_t script, const char* what) override {
        Check(script == script_object && std::string(what) == "failure",
              "report receives script object and exception message");
        events.push_back("report");
        return report_result;
    }
    [[nodiscard]] std::int32_t Classify(
        const std::uint32_t script, const char* what,
        const char* function_name) override {
        Check(script == script_object && std::string(what) == "failure" &&
                  std::string(function_name) == "main",
              "classification receives function and message");
        events.push_back("classify");
        return classification;
    }
    [[nodiscard]] std::uint32_t ConvertStatus(
        const std::uint32_t script, const std::int32_t status,
        const std::uint32_t option) override {
        Check(script == script_object && status == classification && option == 0u,
              "conversion receives classification and zero option");
        events.push_back("convert");
        return 0x6000u;
    }
    [[nodiscard]] std::uint32_t FormatMessage(
        const std::uint32_t script, const char* what,
        const std::uint32_t converted_status) override {
        Check(script == script_object && std::string(what) == "failure" &&
                  converted_status == 0x6000u,
              "format receives converted status and message");
        events.push_back("format");
        return 0x7000u;
    }
    [[nodiscard]] std::uint32_t TryRecovery(
        const std::uint32_t recovery,
        const std::uint32_t message) override {
        Check(recovery == recovery_object && message == 0x7000u,
              "recovery receives formatted message");
        events.push_back("try");
        return try_result;
    }
    [[nodiscard]] std::int32_t NextRecoveryStatus(
        const std::uint32_t recovery,
        const std::uint32_t option) override {
        Check(recovery == recovery_object && option == 0u &&
                  step_index < steps.size(),
              "recovery poll uses object and zero option");
        events.push_back("step");
        return steps[step_index++];
    }
    void ResetRecovery(const std::uint32_t recovery) override {
        Check(recovery == recovery_object,
              "reset receives recovery object");
        events.push_back("reset");
    }
    void ReleaseRecovery(const std::uint32_t recovery) override {
        Check(recovery == recovery_object,
              "release receives recovery object");
        events.push_back("release");
    }
};

void TestMissingScriptAndRecovery() {
    Runtime runtime;
    runtime.script_object = 0u;
    Check(pal4::PalScriptExceptionHandler::Handle(
              0x2000u, 0x3000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"script"},
          "missing script object returns zero without querying exception");
    runtime.script_object = 0x4000u;
    runtime.recovery_object = 0u;
    runtime.events.clear();
    Check(pal4::PalScriptExceptionHandler::Handle(
              0x2000u, 0x3000u, runtime) == 0x1234ab00u &&
              runtime.events == std::vector<std::string>{
                  "script", "recovery", "what", "report"},
          "missing recovery reports once and clears only low return byte");
}

void TestClassificationAndTryFailure() {
    Runtime runtime;
    runtime.classification = -6;
    Check(pal4::PalScriptExceptionHandler::Handle(
              0x2000u, 0x3000u, runtime) == 0x1234ab00u &&
              runtime.events == std::vector<std::string>{
                  "script", "recovery", "function", "what",
                  "classify", "reset", "release", "what", "report"},
          "minus-six classification releases recovery and reports");
    runtime.classification = 2;
    runtime.try_result = 1u;
    runtime.events.clear();
    Check(pal4::PalScriptExceptionHandler::Handle(
              0x2000u, 0x3000u, runtime) == 0x1234ab00u &&
              runtime.events == std::vector<std::string>{
                  "script", "recovery", "function", "what", "classify",
                  "convert", "what", "format", "try",
                  "reset", "release", "what", "report"},
          "nonzero recovery attempt releases object and reports without polling");
}

void TestRecoveryPollTerminalResults() {
    Runtime runtime;
    runtime.steps = {-1};
    Check(pal4::PalScriptExceptionHandler::Handle(
              0x2000u, 0x3000u, runtime) == 0x1234ab00u &&
              runtime.step_index == 1u &&
              runtime.events[runtime.events.size() - 3u] == "release",
          "minus-one poll status returns low byte zero");

    runtime.steps = {0};
    runtime.step_index = 0u;
    runtime.events.clear();
    Check(pal4::PalScriptExceptionHandler::Handle(
              0x2000u, 0x3000u, runtime) == 0x1234ab01u &&
              runtime.events == std::vector<std::string>{
                  "script", "recovery", "function", "what", "classify",
                  "convert", "what", "format", "try", "step",
                  "reset", "release", "what", "report"},
          "zero poll status returns low byte one after cleanup and report");

    runtime.steps = {3, 2, 0};
    runtime.step_index = 0u;
    runtime.events.clear();
    Check(pal4::PalScriptExceptionHandler::Handle(
              0x2000u, 0x3000u, runtime) == 0x1234ab01u &&
              runtime.step_index == 3u,
          "positive statuses continue polling until zero");
}

}  // namespace

int main() {
    TestMissingScriptAndRecovery();
    TestClassificationAndTryFailure();
    TestRecoveryPollTerminalResults();
}
