#include "pal4/script_engine_error_reporter.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::array<const char*, 27> kResultText = {
    "lower array dimension not registered", // -20
    "can't bind all functions",              // -19
    "invalid interface",                     // -18
    "invalid configuration",                 // -17
    "no global var",                         // -16
    "no module",                             // -15
    "multiple functions",                    // -14
    "already registered",                    // -13
    "invalid type",                          // -12
    "invalid object",                        // -11
    "invalid declaration",                   // -10
    "name taken",                            // -9
    "invalid name",                          // -8
    "not supported",                         // -7
    "no function",                           // -6
    "invalid arg",                           // -5
    "context not prepared",                  // -4
    "context not finished",                  // -3
    "context active",                        // -2
    "error",                                 // -1
    "success",                               // 0
    "execution suspended",                   // 1
    "execution aborted",                     // 2
    "execution exception",                   // 3
    "execution prepared",                    // 4
    "execution uninitialized",               // 5
    "execution active",                      // 6
};

}  // namespace

std::string ScriptEngineErrorReporter::Format(int result, const char* message) {
    if (result < -20 || result > 6) {
        return {};
    }
    std::string output = message ? message : "";
    output += kResultText[static_cast<unsigned>(result + 20)];
    output += "\r\n";
    if (result == 3) {
        output += message ? message : "";
        output += kResultText[24];
        output += "\r\n";
    }
    return output;
}

void ScriptEngineErrorReporter::Report(int result, const char* message,
                                       IScriptEngineErrorSink* sink) {
    if (sink) {
        const auto output = Format(result, message);
        sink->Write(output.c_str());
    }
}

}  // namespace pal4
