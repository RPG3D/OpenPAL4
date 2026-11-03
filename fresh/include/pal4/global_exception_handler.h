#pragma once

#include "pal4/exception_info.h"
#include "pal4/exception_log.h"
#include "pal4/exception_handler_options.h"

#include <cstdint>

namespace pal4 {

class IGlobalExceptionRuntime {
public:
    virtual ~IGlobalExceptionRuntime() = default;
    [[nodiscard]] virtual const char* CrashReport(std::uint32_t exception_pointers) = 0;
    [[nodiscard]] virtual const char* Registers(std::uint32_t exception_pointers) = 0;
    [[nodiscard]] virtual const char* PrepareStackTrace(
        std::uint32_t flags, std::uint32_t exception_pointers) = 0;
    [[nodiscard]] virtual const char* NextStackTrace(
        std::uint32_t flags, std::uint32_t exception_pointers) = 0;
    [[nodiscard]] virtual int FirstLine() = 0;
    [[nodiscard]] virtual const char* FirstFile() = 0;
    [[nodiscard]] virtual const char* FirstSymbol() = 0;
    [[noreturn]] virtual void RaiseIntegerZero() = 0;
    [[noreturn]] virtual void RaiseInfo(const ExceptionInfo& info) = 0;
};

class GlobalExceptionHandler final {
public:
    [[noreturn]] static void Handle(
        std::uint32_t code, std::uint32_t exception_pointers,
        const ExceptionHandlerOptions& options, ExceptionLog& log,
        IExceptionLogRuntime& log_runtime, IGlobalExceptionRuntime& runtime);
};

}  // namespace pal4
