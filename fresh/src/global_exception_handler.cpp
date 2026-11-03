#include "pal4/global_exception_handler.h"

#include <array>
#include <cstdio>
#include <string>

namespace pal4 {
namespace {

std::string DetailedMessage(const std::uint32_t code, const std::string& path) {
    std::array<char, 9> hex_code{};
    std::snprintf(hex_code.data(), hex_code.size(), "%08x", code);
    return "An Exception Catched by SEHException\nCode:\t0x" +
           std::string(hex_code.data()) + "\nSee the <" + path + "> Files\n";
}

std::string LimitedMessage(const std::string& path) {
    return "An Exception Catched by SEHException\n"
           "But can't get immediate information because you turn off the StackTrace\n"
           "Please see the <" + path + "> files for more detail\n";
}

}  // namespace

void GlobalExceptionHandler::Handle(
    const std::uint32_t code, const std::uint32_t exception_pointers,
    const ExceptionHandlerOptions& handler_options, ExceptionLog& log,
    IExceptionLogRuntime& log_runtime, IGlobalExceptionRuntime& runtime) {
    const auto& options = handler_options.Values();
    (void)log.Append(log_runtime,
        ":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    (void)log.Append(log_runtime,
        "{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{ExceptionInformation}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}\n");
    if (options[0] == 1u) {
        (void)log.Append(log_runtime, "%s\n\n", runtime.CrashReport(exception_pointers));
    } else {
        (void)log.Append(log_runtime, "Not Output Reason\n\n");
    }

    (void)log.Append(log_runtime,
        "{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{Register Information}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}\n");
    if (options[2] == 1u) {
        (void)log.Append(log_runtime, "%s\n\n", runtime.Registers(exception_pointers));
    } else {
        (void)log.Append(log_runtime, "Not Output Register Information\n\n");
    }

    (void)log.Append(log_runtime,
        "{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{StackInformation}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}\n");
    if (options[1] == 1u) {
        const char* frame = runtime.PrepareStackTrace(options[4], exception_pointers);
        do {
            (void)log.Append(log_runtime, "%s\n", frame);
            frame = runtime.NextStackTrace(options[4], exception_pointers);
        } while (frame != nullptr);
    } else {
        (void)log.Append(log_runtime, "Not Output StackInformation\n\n");
    }
    (void)log.Append(log_runtime,
        ":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n\n\n");

    if (options[3] == 1u) {
        runtime.RaiseIntegerZero();
    }

    ExceptionInfo info;
    if (options[1] != 0u) {
        const int line = runtime.FirstLine();
        const char* file = runtime.FirstFile();
        const char* symbol = runtime.FirstSymbol();
        info.Format(DetailedMessage(code, log.Path()).c_str(), symbol, file, line);
    } else {
        info.Format(LimitedMessage(log.Path()).c_str(), "<Unknown>", "<Unknown>", 0);
    }
    runtime.RaiseInfo(info);
}

}  // namespace pal4
